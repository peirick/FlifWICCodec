#pragma once

#include <memory>
#include <deque>
#include <wincodec.h>
#include <wincodecsdk.h>
#include <flif.h>
#include "utils.h"

class DecodeFrame : public ComObjectBase<IWICBitmapFrameDecode> {
public:
    DecodeFrame();
    ~DecodeFrame();
    // Inherited via IUnknown:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override;
    ULONG STDMETHODCALLTYPE AddRef() override { return ComObjectBase::AddRef(); }
    ULONG STDMETHODCALLTYPE Release() override { return ComObjectBase::Release(); }
    // Inherited via IWICBitmapSource:
    HRESULT STDMETHODCALLTYPE GetSize(UINT *puiWidth, UINT *puiHeight) override;
    HRESULT STDMETHODCALLTYPE GetPixelFormat(WICPixelFormatGUID *pPixelFormat) override;
    HRESULT STDMETHODCALLTYPE GetResolution(double *pDpiX, double *pDpiY) override;
    HRESULT STDMETHODCALLTYPE CopyPalette(IWICPalette *pIPalette) override;
    HRESULT STDMETHODCALLTYPE CopyPixels(const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer) override;
    // Inherited via IWICBitmapFrameDecode:
    HRESULT STDMETHODCALLTYPE GetMetadataQueryReader(IWICMetadataQueryReader **ppIMetadataQueryReader) override;
    HRESULT STDMETHODCALLTYPE GetColorContexts(UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount) override;
    HRESULT STDMETHODCALLTYPE GetThumbnail(IWICBitmapSource **ppIThumbnail) override;

    void SetFlifImage(FLIF_IMAGE* image, UINT totalNumberImages);
private:

    class MetadataBlockReader : public IWICMetadataBlockReader {
    public:
        MetadataBlockReader(DecodeFrame& decodeFrame) : decodeFrame_(decodeFrame) {}
        // Inherited via IUnknown:
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override { return decodeFrame_.QueryInterface(riid, ppvObject); };
        ULONG STDMETHODCALLTYPE AddRef() override { return decodeFrame_.AddRef(); }
        ULONG STDMETHODCALLTYPE Release() override { return decodeFrame_.Release(); }
        // Inherited via IWICMetadataBlockReader
        HRESULT STDMETHODCALLTYPE GetContainerFormat(GUID * pguidContainerFormat) override;
        HRESULT STDMETHODCALLTYPE GetCount(UINT * pcCount) override;
        HRESULT STDMETHODCALLTYPE GetReaderByIndex(UINT nIndex, IWICMetadataReader ** ppIMetadataReader) override;
        HRESULT STDMETHODCALLTYPE GetEnumerator(IEnumUnknown ** ppIEnumMetadata) override;
    private:
        void ReadAllMetadata();
        void ReadMetadata(GUID metadataFormat, const char* name);

        DecodeFrame& decodeFrame_;
        std::deque<ComPtr<IWICMetadataReader>> metadataReader_;
    };

    // No copy and assign.
    DecodeFrame(const DecodeFrame&) = delete;
    void operator=(const DecodeFrame&) = delete;
    HRESULT InitializeFactory();
private:
    UINT GetWidth() const;
    UINT GetHeight() const;
    UINT GetDelay() const;

    CRITICAL_SECTION cs_;
    ComPtr<IWICImagingFactory>   factory_;
    ComPtr<IWICComponentFactory> componentFactory_;
    MetadataBlockReader          metadataBlockReader_;
    FLIF_IMAGE*				     image_;
    UINT                         totalNumberOfImages_;
};
