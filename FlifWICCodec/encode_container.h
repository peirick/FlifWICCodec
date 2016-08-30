#pragma once

#include <wincodec.h>
#include <deque>
#include <library/flif_enc.h>
#include "utils.h"

struct AnimationInformation {
	uint32_t Left;
	uint32_t Top;
	uint8_t Disposal;
	bool TransparencyFlag;
	uint32_t Delay;
};

struct RawFrame {
	uint32_t Width;
	uint32_t Height;
	uint32_t NumberComponents;
	uint32_t Stride;
	size_t BufferSize;
	uint8_t* Buffer;

	RawFrame(uint32_t width, uint32_t height, uint32_t numberComponents, uint32_t stride)
		: Width(width), Height(height), NumberComponents(numberComponents), Stride(stride),
		BufferSize(stride*height), Buffer(nullptr)
	{
		Buffer = (uint8_t*)CoTaskMemAlloc(BufferSize);
	}
	RawFrame()
		: Width(0), Height(0), NumberComponents(0), Stride(0), BufferSize(0), Buffer(nullptr)
	{
	}
	~RawFrame()
	{
		if (Buffer) {
			CoTaskMemFree(Buffer);
		}
	}
};

class EncodeContainer : public ComObjectBase<IWICBitmapEncoder> {
public:
	EncodeContainer();
	~EncodeContainer();
	// IUnknown:
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override;
	ULONG STDMETHODCALLTYPE AddRef() override { return ComObjectBase::AddRef(); }
	ULONG STDMETHODCALLTYPE Release() override { return ComObjectBase::Release(); }
	// Inherited via IWICBitmapEncoder
	HRESULT STDMETHODCALLTYPE Initialize(IStream * pIStream, WICBitmapEncoderCacheOption cacheOption) override;
	HRESULT STDMETHODCALLTYPE GetContainerFormat(GUID* pguidContainerFormat) override;
	HRESULT STDMETHODCALLTYPE GetEncoderInfo(IWICBitmapEncoderInfo ** ppIEncoderInfo) override;
	HRESULT STDMETHODCALLTYPE SetColorContexts(UINT cCount, IWICColorContext ** ppIColorContext) override;
	HRESULT STDMETHODCALLTYPE SetPalette(IWICPalette * pIPalette) override;
	HRESULT STDMETHODCALLTYPE SetThumbnail(IWICBitmapSource * pIThumbnail) override;
	HRESULT STDMETHODCALLTYPE SetPreview(IWICBitmapSource * pIPreview) override;
	HRESULT STDMETHODCALLTYPE CreateNewFrame(IWICBitmapFrameEncode ** ppIFrameEncode, IPropertyBag2 ** ppIEncoderOptions) override;
	HRESULT STDMETHODCALLTYPE Commit(void) override;
	HRESULT STDMETHODCALLTYPE GetMetadataQueryWriter(IWICMetadataQueryWriter ** ppIMetadataQueryWriter) override;
public:
	HRESULT AddImage(RawFrame* frame, AnimationInformation animationInformation);
private:
	// No copy and assign.
	EncodeContainer(const EncodeContainer&) = delete;
	void operator=(const EncodeContainer&) = delete;

	RawFrame* current_frame_;
	FLIF_ENCODER* encoder_;
	IStream* pIStream_;
	ComPtr<IWICImagingFactory> factory_;
	CRITICAL_SECTION cs_;
};
