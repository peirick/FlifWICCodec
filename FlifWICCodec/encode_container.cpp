#include "encode_container.h"
#include "encode_frame.h"
#include "uuid.h"

EncodeContainer::EncodeContainer() : encoder_(nullptr), image_count(0)
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

	if (ppvObject == NULL)
		return E_INVALIDARG;
	*ppvObject = NULL;

	if (!IsEqualGUID(riid, IID_IUnknown) && !IsEqualGUID(riid, IID_IWICBitmapEncoder))
		return E_NOINTERFACE;
	this->AddRef();
	*ppvObject = static_cast<IWICBitmapEncoder*>(this);
	return S_OK;
}

HRESULT EncodeContainer::Initialize(IStream * pIStream, WICBitmapEncoderCacheOption cacheOptions)
{
	TRACE2("(%p, %x)\n", pIStream, cacheOptions);
	if (encoder_) {
		flif_destroy_encoder(encoder_);
		encoder_ = nullptr;
	}
	encoder_ = flif_create_encoder();
	pIStream_ = pIStream;
	return S_OK;
}

HRESULT EncodeContainer::GetContainerFormat(GUID * pguidContainerFormat)
{
	TRACE1("(%p)\n", pguidContainerFormat);
	if (pguidContainerFormat == NULL)
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
		if (factory_.get() == NULL) {
			result = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, (LPVOID*)factory_.get_out_storage());
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
	if (ppIFrameEncode == NULL)
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

	if (image_count > 0) {
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
	if (ppIMetadataQueryWriter == NULL)
		return E_INVALIDARG;
	return E_NOTIMPL;
}

HRESULT EncodeContainer::AddImage(FLIF_IMAGE * image)
{
	if (encoder_ == nullptr) {
		return WINCODEC_ERR_NOTINITIALIZED;
	}
	flif_encoder_add_image(encoder_, image);
	++image_count;
}
