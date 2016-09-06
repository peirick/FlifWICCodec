#pragma once

#include <wincodec.h>
#include <deque>
#include <flif.h>
#include <memory>
#include <string>
#include <vector>
#include "utils.h"

struct AnimationInformation {
    uint32_t Left;
    uint32_t Top;
    uint8_t Disposal;
    bool TransparencyFlag;
    uint32_t Delay;
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
    HRESULT AddImage(std::shared_ptr<RawFrame> frame, AnimationInformation animationInformation, std::deque<std::shared_ptr<Metadata>> metadata);
private:
    // No copy and assign.
    EncodeContainer(const EncodeContainer&) = delete;
    void operator=(const EncodeContainer&) = delete;
    HRESULT InitializeFactory();

    std::shared_ptr<RawFrame> current_frame_;
    FLIF_ENCODER* encoder_;
    IStream* pIStream_;
    ComPtr<IWICImagingFactory> factory_;
    CRITICAL_SECTION cs_;

};
