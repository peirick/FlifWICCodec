#define INITGUID

#include <windows.h>
#include <new>
#include <advpub.h>
#include <shlobj.h>
#include <strsafe.h>
#include <unknwn.h>
#include <Propvarutil.h>
#include "dllmain.h"
#include "decode_container.h"
#include "encode_container.h"
#include "metadata_store.h"
#include "utils.h"
#include "uuid.h"

// Logging in debug mode.
#ifdef FLIF_DEBUG_LOGGING

#include <Shlwapi.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static CRITICAL_SECTION debug_file_section;

void MAIN_debug_printf(const char* prefix, const char* func, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    EnterCriticalSection(&debug_file_section);
    fprintf(stdout, "%s:%s ", prefix, func);
    fflush(stdout);
    vfprintf(stdout, fmt, ap);
    fflush(stdout);
    LeaveCriticalSection(&debug_file_section);
    va_end(ap);
}

static void init_logging() {
    InitializeCriticalSection(&debug_file_section);

    WCHAR path[MAX_PATH];
    DWORD path_size = MAX_PATH;
    DWORD err;
    if ((err = SHRegGetValueW(HKEY_LOCAL_MACHINE, L"Software\\FLIF\\FLIF Codec",
        L"DebugPath", SRRF_RT_REG_SZ, NULL, path, &path_size)) != ERROR_SUCCESS) {
        StringCchCopyW(path, MAX_PATH, L"C:\\DebugOut");
    }

    WCHAR filename[MAX_PATH];
    StringCchPrintfW(filename, MAX_PATH, L"%s\\flif-stdout.txt", path);
    _wfreopen(filename, L"a", stdout);
    StringCchPrintfW(filename, MAX_PATH, L"%s\\flif-stderr.txt", path);
    _wfreopen(filename, L"a", stderr);
}

// Returns a pointer to a string representation of a GUID. The results are
// stored in 32 static buffer - the 33rd call to this methods will overwrite
// the first result.
char *debugstr_guid(REFGUID guid)
{
    static char guidbuf[32][128];
    static int pos = 0;
    pos %= 32;
    StringCchPrintfA(guidbuf[pos], 128,
        "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
        guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1],
        guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5],
        guid.Data4[6], guid.Data4[7]);
    return guidbuf[pos++];
}

WCHAR *debugstr_var(REFPROPVARIANT var)
{
    static WCHAR buf[32][128];
    static int pos = 0;
    pos %= 32;
    HRESULT result = PropVariantToString(var, buf[pos], ARRAYSIZE(buf[pos]));
    if (SUCCEEDED(result) || result == STRSAFE_E_INSUFFICIENT_BUFFER)
    {
        return buf[pos++];
    }
    else {
        return L"error";
    }
}
#endif

// Object and server locks counters
LONG volatile MAIN_nObjects = 0;
LONG volatile MAIN_nServerLocks = 0;
HINSTANCE MAIN_hSelf;


// Class factory

typedef HRESULT(*ObjectConstructor)(IUnknown** ppvObject);

// A default constructor. Creates and instance of T. T should be a subclass of
// IUnknown with a parameter-less constructor.
template<typename T>
HRESULT CreateComObject(IUnknown** output) {
    T* result = new (std::nothrow) T();
    if (result == NULL)
        return E_OUTOFMEMORY;
    *output = static_cast<IUnknown*>(result);
    return S_OK;
}

class MyClassFactory : public ComObjectBase<IClassFactory>
{
public:
    MyClassFactory(ObjectConstructor ctor);
    // IUnknown:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
    ULONG STDMETHODCALLTYPE AddRef() { return ComObjectBase::AddRef(); }
    ULONG STDMETHODCALLTYPE Release() { return ComObjectBase::Release(); }
    // IClassFactory:
    HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObject);
    HRESULT STDMETHODCALLTYPE LockServer(BOOL fLock);

private:
    volatile LONG ref_count_;
    ObjectConstructor ctor_;
};

MyClassFactory::MyClassFactory(ObjectConstructor ctor) {
    InterlockedIncrement(&MAIN_nObjects);
    ref_count_ = 0;
    ctor_ = ctor;
}

HRESULT MyClassFactory::QueryInterface(REFIID riid, void **ppvObject)
{
    if (ppvObject == NULL)
        return E_INVALIDARG;
    *ppvObject = NULL;

    if (!IsEqualGUID(riid, IID_IUnknown) && !IsEqualGUID(riid, IID_IClassFactory))
        return E_NOINTERFACE;
    this->AddRef();
    *ppvObject = static_cast<IClassFactory*>(this);
    return S_OK;
}

HRESULT MyClassFactory::CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObject)
{
    IUnknown* output;
    HRESULT ret;
    TRACE3("(%p, %s, %p)\n", pUnkOuter, debugstr_guid(riid), ppvObject);

    if (ppvObject == NULL)
        return E_INVALIDARG;
    *ppvObject = NULL;

    if (pUnkOuter != NULL)
        return CLASS_E_NOAGGREGATION;

    ret = ctor_(&output);
    if (FAILED(ret))
        return ret;
    ret = output->QueryInterface(riid, ppvObject);
    output->Release();
    if (FAILED(ret))
        ppvObject = NULL;
    TRACE1("ret=%08x\n", ret);
    return ret;
}

HRESULT MyClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
        InterlockedIncrement(&MAIN_nServerLocks);
    else
        InterlockedDecrement(&MAIN_nServerLocks);
    return S_OK;
}

typedef HRESULT(WINAPI *RegInstallFuncA)(HMODULE hm, LPCSTR pszSection, const STRTABLEA* pstTable);
typedef void (STDAPICALLTYPE *SHChangeNotifyFunc)(LONG wEventId, UINT uFlags, LPCVOID dwItem1, LPCVOID dwItem2);


static HRESULT RegisterServer(BOOL fInstall) {
    // Manual loading of advpack not to load it when DLL used in normal opertion.
    HMODULE hAdvPack = LoadLibraryExW(L"advpack.dll", NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (hAdvPack == NULL) {
        TRACE("Couldn't load advpack.dll\n");
        return E_UNEXPECTED;
    }

    // Note: RegInstallA/W not available on Windows XP with MSIE6.
    // Using ANSI version doesn't matter, as the "unicodeness" of the _MOD_PATH
    // depends only on the "unicodeness" of the *.inf file, while other
    // substitutions are ASCII.
    RegInstallFuncA pRegInstallA = (RegInstallFuncA)GetProcAddress(hAdvPack, "RegInstall");
    if (!pRegInstallA) {
        TRACE("Couldn't find RegInstall in advpack.dll\n");
        return E_UNEXPECTED;
    }

    STRENTRYA entries[1] = {
        { "PhotoDir", "Windows Photo Viewer" }
    };
    STRTABLEA strings;
    strings.cEntries = sizeof(entries) / sizeof(entries[0]);
    strings.pse = entries;
    if (LOWORD(GetVersion()) == 0x0006)
        entries[0].pszValue = "Windows Photo Gallery";

    LPCSTR section;
    if (LOBYTE(GetVersion()) < 6) {
        section = (fInstall ? "PrevistaInstall" : "PrevistaUninstall");
    }
    else {
        section = (fInstall ? "DefaultInstall" : "DefaultUninstall");
    }
    TRACE3("Registering install=%d (using %ws) v=%x\n", fInstall, section, GetVersion());
    if (FAILED(pRegInstallA(MAIN_hSelf, section, &strings)))
        return E_UNEXPECTED;

    // Invalidate caches.
    HMODULE hShell32 = LoadLibraryExW(L"shell32.dll", NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!hShell32) {
        TRACE("Couldn't load shell32.dll\n");
        return E_UNEXPECTED;
    }
    SHChangeNotifyFunc pSHChangeNotify = (SHChangeNotifyFunc)GetProcAddress(hShell32, "SHChangeNotify");
    if (pSHChangeNotify)
        pSHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
    return S_OK;
}

STDAPI DllRegisterServer() {
    return RegisterServer(TRUE);
}

STDAPI DllUnregisterServer() {
    return RegisterServer(FALSE);
}

_Check_return_
STDAPI DllGetClassObject(_In_ REFCLSID clsid, _In_ REFIID iid, _Outptr_ LPVOID FAR* ppv)
{
    if (ppv == NULL)
        return E_INVALIDARG;
    *ppv = NULL;
    TRACE3("(%s, %s, %p)\n", debugstr_guid(clsid), debugstr_guid(iid), ppv);
    if (!IsEqualGUID(iid, IID_IClassFactory))
        return E_INVALIDARG;

    if (IsEqualGUID(clsid, CLSID_FLIFWICDecoder)) {
        *ppv = (LPVOID)(new (std::nothrow) MyClassFactory(CreateComObject<DecodeContainer>));
    }
    else if (IsEqualGUID(clsid, CLSID_FLIFWICEncoder)) {
        *ppv = (LPVOID)(new (std::nothrow) MyClassFactory(CreateComObject<EncodeContainer>));
    }
    else if (IsEqualGUID(clsid, GUID_FLIFPropertyStore)) {
        *ppv = (LPVOID)(new (std::nothrow) MyClassFactory(CreateComObject<MetadataStore>));
    }
    else {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    if (*ppv == NULL)
        return E_OUTOFMEMORY;
    return S_OK;
}

STDAPI DllCanUnloadNow() {
    if (MAIN_nObjects == 0 && MAIN_nServerLocks == 0)
        return S_OK;
    else
        return S_FALSE;
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        MAIN_hSelf = hModule;
#ifdef FLIF_DEBUG_LOGGING
        init_logging();
#endif
    }
    TRACE1("(%d)\n", ul_reason_for_call);
    return TRUE;
}
