#include "decode_metadata_reader.h"
#include "uuid.h"

DecodeMetadataQueryReader::DecodeMetadataQueryReader()
{
}

DecodeMetadataQueryReader::~DecodeMetadataQueryReader()
{
}

HRESULT DecodeMetadataQueryReader::QueryInterface(REFIID riid, void ** ppvObject)
{
	TRACE2("(%s, %p)\n", debugstr_guid(riid), ppvObject);

	if (ppvObject == NULL)
		return E_INVALIDARG;
	*ppvObject = NULL;

	if (!IsEqualGUID(riid, IID_IUnknown) && !IsEqualGUID(riid, IID_IWICMetadataQueryReader))
		return E_NOINTERFACE;
	this->AddRef();
	*ppvObject = static_cast<IWICMetadataQueryReader*>(this);
	return S_OK;
}


HRESULT DecodeMetadataQueryReader::GetLocation(UINT cchMaxLength, WCHAR * wzNamespace, UINT * pcchActualLength)
{
	TRACE3("(%d, %p, %p)\n", cchMaxLength, wzNamespace, pcchActualLength);
	return S_OK;
}

HRESULT DecodeMetadataQueryReader::GetMetadataByName(LPCWSTR wzName, PROPVARIANT * pvarValue)
{
	TRACE2("(%s, %p)\n", wzName, pvarValue);
	return S_OK;
}

HRESULT DecodeMetadataQueryReader::GetEnumerator(IEnumString ** ppIEnumString)
{
	TRACE1("(%p)\n", ppIEnumString);
	return S_OK;
}

HRESULT DecodeMetadataQueryReader::GetContainerFormat(GUID * pguidContainerFormat)
{
	TRACE1("(%p)\n", pguidContainerFormat);
	if (pguidContainerFormat == NULL)
		return E_INVALIDARG;
	*pguidContainerFormat = GUID_ContainerFormatFLIF;
	return S_OK;
}


