#include <windows.h>
#include "dllmain.h"
#include "uuid.h"
#include "utils.h"
#include "decode_container.h"
#include "decode_frame.h"
#include "decode_metadata_reader.h"

DecodeContainer::DecodeContainer() : decoder_(nullptr)
{
	TRACE("()\n");
	InitializeCriticalSection(&cs_);
}

DecodeContainer::~DecodeContainer()
{
	TRACE("()\n");
	if (decoder_) {
		flif_destroy_decoder(decoder_);
		decoder_ = nullptr;
	}
}

HRESULT DecodeContainer::QueryInterface(REFIID riid, void** ppvObject) {
	TRACE2("(%s, %p)\n", debugstr_guid(riid), ppvObject);

	if (ppvObject == NULL)
		return E_INVALIDARG;
	*ppvObject = NULL;

	if (!IsEqualGUID(riid, IID_IUnknown) && !IsEqualGUID(riid, IID_IWICBitmapDecoder))
		return E_NOINTERFACE;
	this->AddRef();
	*ppvObject = static_cast<IWICBitmapDecoder*>(this);
	return S_OK;
}

HRESULT IsValidFLIF(IStream* pIStream) {
	// read enough so WebPGetInfo can do basic validation.
	const int kHeaderSize = 4;
	char header[kHeaderSize];
	HRESULT ret;
	ULONG read;

	if (FAILED(ret = pIStream->Read(header, sizeof(header), &read)))
		return ret;
	if (read < kHeaderSize) {
		TRACE("Read error\n");
		return E_UNEXPECTED;
	}

	if (strncmp(header, "FLIF", 4)) {
		TRACE("Not a FLIF file\n");
		return WINCODEC_ERR_BADIMAGE;
	}

	// reset pIStream
	LARGE_INTEGER offset;
	offset.QuadPart = -kHeaderSize;
	ret = pIStream->Seek(offset, STREAM_SEEK_CUR, NULL);
	return ret;
}

HRESULT DecodeContainer::QueryCapability(IStream* pIStream, DWORD* pdwCapability) {
	TRACE2("(%p, %x)\n", pIStream, pdwCapability);
	if (pdwCapability == NULL)
		return E_INVALIDARG;

	HRESULT ret = IsValidFLIF(pIStream);
	if (ret == WINCODEC_ERR_BADHEADER)
		return WINCODEC_ERR_WRONGSTATE;  // That's what Win7 jpeg codec returns.
	if (ret == S_OK)
		// TODO: should we check if we really can decode the VP8 bitstream?
		*pdwCapability = WICBitmapDecoderCapabilityCanDecodeSomeImages;
	return ret;
}

HRESULT DecodeContainer::Initialize(IStream* pIStream, WICDecodeOptions cacheOptions) {
	TRACE2("(%p, %x)\n", pIStream, cacheOptions);

	if (pIStream == NULL)
		return E_INVALIDARG;

	HRESULT ret;
	ret = IsValidFLIF(pIStream);
	if (FAILED(ret)) {
		return ret;
	}

	SectionLock l(&cs_);

	frames_.clear();

	STATSTG stats;
	pIStream->Stat(&stats, STATFLAG_NONAME);
	ULONG stream_size = stats.cbSize.QuadPart;
	TRACE1("stream_size %d\n", stream_size);
	ULONG bytes_read;
	scoped_buffer file_data(stream_size);
	LARGE_INTEGER zero;
	zero.QuadPart = 0;
	pIStream->Seek(zero, STREAM_SEEK_SET, NULL);
	ret = pIStream->Read(file_data.get(), stream_size, &bytes_read);
	TRACE1("bytes_read %d\n", bytes_read);
	if (FAILED(ret)) {
		TRACE("pIStream->Read failed\n");
		return ret;
	}

	if (bytes_read != stream_size) {
		return WINCODEC_ERR_WRONGSTATE;
	}

	if (decoder_) {
		flif_destroy_decoder(decoder_);
		decoder_ = nullptr;
	}
	decoder_ = flif_create_decoder();

	if (flif_decoder_decode_memory(decoder_, file_data.get(), bytes_read) != 0)
	{
		size_t num_images = flif_decoder_num_images(decoder_);
		frames_.resize(num_images);
		for (int i = 0; i < num_images; ++i) {
			FLIF_IMAGE* image = flif_decoder_get_image(decoder_, i);
			DecodeFrame::CreateFromFLIFImage(image, frames_[i]);
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
	if (pguidContainerFormat == NULL)
		return E_INVALIDARG;
	*pguidContainerFormat = GUID_ContainerFormatFLIF;
	return S_OK;
}

HRESULT DecodeContainer::GetDecoderInfo(IWICBitmapDecoderInfo** ppIDecoderInfo) {
	TRACE1("(%p)\n", ppIDecoderInfo);
	HRESULT result;
	ComPtr<IWICImagingFactory> factory;

	{
		SectionLock l(&cs_);
		if (factory_.get() == NULL) {
			result = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, (LPVOID*)factory_.get_out_storage());
			if (FAILED(result))
				return result;
		}
		factory.reset(factory_.new_ref());
	}

	ComPtr<IWICComponentInfo> compInfo;
	result = factory->CreateComponentInfo(CLSID_FLIFWICDecoder, compInfo.get_out_storage());
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
	if (ppIMetadataQueryReader == NULL)
		return E_INVALIDARG;
	if (frames_.size() == 0)
		return WINCODEC_ERR_NOTINITIALIZED;

	UINT width;
	UINT height;
	HRESULT result = frames_[0].get()->GetSize(&width, &height);
	if (FAILED(result))
		return result;
	*ppIMetadataQueryReader = new DecodeMetadataQueryReader(width, height);
	return S_OK;
}

HRESULT DecodeContainer::GetPreview(IWICBitmapSource** ppIBitmapSource) {
	TRACE1("(%p)\n", ppIBitmapSource);
	return WINCODEC_ERR_UNSUPPORTEDOPERATION;;
}

HRESULT DecodeContainer::GetColorContexts(UINT cCount, IWICColorContext** ppIColorContexts, UINT* pcActualCount) {
	TRACE3("(%d, %p, %p)\n", cCount, ppIColorContexts, pcActualCount);
	return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

HRESULT DecodeContainer::GetThumbnail(IWICBitmapSource** ppIThumbnail)
{
	TRACE1("(%p)\n", ppIThumbnail);
	return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

HRESULT DecodeContainer::GetFrameCount(UINT* pCount) {
	TRACE1("(%p)\n", pCount);
	if (pCount == NULL)
		return E_INVALIDARG;
	if (decoder_ == nullptr) {
		return WINCODEC_ERR_NOTINITIALIZED;
	}
	*pCount = frames_.size();
	return S_OK;
}

HRESULT DecodeContainer::GetFrame(UINT index, IWICBitmapFrameDecode** ppIBitmapFrame) {
	TRACE2("(%d, %p)\n", index, ppIBitmapFrame);
	if (ppIBitmapFrame == NULL)
		return E_INVALIDARG;

	if (decoder_ == nullptr) {
		return WINCODEC_ERR_NOTINITIALIZED;
	}

	if (index >= frames_.size())
		return WINCODEC_ERR_FRAMEMISSING;

	SectionLock l(&cs_);
	*ppIBitmapFrame = frames_[index].new_ref();
	return S_OK;
}