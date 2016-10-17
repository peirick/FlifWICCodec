#include <propsys.h>
#include <Propvarutil.h>
#include "metadata_store.h"
#include "decode_frame.h"


struct SupportedMetadata {
    SupportedMetadata(const PROPERTYKEY& key, LPCWSTR photo_metadata_policy, LPCWSTR xmp_metadata_query, bool is_writable)
        : key(key),
          photoMetadataPolicy(photo_metadata_policy),
          xmpMetadataQuery(xmp_metadata_query),
          isWritable(is_writable)
    {
    }

    const PROPERTYKEY& key;
    LPCWSTR            photoMetadataPolicy;
    LPCWSTR            xmpMetadataQuery;
    //LPCWSTR            exifMetadataQuery;
    const bool         isWritable;
};

static const
SupportedMetadata supported_metadatas[] = {
    // Using "Photo Metadata Policies" of Jpeg container. 
    // See also DecodeFrame::MetadataBlockReader::GetContainerFormat
    { PKEY_Title,                     L"System.Title",                L"/xmp/dc:title",           false  },
    { PKEY_Copyright,                 L"System.Copyright",            L"/xmp/dc:rights",          false  },
    { PKEY_Author,                    L"System.Author",               L"/xmp/<xmpseq>dc:creator", false  },
    { PKEY_Subject,                   L"System.Subject",              L"/ifd/{ushort=40095}	",    false  },
    { PKEY_Image_BitDepth,            nullptr,                nullptr,                            false  },
    { PKEY_Image_HorizontalSize,      nullptr,                nullptr,                            false  },
    { PKEY_Image_VerticalSize,        nullptr,                nullptr,                            false  },
    { PKEY_Image_Dimensions,          nullptr,                nullptr,                            false  },
    { PKEY_Rating,                    L"System.Rating",                   L"/xmp/xmp:Rating",     false  },
    { PKEY_Photo_CameraModel,         L"System.Photo.CameraModel",        L"/xmp/tiff:Model",     false  },
    { PKEY_Photo_CameraManufacturer,  L"System.Photo.CameraManufacturer", L"/xmp/tiff:Make",      false  },
    { PKEY_Photo_CameraSerialNumber,  L"System.Photo.CameraSerialNumber", L"/xmp/MicrosoftPhoto:CameraSerialNumber",      false  },

    { PKEY_Photo_Aperture,            L"System.Photo.Aperture",     L"/xmp/exif:ApertureValue",    false  },
    { PKEY_Photo_Brightness,          L"System.Photo.Brightness",   L"/xmp/exif:BrightnessValue",  false  },
    { PKEY_Photo_Contrast,            L"System.Photo.Contrast",     L"/xmp/exif:Contrast",         false  },
    { PKEY_Photo_DateTaken,           L"System.Photo.DateTaken",    L"/xmp/xmp:CreateDate",        false  },
    { PKEY_Photo_DigitalZoom,         L"System.Photo.DigitalZoom",  L"/xmp/exif:DigitalZoomRatio", false  },
    { PKEY_Photo_EXIFVersion,         L"System.Photo.EXIFVersion",  L"/xmp/exif:ExifVersion",      false  },
    { PKEY_Photo_ExposureBias,        L"System.Photo.ExposureBias", L"/xmp/exif:ExposureBiasValue", false  },
    { PKEY_Photo_ExposureTime,        L"System.Photo.ExposureTime", L"/xmp/exif:ExposureTime",     false  },

    { PKEY_GPS_Altitude,              L"System.GPS.Altitude",       L"/xmp/exif:GPSAltitude",      false  },
    { PKEY_GPS_Latitude,              L"System.GPS.Latitude",       L"/xmp/exif:GPSLatitude",    false  },
    { PKEY_GPS_Longitude,             L"System.GPS.Longitude" ,     L"/xmp/exif:GPSLongitude",    false  },
};

MetadataStore::MetadataStore()
    : initializeWithStream_(*this)
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

    if (IsEqualGUID(riid, IID_IInitializeWithStream))
    {
        this->AddRef();
        *ppvObject = static_cast<IInitializeWithStream*>(&this->initializeWithStream_);
        return S_OK;
    }

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
    return propertyStoreCache_.get() ? propertyStoreCache_->GetCount(cProps) : E_UNEXPECTED;
}

HRESULT MetadataStore::GetAt(DWORD iProp, PROPERTYKEY * pkey)
{
    TRACE2("(%d %p)\n", iProp, pkey);
    return  propertyStoreCache_.get() ? propertyStoreCache_->GetAt(iProp, pkey) : E_UNEXPECTED;
}

HRESULT MetadataStore::GetValue(REFPROPERTYKEY key, PROPVARIANT * pv)
{
    TRACE2("(%s, %p)\n", debugstr_guid(key.fmtid), pv);
    return  propertyStoreCache_.get() ? propertyStoreCache_->GetValue(key, pv) : E_UNEXPECTED;
}

HRESULT MetadataStore::SetValue(REFPROPERTYKEY key, REFPROPVARIANT propvar)
{
    TRACE2("(%s, %ls)\n", debugstr_guid(key.fmtid), debugstr_var(propvar));
    return propertyStoreCache_.get() ? propertyStoreCache_->SetValue(key, propvar) : E_UNEXPECTED;
}

HRESULT MetadataStore::Commit(void)
{
    TRACE("()\n");
    return S_OK;
}

HRESULT MetadataStore::InitializeWithStream::Initialize(IStream * pstream, DWORD grfMode)
{
    TRACE2("(%p, %d)\n", pstream, grfMode);
    HRESULT result;
    result = PSCreateMemoryPropertyStore(IID_PPV_ARGS(metadataStore_.propertyStoreCache_.get_out_storage()));
    if (FAILED(result))
        return result;

    DecodeContainer container;
    result = container.Initialize(pstream, WICDecodeMetadataCacheOnDemand);
    if (FAILED(result))
        return result;

    PROPVARIANT propvar;
    PropVariantInit(&propvar);

    // bitdepth
    InitPropVariantFromUInt32(container.GetBitDepth(), &propvar);
    metadataStore_.propertyStoreCache_->SetValueAndState(PKEY_Image_BitDepth, &propvar, PSC_NORMAL);
    PropVariantClear(&propvar);

    // width
    ULONG width = container.GetWidth();
    InitPropVariantFromUInt32(width, &propvar);
    metadataStore_.propertyStoreCache_->SetValueAndState(PKEY_Image_HorizontalSize, &propvar, PSC_NORMAL);
    PropVariantClear(&propvar);

    // height
    ULONG height = container.GetHeight();
    InitPropVariantFromUInt32(height, &propvar);
    metadataStore_.propertyStoreCache_->SetValueAndState(PKEY_Image_VerticalSize, &propvar, PSC_NORMAL);
    PropVariantClear(&propvar);

    // dimensions
    WCHAR buffer[64] = {};
    swprintf_s(buffer, L"%u \u00D7 %u", width, height);
    InitPropVariantFromString(buffer, &propvar);
    metadataStore_.propertyStoreCache_->SetValueAndState(PKEY_Image_Dimensions, &propvar, PSC_NORMAL);
    PropVariantClear(&propvar);


    ComPtr<IWICMetadataQueryReader> queryReader;
    result = container.GetMetadataQueryReader(queryReader.get_out_storage());
    if (FAILED(result))
        return result;
    for (const SupportedMetadata& supportedMetadata : supported_metadatas)
    {
        if (supportedMetadata.photoMetadataPolicy)
        {
            if (SUCCEEDED(queryReader->GetMetadataByName(supportedMetadata.photoMetadataPolicy, &propvar))
                && (propvar.vt != VT_EMPTY))
            {
                result = metadataStore_.propertyStoreCache_->SetValueAndState(supportedMetadata.key, &propvar, PSC_NORMAL);
            }
            PropVariantClear(&propvar);
        }
    }
    return S_OK;
}

HRESULT MetadataStore::PropertyStoreCapabilities::IsPropertyWritable(REFPROPERTYKEY key)
{
    TRACE1("(%s)\n", debugstr_guid(key.fmtid));
    for (const SupportedMetadata& supportedMetadata : supported_metadatas)
    {
        if (IsEqualGUID(supportedMetadata.key.fmtid, key.fmtid))
        {
            return supportedMetadata.isWritable
                ? S_OK
                : S_FALSE;
        }
    }
    return S_FALSE;
}
