#pragma once
#include <wincodec.h>
#include "utils.h"

class DecodeMetadataQueryReader : public ComObjectBase<IWICMetadataQueryReader> {
public:
	DecodeMetadataQueryReader();
	~DecodeMetadataQueryReader();
	// Inherited via IUnknown:
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override;
	ULONG STDMETHODCALLTYPE AddRef() override { return ComObjectBase::AddRef(); }
	ULONG STDMETHODCALLTYPE Release() override { return ComObjectBase::Release(); }
	// Inherited via IWICMetadataQueryReader
	HRESULT STDMETHODCALLTYPE GetLocation(UINT cchMaxLength, WCHAR * wzNamespace, UINT * pcchActualLength) override;
	HRESULT STDMETHODCALLTYPE GetMetadataByName(LPCWSTR wzName, PROPVARIANT * pvarValue) override;
	HRESULT STDMETHODCALLTYPE GetEnumerator(IEnumString ** ppIEnumString) override;
	HRESULT STDMETHODCALLTYPE GetContainerFormat(GUID *pguidContainerFormat) override;
};