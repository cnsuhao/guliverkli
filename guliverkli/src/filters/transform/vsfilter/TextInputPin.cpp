#include "stdafx.h"
#include "DirectVobSubFilter.h"
#include "TextInputPin.h"
#include "..\..\..\DSUtil\DSUtil.h"

// our first format id
#define __GAB1__ "GAB1"

// our tags for __GAB1__ (ushort) + size (ushort)

// "lang" + '0'
#define __GAB1_LANGUAGE__ 0
// (int)start+(int)stop+(char*)line+'0'
#define __GAB1_ENTRY__ 1
// L"lang" + '0'
#define __GAB1_LANGUAGE_UNICODE__ 2
// (int)start+(int)stop+(WCHAR*)line+'0'
#define __GAB1_ENTRY_UNICODE__ 3

// same as __GAB1__, but the size is (uint) and only __GAB1_LANGUAGE_UNICODE__ is valid
#define __GAB2__ "GAB2"

// (BYTE*)
#define __GAB1_RAWTEXTSUBTITLE__ 4

CTextInputPin::CTextInputPin(CDirectVobSubFilter* pFilter, CCritSec* pLock, CCritSec* pSubLock, HRESULT* phr)
	: CBaseInputPin(NAME("CTextInputPin"), pFilter, pLock, phr, L"Input")
	, m_pFilter(pFilter)
	, m_pSubLock(pSubLock)
{
}

HRESULT CTextInputPin::CheckMediaType(const CMediaType* pmt)
{
	return(IsEqualGUID(*pmt->Type(), MEDIATYPE_Text) ? S_OK : E_FAIL);
}

HRESULT CTextInputPin::CompleteConnect(IPin* pReceivePin)
{
	if(!(m_pSubStream = new CRenderedTextSubtitle(m_pSubLock))) return E_FAIL;

	CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)(ISubStream*)m_pSubStream;
	pRTS->m_name = GetPinName(pReceivePin) + _T(" (embeded)");
	pRTS->CreateDefaultStyle(DEFAULT_CHARSET);

	m_pFilter->AddSubStream(m_pSubStream);

    HRESULT hr = m_pFilter->CompleteConnect(PINDIR_INPUT, pReceivePin);
    if(FAILED(hr)) 
		return hr;

    return CBaseInputPin::CompleteConnect(pReceivePin);
}

HRESULT CTextInputPin::BreakConnect()
{
	m_pFilter->RemoveSubStream(m_pSubStream);
	m_pSubStream = NULL;

	ASSERT(IsStopped());

	m_pFilter->BreakConnect(PINDIR_INPUT);

    return CBaseInputPin::BreakConnect();
}

STDMETHODIMP CTextInputPin::Receive(IMediaSample* pSample)
{
	HRESULT hr;

	hr = CBaseInputPin::Receive(pSample);
    if(FAILED(hr)) return hr;

	REFERENCE_TIME tStart, tStop;
    pSample->GetTime(&tStart, &tStop);
	tStart += m_tStart; 
	tStop += m_tStart;

TRACE(_T("CTextInputPin: tStart = %I64d, tStop = %I64d\n"), tStart, tStop);

	BYTE* pData = NULL;
    hr = pSample->GetPointer(&pData);
    if(FAILED(hr) || pData == NULL) return hr;

	bool fInvalidate = false;

	{
		CAutoLock cAutoLock(m_pSubLock);
		CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)(ISubStream*)m_pSubStream;

		int len = pSample->GetActualDataLength();

		if(!strncmp((char*)pData, __GAB1__, strlen(__GAB1__)))
		{
			char* ptr = (char*)&pData[strlen(__GAB1__)+1];
			char* end = (char*)&pData[len];

			while(ptr < end)
			{
				WORD tag = *((WORD*)(ptr)); ptr += 2;
				WORD size = *((WORD*)(ptr)); ptr += 2;

				if(tag == __GAB1_LANGUAGE__)
				{
					pRTS->m_name = CString(ptr);
				}
				else if(tag == __GAB1_ENTRY__)
				{
					pRTS->Add(AToW(&ptr[8]), false, *(int*)ptr, *(int*)(ptr+4));
					fInvalidate = true;
				}
				else if(tag == __GAB1_LANGUAGE_UNICODE__)
				{
					pRTS->m_name = (WCHAR*)ptr;
				}
				else if(tag == __GAB1_ENTRY_UNICODE__)
				{
					pRTS->Add((WCHAR*)(ptr+8), true, *(int*)ptr, *(int*)(ptr+4));
					fInvalidate = true;
				}

				ptr += size;
			}
		}
		else if(!strncmp((char*)pData, __GAB2__, strlen(__GAB2__)))
		{
			char* ptr = (char*)&pData[strlen(__GAB2__)+1];
			char* end = (char*)&pData[len];

			while(ptr < end)
			{
				WORD tag = *((WORD*)(ptr)); ptr += 2;
				DWORD size = *((DWORD*)(ptr)); ptr += 4;

				if(tag == __GAB1_LANGUAGE_UNICODE__)
				{
					pRTS->m_name = (WCHAR*)ptr;
				}
				else if(tag == __GAB1_RAWTEXTSUBTITLE__)
				{
					pRTS->Open((BYTE*)ptr, size, DEFAULT_CHARSET, pRTS->m_name);
					fInvalidate = true;
				}

				ptr += size;
			}
		}
		else if(pData != 0 && len > 1 && *pData != 0)
		{
			CStringA str = (char*)pData;

			str.Replace("\r\n", "\n");
			str.Trim();

			str.Replace("<i>", "{\\i1}");
			str.Replace("</i>", "{\\i}");
			str.Replace("<b>", "{\\b1}");
			str.Replace("</b>", "{\\b}");
			str.Replace("<u>", "{\\u1}");
			str.Replace("</u>", "{\\u}");

			if(!str.IsEmpty())
			{
				pRTS->Add(AToW(str), false, (int)(tStart / 10000), (int)(tStop / 10000));
				fInvalidate = true;

				m_pFilter->Post_EC_OLE_EVENT((char*)pData, (DWORD_PTR)(ISubStream*)m_pSubStream);
			}
		}
	}

	if(fInvalidate)
	{
		// IMPORTANT: m_pSubLock must not be locked when calling this
		m_pFilter->InvalidateSubtitle((DWORD_PTR)(ISubStream*)m_pSubStream);
	}

    return S_OK;
}
