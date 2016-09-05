#include "metadata_store.h"
#include <propsys.h>
#include <Propvarutil.h>
#include <comutil.h>

MetadataStore::MetadataStore()
    : initializeWithStream_(*this)
    , namedPropertyStore_(*this)
    , propertyStoreCapabilities_(*this)
{
    TRACE("()\n");
}

MetadataStore::~MetadataStore()
{
    TRACE("()\n");
}

HRESULT MetadataStore::QueryInterface(REFIID riid, void ** ppvObject)
{
    TRACE2("(%s, %p)\n", debugstr_guid(riid), ppvObject);

    if (ppvObject == nullptr)
        return E_INVALIDARG;
    *ppvObject = nullptr;

    if (IsEqualGUID(riid, IID_IUnknown) ||
        IsEqualGUID(riid, IID_IPropertyStore))
    {
        this->AddRef();
        *ppvObject = static_cast<IPropertyStore*>(this);
        return S_OK;
    }

    // Multiple inheritance needs explicit cast
    if (IsEqualGUID(riid, IID_IInitializeWithStream))
    {
        this->AddRef();
        *ppvObject = static_cast<IInitializeWithStream*>(&this->initializeWithStream_);
        return S_OK;
    }

    // Multiple inheritance needs explicit cast
    if (IsEqualGUID(riid, IID_INamedPropertyStore))
    {
        this->AddRef();
        *ppvObject = static_cast<INamedPropertyStore*>(&this->namedPropertyStore_);
        return S_OK;
    }

    // Multiple inheritance needs explicit cast
    if (IsEqualGUID(riid, IID_IPropertyStoreCapabilities))
    {
        this->AddRef();
        *ppvObject = static_cast<IPropertyStoreCapabilities*>(&this->propertyStoreCapabilities_);
        return S_OK;
    }

    return E_NOINTERFACE;
}

HRESULT MetadataStore::GetCount(DWORD * cProps)
{
    TRACE1("(%p)\n", cProps);
    if (cProps == nullptr)
        return E_INVALIDARG;
    *cProps = metadata_.size();
    return S_OK;
}

HRESULT MetadataStore::GetAt(DWORD iProp, PROPERTYKEY * pkey)
{
    TRACE2("(%d %p)\n", iProp, pkey);
    if (pkey == nullptr)
        return E_INVALIDARG;

    if (iProp >= metadata_.size())
        return E_INVALIDARG;

    *pkey = metadata_[iProp].key;
    return S_OK;
}

HRESULT MetadataStore::GetValue(REFPROPERTYKEY key, PROPVARIANT * pv)
{
    TRACE2("(%s, %p)\n", debugstr_guid(key.fmtid), pv);
    if (pv == nullptr)
        return E_INVALIDARG;

    PropVariantClear(pv);
    for (const PropertyData& data : metadata_) {
        if (data.key == key) {
            PropVariantCopy(pv, &data.value);
            break;
        }
    }
    return S_OK;
}

HRESULT MetadataStore::SetValue(REFPROPERTYKEY key, REFPROPVARIANT propvar)
{
    TRACE2("(%s, %ls)\n", debugstr_guid(key.fmtid), debugstr_var(propvar));
    return S_OK;
}

HRESULT MetadataStore::Commit(void)
{
    return S_OK;
}

HRESULT MetadataStore::InitializeWithStream::Initialize(IStream * pstream, DWORD grfMode)
{
    TRACE2("(%p, %d)\n", pstream, grfMode);
    HRESULT result = S_OK;
    //DecodeContainer decodeContainer;
    //result = decodeContainer.Initialize(pstream, WICDecodeMetadataCacheOnDemand);
    //if (FAILED(result))
    //    return result;
    //ComPtr<IWICMetadataQueryReader> queryReader;
    //result = decodeContainer.GetMetadataQueryReader(queryReader.get_out_storage());
    //if (FAILED(result))
    //    return result;
    //queryReader->GetMetadataByName(A, &propvar);

    //PSCreateMemoryPropertyStore(IID_PPV_ARGS(metadataStore_.metadata_.get_out_storage()));

    PROPVARIANT propvar;
    PropVariantInit(&propvar);

#define ReadAndAdd(A, B)                                         \
        InitPropVariantFromString(A, &propvar);                  \
        metadataStore_.metadata_.emplace_back(B, propvar, A);    \
        PropVariantClear(&propvar);


    //ReadAndAdd(L"", PKEY_Photo_Aperture)
    //    ReadAndAdd(L"", PKEY_Photo_ApertureDenominator)
    //    ReadAndAdd(L"", PKEY_Photo_ApertureNumerator)
    //    ReadAndAdd(L"", PKEY_Photo_Brightness)
    //    ReadAndAdd(L"", PKEY_Photo_BrightnessDenominator)
    //    ReadAndAdd(L"", PKEY_Photo_BrightnessNumerator)
    ReadAndAdd(L"PKEY_Photo_CameraManufacturer", PKEY_Photo_CameraManufacturer);
    ReadAndAdd(L"PKEY_Photo_CameraManufacturer", PKEY_Photo_CameraModel);
    //    ReadAndAdd(L"", PKEY_Photo_CameraSerialNumber)
    //    ReadAndAdd(L"", PKEY_Photo_Contrast)
    //    ReadAndAdd(L"", PKEY_Photo_ContrastText)
    //    ReadAndAdd(L"", PKEY_Photo_DateTaken)
    //    ReadAndAdd(L"", PKEY_Photo_DigitalZoom)
    //    ReadAndAdd(L"", PKEY_Photo_DigitalZoomDenominator)
    //    ReadAndAdd(L"", PKEY_Photo_DigitalZoomNumerator)
    //    ReadAndAdd(L"", PKEY_Photo_Event)
    //    ReadAndAdd(L"", PKEY_Photo_EXIFVersion)
    //    ReadAndAdd(L"", PKEY_Photo_ExposureBias)
    //    ReadAndAdd(L"", PKEY_Photo_ExposureBiasDenominator)
    //    ReadAndAdd(L"", PKEY_Photo_ExposureBiasNumerator)
    //    ReadAndAdd(L"", PKEY_Photo_ExposureIndex)
    //    ReadAndAdd(L"", PKEY_Photo_ExposureIndexDenominator)
    //    ReadAndAdd(L"", PKEY_Photo_ExposureIndexNumerator)


#undef ReadAndAdd


    return result;
}

HRESULT MetadataStore::NamedPropertyStore::GetNamedValue(LPCWSTR pszName, PROPVARIANT * ppropvar)
{
    TRACE2("(%ls, %p)\n", pszName, ppropvar);
    if (pszName == nullptr)
        return E_INVALIDARG;
    if (ppropvar == nullptr)
        return E_INVALIDARG;

    PropVariantClear(ppropvar);
    for (const PropertyData& data : metadataStore_.metadata_) {
        if (StrCmpICW(data.name, pszName) == 0) {
            PropVariantCopy(ppropvar, &data.value);
            break;
        }
    }
    return S_OK;
}

HRESULT MetadataStore::NamedPropertyStore::SetNamedValue(LPCWSTR pszName, REFPROPVARIANT propvar)
{
    TRACE2("(%ls, %ls)\n", pszName, debugstr_var(propvar));
    if (pszName == nullptr)
        return E_INVALIDARG;

    for (PropertyData& data : metadataStore_.metadata_) {
        if (StrCmpICW(data.name, pszName) == 0) {
            PropVariantCopy(&data.value, &propvar);
            break;
        }
    }
    return S_OK;
}

HRESULT MetadataStore::NamedPropertyStore::GetNameCount(DWORD * pdwCount)
{
    TRACE1("(%p)\n", pdwCount);
    if (pdwCount == nullptr)
        return E_INVALIDARG;
    *pdwCount = metadataStore_.metadata_.size();
    return S_OK;
}

HRESULT MetadataStore::NamedPropertyStore::GetNameAt(DWORD iProp, BSTR * pbstrName)
{
    TRACE2("(%d, %p)\n", iProp, pbstrName);
    if (pbstrName == nullptr)
        return E_INVALIDARG;
    if (iProp >= metadataStore_.metadata_.size())
        return E_INVALIDARG;

    *pbstrName = SysAllocString(metadataStore_.metadata_[iProp].name);
    return S_OK;
}

HRESULT MetadataStore::PropertyStoreCapabilities::IsPropertyWritable(REFPROPERTYKEY key)
{
    return S_FALSE;
}
