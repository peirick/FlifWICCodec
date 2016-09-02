#pragma once

#include <wincodec.h>
#include <wincodecsdk.h>
#include <flif.h>
#include <deque>
#include "utils.h"
#include "encode_container.h"

class EncodeFrame : public ComObjectBase<IWICBitmapFrameEncode>, IWICMetadataBlockWriter {
public:
    explicit EncodeFrame(EncodeContainer* container);
    ~EncodeFrame();

    // Inherited via IUnknown:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override;
    ULONG STDMETHODCALLTYPE AddRef() { return ComObjectBase::AddRef(); }
    ULONG STDMETHODCALLTYPE Release() { return ComObjectBase::Release(); }
    // Inherited via IWICBitmapFrameEncode
    HRESULT STDMETHODCALLTYPE Initialize(IPropertyBag2 * pIEncoderOptions) override;
    HRESULT STDMETHODCALLTYPE SetSize(UINT uiWidth, UINT uiHeight) override;
    HRESULT STDMETHODCALLTYPE SetResolution(double dpiX, double dpiY) override;
    HRESULT STDMETHODCALLTYPE SetPixelFormat(WICPixelFormatGUID * pPixelFormat) override;
    HRESULT STDMETHODCALLTYPE SetColorContexts(UINT cCount, IWICColorContext ** ppIColorContext) override;
    HRESULT STDMETHODCALLTYPE SetPalette(IWICPalette * pIPalette) override;
    HRESULT STDMETHODCALLTYPE SetThumbnail(IWICBitmapSource * pIThumbnail) override;
    HRESULT STDMETHODCALLTYPE WritePixels(UINT lineCount, UINT cbStride, UINT cbBufferSize, BYTE * pbPixels) override;
    HRESULT STDMETHODCALLTYPE WriteSource(IWICBitmapSource * pIBitmapSource, WICRect * prc) override;
    HRESULT STDMETHODCALLTYPE Commit(void) override;
    HRESULT STDMETHODCALLTYPE GetMetadataQueryWriter(IWICMetadataQueryWriter ** ppIMetadataQueryWriter) override;
    // Inherited via IWICMetadataBlockWriter
    HRESULT STDMETHODCALLTYPE GetContainerFormat(GUID * pguidContainerFormat) override;
    HRESULT STDMETHODCALLTYPE GetCount(UINT * pcCount) override;
    HRESULT STDMETHODCALLTYPE GetReaderByIndex(UINT nIndex, IWICMetadataReader ** ppIMetadataReader) override;
    HRESULT STDMETHODCALLTYPE GetEnumerator(IEnumUnknown ** ppIEnumMetadata) override;
    HRESULT STDMETHODCALLTYPE InitializeFromBlockReader(IWICMetadataBlockReader * pIMDBlockReader) override;
    HRESULT STDMETHODCALLTYPE GetWriterByIndex(UINT nIndex, IWICMetadataWriter ** ppIMetadataWriter) override;
    HRESULT STDMETHODCALLTYPE AddWriter(IWICMetadataWriter * pIMetadataWriter) override;
    HRESULT STDMETHODCALLTYPE SetWriterByIndex(UINT nIndex, IWICMetadataWriter * pIMetadataWriter) override;
    HRESULT STDMETHODCALLTYPE RemoveWriterByIndex(UINT nIndex) override;
private:
    // No copy and assign.
    EncodeFrame(const EncodeFrame&) = delete;
    void operator=(const EncodeFrame&) = delete;
    EncodeContainer* container_;
    std::shared_ptr<RawFrame> frame_;
    AnimationInformation animation_information_;
    std::deque<std::shared_ptr<Metadata>> metadata_;
};

