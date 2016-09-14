#include <windows.h>
#include <cstdlib>
#include <memory>
#include <Shlwapi.h>
#include <propvarutil.h>
#include "decode_frame.h"
#include "dllmain.h"
#include "uuid.h"

#ifdef FLIF_DEBUG_LOGGING
#include "stopwatch.h"
#endif  // FLIF_DEBUG_LOGGING



DecodeFrame::DecodeFrame()
    : metadataBlockReader_(*this)
{
    TRACE("()\n");
    InitializeCriticalSection(&cs_);
}

void DecodeFrame::SetFlifImage(FLIF_IMAGE * image, UINT totalNumberOfImages)
{
    SectionLock l(&cs_);
    image_ = image;
    totalNumberOfImages_ = totalNumberOfImages;
}

DecodeFrame::~DecodeFrame()
{
    TRACE("()\n");
    DeleteCriticalSection(&cs_);
}


HRESULT DecodeFrame::QueryInterface(REFIID riid, void **ppvObject) {
    TRACE2("(%s, %p)\n", debugstr_guid(riid), ppvObject);

    if (ppvObject == nullptr)
        return E_INVALIDARG;
    *ppvObject = nullptr;

    if (IsEqualGUID(riid, IID_IUnknown) ||
        IsEqualGUID(riid, IID_IWICBitmapFrameDecode) ||
        IsEqualGUID(riid, IID_IWICBitmapSource))
    {
        this->AddRef();
        *ppvObject = static_cast<IWICBitmapFrameDecode*>(this);
        return S_OK;
    }

    if (IsEqualGUID(riid, IID_IWICMetadataBlockReader))
    {
        this->AddRef();
        *ppvObject = static_cast<IWICMetadataBlockReader*>(&this->metadataBlockReader_);
        return S_OK;
    }

    return E_NOINTERFACE;
}

HRESULT DecodeFrame::GetSize(UINT *puiWidth, UINT *puiHeight) {
    TRACE2("(%p, %p)\n", puiWidth, puiHeight);
    if (puiWidth == nullptr || puiHeight == nullptr)
        return E_INVALIDARG;
    *puiWidth = GetWidth();
    *puiHeight = GetHeight();
    TRACE2("ret: %u x %u\n", *puiWidth, *puiHeight);
    return S_OK;
}

HRESULT DecodeFrame::GetPixelFormat(WICPixelFormatGUID *pPixelFormat) {
    TRACE1("(%p)\n", pPixelFormat);
    if (pPixelFormat == nullptr)
        return E_INVALIDARG;
    //if (nb_channels >= 4) {
    *pPixelFormat = GUID_WICPixelFormat32bppRGBA;
    return S_OK;
}

HRESULT DecodeFrame::GetResolution(double *pDpiX, double *pDpiY) {
    TRACE2("(%p, %p)\n", pDpiX, pDpiY);
    if (pDpiX == nullptr)
        return E_INVALIDARG;
    if (pDpiY == nullptr)
        return E_INVALIDARG;
    // Let's assume square pixels. 96dpi seems to be a reasonable default.
    *pDpiX = 96;
    *pDpiY = 96;
    return S_OK;
}

HRESULT DecodeFrame::CopyPalette(IWICPalette *pIPalette) {
    TRACE1("(%p)\n", pIPalette);
    return WINCODEC_ERR_PALETTEUNAVAILABLE;
}

HRESULT DecodeFrame::CopyPixels(const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer) {
    TRACE4("(%p, %u, %u, %p)\n", prc, cbStride, cbBufferSize, pbBuffer);
    if (pbBuffer == nullptr)
        return E_INVALIDARG;
    uint32_t image_width = GetWidth();
    uint32_t image_height = GetHeight();
    uint8_t nb_channels = 4; // flif_image_get_nb_channels(image_);

    WICRect rect = { 0, 0, image_width, image_height };
    if (prc)
        rect = *prc;
    if (rect.Width < 0 || rect.Height < 0 || rect.X < 0 || rect.Y < 0)
        return E_INVALIDARG;
    if (rect.X + rect.Width > image_width ||
        rect.Y + rect.Height > image_height)
        return E_INVALIDARG;

    // Divisions instead of multiplications to avoid integer overflows:
    if (cbStride / nb_channels < static_cast<UINT>(rect.Width))
        return E_INVALIDARG;
    if (cbBufferSize / cbStride < static_cast<UINT>(rect.Height))
        return WINCODEC_ERR_INSUFFICIENTBUFFER;

    if (rect.Width == 0 || rect.Height == 0)
        return S_OK;

    size_t buffer_size = image_width*nb_channels;
    scoped_buffer temp_row(buffer_size);
    if (temp_row.alloc_failed()) {
        return WINCODEC_ERR_OUTOFMEMORY;
    }

    BYTE* dst_buffer = pbBuffer;
    const int x_offset = rect.X * nb_channels;
    const int width = rect.Width * nb_channels;

    for (int src_y = rect.Y; src_y < rect.Y + rect.Height; ++src_y) {
        flif_image_read_row_RGBA8(image_, src_y, temp_row.get(), buffer_size);
        memcpy(dst_buffer, temp_row.get() + x_offset, width);
        dst_buffer += cbStride;
    }
    return S_OK;
}

HRESULT DecodeFrame::GetMetadataQueryReader(IWICMetadataQueryReader **ppIMetadataQueryReader) {
    TRACE1("(%p)\n", ppIMetadataQueryReader);
    if (ppIMetadataQueryReader == nullptr)
        return E_INVALIDARG;

    HRESULT result = S_OK;
    //Create factory
    result = InitializeFactory();
    if (FAILED(result))
        return result;

    return componentFactory_->CreateQueryReaderFromBlockReader(static_cast<IWICMetadataBlockReader*>(&this->metadataBlockReader_), ppIMetadataQueryReader);
}

HRESULT DecodeFrame::GetColorContexts(UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount) {
    TRACE3("(%d, %p, %p)\n", cCount, ppIColorContexts, pcActualCount);
    if (pcActualCount == nullptr)
        return E_INVALIDARG;
    *pcActualCount = 0;
    return S_OK;
}

HRESULT DecodeFrame::GetThumbnail(IWICBitmapSource **ppIThumbnail) {
    TRACE1("(%p)\n", ppIThumbnail);
    return WINCODEC_ERR_CODECNOTHUMBNAIL;
}



HRESULT DecodeFrame::InitializeFactory()
{
    SectionLock l(&cs_);
    HRESULT result = S_OK;
    if (factory_.get() == nullptr)
    {
        result = CoCreateInstance(CLSID_WICImagingFactory,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_IWICImagingFactory,
            (LPVOID*)factory_.get_out_storage());
        if (FAILED(result))
            return result;
    }
    if (factory_.get() != nullptr &&
        componentFactory_.get() == nullptr)
    {
        result = factory_->QueryInterface(componentFactory_.get_out_storage());
        if (FAILED(result)) {
            return result;
        }
    }
    return result;
}

UINT DecodeFrame::GetWidth()
{
    return flif_image_get_width(image_);
}

UINT DecodeFrame::GetHeight()
{
    return flif_image_get_height(image_);
}

UINT DecodeFrame::GetDelay()
{
    return flif_image_get_frame_delay(image_);
}

HRESULT DecodeFrame::MetadataBlockReader::GetContainerFormat(GUID * pguidContainerFormat)
{
    TRACE1("(%p)\n", pguidContainerFormat);
    if (pguidContainerFormat == nullptr)
        return E_INVALIDARG;
    //Return container format of Jpeg so that "Photo Metadata Policies" will work.
    //https://msdn.microsoft.com/en-us/library/windows/desktop/ee872003(v=vs.85).aspx
    *pguidContainerFormat = GUID_ContainerFormatJpeg;
    //*pguidContainerFormat = GUID_ContainerFormatFLIF;
    return S_OK;
}

HRESULT DecodeFrame::MetadataBlockReader::GetCount(UINT * pcCount)
{
    TRACE1("(%p)\n", pcCount);
    if (pcCount == nullptr)
        return E_INVALIDARG;

    HRESULT result;

    //Create factory
    result = decodeFrame_.InitializeFactory();
    if (FAILED(result))
        return result;

    ReadAllMetadata();
    *pcCount = metadataReader_.size();
    return S_OK;
}

HRESULT DecodeFrame::MetadataBlockReader::GetReaderByIndex(UINT nIndex, IWICMetadataReader ** ppIMetadataReader)
{
    TRACE2("(%d, %p)\n", nIndex, ppIMetadataReader);
    if (ppIMetadataReader == nullptr)
        return E_INVALIDARG;
    if (nIndex >= metadataReader_.size())
        return E_INVALIDARG;

    *ppIMetadataReader = metadataReader_[nIndex].new_ref();
    return S_OK;
}

HRESULT DecodeFrame::MetadataBlockReader::GetEnumerator(IEnumUnknown ** ppIEnumMetadata)
{
    TRACE1("(%p)\n", ppIEnumMetadata);
    return E_NOTIMPL;
}

static
inline HRESULT InitPropVariantFromUInt8(_In_ UCHAR uiVal, _Out_ PROPVARIANT *ppropvar)
{
    ppropvar->vt = VT_UI1;
    ppropvar->bVal = uiVal;
    return S_OK;
}

void  DecodeFrame::MetadataBlockReader::ReadAllMetadata()
{
    TRACE("()\n");
    ReadMetadata(GUID_MetadataFormatExif, "eXif");
    ReadMetadata(GUID_MetadataFormatXMP, "eXmp");
    ReadMetadata(GUID_MetadataFormatChunkiCCP, "iCCP");

    if (decodeFrame_.totalNumberOfImages_ > 1)
    {
        ComPtr<IWICMetadataWriter> writer;

        if (SUCCEEDED(decodeFrame_.componentFactory_->CreateMetadataWriter(
            GUID_MetadataFormatIMD,
            nullptr,
            WICPersistOptionDefault,
            writer.get_out_storage())))
        {
            PROPVARIANT key;
            PROPVARIANT value;

            InitPropVariantFromString(L"Left", &key);
            InitPropVariantFromUInt16(0, &value);
            writer->SetValue(nullptr, &key, &value);
            PropVariantClear(&key);
            PropVariantClear(&value);

            InitPropVariantFromString(L"Top", &key);
            InitPropVariantFromUInt16(0, &value);
            writer->SetValue(nullptr, &key, &value);
            PropVariantClear(&key);
            PropVariantClear(&value);

            InitPropVariantFromString(L"Width", &key);
            InitPropVariantFromUInt16(decodeFrame_.GetWidth(), &value);
            writer->SetValue(nullptr, &key, &value);
            PropVariantClear(&key);
            PropVariantClear(&value);

            InitPropVariantFromString(L"Height", &key);
            InitPropVariantFromUInt16(decodeFrame_.GetHeight(), &value);
            writer->SetValue(nullptr, &key, &value);
            PropVariantClear(&key);
            PropVariantClear(&value);

            // Store for later use
            ComPtr<IWICMetadataReader> reader;
            if (SUCCEEDED(writer->QueryInterface(reader.get_out_storage())))
                metadataReader_.emplace_back(reader.new_ref());
        }

        if (SUCCEEDED(decodeFrame_.componentFactory_->CreateMetadataWriter(
            GUID_MetadataFormatGCE,
            nullptr,
            WICPersistOptionDefault,
            writer.get_out_storage())))
        {
            PROPVARIANT key;
            PROPVARIANT value;

            InitPropVariantFromString(L"Disposal", &key);
            InitPropVariantFromUInt8(0, &value);
            writer->SetValue(nullptr, &key, &value);
            PropVariantClear(&key);
            PropVariantClear(&value);

            InitPropVariantFromString(L"Delay", &key);
            InitPropVariantFromUInt16(decodeFrame_.GetDelay() / 10, &value);
            writer->SetValue(nullptr, &key, &value);
            PropVariantClear(&key);
            PropVariantClear(&value);

            InitPropVariantFromString(L"TransparencyFlag", &key);
            InitPropVariantFromBoolean(FALSE, &value);
            writer->SetValue(nullptr, &key, &value);
            PropVariantClear(&key);
            PropVariantClear(&value);

            // Store for later use
            ComPtr<IWICMetadataReader> reader;
            if (SUCCEEDED(writer->QueryInterface(reader.get_out_storage())))
                metadataReader_.emplace_back(reader.new_ref());
        }
    }
}

void DecodeFrame::MetadataBlockReader::ReadMetadata(GUID metadataFormat, const char* name)
{
    TRACE("()\n");
    uint8_t* data = nullptr;
    size_t length = 0;
    flif_image_get_metadata(decodeFrame_.image_, name, &data, &length);
    if (data && length > 0)
    {
        //Create stream
        ComPtr<IStream> stream(SHCreateMemStream(data, length));

        // Create reader of stream
        ComPtr<IWICMetadataReader> reader;
        if (SUCCEEDED(decodeFrame_.componentFactory_->CreateMetadataReader(
            metadataFormat,
            nullptr,
            WICPersistOptionDefault,
            stream.get(),
            reader.get_out_storage()))) {
            // Store Reader for later use
            metadataReader_.emplace_back(reader.new_ref());
        }
    }
    flif_image_free_metadata(decodeFrame_.image_, data);
}


