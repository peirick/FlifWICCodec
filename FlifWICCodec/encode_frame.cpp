#include <Shlwapi.h>
#include "encode_frame.h"
#include "pixel_converter.h"
#include "uuid.h"

EncodeFrame::EncodeFrame(EncodeContainer* container)
    : container_(container), frame_(nullptr), metadataBlockWriter_(*this)
{
    TRACE("()\n");
    container_->AddRef();
    InitializeCriticalSection(&cs_);
}

EncodeFrame::~EncodeFrame()
{
    TRACE("()\n");
    container_->Release();
    DeleteCriticalSection(&cs_);
}


HRESULT EncodeFrame::QueryInterface(REFIID riid, void ** ppvObject)
{
    TRACE2("(%s, %p)\n", debugstr_guid(riid), ppvObject);

    if (ppvObject == nullptr)
        return E_INVALIDARG;
    *ppvObject = nullptr;

    if (IsEqualGUID(riid, IID_IUnknown) ||
        IsEqualGUID(riid, IID_IWICBitmapFrameEncode))
    {
        this->AddRef();
        *ppvObject = static_cast<IWICBitmapFrameEncode*>(this);
        return S_OK;
    }

    // Multiple inheritence needs explicit cast
    if (IsEqualGUID(riid, IID_IWICMetadataBlockWriter) ||
        IsEqualGUID(riid, IID_IWICMetadataBlockReader))
    {
        this->AddRef();
        *ppvObject = static_cast<IWICMetadataBlockWriter*>(&this->metadataBlockWriter_);
        return S_OK;
    }
    return E_NOINTERFACE;
}

HRESULT EncodeFrame::Initialize(IPropertyBag2 * pIEncoderOptions)
{
    TRACE1("(%p)\n", pIEncoderOptions);
    return S_OK;
}

HRESULT EncodeFrame::SetSize(UINT uiWidth, UINT uiHeight)
{
    TRACE2("(%d, %d)\n", uiWidth, uiHeight);
    return S_OK;
}

HRESULT EncodeFrame::SetResolution(double dpiX, double dpiY)
{
    TRACE2("(%f, %f)\n", dpiX, dpiY);
    return S_OK;
}

HRESULT EncodeFrame::SetPixelFormat(WICPixelFormatGUID * pPixelFormat)
{
    TRACE1("(%p)\n", pPixelFormat);
    if (pPixelFormat == nullptr)
        return E_INVALIDARG;

    //supported nativly
    if (*pPixelFormat == GUID_WICPixelFormat32bppRGBA ||
        *pPixelFormat == GUID_WICPixelFormat24bppRGB ||
        *pPixelFormat == GUID_WICPixelFormat8bppGray)
    {
        return S_OK;
    }

    //supported through converter
    if (*pPixelFormat == GUID_WICPixelFormatBlackWhite ||
        *pPixelFormat == GUID_WICPixelFormat2bppGray ||
        *pPixelFormat == GUID_WICPixelFormat4bppGray ||
        *pPixelFormat == GUID_WICPixelFormat32bppBGRA ||
        *pPixelFormat == GUID_WICPixelFormat16bppBGRA5551 ||
        *pPixelFormat == GUID_WICPixelFormat16bppBGR555 ||
        *pPixelFormat == GUID_WICPixelFormat16bppBGR565 ||
        *pPixelFormat == GUID_WICPixelFormat24bppBGR ||
        *pPixelFormat == GUID_WICPixelFormat32bppRGB ||
        *pPixelFormat == GUID_WICPixelFormat32bppBGR ||
        *pPixelFormat == GUID_WICPixelFormat1bppIndexed ||
        *pPixelFormat == GUID_WICPixelFormat2bppIndexed ||
        *pPixelFormat == GUID_WICPixelFormat4bppIndexed ||
        *pPixelFormat == GUID_WICPixelFormat8bppIndexed)
    {
        return S_OK;
    }

    *pPixelFormat = GUID_WICPixelFormatUndefined;
    return WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT;
}

HRESULT EncodeFrame::SetColorContexts(UINT cCount, IWICColorContext ** ppIColorContext)
{
    TRACE2("(%d, %p)\n", cCount, ppIColorContext);
    return E_NOTIMPL;
}

HRESULT EncodeFrame::SetPalette(IWICPalette * pIPalette)
{
    TRACE1("(%p)\n", pIPalette);
    return E_NOTIMPL;
}

HRESULT EncodeFrame::SetThumbnail(IWICBitmapSource * pIThumbnail)
{
    TRACE1("(%p)\n", pIThumbnail);
    return E_NOTIMPL;
}

HRESULT EncodeFrame::WritePixels(UINT lineCount, UINT cbStride, UINT cbBufferSize, BYTE * pbPixels)
{
    TRACE4("(%d, %d, %d, %p)\n", lineCount, cbStride, cbBufferSize, pbPixels);
    return E_NOTIMPL;
}

HRESULT EncodeFrame::WriteSource(IWICBitmapSource* pIBitmapSource, WICRect * prc)
{
    TRACE2("(%p, %p)\n", pIBitmapSource, prc);
    if (pIBitmapSource == nullptr)
        return E_INVALIDARG;

    HRESULT result = S_OK;

    //For GIFs get animation information
    animation_information_ = {};
    ComPtr<IWICBitmapFrameDecode> decodeframe;
    if (SUCCEEDED(pIBitmapSource->QueryInterface(decodeframe.get_out_storage()))) {
        ComPtr<IWICMetadataQueryReader> metadataReader;
        if (SUCCEEDED(decodeframe->GetMetadataQueryReader(metadataReader.get_out_storage())))
        {
            PROPVARIANT propValue;
            PropVariantInit(&propValue);

            if (SUCCEEDED(metadataReader->GetMetadataByName(L"/imgdesc/Left", &propValue))) {
                if (propValue.vt == VT_UI2) {
                    animation_information_.Left = propValue.uiVal;
                }
            }
            PropVariantClear(&propValue);

            if (SUCCEEDED(metadataReader->GetMetadataByName(L"/imgdesc/Top", &propValue))) {
                if (propValue.vt == VT_UI2) {
                    animation_information_.Top = propValue.uiVal;
                }
            }
            PropVariantClear(&propValue);

            if (SUCCEEDED(metadataReader->GetMetadataByName(L"/grctlext/Disposal", &propValue))) {
                if (propValue.vt == VT_UI1) {
                    animation_information_.Disposal = propValue.bVal;
                }
            }
            PropVariantClear(&propValue);

            if (SUCCEEDED(metadataReader->GetMetadataByName(L"/grctlext/Delay", &propValue))) {
                if (propValue.vt == VT_UI2) {
                    // From 10th ms to ms
                    animation_information_.Delay = propValue.uiVal * 10;
                }
            }
            PropVariantClear(&propValue);

            if (SUCCEEDED(metadataReader->GetMetadataByName(L"/grctlext/TransparencyFlag", &propValue))) {
                if (propValue.vt == VT_BOOL) {
                    animation_information_.TransparencyFlag = propValue.boolVal;
                }
            }
            PropVariantClear(&propValue);
        }
    }

    //Check Rect
    UINT image_width = 0;
    UINT image_height = 0;
    result = pIBitmapSource->GetSize(&image_width, &image_height);
    if (FAILED(result)) {
        return result;
    }
    WICRect rect = { 0, 0, image_width, image_height };
    if (prc)
        rect = *prc;
    if (rect.Width < 0 || rect.Height < 0 || rect.X < 0 || rect.Y < 0)
        return E_INVALIDARG;
    if (rect.X + rect.Width > image_width ||
        rect.Y + rect.Height > image_height)
        return E_INVALIDARG;

    //Convert Image
    WICPixelFormatGUID source_pixel_format = { 0 };
    result = pIBitmapSource->GetPixelFormat(&source_pixel_format);
    if (FAILED(result)) {
        return result;
    }
    IWICBitmapSource* source_image = pIBitmapSource;
    ComPtr<IWICFormatConverter> converter;
    if (source_pixel_format != GUID_WICPixelFormat32bppRGBA &&
        source_pixel_format != GUID_WICPixelFormat24bppRGB &&
        source_pixel_format != GUID_WICPixelFormat8bppGray)
    {
        //Create factory
        result = InitializeFactory();
        if (FAILED(result)) {
            return result;
        }

        //Set destination pixelformat
        WICPixelFormatGUID dest_pixel_format = { 0 };
        if (source_pixel_format == GUID_WICPixelFormatBlackWhite ||
            source_pixel_format == GUID_WICPixelFormat2bppGray ||
            source_pixel_format == GUID_WICPixelFormat4bppGray)
        {
            dest_pixel_format = GUID_WICPixelFormat8bppGray;
        }
        else if (source_pixel_format == GUID_WICPixelFormat32bppBGRA ||
            source_pixel_format == GUID_WICPixelFormat16bppBGRA5551)
        {
            dest_pixel_format = GUID_WICPixelFormat32bppRGBA;
        }
        else if (
            source_pixel_format == GUID_WICPixelFormat16bppBGR555 ||
            source_pixel_format == GUID_WICPixelFormat16bppBGR565 ||
            source_pixel_format == GUID_WICPixelFormat24bppBGR ||
            source_pixel_format == GUID_WICPixelFormat32bppBGR ||
            source_pixel_format == GUID_WICPixelFormat32bppRGB)
        {
            dest_pixel_format = GUID_WICPixelFormat24bppRGB;
        }
        else if (source_pixel_format == GUID_WICPixelFormat1bppIndexed ||
            source_pixel_format == GUID_WICPixelFormat2bppIndexed ||
            source_pixel_format == GUID_WICPixelFormat4bppIndexed ||
            source_pixel_format == GUID_WICPixelFormat8bppIndexed)
        {
            //Set destination pixelformat from palette info
            ComPtr<IWICPalette> palette;
            result = factory_->CreatePalette(palette.get_out_storage());
            if (FAILED(result)) {
                return result;
            }
            result = pIBitmapSource->CopyPalette(palette.get());
            if (FAILED(result)) {
                return result;
            }
            dest_pixel_format = GUID_WICPixelFormat24bppRGB;
            BOOL HasAlpha = false;
            result = palette->HasAlpha(&HasAlpha);
            if (FAILED(result)) {
                return result;
            }
            if (HasAlpha) {
                dest_pixel_format = GUID_WICPixelFormat32bppRGBA;
            }
            else
            {
                BOOL is_grayscale = false;
                result = palette->IsGrayscale(&is_grayscale);
                if (FAILED(result)) {
                    return result;
                }
                BOOL is_blackwhite = false;
                result = palette->IsBlackWhite(&is_blackwhite);
                if (FAILED(result)) {
                    return result;
                }
                if (is_grayscale || is_blackwhite) {
                    dest_pixel_format = GUID_WICPixelFormat8bppGray;
                }
            }
        }
        else
        {
            return WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT;
        }

        //Create Converter		
        result = factory_->CreateFormatConverter(converter.get_out_storage());
        if (FAILED(result)) {
            return result;
        }
        BOOL can_convert = false;
        result = converter->CanConvert(source_pixel_format, dest_pixel_format, &can_convert);
        if (FAILED(result))
        {
            return result;
        }
        if (!can_convert) {
            return WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT;
        }
        result = converter->Initialize(
            pIBitmapSource, dest_pixel_format,
            WICBitmapDitherTypeNone, nullptr, 0.f,
            WICBitmapPaletteTypeMedianCut);
        if (FAILED(result)) {
            return result;
        }
        source_image = converter.get();
    }

    //Create raw frame
    result = source_image->GetPixelFormat(&source_pixel_format);
    if (FAILED(result)) {
        return result;
    }
    assert(source_pixel_format == GUID_WICPixelFormat32bppRGBA ||
        source_pixel_format == GUID_WICPixelFormat24bppRGB ||
        source_pixel_format == GUID_WICPixelFormat8bppGray);

    if (source_pixel_format == GUID_WICPixelFormat32bppRGBA)
    {
        uint32_t stride = rect.Width * 4;
        frame_.reset(new RawFrame(rect.Width, rect.Height, 4, stride));
    }
    else if (source_pixel_format == GUID_WICPixelFormat24bppRGB)
    {
        uint32_t stride = 4 * ((24 * (UINT)rect.Width + 31) / 32);
        frame_.reset(new RawFrame(rect.Width, rect.Height, 3, stride));
    }
    else if (source_pixel_format == GUID_WICPixelFormat8bppGray)
    {
        uint32_t stride = rect.Width;
        frame_.reset(new RawFrame(rect.Width, rect.Height, 1, stride));
    }
    else
    {
        return WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT;
    }
    if (frame_->Buffer == nullptr)
    {
        frame_.reset();
        return E_OUTOFMEMORY;
    }

    //Read Source Pixel
    result = source_image->CopyPixels(&rect, frame_->Stride, frame_->BufferSize, frame_->Buffer);
    if (FAILED(result))
    {
        frame_.reset();
        return result;
    }


    return S_OK;

}

HRESULT EncodeFrame::Commit(void)
{
    TRACE("()\n");
    if (!frame_.get())
        return WINCODEC_ERR_NOTINITIALIZED;

    HRESULT result = S_OK;

    std::deque<std::shared_ptr<Metadata>> metadatas;
    result = metadataBlockWriter_.GetMetadatas(metadatas);
    if (FAILED(result))
        return result;

    result = container_->AddImage(frame_, animation_information_, metadatas);
    frame_.reset();
    return result;
}

HRESULT EncodeFrame::InitializeFactory()
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

HRESULT EncodeFrame::GetMetadataQueryWriter(IWICMetadataQueryWriter ** ppIMetadataQueryWriter)
{
    TRACE1("(%p)\n", ppIMetadataQueryWriter);
    if (ppIMetadataQueryWriter == nullptr)
        return E_INVALIDARG;

    HRESULT result;

    //Create factory   
    result = InitializeFactory();
    if (FAILED(result))
        return result;

    return  componentFactory_->CreateQueryWriterFromBlockWriter(static_cast<IWICMetadataBlockWriter*>(&this->metadataBlockWriter_), ppIMetadataQueryWriter);
}

HRESULT EncodeFrame::MetadataBlockWriter::GetContainerFormat(GUID * pguidContainerFormat)
{
    TRACE1("(%p)\n", pguidContainerFormat);
    if (pguidContainerFormat == nullptr)
        return E_INVALIDARG;
    *pguidContainerFormat = GUID_ContainerFormatFLIF;
    return S_OK;
}

HRESULT EncodeFrame::MetadataBlockWriter::GetCount(UINT * pcCount)
{
    TRACE1("(%p)\n", pcCount);
    if (pcCount == nullptr)
        return E_INVALIDARG;
    *pcCount = metadataWriter_.size();
    return S_OK;
}

HRESULT EncodeFrame::MetadataBlockWriter::GetEnumerator(IEnumUnknown ** ppIEnumMetadata)
{
    TRACE1("(%p)\n", ppIEnumMetadata);
    return E_NOTIMPL;
}

HRESULT EncodeFrame::MetadataBlockWriter::InitializeFromBlockReader(IWICMetadataBlockReader * pIMDBlockReader)
{
    TRACE1("(%p)\n", pIMDBlockReader);
    if (pIMDBlockReader == nullptr)
        return E_INVALIDARG;

    HRESULT result;

    UINT blockCount = 0;
    result = pIMDBlockReader->GetCount(&blockCount);
    if (FAILED(result))
        return result;

    if (blockCount > 0)
    {
        //Create factory   
        result = encodeFrame_.InitializeFactory();
        if (FAILED(result))
            return result;

        for (UINT i = 0; i < blockCount; ++i)
        {
            IWICMetadataReader* metadataReader;
            if (SUCCEEDED(pIMDBlockReader->GetReaderByIndex(i, &metadataReader)))
            {
                ComPtr<IWICMetadataWriter> metadataWriter;
                if (SUCCEEDED(encodeFrame_.componentFactory_->CreateMetadataWriterFromReader(metadataReader, NULL, metadataWriter.get_out_storage())))
                {
                    metadataWriter_.emplace_back(metadataWriter.new_ref());
                }
            }
        }
    }
    return S_OK;
}

HRESULT EncodeFrame::MetadataBlockWriter::GetReaderByIndex(UINT nIndex, IWICMetadataReader ** ppIMetadataReader)
{
    TRACE2("(%d, %p)\n", nIndex, ppIMetadataReader);
    if (ppIMetadataReader == nullptr)
        return E_INVALIDARG;
    if (nIndex >= metadataWriter_.size())
        return E_INVALIDARG;
    *ppIMetadataReader = static_cast<IWICMetadataReader*>(metadataWriter_[nIndex].new_ref());
    return S_OK;
}

HRESULT EncodeFrame::MetadataBlockWriter::GetWriterByIndex(UINT nIndex, IWICMetadataWriter ** ppIMetadataWriter)
{
    TRACE2("(%d, %p)\n", nIndex, ppIMetadataWriter);
    if (ppIMetadataWriter == nullptr)
        return E_INVALIDARG;
    if (nIndex >= metadataWriter_.size())
        return E_INVALIDARG;
    *ppIMetadataWriter = metadataWriter_[nIndex].new_ref();
    return S_OK;
}

HRESULT EncodeFrame::MetadataBlockWriter::AddWriter(IWICMetadataWriter * pIMetadataWriter)
{
    TRACE1("(%p)\n", pIMetadataWriter);
    if (pIMetadataWriter == nullptr)
        return E_INVALIDARG;
    metadataWriter_.emplace_back(pIMetadataWriter);
    return S_OK;
}

HRESULT EncodeFrame::MetadataBlockWriter::SetWriterByIndex(UINT nIndex, IWICMetadataWriter * pIMetadataWriter)
{
    TRACE2("(%d, %p)\n", nIndex, pIMetadataWriter);
    if (pIMetadataWriter == nullptr)
        return E_INVALIDARG;
    if (nIndex >= metadataWriter_.size())
        return E_INVALIDARG;
    metadataWriter_[nIndex].reset(pIMetadataWriter);
    return S_OK;
}

HRESULT EncodeFrame::MetadataBlockWriter::RemoveWriterByIndex(UINT nIndex)
{
    TRACE1("(%d)\n", nIndex);
    if (nIndex >= metadataWriter_.size())
        return E_INVALIDARG;
    //metadataWriter_.erase(metadataWriter_.begin() + nIndex);
    return S_OK;
}


static
HRESULT SaveMetadata(IWICMetadataReader* reader,
    const std::string metadataName,
    std::deque<std::shared_ptr<Metadata>>& metadatas)
{
    ComPtr<IWICPersistStream> persistStream;
    if (SUCCEEDED(reader->QueryInterface(persistStream.get_out_storage())))
    {
        IStream* stream = SHCreateMemStream(NULL, 0);
        if (SUCCEEDED(persistStream->SaveEx(stream, WICMetadataCreationAllowUnknown | WICPersistOptionDefault, FALSE))) {
            // Allocates enough memeory for the content.
            STATSTG ssStreamData = {};
            if (SUCCEEDED(stream->Stat(&ssStreamData, STATFLAG_NONAME))) {
                SIZE_T cbSize = ssStreamData.cbSize.LowPart;
                std::shared_ptr<Metadata> metadata(new (std::nothrow) Metadata(metadataName, cbSize));
                if (!metadata->Buffer)
                    return E_OUTOFMEMORY;

                // Copies the content from the stream to the buffer.
                LARGE_INTEGER position;
                position.QuadPart = 0;
                if (SUCCEEDED((stream->Seek(position, STREAM_SEEK_SET, NULL)))) {
                    ULONG cbRead;
                    if (SUCCEEDED(stream->Read(metadata->Buffer, cbSize, &cbRead))) {
                        metadatas.emplace_back(metadata);
                    }
                }
            }
        }
    }
    return S_OK;
}

static
void ReadMetadataReqursive(IWICMetadataReader* reader, std::deque<std::shared_ptr<Metadata>>& metadatas)
{
    HRESULT result = S_OK;

    GUID metadataFormat;
    if (SUCCEEDED(reader->GetMetadataFormat(&metadataFormat)))
    {
        if (IsEqualGUID(metadataFormat, GUID_MetadataFormatExif))
        {
            SaveMetadata(reader, "eXif", metadatas);
            return;
        }
        else if (IsEqualGUID(metadataFormat, GUID_MetadataFormatXMP)) {
            SaveMetadata(reader, "eXmp", metadatas);
            return;
        }
        else if (IsEqualGUID(metadataFormat, GUID_MetadataFormatChunkiCCP))
        {
            SaveMetadata(reader, "iCCP", metadatas);
            return;
        }
    }

    UINT count = 0;
    result = reader->GetCount(&count);
    if (FAILED(result))
        return;
    for (UINT i = 0; i < count; ++i)
    {
        PROPVARIANT id, value;

        PropVariantInit(&id);
        PropVariantInit(&value);
        result = reader->GetValueByIndex(i, nullptr, &id, &value);
        if (SUCCEEDED(result)) {
            if (VT_UNKNOWN == value.vt)
            {
                ComPtr<IWICMetadataReader> subReader;
                result = value.punkVal->QueryInterface(subReader.get_out_storage());
                if (SUCCEEDED(result))
                {
                    ReadMetadataReqursive(subReader.get(), metadatas);
                }
            }
            PropVariantClear(&id);
            PropVariantClear(&value);
        }
    }
}

HRESULT  EncodeFrame::MetadataBlockWriter::GetMetadatas(std::deque<std::shared_ptr<Metadata>>& metadatas)
{
    TRACE("()\n");
    for (int i = 0; i < metadataWriter_.size(); ++i)
    {
        ReadMetadataReqursive(metadataWriter_[i].get(), metadatas);
    }
    return S_OK;
}
