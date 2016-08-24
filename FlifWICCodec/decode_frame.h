#pragma once

#include <wincodec.h>
#include <library/flif_dec.h>
#include "utils.h"

class DecodeFrame : public ComObjectBase<IWICBitmapFrameDecode> {
public:
	explicit DecodeFrame(FLIF_IMAGE* image);
	~DecodeFrame();
	
	static HRESULT DecodeFrame::CreateFromFLIFImage(FLIF_IMAGE* image, ComPtr<DecodeFrame>& ppOutput);

	// IUnknown:
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
	ULONG STDMETHODCALLTYPE AddRef() { return ComObjectBase::AddRef(); }
	ULONG STDMETHODCALLTYPE Release() { return ComObjectBase::Release(); }
	// IWICBitmapSource:
	virtual HRESULT STDMETHODCALLTYPE GetSize(UINT *puiWidth, UINT *puiHeight);
	virtual HRESULT STDMETHODCALLTYPE GetPixelFormat(WICPixelFormatGUID *pPixelFormat);
	virtual HRESULT STDMETHODCALLTYPE GetResolution(double *pDpiX, double *pDpiY);
	virtual HRESULT STDMETHODCALLTYPE CopyPalette(IWICPalette *pIPalette);
	virtual HRESULT STDMETHODCALLTYPE CopyPixels(const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer);
	// IWICBitmapFrameDecode:
	virtual HRESULT STDMETHODCALLTYPE GetMetadataQueryReader(IWICMetadataQueryReader **ppIMetadataQueryReader);
	virtual HRESULT STDMETHODCALLTYPE GetColorContexts(UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount);
	virtual HRESULT STDMETHODCALLTYPE GetThumbnail(IWICBitmapSource **ppIThumbnail);
private:
	// No copy and assign.
	DecodeFrame(const DecodeFrame&) = delete;
	void operator=(const DecodeFrame&) = delete;

	FLIF_IMAGE* image_;
	ComPtr<IWICImagingFactory> factory_;
};