#include <windows.h>
#include "dllmain.h"
#include "uuid.h"
#include "utils.h"
#include "decode_container.h"
#include "decode_frame.h"

DecodeContainer::DecodeContainer()
    : has_been_decoded_(false),
    has_only_infos_decoded_(false),
    last_decode_desult(0),
    info_(nullptr),
    decoder_(nullptr)
{
    TRACE("()\n");
    InitializeCriticalSection(&cs_);
}

DecodeContainer::~DecodeContainer()
{
    TRACE("()\n");
    if (info_) {
        flif_destroy_info(info_);
        info_ = nullptr;
    }
    if (decoder_) {
        flif_destroy_decoder(decoder_);
        decoder_ = nullptr;
    }
}

HRESULT DecodeContainer::QueryInterface(REFIID riid, void** ppvObject) {
    TRACE2("(%s, %p)\n", debugstr_guid(riid), ppvObject);

    if (ppvObject == nullptr)
        return E_INVALIDARG;
    *ppvObject = nullptr;

    if (!IsEqualGUID(riid, IID_IUnknown) &&
        !IsEqualGUID(riid, IID_IWICBitmapDecoder))
        return E_NOINTERFACE;
    this->AddRef();
    *ppvObject = static_cast<IWICBitmapDecoder*>(this);
    return S_OK;
}

HRESULT DecodeContainer::ReadInfo(IStream* pIStream)
{
    const int header_size_bytes = 100;
    uint8_t header[header_size_bytes];
    HRESULT ret;
    ULONG read;

    if (FAILED(ret = pIStream->Read(header, sizeof(header), &read)))
        return ret;

    if (info_) {
        flif_destroy_info(info_);
        info_ = nullptr;
    }
    info_ = flif_read_info_from_memory(header, read);
    if (!info_) {
        return WINCODEC_ERR_BADHEADER;
    }

    // reset pIStream
    LARGE_INTEGER zero;
    zero.QuadPart = 0;
    pIStream->Seek(zero, STREAM_SEEK_SET, nullptr);
    return S_OK;
}

HRESULT DecodeContainer::QueryCapability(IStream* pIStream, DWORD* pdwCapability) {
    TRACE2("(%p, %x)\n", pIStream, pdwCapability);
    if (pdwCapability == nullptr)
        return E_INVALIDARG;

    HRESULT ret = ReadInfo(pIStream);
    if (ret == WINCODEC_ERR_BADHEADER)
        return WINCODEC_ERR_WRONGSTATE;  // That's what Win7 jpeg codec returns.

    if (ret == S_OK)
        *pdwCapability =
        WICBitmapDecoderCapabilityCanDecodeSomeImages
        | WICBitmapDecoderCapabilityCanDecodeAllImages
        //| WICBitmapDecoderCapabilityCanDecodeThumbnail
        | WICBitmapDecoderCapabilityCanEnumerateMetadata;
    return ret;
}

HRESULT DecodeContainer::Initialize(IStream* pIStream, WICDecodeOptions cacheOptions) {
    TRACE2("(%p, %x)\n", pIStream, cacheOptions);

    if (pIStream == nullptr)
        return E_INVALIDARG;

    SectionLock l(&cs_);

    HRESULT ret;
    ret = ReadInfo(pIStream);
    if (FAILED(ret)) {
        return ret;
    }

    if (has_been_decoded_) {
        has_been_decoded_ = false;
        frames_.clear();
    }
    // Save stream for later use
    return pIStream->QueryInterface(stream_.get_out_storage());
}

HRESULT DecodeContainer::DecodeCached(bool decode_only_infos)
{
    SectionLock l(&cs_);
    if (!has_been_decoded_
        || (decode_only_infos == false && has_only_infos_decoded_))
    {
        last_decode_desult = Decode(decode_only_infos);
        has_been_decoded_ = true;
        has_only_infos_decoded_ = decode_only_infos;
    }
    return last_decode_desult;
}

HRESULT DecodeContainer::Decode(bool onlyInfos)
{
    if (stream_.get() == nullptr)
        return WINCODEC_ERR_NOTINITIALIZED;

    HRESULT ret;
    STATSTG stats;
    ret = stream_->Stat(&stats, STATFLAG_NONAME);
    if (FAILED(ret))
        return ret;
    ULONG stream_size = stats.cbSize.QuadPart;
    TRACE1("stream_size %d\n", stream_size);
    ULONG bytes_read;
    scoped_buffer file_data(stream_size);
    if (file_data.get() == nullptr)
        return WINCODEC_ERR_OUTOFMEMORY;

    LARGE_INTEGER zero;
    zero.QuadPart = 0;
    ret = stream_->Seek(zero, STREAM_SEEK_SET, nullptr);
    if (FAILED(ret))
        return ret;

    ret = stream_->Read(file_data.get(), stream_size, &bytes_read);
    TRACE1("bytes_read %d\n", bytes_read);
    if (FAILED(ret)) {
        TRACE("pIStream->Read failed\n");
        return ret;
    }

    if (bytes_read != stream_size) {
        return WINCODEC_ERR_WRONGSTATE;
    }

    if (!decoder_) {
        decoder_ = flif_create_decoder();
    }

    if (onlyInfos) {
        flif_decoder_set_scale(decoder_, -2);
    }
    else {
        flif_decoder_set_scale(decoder_, 1);
    }

    if (flif_decoder_decode_memory(decoder_, file_data.get(), bytes_read) != 0)
    {
        size_t num_images = flif_decoder_num_images(decoder_);
        frames_.resize(num_images);
        for (int i = 0; i < num_images; ++i) {
            FLIF_IMAGE* image = flif_decoder_get_image(decoder_, i);
            if (image)
            {
                if (frames_[i].get() == nullptr) {
                    frames_[i].reset(new (std::nothrow) DecodeFrame());
                    if (frames_[i].get() == nullptr)
                        return E_OUTOFMEMORY;
                }
                frames_[i]->SetFlifImage(image, num_images);
            }
            else {
                frames_[i].reset(nullptr);
            }
        }
    }
    else {
        TRACE("decode failed\n");
        ret = WINCODEC_ERR_WRONGSTATE;
    }
    if (FAILED(ret))
        return ret;
    return S_OK;
}

HRESULT DecodeContainer::GetContainerFormat(GUID* pguidContainerFormat) {
    TRACE1("(%p)\n", pguidContainerFormat);
    if (pguidContainerFormat == nullptr)
        return E_INVALIDARG;
    *pguidContainerFormat = GUID_ContainerFormatFLIF;
    return S_OK;
}

HRESULT DecodeContainer::InitializeFactory()
{
    SectionLock l(&cs_);
    HRESULT result = S_OK;
    if (factory_.get() == nullptr) {
        result = CoCreateInstance(CLSID_WICImagingFactory,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_IWICImagingFactory,
            (LPVOID*)factory_.get_out_storage());
    }
    return result;
}

HRESULT DecodeContainer::GetDecoderInfo(IWICBitmapDecoderInfo** ppIDecoderInfo) {
    TRACE1("(%p)\n", ppIDecoderInfo);
    HRESULT result;

    result = InitializeFactory();
    if (FAILED(result))
        return result;

    ComPtr<IWICComponentInfo> compInfo;
    result = factory_->CreateComponentInfo(CLSID_FLIFWICDecoder, compInfo.get_out_storage());
    if (FAILED(result))
        return result;

    result = compInfo->QueryInterface(IID_IWICBitmapDecoderInfo, (void**)ppIDecoderInfo);
    if (FAILED(result))
        return result;

    return S_OK;
}

HRESULT DecodeContainer::CopyPalette(IWICPalette* pIPalette) {
    TRACE1("(%p)\n", pIPalette);
    return WINCODEC_ERR_PALETTEUNAVAILABLE;
}

HRESULT DecodeContainer::GetMetadataQueryReader(IWICMetadataQueryReader** ppIMetadataQueryReader) {
    TRACE1("(%p)\n", ppIMetadataQueryReader);
    if (ppIMetadataQueryReader == nullptr)
        return E_INVALIDARG;

    HRESULT result = DecodeCached(true);
    if (FAILED(result))
        return result;

    return frames_[0].get()->GetMetadataQueryReader(ppIMetadataQueryReader);
}

HRESULT DecodeContainer::GetPreview(IWICBitmapSource** ppIBitmapSource) {
    TRACE1("(%p)\n", ppIBitmapSource);
    if (ppIBitmapSource == nullptr)
        return E_INVALIDARG;

    HRESULT result = DecodeCached(false);
    if (FAILED(result))
        return result;

    *ppIBitmapSource = frames_[0].new_ref();
    return S_OK;
}

HRESULT DecodeContainer::GetColorContexts(UINT cCount, IWICColorContext** ppIColorContexts, UINT* pcActualCount) {
    TRACE3("(%d, %p, %p)\n", cCount, ppIColorContexts, pcActualCount);
    return S_OK;
}

HRESULT DecodeContainer::GetThumbnail(IWICBitmapSource** ppIThumbnail)
{
    TRACE1("(%p)\n", ppIThumbnail);
    if (ppIThumbnail == nullptr)
        return E_INVALIDARG;

    HRESULT result = DecodeCached(false);
    if (FAILED(result))
        return result;

    *ppIThumbnail = frames_[0].new_ref();
    return S_OK;
}

HRESULT DecodeContainer::GetFrameCount(UINT* pCount) {
    TRACE1("(%p)\n", pCount);
    if (pCount == nullptr)
        return E_INVALIDARG;

    if (info_ == nullptr)
        return WINCODEC_ERR_NOTINITIALIZED;

    *pCount = GetFrameCount();
    return S_OK;
}

HRESULT DecodeContainer::GetFrame(UINT index, IWICBitmapFrameDecode** ppIBitmapFrame) {
    TRACE2("(%d, %p)\n", index, ppIBitmapFrame);
    if (ppIBitmapFrame == nullptr)
        return E_INVALIDARG;

    HRESULT result = DecodeCached(false);
    if (FAILED(result))
        return result;

    SectionLock l(&cs_);
    *ppIBitmapFrame = frames_[index].new_ref();
    return S_OK;
}

UINT DecodeContainer::GetWidth() const
{
    return info_ ? flif_info_get_width(info_) : 0;
}

UINT DecodeContainer::GetHeight() const
{
    return info_ ? flif_info_get_height(info_) : 0;
}

UINT DecodeContainer::GetBitDepth() const
{
    return info_ ? flif_info_get_depth(info_) : 0;
}

UINT DecodeContainer::GetFrameCount() const
{
    return info_ ? flif_info_num_images(info_) : 0;
}



