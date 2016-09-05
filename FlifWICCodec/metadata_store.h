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
        InitializeWithStream(MetadataStore& metadataStore) : metadataStore_(metadataStore) {}
        // Inherited via IUnknown:
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override { return metadataStore_.QueryInterface(riid, ppvObject); };
        ULONG STDMETHODCALLTYPE AddRef() override { return metadataStore_.AddRef(); }
        ULONG STDMETHODCALLTYPE Release() override { return metadataStore_.Release(); }
        // Inherited via IInitializeWithStream
        HRESULT STDMETHODCALLTYPE Initialize(IStream * pstream, DWORD grfMode) override;
    private:
        MetadataStore& metadataStore_;
    };

    class NamedPropertyStore : public INamedPropertyStore
    {
    public:
        NamedPropertyStore(MetadataStore& metadataStore) : metadataStore_(metadataStore) {}
        // Inherited via IUnknown:
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override { return metadataStore_.QueryInterface(riid, ppvObject); };
        ULONG STDMETHODCALLTYPE AddRef() override { return metadataStore_.AddRef(); }
        ULONG STDMETHODCALLTYPE Release() override { return metadataStore_.Release(); }
        // Inherited via INamedPropertyStore
        virtual HRESULT STDMETHODCALLTYPE GetNamedValue(LPCWSTR pszName, PROPVARIANT * ppropvar) override;
        virtual HRESULT STDMETHODCALLTYPE SetNamedValue(LPCWSTR pszName, REFPROPVARIANT propvar) override;
        virtual HRESULT STDMETHODCALLTYPE GetNameCount(DWORD * pdwCount) override;
        virtual HRESULT STDMETHODCALLTYPE GetNameAt(DWORD iProp, BSTR * pbstrName) override;
    private:
        MetadataStore& metadataStore_;


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

    struct PropertyData {
        PROPERTYKEY key;
        PROPVARIANT value;
        LPCWSTR name;

        PropertyData(PROPERTYKEY k, PROPVARIANT v, LPCWSTR n) : key(k), value(v), name(n) {}
    };

    std::vector<PropertyData> metadata_;
    InitializeWithStream initializeWithStream_;
    NamedPropertyStore namedPropertyStore_;
    PropertyStoreCapabilities propertyStoreCapabilities_;
};