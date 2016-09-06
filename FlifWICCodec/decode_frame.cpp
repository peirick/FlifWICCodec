#include <windows.h>
#include <cstdlib>
#include <memory>
#include <Shlwapi.h>
#include "decode_frame.h"
#include "dllmain.h"
#include "uuid.h"

#ifdef FLIF_DEBUG_LOGGING
#include "stopwatch.h"
#endif  // FLIF_DEBUG_LOGGING


//const int kBytesPerPixel = 4;

DecodeFrame::DecodeFrame(FLIF_IMAGE * image) :image_(image)
{
    TRACE("()\n");
}

DecodeFrame::~DecodeFrame()
{
    TRACE("()\n");
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

    // Multiple inheritance needs explicit cast
    if (IsEqualGUID(riid, IID_IWICMetadataBlockReader))
    {
        this->AddRef();
        *ppvObject = static_cast<IWICMetadataBlockReader*>(this);
        return S_OK;
    }

    return E_NOINTERFACE;
}

HRESULT DecodeFrame::GetSize(UINT *puiWidth, UINT *puiHeight) {
    TRACE2("(%p, %p)\n", puiWidth, puiHeight);
    if (puiWidth == nullptr || puiHeight == nullptr)
        return E_INVALIDARG;
    *puiWidth = flif_image_get_width(image_);
    *puiHeight = flif_image_get_height(image_);
    TRACE2("ret: %u x %u\n", *puiWidth, *puiHeight);
    return S_OK;
}

HRESULT DecodeFrame::GetPixelFormat(WICPixelFormatGUID *pPixelFormat) {
    TRACE1("(%p)\n", pPixelFormat);
    if (pPixelFormat == nullptr)
        return E_INVALIDARG;
    //uint8_t nb_channels = flif_image_get_nb_channels(image_);
    //if (nb_channels >= 4) {
    *pPixelFormat = GUID_WICPixelFormat32bppRGBA;
    //}
    //else if (nb_channels == 3) {
    //	*pPixelFormat = GUID_WICPixelFormat24bppRGB;
    //}
    //else if (nb_channels == 1) {
    //	*pPixelFormat = GUID_WICPixelFormat8bppGray;
    //}
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
    uint32_t image_width = flif_image_get_width(image_);
    uint32_t image_height = flif_image_get_height(image_);
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
    //uint32_t image_width = flif_image_get_width(image_);
    //uint32_t image_height = flif_image_get_height(image_);
    //uint32_t frame_delay = flif_image_get_frame_delay(image_);
    //*ppIMetadataQueryReader = new DecodeMetadataQueryReader(image_width, image_height, frame_delay)
    //return S_OK;

    //Create factory
    ComPtr<IWICImagingFactory> factory;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, (LPVOID*)factory.get_out_storage())))
        return S_FALSE;

    ComPtr<IWICComponentFactory> componentFactory;
    if (FAILED(factory->QueryInterface(componentFactory.get_out_storage())))
        return S_FALSE;

    return componentFactory->CreateQueryReaderFromBlockReader(static_cast<IWICMetadataBlockReader*>(this), ppIMetadataQueryReader);
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

HRESULT DecodeFrame::GetContainerFormat(GUID * pguidContainerFormat)
{
    TRACE1("(%p)\n", pguidContainerFormat);
    if (pguidContainerFormat == nullptr)
        return E_INVALIDARG;
    *pguidContainerFormat = GUID_ContainerFormatFLIF;
    return S_OK;
}

void DecodeFrame::ReadMetadata(GUID metadataFormat, const char* name)
{
    uint8_t* data = nullptr;
    size_t length = 0;
    flif_image_get_metadata(image_, name, &data, &length);
    if (data && length > 0)
    {
        //Create stream
        ComPtr<IStream> stream(SHCreateMemStream(data, length));

        //Create factory
        ComPtr<IWICImagingFactory> factory;
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, (LPVOID*)factory.get_out_storage())))
            return;
        ComPtr<IWICComponentFactory> componentFactory;
        if (FAILED(factory->QueryInterface(componentFactory.get_out_storage())))
            return;

        // Create reader of stream
        ComPtr<IWICMetadataReader> reader;
        if (FAILED(componentFactory->CreateMetadataReader(metadataFormat, nullptr, WICPersistOptionDefault, stream.get(), reader.get_out_storage())))
            return;

        // Store Reader for later use
        metadataReader_.emplace_back(reader.new_ref());
    }
}

HRESULT DecodeFrame::GetCount(UINT * pcCount)
{
    TRACE1("(%p)\n", pcCount);
    if (pcCount == nullptr)
        return E_INVALIDARG;

    ReadMetadata(GUID_MetadataFormatExif, "eXif");
    ReadMetadata(GUID_MetadataFormatXMP, "eXmp");
    ReadMetadata(GUID_MetadataFormatChunkiCCP, "iCCP");
    *pcCount = metadataReader_.size();
    return S_OK;
}

HRESULT DecodeFrame::GetReaderByIndex(UINT nIndex, IWICMetadataReader ** ppIMetadataReader)
{
    TRACE2("(%d, %p)\n", nIndex, ppIMetadataReader);
    if (ppIMetadataReader == nullptr)
        return E_INVALIDARG;
    if (nIndex >= metadataReader_.size())
        return E_INVALIDARG;

    *ppIMetadataReader = metadataReader_[nIndex].new_ref();
    return S_OK;
}

HRESULT DecodeFrame::GetEnumerator(IEnumUnknown ** ppIEnumMetadata)
{
    TRACE1("(%p)\n", ppIEnumMetadata);
    return E_NOTIMPL;
}

