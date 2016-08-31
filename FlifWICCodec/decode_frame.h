#pragma once

#include <wincodec.h>
#include <flif.h>
#include "utils.h"

class DecodeFrame : public ComObjectBase<IWICBitmapFrameDecode> {
public:
	explicit DecodeFrame(FLIF_IMAGE* image);
	~DecodeFrame();
	// IUnknown:
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override;
	ULONG STDMETHODCALLTYPE AddRef() override { return ComObjectBase::AddRef(); }
	ULONG STDMETHODCALLTYPE Release() override { return ComObjectBase::Release(); }
	// IWICBitmapSource:
	HRESULT STDMETHODCALLTYPE GetSize(UINT *puiWidth, UINT *puiHeight) override;
	HRESULT STDMETHODCALLTYPE GetPixelFormat(WICPixelFormatGUID *pPixelFormat) override;
	HRESULT STDMETHODCALLTYPE GetResolution(double *pDpiX, double *pDpiY) override;
	HRESULT STDMETHODCALLTYPE CopyPalette(IWICPalette *pIPalette) override;
	HRESULT STDMETHODCALLTYPE CopyPixels(const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer) override;
	// IWICBitmapFrameDecode:
	HRESULT STDMETHODCALLTYPE GetMetadataQueryReader(IWICMetadataQueryReader **ppIMetadataQueryReader) override;
	HRESULT STDMETHODCALLTYPE GetColorContexts(UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount) override;
	HRESULT STDMETHODCALLTYPE GetThumbnail(IWICBitmapSource **ppIThumbnail) override;
private:
	// No copy and assign.
	DecodeFrame(const DecodeFrame&) = delete;
	void operator=(const DecodeFrame&) = delete;

	FLIF_IMAGE*				   image_;
	ComPtr<IWICImagingFactory> factory_;
};
