/* 
 *	Copyright (C) 2003-2004 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "StdAfx.h"
#include "OggSplitter.h"

#include <initguid.h>
#include "..\..\..\..\include\ogg\OggDS.h"
#include "..\..\..\..\include\moreuuids.h"

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] =
{
	{&MEDIATYPE_Stream, &MEDIASUBTYPE_NULL},
};

const AMOVIESETUP_PIN sudpPins[] =
{
    { L"Input",             // Pins string name
      FALSE,                // Is it rendered
      FALSE,                // Is it an output
      FALSE,                // Are we allowed none
      FALSE,                // And allowed many
      &CLSID_NULL,          // Connects to filter
      NULL,                 // Connects to pin
      countof(sudPinTypesIn), // Number of types
      sudPinTypesIn         // Pin information
    },
    { L"Output",            // Pins string name
      FALSE,                // Is it rendered
      TRUE,                 // Is it an output
      FALSE,                // Are we allowed none
      FALSE,                // And allowed many
      &CLSID_NULL,          // Connects to filter
      NULL,                 // Connects to pin
      0,					// Number of types
      NULL					// Pin information
    },
};

const AMOVIESETUP_FILTER sudFilter[] =
{
	{&__uuidof(COggSplitterFilter), L"Ogg Splitter", MERIT_NORMAL+1, countof(sudpPins), sudpPins},
	{&__uuidof(COggSourceFilter), L"Ogg Source", MERIT_NORMAL+1, 0, NULL},
};

CFactoryTemplate g_Templates[] =
{
	{L"Ogg Splitter", &__uuidof(COggSplitterFilter), COggSplitterFilter::CreateInstance, NULL, &sudFilter[0]},
	{L"Ogg Source", &__uuidof(COggSourceFilter), COggSourceFilter::CreateInstance, NULL, &sudFilter[1]},
};

int g_cTemplates = countof(g_Templates);

#include "..\..\registry.cpp"

STDAPI DllRegisterServer()
{
	RegisterSourceFilter(
		CLSID_AsyncReader, 
		MEDIASUBTYPE_Ogg, 
		_T("0,4,,4F676753"), // OggS
		_T(".ogg"), _T(".ogm"), NULL);

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	UnRegisterSourceFilter(MEDIASUBTYPE_Ogg);

	return AMovieDllRegisterServer2(FALSE);
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    return DllEntryPoint((HINSTANCE)hModule, ul_reason_for_call, 0); // "DllMain" of the dshow baseclasses;
}

CUnknown* WINAPI COggSplitterFilter::CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
    CUnknown* punk = new COggSplitterFilter(lpunk, phr);
    if(punk == NULL) *phr = E_OUTOFMEMORY;
	return punk;
}

CUnknown* WINAPI COggSourceFilter::CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
    CUnknown* punk = new COggSourceFilter(lpunk, phr);
    if(punk == NULL) *phr = E_OUTOFMEMORY;
	return punk;
}

#endif

//
// bitstream
//

class bitstream
{
	BYTE* m_p;
	int m_len, m_pos;
public:
	bitstream(BYTE* p, int len, bool rev = false) : m_p(p), m_len(len*8) {m_pos = !rev ? 0 : len*8;}
	bool hasbits(int cnt)
	{
		int pos = m_pos+cnt;
		return(pos >= 0 && pos < m_len);
	}
	unsigned int showbits(int cnt) // a bit unclean, but works and can read backwards too! :P
	{
		if(!hasbits(cnt)) {ASSERT(0); return 0;}
		unsigned int ret = 0, off = 0;
		BYTE* p = m_p;
		if(cnt < 0)
		{
			p += (m_pos+cnt)>>3;
			off = (m_pos+cnt)&7;
			cnt = abs(cnt);
			ret = (*p++&(~0<<off))>>off; off = 8 - off; cnt -= off;
		}
		else
		{
			p += m_pos>>3;
			off = m_pos&7;
			ret = (*p++>>off)&((1<<min(cnt,8))-1); off = 0; cnt -= 8 - off;
		}
		while(cnt > 0) {ret |= (*p++&((1<<min(cnt,8))-1)) << off; off += 8; cnt -= 8;}
		return ret;
	}
	unsigned int getbits(int cnt)
	{
		unsigned int ret = showbits(cnt);
		m_pos += cnt;
		return ret;
	}
};

//
// COggSplitterFilter
//

COggSplitterFilter::COggSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(NAME("COggSplitterFilter"), pUnk, phr, __uuidof(this))
{
}

COggSplitterFilter::~COggSplitterFilter()
{
}

HRESULT COggSplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;

	m_pFile.Free();
	m_pChapters.RemoveAll();

	m_pFile.Attach(new COggFile(pAsyncReader, hr));
	if(!m_pFile) return E_OUTOFMEMORY;
	if(FAILED(hr)) {m_pFile.Free(); return hr;}

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = 0;

	m_rtDuration = 0;

	m_pFile->Seek(0);
	OggPage page;
	for(int i = 0, nWaitForMore = 0; m_pFile->Read(page); i++)
	{
		BYTE* p = page.GetData();

		if(!(page.m_hdr.header_type_flag & OggPageHeader::continued))
		{
			BYTE type = *p++;
			if(!(type&1) && nWaitForMore == 0)
				break;

			CStringW name;
			name.Format(L"Stream %d", i);

			HRESULT hr;

			if(type == 1 && (page.m_hdr.header_type_flag & OggPageHeader::first))
			{
				CAutoPtr<CBaseSplitterOutputPin> pPinOut;

				if(!memcmp(p, "vorbis", 6))
				{
					name.Format(L"Vorbis %d", i);
					pPinOut.Attach(new COggVorbisOutputPin((OggVorbisIdHeader*)(p+6), name, this, this, &hr));
					nWaitForMore++;
				}
				else if(!memcmp(p, "video", 5))
				{
					name.Format(L"Video %d", i);
					pPinOut.Attach(new COggVideoOutputPin((OggStreamHeader*)p, name, this, this, &hr));
				}
				else if(!memcmp(p, "audio", 5))
				{
					name.Format(L"Audio %d", i);
					pPinOut.Attach(new COggAudioOutputPin((OggStreamHeader*)p, name, this, this, &hr));
				}
				else if(!memcmp(p, "text", 4))
				{
					name.Format(L"Text %d", i);
					pPinOut.Attach(new COggTextOutputPin((OggStreamHeader*)p, name, this, this, &hr));
				}
				else if(!memcmp(p, "Direct Show Samples embedded in Ogg", 35))
				{
					name.Format(L"DirectShow %d", i);
					pPinOut.Attach(new COggDirectShowOutputPin((AM_MEDIA_TYPE*)(p+35+sizeof(GUID)), name, this, this, &hr));
				}

				AddOutputPin(page.m_hdr.bitstream_serial_number, pPinOut);
			}

			if(type == 3 && !memcmp(p, "vorbis", 6))
			{
				if(COggSplitterOutputPin* pOggPin = 
					dynamic_cast<COggSplitterOutputPin*>(GetOutputPin(page.m_hdr.bitstream_serial_number)))
				{
					pOggPin->AddComment(p+6, page.GetSize()-6-1);
				}
			}
		}

		if(COggVorbisOutputPin* pOggVorbisPin = 
			dynamic_cast<COggVorbisOutputPin*>(GetOutputPin(page.m_hdr.bitstream_serial_number)))
		{
			pOggVorbisPin->UnpackInitPage(page);
			if(pOggVorbisPin->IsInitialized()) nWaitForMore--;
		}
	}

	if(m_pOutputs.IsEmpty())
		return E_FAIL;

	m_pFile->Seek(max(m_pFile->GetLength()-65536, 0));
	while(m_pFile->Read(page))
	{
		COggSplitterOutputPin* pOggPin = dynamic_cast<COggSplitterOutputPin*>(GetOutputPin(page.m_hdr.bitstream_serial_number));
		if(!pOggPin) {ASSERT(0); continue;}
		if(page.m_hdr.granule_position == -1) continue;
		REFERENCE_TIME rt = pOggPin->GetRefTime(page.m_hdr.granule_position);
		m_rtDuration = max(rt, m_rtDuration);
	}

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration;

	// comments

	{
		CStringW fmt;
		fmt.Format(L"TITLE=%d,ARTIST=%d,COPYRIGHT=%d,DESCRIPTION=%d",
			Title, AuthorName, Copyright, Description);

		CList<CStringW> keys;
		Explode(fmt, keys, ',');

		POSITION pos2 = keys.GetHeadPosition();
		while(pos2)
		{
			CList<CStringW> sl;
			Explode(keys.GetNext(pos2), sl, '=', 2);

			CStringW key = sl.GetHead();
			mctype mct = (mctype)wcstol(sl.GetTail(), NULL, 10);

			POSITION pos = m_pOutputs.GetHeadPosition();
			while(pos)
			{
				CBaseSplitterOutputPin* pPin = m_pOutputs.GetNext(pos);
				COggSplitterOutputPin* pOggPin = dynamic_cast<COggSplitterOutputPin*>(pPin);
				CStringW value = pOggPin->GetComment(key);
				if(!value.IsEmpty())
				{
					SetMediaContentStr(value, mct);
					break;
				}
			}
		}

		POSITION pos = m_pOutputs.GetHeadPosition();
		while(pos && m_pChapters.GetCount() == 0)
		{
			CBaseSplitterOutputPin* pPin = m_pOutputs.GetNext(pos);
			COggSplitterOutputPin* pOggPin = dynamic_cast<COggSplitterOutputPin*>(pPin);

			for(int i = 1; pOggPin; i++)
			{
				CStringW key; 
				key.Format(L"CHAPTER%02d", i);
				CStringW time = pOggPin->GetComment(key);
				if(time.IsEmpty()) break;
				key.Format(L"CHAPTER%02dNAME", i);
				CStringW name = pOggPin->GetComment(key);
				if(name.IsEmpty()) name.Format(L"Chapter %d", i);
				int h, m, s, ms;
				WCHAR c;
				if(7 != swscanf(time, L"%d%c%d%c%d%c%d", &h, &c, &m, &c, &s, &c, &ms)) break;
				REFERENCE_TIME rt = ((((REFERENCE_TIME)h*60+m)*60+s)*1000+ms)*10000;
				CAutoPtr<CChapter> p(new CChapter(rt, name));
				m_pChapters.AddTail(p);
			}
		}
	}

	return m_pOutputs.GetCount() > 0 ? S_OK : E_FAIL;
}

bool COggSplitterFilter::InitDeliverLoop()
{
	if(!m_pFile) return(false);

	return(true);
}

void COggSplitterFilter::SeekDeliverLoop(REFERENCE_TIME rt)
{
	if(rt <= 0)
	{
		m_pFile->Seek(0);
	}
	else if(m_rtDuration > 0)
	{
		// oh, the horror...
clock_t t1 = clock();
		__int64 len = m_pFile->GetLength();
		__int64 startpos = len * rt/m_rtDuration;
		__int64 diff = 0;

		REFERENCE_TIME rtMinDiff = _I64_MAX;

		while(1)
		{
			__int64 endpos = startpos;
			REFERENCE_TIME rtPos = -1;

			OggPage page;
			m_pFile->Seek(startpos);
			while(m_pFile->Read(page, false))
			{
				if(page.m_hdr.granule_position == -1) continue;

				COggSplitterOutputPin* pOggPin = dynamic_cast<COggSplitterOutputPin*>(GetOutputPin(page.m_hdr.bitstream_serial_number));
				if(!pOggPin) {ASSERT(0); continue;}

				rtPos = pOggPin->GetRefTime(page.m_hdr.granule_position);
				endpos = m_pFile->GetPos();

				break;
			}

			__int64 rtDiff = rtPos - rt;

			if(rtDiff < 0)
			{
				rtDiff = -rtDiff;

				if(rtDiff < 1000000 || rtDiff >= rtMinDiff)
				{
					m_pFile->Seek(startpos);
					break;
				}

				rtMinDiff = rtDiff;
			}

			__int64 newpos = startpos;

			if(rtPos < rt && rtPos < m_rtDuration)
			{
				newpos = startpos + (__int64)((1.0*(rt - rtPos)/(m_rtDuration - rtPos)) * (len - startpos)) + 1024;
	            if(newpos < endpos) newpos = endpos + 1024;
			}
			else if(rtPos > rt && rtPos > 0)
			{
				newpos = startpos - (__int64)((1.0*(rtPos - rt)/(rtPos - 0)) * (startpos - 0)) - 1024;
	            if(newpos >= startpos) newpos = startpos - 1024;
			}
			else if(rtPos == rt)
			{
				m_pFile->Seek(startpos);
				break;
			}
			else
			{
				ASSERT(0);
				m_pFile->Seek(0);
				break;
			}

			diff = newpos - startpos;

			startpos = max(min(newpos, len), 0);
		}

TRACE(_T("****** t1: %d\n"), clock() - t1);
t1 = clock();
		m_pFile->Seek(startpos);

		POSITION pos = m_pOutputs.GetHeadPosition();
		while(pos)
		{
			CBaseSplitterOutputPin* pPin = m_pOutputs.GetNext(pos);
			COggVideoOutputPin* pOggVideoPin = dynamic_cast<COggVideoOutputPin*>(pPin);
			if(!pOggVideoPin) continue;

			bool fKeyFrameFound = false, fSkipKeyFrame = true;
			__int64 endpos = _I64_MAX;

			while(!(fKeyFrameFound && !fSkipKeyFrame) && startpos > 0)
			{
				OggPage page;
				while(!(fKeyFrameFound && !fSkipKeyFrame) && m_pFile->GetPos() < endpos && m_pFile->Read(page))
				{
					if(page.m_hdr.granule_position == -1) continue;

					COggSplitterOutputPin* pOggPin = dynamic_cast<COggSplitterOutputPin*>(GetOutputPin(page.m_hdr.bitstream_serial_number));
					if(pOggPin != pOggVideoPin) continue;

					REFERENCE_TIME rtPos = pOggPin->GetRefTime(page.m_hdr.granule_position);

					if(rtPos > rt)
						break;

					if(!fKeyFrameFound)
					{
						pOggPin->UnpackPage(page);

						CAutoPtr<OggPacket> p;
						while(p = pOggPin->GetPacket())
						{
							if(p->bSyncPoint)
							{
								fKeyFrameFound = true;
								fSkipKeyFrame = p->fSkip;
							}
						}

						if(fKeyFrameFound) break;
					}
					else
					{
						pOggPin->UnpackPage(page);

						CAutoPtr<OggPacket> p;
						while(p = pOggPin->GetPacket())
						{
							if(!p->fSkip)
							{
								fSkipKeyFrame = false;
								break;
							}
						}
					}
				}

				if(!(fKeyFrameFound && !fSkipKeyFrame)) {endpos = startpos; startpos = max(startpos - 10*65536, 0);}

				m_pFile->Seek(startpos);
			}
TRACE(_T("****** t2: %d\n"), clock() - t1);
t1 = clock();

#ifdef DEBUG
			// verify kf

			{
				fKeyFrameFound = false;

				OggPage page;
				while(!fKeyFrameFound && m_pFile->Read(page))
				{
					if(page.m_hdr.granule_position == -1) continue;

					COggSplitterOutputPin* pOggPin = dynamic_cast<COggSplitterOutputPin*>(GetOutputPin(page.m_hdr.bitstream_serial_number));
					if(pOggPin != pOggVideoPin) continue;

					REFERENCE_TIME rtPos = pOggPin->GetRefTime(page.m_hdr.granule_position);
					if(rtPos > rt)
						break;

					pOggPin->UnpackPage(page);

					CAutoPtr<OggPacket> p;
					while(p = pOggPin->GetPacket())
					{
						if(p->bSyncPoint)
						{
							fKeyFrameFound = true;
							break;
						}
					}
				}

				ASSERT(fKeyFrameFound);

				m_pFile->Seek(startpos);
			}
TRACE(_T("****** t3: %d\n"), clock() - t1);
t1 = clock();
#endif
			break;
		}
	}
}

bool COggSplitterFilter::DoDeliverLoop()
{
	HRESULT hr = S_OK;

	OggPage page;
	while(SUCCEEDED(hr) && !CheckRequest(NULL) && m_pFile->Read(page, true, GetRequestHandle()))
	{
		COggSplitterOutputPin* pOggPin = dynamic_cast<COggSplitterOutputPin*>(GetOutputPin(page.m_hdr.bitstream_serial_number));
		if(!pOggPin) {ASSERT(0); continue;}
		if(!pOggPin->IsConnected()) continue;
		if(FAILED(hr = pOggPin->UnpackPage(page))) {ASSERT(0); break;}
		CAutoPtr<OggPacket> p;
		while(!CheckRequest(NULL) && SUCCEEDED(hr) && (p = pOggPin->GetPacket()))
		{
			if(!p->fSkip)
				hr = DeliverPacket(p);
		}
	}

	return(true);
}

// IChapterInfo

STDMETHODIMP_(UINT) COggSplitterFilter::GetChapterCount(UINT aChapterID)
{
	return aChapterID == CHAPTER_ROOT_ID ? m_pChapters.GetCount() : 0;
}

STDMETHODIMP_(UINT) COggSplitterFilter::GetChapterId(UINT aParentChapterId, UINT aIndex)
{
	POSITION pos = m_pChapters.FindIndex(aIndex-1);
	if(aParentChapterId != CHAPTER_ROOT_ID || !pos)
		return CHAPTER_BAD_ID;
	return aIndex;
}

STDMETHODIMP_(BOOL) COggSplitterFilter::GetChapterInfo(UINT aChapterID, struct ChapterElement* pToFill)
{
	CheckPointer(pToFill, E_POINTER);
	POSITION pos = m_pChapters.FindIndex(aChapterID-1);
	if(!pos) return FALSE;
	CChapter* p = m_pChapters.GetNext(pos);
	WORD Size = pToFill->Size;
	if(Size >= sizeof(ChapterElement))
	{
		pToFill->Size = sizeof(ChapterElement);
		pToFill->Type = AtomicChapter;
		pToFill->ChapterId = aChapterID;
		pToFill->rtStart = p->m_rt;
		pToFill->rtStop = pos ? m_pChapters.GetNext(pos)->m_rt : m_rtDuration;
	}
	return TRUE;
}

STDMETHODIMP_(BSTR) COggSplitterFilter::GetChapterStringInfo(UINT aChapterID, CHAR PreferredLanguage[3], CHAR CountryCode[2])
{
	POSITION pos = m_pChapters.FindIndex(aChapterID-1);
	if(!pos) return NULL;
	return m_pChapters.GetAt(pos)->m_name.AllocSysString();
}

//
// COggSourceFilter
//

COggSourceFilter::COggSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: COggSplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);
	m_pInput.Free();
}

//
// COggSplitterOutputPin
//

COggSplitterOutputPin::COggSplitterOutputPin(LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseSplitterOutputPin(pName, pFilter, pLock, phr)
{
	ResetState(-1);
}

void COggSplitterOutputPin::AddComment(BYTE* p, int len)
{
	bitstream bs(p, len);
	bs.getbits(bs.getbits(32)*8);
	for(int n = bs.getbits(32); n-- > 0; )
	{
		CStringA str;
		for(int len = bs.getbits(32); len-- > 0; )
			str += (CHAR)bs.getbits(8);

		CList<CStringA> sl;
		Explode(str, sl, '=', 2);
		if(sl.GetCount() == 2)
		{
			CAutoPtr<CComment> p(new CComment(UTF8To16(sl.GetHead()), UTF8To16(sl.GetTail())));
			if(p->m_key == L"LANGUAGE") SetName(p->m_value /*+ L" (" + m_pName + L")"*/);
			m_pComments.AddTail(p);
		}
	}
	ASSERT(bs.getbits(1) == 1);
}

CStringW COggSplitterOutputPin::GetComment(CStringW key)
{
	key.MakeUpper();
	CList<CStringW> sl;
	POSITION pos = m_pComments.GetHeadPosition();
	while(pos)
	{
		CComment* p = m_pComments.GetNext(pos);
		if(key == p->m_key) sl.AddTail(p->m_value);
	}
	return Implode(sl, ';');
}

void COggSplitterOutputPin::ResetState(DWORD seqnum)
{
	CAutoLock csAutoLock(&m_csPackets);
	m_packets.RemoveAll();
	m_lastpacket.Free();
	m_lastseqnum = seqnum;
	m_rtLast = 0;
	m_fSkip = true;
}

HRESULT COggSplitterOutputPin::UnpackPage(OggPage& page)
{
	if(m_lastseqnum != page.m_hdr.page_sequence_number-1)
	{
		ResetState(page.m_hdr.page_sequence_number);
	}
	else
	{
		m_lastseqnum = page.m_hdr.page_sequence_number;
	}

	POSITION first = page.m_lens.GetHeadPosition();
	while(first && page.m_lens.GetAt(first) == 255) page.m_lens.GetNext(first);
	if(!first) first = page.m_lens.GetTailPosition();

	POSITION last = page.m_lens.GetTailPosition();
	while(last && page.m_lens.GetAt(last) == 255) page.m_lens.GetPrev(last);
	if(!last) last = page.m_lens.GetTailPosition();

	BYTE* pData = page.GetData();

	int i = 0, j = 0, len = 0;

    for(POSITION pos = page.m_lens.GetHeadPosition(); pos; page.m_lens.GetNext(pos))
	{
		len = page.m_lens.GetAt(pos);
		j += len;

		if(len < 255 || pos == page.m_lens.GetTailPosition())
		{
			if(first == pos && (page.m_hdr.header_type_flag & OggPageHeader::continued))
			{
//				ASSERT(m_lastpacket);

				if(m_lastpacket)
				{
					int size = m_lastpacket->pData.GetSize();
					m_lastpacket->pData.SetSize(size + j-i);
					memcpy(m_lastpacket->pData.GetData() + size, pData + i, j-i);

					CAutoLock csAutoLock(&m_csPackets);

					if(len < 255) m_packets.AddTail(m_lastpacket);
				}
			}
			else
			{
				CAutoPtr<OggPacket> p(new OggPacket());

				if(last == pos && page.m_hdr.granule_position != -1)
				{
					p->bDiscontinuity = m_fSkip;
REFERENCE_TIME rtLast = m_rtLast;
					m_rtLast = GetRefTime(page.m_hdr.granule_position);
// some bad encodings have a +/-1 frame difference from the normal timeline, 
// but these seem to cancel eachother out nicely so we can just ignore them 
// to make it play a bit more smooth.
if(abs(rtLast - m_rtLast) == GetRefTime(1)) m_rtLast = rtLast; // FIXME
					m_fSkip = false;
				}

				p->TrackNumber = page.m_hdr.bitstream_serial_number;

				if(S_OK == UnpackPacket(p, pData + i, j-i))
				{

if(p->TrackNumber == 0)
TRACE(_T("[%d]: %d, %I64d -> %I64d (skip=%d, disc=%d, sync=%d)\n"), 
		(int)p->TrackNumber, p->pData.GetSize(), p->rtStart, p->rtStop,
		(int)m_fSkip, (int)p->bDiscontinuity, (int)p->bSyncPoint);

					CAutoLock csAutoLock(&m_csPackets);

					m_rtLast = p->rtStop;
					p->fSkip = m_fSkip;

					if(len < 255) m_packets.AddTail(p);
					else m_lastpacket = p;
				}
			}

			i = j;
		}
	}

	return S_OK;
}

CAutoPtr<OggPacket> COggSplitterOutputPin::GetPacket()
{
	CAutoPtr<OggPacket> p;
	CAutoLock csAutoLock(&m_csPackets);
	if(m_packets.GetCount()) p = m_packets.RemoveHead();
	return p;
}

HRESULT COggSplitterOutputPin::DeliverEndFlush()
{
	ResetState();
	return __super::DeliverEndFlush();
}

HRESULT COggSplitterOutputPin::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	ResetState();
	return __super::DeliverNewSegment(tStart, tStop, dRate);
}

//
// COggVorbisOutputPin
//

COggVorbisOutputPin::COggVorbisOutputPin(OggVorbisIdHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: COggSplitterOutputPin(pName, pFilter, pLock, phr)
{
	m_audio_sample_rate = h->audio_sample_rate;
	m_blocksize[0] = 1<<h->blocksize_0;
	m_blocksize[1] = 1<<h->blocksize_1;
	m_lastblocksize = 0;

	CMediaType mt;

	mt.InitMediaType();
	mt.majortype = MEDIATYPE_Audio;
	mt.subtype = MEDIASUBTYPE_Vorbis;
	mt.formattype = FORMAT_VorbisFormat;
	VORBISFORMAT* vf = (VORBISFORMAT*)mt.AllocFormatBuffer(sizeof(VORBISFORMAT));
	memset(mt.Format(), 0, mt.FormatLength());
	vf->nChannels = h->audio_channels;
	vf->nSamplesPerSec = h->audio_sample_rate;
	vf->nAvgBitsPerSec = h->bitrate_nominal;
	vf->nMinBitsPerSec = h->bitrate_minimum;
	vf->nMaxBitsPerSec = h->bitrate_maximum;
	vf->fQuality = -1;
	mt.SetSampleSize(8192);
	m_mts.Add(mt);

	mt.InitMediaType();
	mt.majortype = MEDIATYPE_Audio;
	mt.subtype = MEDIASUBTYPE_Vorbis2;
	mt.formattype = FORMAT_VorbisFormat2;
	VORBISFORMAT2* vf2 = (VORBISFORMAT2*)mt.AllocFormatBuffer(sizeof(VORBISFORMAT2));
	memset(mt.Format(), 0, mt.FormatLength());
	vf2->Channels = h->audio_channels;
	vf2->SamplesPerSec = h->audio_sample_rate;
	mt.SetSampleSize(8192);
	m_mts.InsertAt(0, mt);
}

REFERENCE_TIME COggVorbisOutputPin::GetRefTime(__int64 granule_position)
{
	REFERENCE_TIME rt = granule_position * 10000000 / m_audio_sample_rate;
	return rt;
}

HRESULT COggVorbisOutputPin::UnpackInitPage(OggPage& page)
{
	HRESULT hr = __super::UnpackPage(page);

	while(m_packets.GetCount())
	{
		Packet* p = m_packets.GetHead();

		if(p->pData.GetCount() >= 6 && p->pData.GetData()[0] == 0x05)
		{
			// yeah, right, we are going to be parsing this backwards! :P
			bitstream bs(p->pData.GetData(), p->pData.GetCount(), true);
			while(bs.hasbits(-1) && bs.getbits(-1) != 1);
			for(int cnt = 0; bs.hasbits(-8-16-16-1-6); cnt++)
			{
				unsigned int modes = bs.showbits(-6)+1;

				unsigned int mapping = bs.getbits(-8);
				unsigned int transformtype = bs.getbits(-16);
				unsigned int windowtype = bs.getbits(-16);
				unsigned int blockflag = bs.getbits(-1);

				if(transformtype != 0 || windowtype != 0)
				{
					ASSERT(modes == cnt);
					break;
				}

				m_blockflags.InsertAt(0, !!blockflag);
			}
		}

		int cnt = m_initpackets.GetCount();
		if(cnt <= 3)
		{
			ASSERT(p->pData.GetCount() >= 6 && p->pData.GetData()[0] == 1+cnt*2);
			VORBISFORMAT2* vf2 = (VORBISFORMAT2*)m_mts[0].Format();
			vf2->HeaderSize[cnt] = p->pData.GetSize();
			int len = m_mts[0].FormatLength();
			memcpy(
				m_mts[0].ReallocFormatBuffer(len + p->pData.GetSize()) + len, 
				p->pData.GetData(), p->pData.GetSize());
		}

		m_initpackets.AddTail(m_packets.RemoveHead());
	}

	return hr;
}

HRESULT COggVorbisOutputPin::UnpackPacket(CAutoPtr<OggPacket>& p, BYTE* pData, int len)
{
	if(len > 0 && m_blockflags.GetCount())
	{
		bitstream bs(pData, len);
		if(bs.getbits(1) == 0)
		{
			int x = m_blockflags.GetCount()-1, n = 0;
			while(x) {n++; x >>= 1;}
			DWORD blocksize = m_blocksize[m_blockflags[bs.getbits(n)]?1:0];
			if(m_lastblocksize) m_rtLast += GetRefTime((m_lastblocksize + blocksize) >> 2);
			m_lastblocksize = blocksize;
		}
	}

	p->bSyncPoint = TRUE;
	p->rtStart = m_rtLast;
	p->rtStop = m_rtLast+1;
	p->pData.SetSize(len);
	memcpy(p->pData.GetData(), pData, len);

	return S_OK;
}

HRESULT COggVorbisOutputPin::DeliverPacket(CAutoPtr<OggPacket> p)
{
	if(p->pData.GetSize() > 0 && (p->pData.GetData()[0]&1))
		return S_OK;

	return __super::DeliverPacket(p);
}

HRESULT COggVorbisOutputPin::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	HRESULT hr = __super::DeliverNewSegment(tStart, tStop, dRate);

	m_lastblocksize = 0;

	if(m_mt.subtype == MEDIASUBTYPE_Vorbis)
	{
		POSITION pos = m_initpackets.GetHeadPosition();
		while(pos)
		{
			Packet* pi = m_initpackets.GetNext(pos);
			CAutoPtr<OggPacket> p(new OggPacket());
			p->TrackNumber = pi->TrackNumber;
			p->bDiscontinuity = p->bSyncPoint = TRUE;
			p->rtStart = p->rtStop = 0;
			p->pData.Copy(pi->pData);
			__super::DeliverPacket(p);
		}
	}

	return hr;
}

//
// COggDirectShowOutputPin
//

COggDirectShowOutputPin::COggDirectShowOutputPin(AM_MEDIA_TYPE* pmt, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: COggSplitterOutputPin(pName, pFilter, pLock, phr)
{
	CMediaType mt;
	memcpy((AM_MEDIA_TYPE*)&mt, pmt, FIELD_OFFSET(AM_MEDIA_TYPE, pUnk));
	mt.SetFormat((BYTE*)(pmt+1), pmt->cbFormat);
	mt.SetSampleSize(1);
	if(mt.majortype == MEDIATYPE_Video) // TODO: find samples for audio and find out what to return in GetRefTime...
		m_mts.Add(mt);
}

REFERENCE_TIME COggDirectShowOutputPin::GetRefTime(__int64 granule_position)
{
	REFERENCE_TIME rt = 0;

	if(m_mt.majortype == MEDIATYPE_Video)
	{
		rt = granule_position * ((VIDEOINFOHEADER*)m_mt.Format())->AvgTimePerFrame;
	}
	else if(m_mt.majortype == MEDIATYPE_Audio)
	{
		rt = granule_position; // ((WAVEFORMATEX*)m_mt.Format())-> // TODO
	}

	return rt;
}

HRESULT COggDirectShowOutputPin::UnpackPacket(CAutoPtr<OggPacket>& p, BYTE* pData, int len)
{
	int i = 0;

	BYTE hdr = pData[i++];

	if(!(hdr&1))
	{
		// TODO: verify if this was still present in the old format (haven't found one sample yet)
		BYTE nLenBytes = (hdr>>6)|((hdr&2)<<1);
		__int64 Length = 0;
		for(int j = 0; j < nLenBytes; j++)
			Length |= (__int64)pData[i++] << (j << 3);

		p->bSyncPoint = !!(hdr&8);
		p->rtStart = m_rtLast;
		p->rtStop = m_rtLast + (nLenBytes ? GetRefTime(Length) : GetRefTime(1));
		p->pData.SetSize(len-i);
		memcpy(p->pData.GetData(), &pData[i], len-i);

		return S_OK;
	}

	return S_FALSE;
}

//
// COggStreamOutputPin
//

COggStreamOutputPin::COggStreamOutputPin(OggStreamHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: COggSplitterOutputPin(pName, pFilter, pLock, phr)
{
	m_time_unit = h->time_unit;
	m_samples_per_unit = h->samples_per_unit;
	m_default_len = h->default_len;
}

REFERENCE_TIME COggStreamOutputPin::GetRefTime(__int64 granule_position)
{
	return granule_position * m_time_unit / m_samples_per_unit;
}

HRESULT COggStreamOutputPin::UnpackPacket(CAutoPtr<OggPacket>& p, BYTE* pData, int len)
{
	int i = 0;

	BYTE hdr = pData[i++];

	if(!(hdr&1))
	{
		BYTE nLenBytes = (hdr>>6)|((hdr&2)<<1);
		__int64 Length = 0;
		for(int j = 0; j < nLenBytes; j++)
			Length |= (__int64)pData[i++] << (j << 3);

		p->bSyncPoint = !!(hdr&8);
		p->rtStart = m_rtLast;
		p->rtStop = m_rtLast + (nLenBytes ? GetRefTime(Length) : GetRefTime(m_default_len));
		p->pData.SetSize(len-i);
		memcpy(p->pData.GetData(), &pData[i], len-i);

		return S_OK;
	}

	return S_FALSE;
}

//
// COggVideoOutputPin
//

COggVideoOutputPin::COggVideoOutputPin(OggStreamHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: COggStreamOutputPin(h, pName, pFilter, pLock, phr)
{
	int extra = (int)h->size - sizeof(OggStreamHeader);
	extra = max(extra, 0);	

	CMediaType mt;
	mt.majortype = MEDIATYPE_Video;
	mt.subtype = FOURCCMap(MAKEFOURCC(h->subtype[0],h->subtype[1],h->subtype[2],h->subtype[3]));
	mt.formattype = FORMAT_VideoInfo;
	VIDEOINFOHEADER* pvih = (VIDEOINFOHEADER*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER) + extra);
	memset(mt.Format(), 0, mt.FormatLength());
	memcpy(mt.Format() + sizeof(VIDEOINFOHEADER), h+1, extra);
	pvih->AvgTimePerFrame = h->time_unit / h->samples_per_unit;
	pvih->bmiHeader.biWidth = h->v.w;
	pvih->bmiHeader.biHeight = h->v.h;
	pvih->bmiHeader.biBitCount = (WORD)h->bps;
	pvih->bmiHeader.biCompression = mt.subtype.Data1;
	switch(pvih->bmiHeader.biCompression)
	{
	case BI_RGB: case BI_BITFIELDS: mt.subtype = 
		pvih->bmiHeader.biBitCount == 1 ? MEDIASUBTYPE_RGB1 :
		pvih->bmiHeader.biBitCount == 4 ? MEDIASUBTYPE_RGB4 :
		pvih->bmiHeader.biBitCount == 8 ? MEDIASUBTYPE_RGB8 :
		pvih->bmiHeader.biBitCount == 16 ? MEDIASUBTYPE_RGB565 :
		pvih->bmiHeader.biBitCount == 24 ? MEDIASUBTYPE_RGB24 :
		pvih->bmiHeader.biBitCount == 32 ? MEDIASUBTYPE_RGB32 :
		MEDIASUBTYPE_NULL;
		break;
	case BI_RLE8: mt.subtype = MEDIASUBTYPE_RGB8; break;
	case BI_RLE4: mt.subtype = MEDIASUBTYPE_RGB4; break;
	}
	mt.SetSampleSize(max(h->buffersize, 1));
	m_mts.Add(mt);
}

//
// COggAudioOutputPin
//

COggAudioOutputPin::COggAudioOutputPin(OggStreamHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: COggStreamOutputPin(h, pName, pFilter, pLock, phr)
{
	int extra = (int)h->size - sizeof(OggStreamHeader);
	extra = max(extra, 0);	

	CMediaType mt;
	mt.majortype = MEDIATYPE_Audio;
	mt.subtype = FOURCCMap(strtol(CStringA(h->subtype, 4), NULL, 16));
	mt.formattype = FORMAT_WaveFormatEx;
	WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX) + extra);
	memset(mt.Format(), 0, mt.FormatLength());
	memcpy(mt.Format() + sizeof(WAVEFORMATEX), h+1, extra);
	wfe->cbSize = extra;
	wfe->wFormatTag = (WORD)mt.subtype.Data1;
	wfe->nChannels = h->a.nChannels;
	wfe->nSamplesPerSec = (DWORD)(10000000i64 * h->samples_per_unit / h->time_unit);
	wfe->wBitsPerSample = (WORD)h->bps;
	wfe->nAvgBytesPerSec = h->a.nAvgBytesPerSec; // TODO: verify for PCM
	wfe->nBlockAlign = h->a.nBlockAlign; // TODO: verify for PCM
	mt.SetSampleSize(max(h->buffersize, 1));
	m_mts.Add(mt);
}

//
// COggTextOutputPin
//

COggTextOutputPin::COggTextOutputPin(OggStreamHeader* h, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: COggStreamOutputPin(h, pName, pFilter, pLock, phr)
{
	CMediaType mt;
	mt.majortype = MEDIATYPE_Text;
	mt.subtype = MEDIASUBTYPE_NULL;
	mt.formattype = FORMAT_None;
	mt.SetSampleSize(1);
	m_mts.Add(mt);
}
