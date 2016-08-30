#pragma once

#include <wincodec.h>
#include <deque>
#include <library/flif_dec.h>
#include "decode_frame.h"
#include "utils.h"

class DecodeContainer : public ComObjectBase<IWICBitmapDecoder> {
public:
	DecodeContainer();
	~DecodeContainer();
	// Inherited via IUnknown:
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override;
	ULONG STDMETHODCALLTYPE AddRef() override { return ComObjectBase::AddRef(); }
	ULONG STDMETHODCALLTYPE Release() override { return ComObjectBase::Release(); }
	// Inherited via IWICBitmapDecoder:
	HRESULT STDMETHODCALLTYPE QueryCapability(IStream *pIStream, DWORD *pdwCapability) override;
	HRESULT STDMETHODCALLTYPE Initialize(IStream *pIStream, WICDecodeOptions cacheOptions) override;
	HRESULT STDMETHODCALLTYPE GetContainerFormat(GUID *pguidContainerFormat) override;
	HRESULT STDMETHODCALLTYPE GetDecoderInfo(IWICBitmapDecoderInfo **ppIDecoderInfo) override;
	HRESULT STDMETHODCALLTYPE CopyPalette(IWICPalette *pIPalette)override;
	HRESULT STDMETHODCALLTYPE GetMetadataQueryReader(IWICMetadataQueryReader **ppIMetadataQueryReader)override;
	HRESULT STDMETHODCALLTYPE GetPreview(IWICBitmapSource **ppIBitmapSource)override;
	HRESULT STDMETHODCALLTYPE GetColorContexts(UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)override;
	HRESULT STDMETHODCALLTYPE GetThumbnail(IWICBitmapSource **ppIThumbnail)override;
	HRESULT STDMETHODCALLTYPE GetFrameCount(UINT *pCount)override;
	HRESULT STDMETHODCALLTYPE GetFrame(UINT index, IWICBitmapFrameDecode **ppIBitmapFrame)override;
private:
	// No copy and assign.
	DecodeContainer(const DecodeContainer&) = delete;
	void operator=(const DecodeContainer&) = delete;
	FLIF_DECODER* decoder_;
	std::deque<ComPtr<DecodeFrame>> frames_;
	ComPtr<IWICImagingFactory> factory_;
	CRITICAL_SECTION cs_;
};
