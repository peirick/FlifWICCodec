#include "encode_container.h"
#include "encode_frame.h"
#include "uuid.h"
#include "pixel_converter.h"

EncodeContainer::EncodeContainer()
    : encoder_(nullptr), current_frame_(nullptr)
{
    TRACE("()\n");
    InitializeCriticalSection(&cs_);
}

EncodeContainer::~EncodeContainer()
{
    TRACE("()\n");
    if (encoder_) {
        flif_destroy_encoder(encoder_);
        encoder_ = nullptr;
    }
}

HRESULT EncodeContainer::QueryInterface(REFIID riid, void ** ppvObject)
{
    TRACE2("(%s, %p)\n", debugstr_guid(riid), ppvObject);

    if (ppvObject == nullptr)
        return E_INVALIDARG;
    *ppvObject = nullptr;

    if (!IsEqualGUID(riid, IID_IUnknown) && !IsEqualGUID(riid, IID_IWICBitmapEncoder))
        return E_NOINTERFACE;
    this->AddRef();
    *ppvObject = static_cast<IWICBitmapEncoder*>(this);
    return S_OK;
}

HRESULT EncodeContainer::Initialize(IStream * pIStream, WICBitmapEncoderCacheOption cacheOptions)
{
    TRACE2("(%p, %x)\n", pIStream, cacheOptions);
    if (pIStream == nullptr)
        return E_INVALIDARG;
    pIStream_ = pIStream;

    //reset encoder
    if (encoder_) {
        flif_destroy_encoder(encoder_);
        encoder_ = nullptr;
    }
    encoder_ = flif_create_encoder();
    return S_OK;
}

HRESULT EncodeContainer::GetContainerFormat(GUID * pguidContainerFormat)
{
    TRACE1("(%p)\n", pguidContainerFormat);
    if (pguidContainerFormat == nullptr)
        return E_INVALIDARG;
    *pguidContainerFormat = GUID_ContainerFormatFLIF;
    return S_OK;
}

HRESULT EncodeContainer::GetEncoderInfo(IWICBitmapEncoderInfo ** ppIEncoderInfo)
{
    TRACE1("(%p)\n", ppIEncoderInfo);
    HRESULT result;
    ComPtr<IWICImagingFactory> factory;

    {
        SectionLock l(&cs_);
        if (factory_.get() == nullptr) {
            result = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, (LPVOID*)factory_.get_out_storage());
            if (FAILED(result))
                return result;
        }
        factory.reset(factory_.new_ref());
    }

    ComPtr<IWICComponentInfo> compInfo;
    result = factory->CreateComponentInfo(CLSID_FLIFWICEncoder, compInfo.get_out_storage());
    if (FAILED(result))
        return result;

    result = compInfo->QueryInterface(IID_IWICBitmapEncoderInfo, (void**)ppIEncoderInfo);
    if (FAILED(result))
        return result;

    return S_OK;
}

HRESULT EncodeContainer::SetColorContexts(UINT cCount, IWICColorContext ** ppIColorContext)
{
    TRACE2("(%d, %p)\n", cCount, ppIColorContext);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;;
}

HRESULT EncodeContainer::SetPalette(IWICPalette * pIPalette)
{
    TRACE1("(%p)\n", pIPalette);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

HRESULT EncodeContainer::SetThumbnail(IWICBitmapSource * pIThumbnail)
{
    TRACE1("(%p)\n", pIThumbnail);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

HRESULT EncodeContainer::SetPreview(IWICBitmapSource * pIPreview)
{
    TRACE1("(%p)\n", pIPreview);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

HRESULT EncodeContainer::CreateNewFrame(IWICBitmapFrameEncode ** ppIFrameEncode, IPropertyBag2 ** ppIEncoderOptions)
{
    TRACE2("(%p, %p)\n", ppIFrameEncode, ppIEncoderOptions);
    if (ppIFrameEncode == nullptr)
        return E_INVALIDARG;

    if (encoder_ == nullptr) {
        return WINCODEC_ERR_NOTINITIALIZED;
    }

    SectionLock l(&cs_);

    ComPtr<EncodeFrame> output;
    output.reset(new (std::nothrow) EncodeFrame(this));
    *ppIFrameEncode = output.new_ref();
    return S_OK;
}

HRESULT EncodeContainer::Commit(void)
{
    TRACE("()\n");
    if (encoder_ == nullptr) {
        return WINCODEC_ERR_NOTINITIALIZED;
    }

    if (current_frame_.get())
    {
        uint8_t* buffer = nullptr;
        size_t buffer_size = 0;
        if (flif_encoder_encode_memory(encoder_, reinterpret_cast<void**>(&buffer), &buffer_size) != 0) {
            ULONG written = 0;
            do {
                pIStream_->Write(buffer, buffer_size, &written);
                buffer_size -= written;
                buffer += written;
            } while (buffer_size > 0);
        }
    }
    return S_OK;
}

HRESULT EncodeContainer::GetMetadataQueryWriter(IWICMetadataQueryWriter ** ppIMetadataQueryWriter)
{
    TRACE1("(%p)\n", ppIMetadataQueryWriter);
    if (ppIMetadataQueryWriter == nullptr)
        return E_INVALIDARG;
    return E_NOTIMPL;
}

HRESULT EncodeContainer::AddImage(std::shared_ptr<RawFrame> frame, AnimationInformation animation_information, std::deque<std::shared_ptr<Metadata>> metadata)
{
    if (encoder_ == nullptr) {
        return WINCODEC_ERR_NOTINITIALIZED;
    }

    //Merge current frame
    if (current_frame_.get()) {
        if (current_frame_->NumberComponents != frame->NumberComponents) {
            return WINCODEC_ERR_INTERNALERROR;
        }

        if (animation_information.TransparencyFlag && frame->NumberComponents == 4) {
            //Must be RGBA and Alpha channel has only values 0 or FF
            assert(frame->NumberComponents == 4);
            for (UINT i = 0; i < frame->Height; ++i) {
                BYTE* srcrow = frame->Buffer + i * frame->Stride;
                BYTE* destrow = current_frame_->Buffer + (i + animation_information.Top) * current_frame_->Stride;
                BYTE* destrowstart = destrow + animation_information.Left * current_frame_->NumberComponents;
                CopyAllButTransparentPixelRGBA8(frame->Width, srcrow, destrowstart);
            }
        }
        else {
            //Must be RGB or Gray
            assert(frame->NumberComponents < 4);
            for (UINT i = 0; i < frame->Height; ++i) {
                BYTE* srcrow = frame->Buffer + i * frame->Stride;
                BYTE* destrow = current_frame_->Buffer + (i + animation_information.Top) * current_frame_->Stride;
                BYTE* destrowstart = destrow + animation_information.Left * current_frame_->NumberComponents;
                memcpy(destrowstart, srcrow, frame->Width*frame->NumberComponents);
            }
        }
    }
    else
    {
        current_frame_ = frame;
    }

    //Write and encode rows
    FLIF_IMAGE* image = nullptr;
    if (current_frame_->NumberComponents == 1) {
        image = flif_import_image_GRAY(current_frame_->Width, current_frame_->Height, current_frame_->Buffer, current_frame_->Stride);
    }
    else if (current_frame_->NumberComponents == 3) {
        image = flif_import_image_RGB(current_frame_->Width, current_frame_->Height, current_frame_->Buffer, current_frame_->Stride);
    }
    else if (current_frame_->NumberComponents == 4) {
        image = flif_import_image_RGBA(current_frame_->Width, current_frame_->Height, current_frame_->Buffer, current_frame_->Stride);
    }

    if (image == nullptr)
        return WINCODEC_ERR_INTERNALERROR;

    //Animation
    if (animation_information.Delay > 0) {
        flif_image_set_frame_delay(image, animation_information.Delay);
    }

    //Metadata
    if (metadata.size() > 0) {
        for (int i = 0; i < metadata.size(); ++i) {
            flif_image_set_metadata(image, metadata[i]->Chunkname.c_str(), metadata[i]->Buffer, metadata[i]->BufferSize);
        }
    }

    flif_encoder_add_image(encoder_, image);
    flif_destroy_image(image);

    return S_OK;
}

RawFrame::RawFrame(uint32_t width, uint32_t height, uint32_t numberComponents, uint32_t stride)
    : Width(width), Height(height), NumberComponents(numberComponents), Stride(stride),
    BufferSize(stride*height), Buffer((uint8_t*)CoTaskMemAlloc(BufferSize))
{
}

RawFrame::~RawFrame()
{
    if (Buffer) {
        CoTaskMemFree(Buffer);
    }
}

Metadata::Metadata(std::string chunkname, size_t bufferSize)
    : Chunkname(chunkname), BufferSize(bufferSize), Buffer((uint8_t*)CoTaskMemAlloc(BufferSize))
{
}

Metadata::~Metadata()
{
    if (Buffer) {
        CoTaskMemFree(Buffer);
    }
}