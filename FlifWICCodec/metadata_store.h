#pragma once

#include <windows.h> 
#include <shlwapi.h> 
#include <propkey.h> 
#include <propsys.h>
#include <vector>
#include "decode_container.h"
#include "utils.h"

class MetadataStore : public ComObjectBase<IPropertyStore> {
public:
    explicit MetadataStore();
    ~MetadataStore();
    // Inherited via IUnknown:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override;
    ULONG STDMETHODCALLTYPE AddRef() override { return ComObjectBase::AddRef(); }
    ULONG STDMETHODCALLTYPE Release() override { return ComObjectBase::Release(); }
    // Inherited via ComObjectBase
    HRESULT STDMETHODCALLTYPE GetCount(DWORD * cProps) override;
    HRESULT STDMETHODCALLTYPE GetAt(DWORD iProp, PROPERTYKEY * pkey) override;
    HRESULT STDMETHODCALLTYPE GetValue(REFPROPERTYKEY key, PROPVARIANT * pv) override;
    HRESULT STDMETHODCALLTYPE SetValue(REFPROPERTYKEY key, REFPROPVARIANT propvar) override;
    HRESULT STDMETHODCALLTYPE Commit(void) override;
private:
    class InitializeWithStream : public IInitializeWithStream
    {
    public:
        InitializeWithStream(MetadataStore& metadataStore) : metadataStore_(metadataStore) { InitializeCriticalSection(&cs_); }
        ~InitializeWithStream() { DeleteCriticalSection(&cs_); }
        // Inherited via IUnknown:
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override { return metadataStore_.QueryInterface(riid, ppvObject); };
        ULONG STDMETHODCALLTYPE AddRef() override { return metadataStore_.AddRef(); }
        ULONG STDMETHODCALLTYPE Release() override { return metadataStore_.Release(); }
        // Inherited via IInitializeWithStream
        HRESULT STDMETHODCALLTYPE Initialize(IStream * pstream, DWORD grfMode) override;
    private:
        MetadataStore& metadataStore_;
        CRITICAL_SECTION cs_;
    };

    class PropertyStoreCapabilities : public IPropertyStoreCapabilities
    {
    public:
        PropertyStoreCapabilities(MetadataStore& metadataStore) : metadataStore_(metadataStore) {}
        // Inherited via IUnknown:
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override { return metadataStore_.QueryInterface(riid, ppvObject); };
        ULONG STDMETHODCALLTYPE AddRef() override { return metadataStore_.AddRef(); }
        ULONG STDMETHODCALLTYPE Release() override { return metadataStore_.Release(); }
        // Inherited via IPropertyStoreCapabilities
        virtual HRESULT STDMETHODCALLTYPE IsPropertyWritable(REFPROPERTYKEY key) override;
    private:
        MetadataStore& metadataStore_;
    };

    ComPtr<IPropertyStoreCache> propertyStoreCache_;
    InitializeWithStream        initializeWithStream_;
    PropertyStoreCapabilities   propertyStoreCapabilities_;
};