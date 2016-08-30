#include <library/flif_enc.h>
#include "encode_frame.h"
#include "pixel_converter.h"

EncodeFrame::EncodeFrame(EncodeContainer* container)
	: container_(container)
{
	container_->AddRef();
}

EncodeFrame::~EncodeFrame()
{
	container_->Release();
}

HRESULT EncodeFrame::QueryInterface(REFIID riid, void ** ppvObject)
{
	TRACE2("(%s, %p)\n", debugstr_guid(riid), ppvObject);

	if (ppvObject == nullptr)
		return E_INVALIDARG;
	*ppvObject = nullptr;

	if (!IsEqualGUID(riid, IID_IUnknown) &&
		!IsEqualGUID(riid, IID_IWICBitmapFrameEncode))
		return E_NOINTERFACE;
	this->AddRef();
	*ppvObject = static_cast<IWICBitmapFrameEncode*>(this);
	return S_OK;
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
	AnimationInformation animation_information = {};
	ComPtr<IWICBitmapFrameDecode> decodeframe;
	if (SUCCEEDED(pIBitmapSource->QueryInterface(decodeframe.get_out_storage()))) {
		ComPtr<IWICMetadataQueryReader> metadataReader;
		if (SUCCEEDED(decodeframe->GetMetadataQueryReader(metadataReader.get_out_storage())))
		{
			PROPVARIANT propValue;
			PropVariantInit(&propValue);

			if (SUCCEEDED(metadataReader->GetMetadataByName(L"/imgdesc/Left", &propValue))) {
				if (propValue.vt == VT_UI2) {
					animation_information.Left = propValue.uiVal;
				}
			}
			PropVariantClear(&propValue);

			if (SUCCEEDED(metadataReader->GetMetadataByName(L"/imgdesc/Top", &propValue))) {
				if (propValue.vt == VT_UI2) {
					animation_information.Top = propValue.uiVal;
				}
			}
			PropVariantClear(&propValue);

			if (SUCCEEDED(metadataReader->GetMetadataByName(L"/grctlext/Disposal", &propValue))) {
				if (propValue.vt == VT_UI1) {
					animation_information.Disposal = propValue.bVal;
				}
			}
			PropVariantClear(&propValue);

			if (SUCCEEDED(metadataReader->GetMetadataByName(L"/grctlext/Delay", &propValue))) {
				if (propValue.vt == VT_UI2) {
					// From 10th ms to ms
					animation_information.Delay = propValue.uiVal * 10;
				}
			}
			PropVariantClear(&propValue);

			if (SUCCEEDED(metadataReader->GetMetadataByName(L"/grctlext/TransparencyFlag", &propValue))) {
				if (propValue.vt == VT_BOOL) {
					animation_information.TransparencyFlag = propValue.boolVal;
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
		ComPtr<IWICImagingFactory> factory;
		result = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, (LPVOID*)factory.get_out_storage());
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
			result = factory->CreatePalette(palette.get_out_storage());
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
		result = factory->CreateFormatConverter(converter.get_out_storage());
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
	RawFrame* frame = nullptr;
	if (source_pixel_format == GUID_WICPixelFormat32bppRGBA)
	{
		uint32_t stride = rect.Width * 4;
		frame = new RawFrame(rect.Width, rect.Height, 4, stride);
	}
	else if (source_pixel_format == GUID_WICPixelFormat24bppRGB)
	{
		uint32_t stride = 4 * ((24 * (UINT)rect.Width + 31) / 32);
		frame = new RawFrame(rect.Width, rect.Height, 3, stride);
	}
	else if (source_pixel_format == GUID_WICPixelFormat8bppGray)
	{
		uint32_t stride = rect.Width;
		frame = new RawFrame(rect.Width, rect.Height, 1, stride);
	}
	else
	{
		return WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT;
	}
	if (frame->Buffer == nullptr)
	{
		delete frame;
		return E_OUTOFMEMORY;
	}

	//Read Source Pixel
	result = source_image->CopyPixels(&rect, frame->Stride, frame->BufferSize, frame->Buffer);
	if (FAILED(result))
	{
		delete frame;
		return result;
	}

	container_->AddImage(frame, animation_information);
	return S_OK;

}

HRESULT EncodeFrame::Commit(void)
{
	TRACE("()\n");
	return S_OK;
}

HRESULT EncodeFrame::GetMetadataQueryWriter(IWICMetadataQueryWriter ** ppIMetadataQueryWriter)
{
	TRACE1("(%p)\n", ppIMetadataQueryWriter);
	if (ppIMetadataQueryWriter == nullptr)
		return E_INVALIDARG;
	*ppIMetadataQueryWriter = nullptr;
	return S_OK;
}
