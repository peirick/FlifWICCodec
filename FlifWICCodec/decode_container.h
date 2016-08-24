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
	// IUnknown:
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override;
	ULONG STDMETHODCALLTYPE AddRef() override { return ComObjectBase::AddRef(); }
	ULONG STDMETHODCALLTYPE Release() override { return ComObjectBase::Release(); }
	// IWICBitmapDecoder:
	virtual HRESULT STDMETHODCALLTYPE QueryCapability(IStream *pIStream, DWORD *pdwCapability) override;
	virtual HRESULT STDMETHODCALLTYPE Initialize(IStream *pIStream, WICDecodeOptions cacheOptions) override;
	virtual HRESULT STDMETHODCALLTYPE GetContainerFormat(GUID *pguidContainerFormat) override;
	virtual HRESULT STDMETHODCALLTYPE GetDecoderInfo(IWICBitmapDecoderInfo **ppIDecoderInfo) override;
	virtual HRESULT STDMETHODCALLTYPE CopyPalette(IWICPalette *pIPalette)override;
	virtual HRESULT STDMETHODCALLTYPE GetMetadataQueryReader(IWICMetadataQueryReader **ppIMetadataQueryReader)override;
	virtual HRESULT STDMETHODCALLTYPE GetPreview(IWICBitmapSource **ppIBitmapSource)override;
	virtual HRESULT STDMETHODCALLTYPE GetColorContexts(UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)override;
	virtual HRESULT STDMETHODCALLTYPE GetThumbnail(IWICBitmapSource **ppIThumbnail)override;
	virtual HRESULT STDMETHODCALLTYPE GetFrameCount(UINT *pCount)override;
	virtual HRESULT STDMETHODCALLTYPE GetFrame(UINT index, IWICBitmapFrameDecode **ppIBitmapFrame)override;
private:
	// No copy and assign.
	DecodeContainer(const DecodeContainer&) = delete;
	void operator=(const DecodeContainer&) = delete;

	FLIF_DECODER* decoder_;
	std::deque<ComPtr<DecodeFrame>> frames_;
	ComPtr<IWICImagingFactory> factory_;
	CRITICAL_SECTION cs_;
};