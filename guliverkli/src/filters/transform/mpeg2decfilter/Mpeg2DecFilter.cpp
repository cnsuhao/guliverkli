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

#include "stdafx.h"
#include <math.h>
#include <atlbase.h>
#include <ks.h>
#include <ksmedia.h>
#include "libmpeg2.h"
#include "Mpeg2DecFilter.h"

#include "..\..\..\DSUtil\DSUtil.h"
#include "..\..\..\DSUtil\MediaTypes.h"

#include <initguid.h>
#include "..\..\..\..\include\moreuuids.h"
#include "..\..\..\..\include\matroska\matroska.h"

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] =
{
	{&MEDIATYPE_DVD_ENCRYPTED_PACK, &MEDIASUBTYPE_MPEG2_VIDEO},
	{&MEDIATYPE_MPEG2_PACK, &MEDIASUBTYPE_MPEG2_VIDEO},
	{&MEDIATYPE_MPEG2_PES, &MEDIASUBTYPE_MPEG2_VIDEO},
	{&MEDIATYPE_Video, &MEDIASUBTYPE_MPEG2_VIDEO},
	{&MEDIATYPE_Video, &MEDIASUBTYPE_MPEG1Packet},
	{&MEDIATYPE_Video, &MEDIASUBTYPE_MPEG1Payload},
};

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] =
{
	{&MEDIATYPE_Video, &MEDIASUBTYPE_IYUV},
};

const AMOVIESETUP_PIN sudpPins[] =
{
    {L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, countof(sudPinTypesIn), sudPinTypesIn},
    {L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, countof(sudPinTypesOut), sudPinTypesOut}
};

const AMOVIESETUP_FILTER sudFilter[] =
{
	{&__uuidof(CMpeg2DecFilter), L"Mpeg2Dec Filter", 0x40000002, countof(sudpPins), sudpPins},
};

CFactoryTemplate g_Templates[] =
{
    {sudFilter[0].strName, sudFilter[0].clsID, CreateInstance<CMpeg2DecFilter>, NULL, &sudFilter[0]},
};

int g_cTemplates = countof(g_Templates);

STDAPI DllRegisterServer()
{
	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2(FALSE);
}

//

#include "..\..\..\..\include\detours\detours.h"

DETOUR_TRAMPOLINE(BOOL WINAPI Real_IsDebuggerPresent(), IsDebuggerPresent);
BOOL WINAPI Mine_IsDebuggerPresent()
{
	TRACE(_T("Oops, somebody was trying to be naughty! (called IsDebuggerPresent)\n")); 
	return FALSE;
}

DETOUR_TRAMPOLINE(LONG WINAPI Real_ChangeDisplaySettingsExA(LPCSTR lpszDeviceName, LPDEVMODEA lpDevMode, HWND hwnd, DWORD dwFlags, LPVOID lParam), ChangeDisplaySettingsExA);
DETOUR_TRAMPOLINE(LONG WINAPI Real_ChangeDisplaySettingsExW(LPCWSTR lpszDeviceName, LPDEVMODEW lpDevMode, HWND hwnd, DWORD dwFlags, LPVOID lParam), ChangeDisplaySettingsExW);
LONG WINAPI Mine_ChangeDisplaySettingsEx(LONG ret, DWORD dwFlags, LPVOID lParam)
{
	if(dwFlags&CDS_VIDEOPARAMETERS)
	{
		VIDEOPARAMETERS* vp = (VIDEOPARAMETERS*)lParam;

		if(vp->Guid == GUIDFromCString(_T("{02C62061-1097-11d1-920F-00A024DF156E}"))
		&& (vp->dwFlags&VP_FLAGS_COPYPROTECT))
		{
			if(vp->dwCommand == VP_COMMAND_GET)
			{
				if((vp->dwTVStandard&VP_TV_STANDARD_WIN_VGA) && vp->dwTVStandard != VP_TV_STANDARD_WIN_VGA)
				{
					TRACE(_T("Ooops, tv-out enabled? macrovision checks suck..."));
					vp->dwTVStandard = VP_TV_STANDARD_WIN_VGA;
				}
			}
			else if(vp->dwCommand == VP_COMMAND_SET)
			{
				TRACE(_T("Ooops, as I already told ya, no need for any macrovision bs here"));
				return 0;
			}
		}
	}

	return ret;
}
LONG WINAPI Mine_ChangeDisplaySettingsExA(LPCSTR lpszDeviceName, LPDEVMODEA lpDevMode, HWND hwnd, DWORD dwFlags, LPVOID lParam)
{
	return Mine_ChangeDisplaySettingsEx(Real_ChangeDisplaySettingsExA(lpszDeviceName, lpDevMode, hwnd, dwFlags, lParam), dwFlags, lParam);
}
LONG WINAPI Mine_ChangeDisplaySettingsExW(LPCWSTR lpszDeviceName, LPDEVMODEW lpDevMode, HWND hwnd, DWORD dwFlags, LPVOID lParam)
{
	return Mine_ChangeDisplaySettingsEx(Real_ChangeDisplaySettingsExW(lpszDeviceName, lpDevMode, hwnd, dwFlags, lParam), dwFlags, lParam);
}

bool fDetourInited = false;

//

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	if(!fDetourInited)
	{
		DetourFunctionWithTrampoline((PBYTE)Real_IsDebuggerPresent, (PBYTE)Mine_IsDebuggerPresent);
		DetourFunctionWithTrampoline((PBYTE)Real_ChangeDisplaySettingsExA, (PBYTE)Mine_ChangeDisplaySettingsExA);
		DetourFunctionWithTrampoline((PBYTE)Real_ChangeDisplaySettingsExW, (PBYTE)Mine_ChangeDisplaySettingsExW);
		fDetourInited = true;
	}

	return DllEntryPoint((HINSTANCE)hModule, dwReason, 0); // "DllMain" of the dshow baseclasses;
}

#endif

//
// CMpeg2DecFilter
//

CMpeg2DecFilter::CMpeg2DecFilter(LPUNKNOWN lpunk, HRESULT* phr) 
	: CTransformFilter(NAME("CMpeg2DecFilter"), lpunk, __uuidof(this))
	, m_fWaitForKeyFrame(true)
{
	if(phr) *phr = S_OK;

	if(!(m_pInput = new CMpeg2DecInputPin(this, phr, L"Video"))) *phr = E_OUTOFMEMORY;
	if(FAILED(*phr)) return;

	if(!(m_pOutput = new CMpeg2DecOutputPin(this, phr))) *phr = E_OUTOFMEMORY;
	if(FAILED(*phr))  {delete m_pInput, m_pInput = NULL; return;}

	if(!(m_pSubpicInput = new CSubpicInputPin(this, phr))) *phr = E_OUTOFMEMORY;
	if(FAILED(*phr)) return;

	if(!(m_pClosedCaptionOutput = new CClosedCaptionOutputPin(this, m_pLock, phr))) *phr = E_OUTOFMEMORY;
	if(FAILED(*phr)) return;

	SetDeinterlaceMethod(DIAuto);
	SetBrightness(0.0);
	SetContrast(1.0);
	SetHue(0.0);
	SetSaturation(1.0);
	EnableForcedSubtitles(true);
	EnablePlanarYUV(true);

	m_rate.Rate = 10000;
	m_rate.StartTime = 0;
}

CMpeg2DecFilter::~CMpeg2DecFilter()
{
	delete m_pSubpicInput;
	delete m_pClosedCaptionOutput;
}

STDMETHODIMP CMpeg2DecFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(IMpeg2DecFilter)
		 __super::NonDelegatingQueryInterface(riid, ppv);
}

int CMpeg2DecFilter::GetPinCount()
{
	return 4;
}

CBasePin* CMpeg2DecFilter::GetPin(int n)
{
	switch(n)
	{
	case 0: return m_pInput;
	case 1: return m_pOutput;
	case 2: return m_pSubpicInput;
	case 3: return m_pClosedCaptionOutput;
	}
	return NULL;
}

HRESULT CMpeg2DecFilter::EndOfStream()
{
	CAutoLock cAutoLock(&m_csReceive);
	m_pClosedCaptionOutput->EndOfStream();
	return __super::EndOfStream();
}

HRESULT CMpeg2DecFilter::BeginFlush()
{
	m_pClosedCaptionOutput->DeliverBeginFlush();
	return __super::BeginFlush();
}

HRESULT CMpeg2DecFilter::EndFlush()
{
	m_pClosedCaptionOutput->DeliverEndFlush();
	return __super::EndFlush();
}

HRESULT CMpeg2DecFilter::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	CAutoLock cAutoLock(&m_csReceive);
	m_pClosedCaptionOutput->DeliverNewSegment(tStart, tStop, dRate);
	m_fDropFrames = false;
	return __super::NewSegment(tStart, tStop, dRate);
}

HRESULT CMpeg2DecFilter::Receive(IMediaSample* pIn)
{
	CAutoLock cAutoLock(&m_csReceive);

	HRESULT hr;

    AM_SAMPLE2_PROPERTIES* const pProps = m_pInput->SampleProps();
    if(pProps->dwStreamId != AM_STREAM_MEDIA)
		return m_pOutput->Deliver(pIn);

	AM_MEDIA_TYPE* pmt;
	if(SUCCEEDED(pIn->GetMediaType(&pmt)) && pmt)
	{
		CMediaType mt(*pmt);
		m_pInput->SetMediaType(&mt);
		DeleteMediaType(pmt);

		ResetMpeg2Decoder();
	}

	BYTE* pDataIn = NULL;
	if(FAILED(hr = pIn->GetPointer(&pDataIn))) return hr;

	long len = pIn->GetActualDataLength();

	((CDeCSSInputPin*)m_pInput)->StripPacket(pDataIn, len);

//TRACE(_T("%d\n"), len);

	if(pIn->IsDiscontinuity() == S_OK)
	{
		ResetMpeg2Decoder();
	}

	REFERENCE_TIME rtStart = _I64_MIN, rtStop = _I64_MIN;
	hr = pIn->GetTime(&rtStart, &rtStop);
	if(FAILED(hr)) rtStart = rtStop = _I64_MIN;
//else TRACE(_T("rtStart = %I64d\n"), rtStart);

	while(len >= 0)
	{
		mpeg2_state_t state = m_dec->mpeg2_parse();

		__asm emms; // this one is missing somewhere in the precompiled mmx obj files

		switch(state)
		{
		case STATE_BUFFER:
			if(len == 0) len = -1;
			else {m_dec->mpeg2_buffer(pDataIn, pDataIn + len); len = 0;}
			break;
		case STATE_INVALID:
			TRACE(_T("STATE_INVALID\n"));
//			if(m_fWaitForKeyFrame)
//				ResetMpeg2Decoder();
			break;
		case STATE_GOP:
			// TRACE(_T("STATE_GOP\n"));
			if(m_dec->m_info.m_user_data_len > 4 && *(DWORD*)m_dec->m_info.m_user_data == 0xf8014343
			&& m_pClosedCaptionOutput->IsConnected())
			{
				CComPtr<IMediaSample> pSample;
				m_pClosedCaptionOutput->GetDeliveryBuffer(&pSample, NULL, NULL, 0);
				BYTE* pData = NULL;
				pSample->GetPointer(&pData);
				*(DWORD*)pData = 0xb2010000;
				memcpy(pData + 4, m_dec->m_info.m_user_data, m_dec->m_info.m_user_data_len);
				pSample->SetActualDataLength(m_dec->m_info.m_user_data_len + 4);
				m_pClosedCaptionOutput->Deliver(pSample);
			}
			break;
		case STATE_SEQUENCE:
			TRACE(_T("STATE_SEQUENCE\n"));
			m_AvgTimePerFrame = 10i64 * m_dec->m_info.m_sequence->frame_period / 27;
			if(m_AvgTimePerFrame == 0) m_AvgTimePerFrame = ((VIDEOINFOHEADER*)m_pInput->CurrentMediaType().Format())->AvgTimePerFrame;
			break;
		case STATE_PICTURE:
/*			{
			TCHAR frametype[] = {'?','I', 'P', 'B', 'D'};
			TRACE(_T("STATE_PICTURE %010I64d [%c]\n"), rtStart, frametype[m_dec->m_picture->flags&PIC_MASK_CODING_TYPE]);
			}
*/			m_dec->m_picture->rtStart = rtStart;
			rtStart = _I64_MIN;
			m_dec->m_picture->fDelivered = false;

			m_dec->mpeg2_skip(m_fDropFrames && (m_dec->m_picture->flags&PIC_MASK_CODING_TYPE) == PIC_FLAG_CODING_TYPE_B);

			break;
		case STATE_SLICE:
		case STATE_END:
			{
				mpeg2_picture_t* picture = m_dec->m_info.m_display_picture;
				mpeg2_picture_t* picture_2nd = m_dec->m_info.m_display_picture_2nd;
				mpeg2_fbuf_t* fbuf = m_dec->m_info.m_display_fbuf;

				if(picture && fbuf && !(picture->flags&PIC_FLAG_SKIP))
				{
					ASSERT(!picture->fDelivered);

					// start - end

					m_fb.rtStart = picture->rtStart;
					if(m_fb.rtStart == _I64_MIN) m_fb.rtStart = m_fb.rtStop;
					m_fb.rtStop = m_fb.rtStart + m_AvgTimePerFrame * picture->nb_fields / (picture_2nd ? 1 : 2);

					REFERENCE_TIME rtStart = m_fb.rtStart;
					REFERENCE_TIME rtStop = m_fb.rtStop;

					// flags

					if(!(m_dec->m_info.m_sequence->flags&SEQ_FLAG_PROGRESSIVE_SEQUENCE)
					&& (picture->flags&PIC_FLAG_PROGRESSIVE_FRAME))
					{
						if(!m_fFilm
						&& (picture->flags&PIC_FLAG_REPEAT_FIRST_FIELD)
						&& !(m_fb.flags&PIC_FLAG_REPEAT_FIRST_FIELD))
						{
							TRACE(_T("m_fFilm = true\n"));
							m_fFilm = true;
						}
						else if(m_fFilm
						&& !(picture->flags&PIC_FLAG_REPEAT_FIRST_FIELD)
						&& !(m_fb.flags&PIC_FLAG_REPEAT_FIRST_FIELD))
						{
							TRACE(_T("m_fFilm = false\n"));
							m_fFilm = false;
						}
					}

					m_fb.flags = picture->flags;

ASSERT(!(m_fb.flags&PIC_FLAG_SKIP));

					// frame buffer

					int w = m_dec->m_info.m_sequence->picture_width;
					int h = m_dec->m_info.m_sequence->picture_height;
					int pitch = m_dec->m_info.m_sequence->width;

					if(m_fb.w != w || m_fb.h != h || m_fb.pitch != pitch)
						m_fb.alloc(w, h, pitch);

					// deinterlace

					ditype di = GetDeinterlaceMethod();

					if(di == DIAuto || di != DIWeave && di != DIBlend && di != DIBob)
					{
						if(!!(m_dec->m_info.m_sequence->flags&SEQ_FLAG_PROGRESSIVE_SEQUENCE))
							di = DIWeave; // hurray!
						else if(m_fFilm)
							di = DIWeave; // we are lucky
						else if(!(m_fb.flags&PIC_FLAG_PROGRESSIVE_FRAME))
							di = DIBlend; // ok, clear thing
						else
							// big trouble here, the progressive_frame bit is not reliable :'(
							// frames without temporal field diffs can be only detected when ntsc 
							// uses the repeat field flag (signaled with m_fFilm), if it's not set 
							// or we have pal then we might end up blending the fields unnecessarily...
							di = DIBlend;
							// TODO: find out if the pic is really interlaced by analysing it
					}

					if(di == DIWeave)
					{
						BitBltFromI420ToI420(w, h, 
							m_fb.buf[0], m_fb.buf[1], m_fb.buf[2], pitch, 
							fbuf->buf[0], fbuf->buf[1], fbuf->buf[2], pitch);
					}
					else if(di == DIBlend)
					{
						DeinterlaceBlend(m_fb.buf[0], fbuf->buf[0], w, h, pitch);
						DeinterlaceBlend(m_fb.buf[1], fbuf->buf[1], w/2, h/2, pitch/2);
						DeinterlaceBlend(m_fb.buf[2], fbuf->buf[2], w/2, h/2, pitch/2);
					}
					else if(di == DIBob)
					{
						if(m_fb.flags&PIC_FLAG_TOP_FIELD_FIRST)
						{
							BitBltFromRGBToRGB(w, h/2, m_fb.buf[0], pitch*2, 8, fbuf->buf[0], pitch*2, 8);
							AvgLines8(m_fb.buf[0], h, pitch);
							BitBltFromRGBToRGB(w/2, h/4, m_fb.buf[1], pitch, 8, fbuf->buf[1], pitch, 8);
							AvgLines8(m_fb.buf[1], h/2, pitch/2);
							BitBltFromRGBToRGB(w/2, h/4, m_fb.buf[2], pitch, 8, fbuf->buf[2], pitch, 8);
							AvgLines8(m_fb.buf[2], h/2, pitch/2);
						}
						else
						{
							BitBltFromRGBToRGB(w, h/2, m_fb.buf[0]+pitch, pitch*2, 8, fbuf->buf[0]+pitch, pitch*2, 8);
							AvgLines8(m_fb.buf[0]+pitch, h-1, pitch);
							BitBltFromRGBToRGB(w/2, h/4, m_fb.buf[1]+pitch/2, pitch, 8, fbuf->buf[1]+pitch/2, pitch, 8);
							AvgLines8(m_fb.buf[1]+pitch/2, (h-1)/2, pitch/2);
							BitBltFromRGBToRGB(w/2, h/4, m_fb.buf[2]+pitch/2, pitch, 8, fbuf->buf[2]+pitch/2, pitch, 8);
							AvgLines8(m_fb.buf[2]+pitch/2, (h-1)/2, pitch/2);
						}

						m_fb.rtStart = rtStart;
						m_fb.rtStop = (rtStart + rtStop) / 2;
					}

					// postproc

					ApplyBrContHueSat(m_fb.buf[0], m_fb.buf[1], m_fb.buf[2], w, h, pitch);

					// deliver

					picture->fDelivered = true;

					if(FAILED(hr = Deliver(false)))
						return hr;

					// spec code for bob

					if(di == DIBob)
					{
						if(m_fb.flags&PIC_FLAG_TOP_FIELD_FIRST)
						{
							BitBltFromRGBToRGB(w, h/2, m_fb.buf[0]+pitch, pitch*2, 8, fbuf->buf[0]+pitch, pitch*2, 8);
							AvgLines8(m_fb.buf[0]+pitch, h-1, pitch);
							BitBltFromRGBToRGB(w/2, h/4, m_fb.buf[1]+pitch/2, pitch, 8, fbuf->buf[1]+pitch/2, pitch, 8);
							AvgLines8(m_fb.buf[1]+pitch/2, (h-1)/2, pitch/2);
							BitBltFromRGBToRGB(w/2, h/4, m_fb.buf[2]+pitch/2, pitch, 8, fbuf->buf[2]+pitch/2, pitch, 8);
							AvgLines8(m_fb.buf[2]+pitch/2, (h-1)/2, pitch/2);
						}
						else
						{
							BitBltFromRGBToRGB(w, h/2, m_fb.buf[0], pitch*2, 8, fbuf->buf[0], pitch*2, 8);
							AvgLines8(m_fb.buf[0], h, pitch);
							BitBltFromRGBToRGB(w/2, h/4, m_fb.buf[1], pitch, 8, fbuf->buf[1], pitch, 8);
							AvgLines8(m_fb.buf[1], h/2, pitch/2);
							BitBltFromRGBToRGB(w/2, h/4, m_fb.buf[2], pitch, 8, fbuf->buf[2], pitch, 8);
							AvgLines8(m_fb.buf[2], h/2, pitch/2);
						}

						m_fb.rtStart = (rtStart + rtStop) / 2;
						m_fb.rtStop = rtStop;

						// postproc

						ApplyBrContHueSat(m_fb.buf[0], m_fb.buf[1], m_fb.buf[2], w, h, pitch);

						// deliver

						picture->fDelivered = true;

						if(FAILED(hr = Deliver(false)))
							return hr;
					}
				}
			}
			break;
		default:
		    break;
		}
    }

	return S_OK;
}

HRESULT CMpeg2DecFilter::Deliver(bool fRepeatLast)
{
	CAutoLock cAutoLock(&m_csReceive);

	if((m_fb.flags&PIC_MASK_CODING_TYPE) == PIC_FLAG_CODING_TYPE_I)
		m_fWaitForKeyFrame = false;

	TCHAR frametype[] = {'?','I', 'P', 'B', 'D'};
//	TRACE(_T("%010I64d - %010I64d [%c] [prsq %d prfr %d tff %d rff %d nb_fields %d ref %d] (%dx%d/%dx%d)\n"), 
/*
	TRACE(_T("%010I64d - %010I64d [%c] [prsq %d prfr %d tff %d rff %d] (%dx%d %d) (preroll %d)\n"), 
		m_fb.rtStart, m_fb.rtStop,
		frametype[m_fb.flags&PIC_MASK_CODING_TYPE],
		!!(m_dec->m_info.m_sequence->flags&SEQ_FLAG_PROGRESSIVE_SEQUENCE),
		!!(m_fb.flags&PIC_FLAG_PROGRESSIVE_FRAME),
		!!(m_fb.flags&PIC_FLAG_TOP_FIELD_FIRST),
		!!(m_fb.flags&PIC_FLAG_REPEAT_FIRST_FIELD),
//		m_dec->m_info.m_display_picture->nb_fields,
//		m_dec->m_info.m_display_picture->temporal_reference,
		m_fb.w, m_fb.h, m_fb.pitch,
		!!(m_fb.rtStart < 0 || m_fWaitForKeyFrame));
*/
	if(m_fb.rtStart < 0 || m_fWaitForKeyFrame)
		return S_OK;

	HRESULT hr;

	if(FAILED(hr = ReconnectOutput(m_fb.w, m_fb.h)))
		return hr;

	CComPtr<IMediaSample> pOut;
	BYTE* pDataOut = NULL;
	if(FAILED(hr = m_pOutput->GetDeliveryBuffer(&pOut, NULL, NULL, 0))
	|| FAILED(hr = pOut->GetPointer(&pDataOut)))
		return hr;

	long size = pOut->GetSize();

	AM_MEDIA_TYPE* pmt;
	if(SUCCEEDED(pOut->GetMediaType(&pmt)) && pmt)
	{
		CMediaType mt = *pmt;
		m_pOutput->SetMediaType(&mt);
		DeleteMediaType(pmt);
	}

	{
		CMpeg2DecInputPin* pPin = (CMpeg2DecInputPin*)m_pInput;
		CAutoLock cAutoLock(&pPin->m_csRateLock);
		if(m_rate.Rate != pPin->m_ratechange.Rate)
		{
			m_rate.Rate = pPin->m_ratechange.Rate;
			m_rate.StartTime = m_fb.rtStart;
		}
	}

	REFERENCE_TIME rtStart = m_fb.rtStart;
	REFERENCE_TIME rtStop = m_fb.rtStop;
	rtStart = m_rate.StartTime + (rtStart - m_rate.StartTime) * m_rate.Rate / 10000;
	rtStop = m_rate.StartTime + (rtStop - m_rate.StartTime) * m_rate.Rate / 10000;

	pOut->SetTime(&rtStart, &rtStop);
	pOut->SetMediaTime(NULL, NULL);

	pOut->SetDiscontinuity(FALSE);
	pOut->SetSyncPoint(TRUE);

	// FIXME: hell knows why but without this the overlay mixer starts very skippy
	// (don't enable this for other renderers, the old for example will go crazy if you do)
	if(GetCLSID(m_pOutput->GetConnected()) == CLSID_OverlayMixer)
		pOut->SetDiscontinuity(TRUE);

	BYTE** buf = &m_fb.buf[0];

	if(m_pSubpicInput->HasAnythingToRender(m_fb.rtStart))
	{
		BitBltFromI420ToI420(m_fb.w, m_fb.h, 
			m_fb.buf[3], m_fb.buf[4], m_fb.buf[5], m_fb.pitch, 
			m_fb.buf[0], m_fb.buf[1], m_fb.buf[2], m_fb.pitch);

		buf = &m_fb.buf[3];

		m_pSubpicInput->RenderSubpics(m_fb.rtStart, buf, m_fb.pitch, m_fb.h);
	}

	Copy(pDataOut, buf, m_fb.w, m_fb.h, m_fb.pitch);

	if(FAILED(hr = m_pOutput->Deliver(pOut)))
		return hr;

	return S_OK;
}

void CMpeg2DecFilter::Copy(BYTE* pOut, BYTE** ppIn, DWORD w, DWORD h, DWORD pitchIn)
{
	BITMAPINFOHEADER bihOut;
	ExtractBIH(&m_pOutput->CurrentMediaType(), &bihOut);

	BYTE* pIn = ppIn[0];
	BYTE* pInU = ppIn[1];
	BYTE* pInV = ppIn[2];

	w = (w+7)&~7;
	ASSERT(w <= pitchIn);

	if(bihOut.biCompression == '2YUY')
	{
		BitBltFromI420ToYUY2(w, h, pOut, bihOut.biWidth*2, pIn, pInU, pInV, pitchIn);
	}
	else if(bihOut.biCompression == 'I420' || bihOut.biCompression == 'VUYI')
	{
		BitBltFromI420ToI420(w, h, pOut, pOut + bihOut.biWidth*h, pOut + bihOut.biWidth*h*5/4, bihOut.biWidth, pIn, pInU, pInV, pitchIn);
	}
	else if(bihOut.biCompression == '21VY')
	{
		BitBltFromI420ToI420(w, h, pOut, pOut + bihOut.biWidth*h*5/4, pOut + bihOut.biWidth*h, bihOut.biWidth, pIn, pInU, pInV, pitchIn);
	}
	else if(bihOut.biCompression == BI_RGB || bihOut.biCompression == BI_BITFIELDS)
	{
		int pitchOut = bihOut.biWidth*bihOut.biBitCount>>3;

		if(bihOut.biHeight > 0)
		{
			pOut += pitchOut*(h-1);
			pitchOut = -pitchOut;
		}

		if(!BitBltFromI420ToRGB(w, h, pOut, pitchOut, bihOut.biBitCount, pIn, pInU, pInV, pitchIn))
		{
			for(DWORD y = 0; y < h; y++, pIn += pitchIn, pOut += pitchOut)
				memset(pOut, 0, pitchOut);
		}
	}
}

void CMpeg2DecFilter::ResetMpeg2Decoder()
{
	CAutoLock cAutoLock(&m_csReceive);
TRACE(_T("ResetMpeg2Decoder()\n"));

	for(int i = 0; i < countof(m_dec->m_pictures); i++)
	{
		m_dec->m_pictures[i].rtStart = m_dec->m_pictures[i].rtStop = _I64_MIN+1;
		m_dec->m_pictures[i].fDelivered = false;
		m_dec->m_pictures[i].flags &= ~PIC_MASK_CODING_TYPE;
	}

	CMediaType& mt = m_pInput->CurrentMediaType();

	BYTE* pSequenceHeader = NULL;
	DWORD cbSequenceHeader = 0;

	if(mt.formattype == FORMAT_MPEGVideo)
	{
		pSequenceHeader = ((MPEG1VIDEOINFO*)mt.Format())->bSequenceHeader;
		cbSequenceHeader = ((MPEG1VIDEOINFO*)mt.Format())->cbSequenceHeader;
	}
	else if(mt.formattype == FORMAT_MPEG2_VIDEO)
	{
		pSequenceHeader = (BYTE*)((MPEG2VIDEOINFO*)mt.Format())->dwSequenceHeader;
		cbSequenceHeader = ((MPEG2VIDEOINFO*)mt.Format())->cbSequenceHeader;
	}

	m_dec->mpeg2_close();
	m_dec->mpeg2_init();

	m_dec->mpeg2_buffer(pSequenceHeader, pSequenceHeader + cbSequenceHeader);

	m_fWaitForKeyFrame = true;

	m_fFilm = false;
	m_fb.flags = 0;
}

HRESULT CMpeg2DecFilter::ReconnectOutput(int w, int h)
{
	CMediaType& mt = m_pOutput->CurrentMediaType();

	bool fForceReconnection = false;
	if(w != m_win || h != m_hin)
	{
		fForceReconnection = true;
		m_win = w;
		m_hin = h;
	}

	HRESULT hr = S_OK;

	if(fForceReconnection || m_win != m_wout || m_hin != m_hout || m_arxin != m_arxout || m_aryin != m_aryout)
	{
		BITMAPINFOHEADER* bmi = NULL;

		if(mt.formattype == FORMAT_VideoInfo)
		{
			VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)mt.Format();
			SetRect(&vih->rcSource, 0, 0, m_win, m_hin);
			SetRect(&vih->rcTarget, 0, 0, m_win, m_hin);
			bmi = &vih->bmiHeader;
			bmi->biXPelsPerMeter = m_win * m_aryin;
			bmi->biYPelsPerMeter = m_hin * m_arxin;
		}
		else if(mt.formattype == FORMAT_VideoInfo2)
		{
			VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)mt.Format();
			SetRect(&vih->rcSource, 0, 0, m_win, m_hin);
			SetRect(&vih->rcTarget, 0, 0, m_win, m_hin);
			bmi = &vih->bmiHeader;
			vih->dwPictAspectRatioX = m_arxin;
			vih->dwPictAspectRatioY = m_aryin;
		}

		bmi->biWidth = m_win;
		bmi->biHeight = m_hin;
		bmi->biSizeImage = m_win*m_hin*bmi->biBitCount>>3;

		hr = m_pOutput->GetConnected()->QueryAccept(&mt);

		if(FAILED(hr = m_pOutput->GetConnected()->ReceiveConnection(m_pOutput, &mt)))
			return hr;

		CComPtr<IMediaSample> pOut;
		if(SUCCEEDED(m_pOutput->GetDeliveryBuffer(&pOut, NULL, NULL, 0)))
		{
			AM_MEDIA_TYPE* pmt;
			if(SUCCEEDED(pOut->GetMediaType(&pmt)) && pmt)
			{
				CMediaType mt = *pmt;
				m_pOutput->SetMediaType(&mt);
				DeleteMediaType(pmt);
			}
			else // stupid overlay mixer won't let us know the new pitch...
			{
				long size = pOut->GetSize();
				bmi->biWidth = size / bmi->biHeight * 8 / bmi->biBitCount;
			}
		}

		m_wout = m_win;
		m_hout = m_hin;
		m_arxout = m_arxin;
		m_aryout = m_aryin;

		// some renderers don't send this
		NotifyEvent(EC_VIDEO_SIZE_CHANGED, MAKELPARAM(m_win, m_hin), 0);

		return S_OK;
	}

	return S_FALSE;
}

HRESULT CMpeg2DecFilter::CheckConnect(PIN_DIRECTION dir, IPin* pPin)
{
	if(dir == PINDIR_OUTPUT)
	{
		if(GetCLSID(m_pInput->GetConnected()) == CLSID_DVDNavigator)
		{
			// one of these needed for dynamic format changes

			CLSID clsid = GetCLSID(pPin);
			if(clsid != CLSID_OverlayMixer
			/*&& clsid != CLSID_OverlayMixer2*/
			&& clsid != CLSID_VideoMixingRenderer 
			&& clsid != CLSID_VideoMixingRenderer9)
				return E_FAIL;
		}
	}

	return __super::CheckConnect(dir, pPin);
}

HRESULT CMpeg2DecFilter::CheckInputType(const CMediaType* mtIn)
{
	return (mtIn->majortype == MEDIATYPE_DVD_ENCRYPTED_PACK && mtIn->subtype == MEDIASUBTYPE_MPEG2_VIDEO
			|| mtIn->majortype == MEDIATYPE_MPEG2_PACK && mtIn->subtype == MEDIASUBTYPE_MPEG2_VIDEO
			|| mtIn->majortype == MEDIATYPE_MPEG2_PES && mtIn->subtype == MEDIASUBTYPE_MPEG2_VIDEO
			|| mtIn->majortype == MEDIATYPE_Video && mtIn->subtype == MEDIASUBTYPE_MPEG2_VIDEO
			|| mtIn->majortype == MEDIATYPE_Video && mtIn->subtype == MEDIASUBTYPE_MPEG1Packet
			|| mtIn->majortype == MEDIATYPE_Video && mtIn->subtype == MEDIASUBTYPE_MPEG1Payload)
		? S_OK
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CMpeg2DecFilter::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	return SUCCEEDED(CheckInputType(mtIn))
		&& mtOut->majortype == MEDIATYPE_Video && (mtOut->subtype == MEDIASUBTYPE_YV12 && IsPlanarYUVEnabled()
												|| mtOut->subtype == MEDIASUBTYPE_I420 && IsPlanarYUVEnabled()
												|| mtOut->subtype == MEDIASUBTYPE_IYUV && IsPlanarYUVEnabled()
												|| mtOut->subtype == MEDIASUBTYPE_YUY2
												|| mtOut->subtype == MEDIASUBTYPE_ARGB32
												|| mtOut->subtype == MEDIASUBTYPE_RGB32
												|| mtOut->subtype == MEDIASUBTYPE_RGB24
												|| mtOut->subtype == MEDIASUBTYPE_RGB565
												/*|| mtOut->subtype == MEDIASUBTYPE_RGB555*/)
		? S_OK
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CMpeg2DecFilter::CheckOutputMediaType(const CMediaType& mtOut)
{
	DWORD wout = 0, hout = 0, arxout = 0, aryout = 0;
	return ExtractDim(&mtOut, wout, hout, arxout, aryout)
		&& m_hin == abs((int)hout)
		? S_OK
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CMpeg2DecFilter::DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties)
{
	if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	BITMAPINFOHEADER bih;
	ExtractBIH(&m_pOutput->CurrentMediaType(), &bih);

	pProperties->cBuffers = 1;
	pProperties->cbBuffer = bih.biSizeImage;
	pProperties->cbAlign = 1;
	pProperties->cbPrefix = 0;

	HRESULT hr;
	ALLOCATOR_PROPERTIES Actual;
    if(FAILED(hr = pAllocator->SetProperties(pProperties, &Actual))) 
		return hr;

    return pProperties->cBuffers > Actual.cBuffers || pProperties->cbBuffer > Actual.cbBuffer
		? E_FAIL
		: NOERROR;
}

HRESULT CMpeg2DecFilter::GetMediaType(int iPosition, CMediaType* pmt)
{
    if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	struct {const GUID* subtype; WORD biPlanes, biBitCount; DWORD biCompression;} fmts[] =
	{
		{&MEDIASUBTYPE_YV12, 3, 12, '21VY'},
		{&MEDIASUBTYPE_I420, 3, 12, '024I'},
		{&MEDIASUBTYPE_IYUV, 3, 12, 'VUYI'},
		{&MEDIASUBTYPE_YUY2, 1, 16, '2YUY'},
		{&MEDIASUBTYPE_ARGB32, 1, 32, BI_RGB},
		{&MEDIASUBTYPE_RGB32, 1, 32, BI_RGB},
		{&MEDIASUBTYPE_RGB24, 1, 24, BI_RGB},
		{&MEDIASUBTYPE_RGB565, 1, 16, BI_RGB},
		{&MEDIASUBTYPE_RGB555, 1, 16, BI_RGB},
		{&MEDIASUBTYPE_ARGB32, 1, 32, BI_BITFIELDS},
		{&MEDIASUBTYPE_RGB32, 1, 32, BI_BITFIELDS},
		{&MEDIASUBTYPE_RGB24, 1, 24, BI_BITFIELDS},
		{&MEDIASUBTYPE_RGB565, 1, 16, BI_BITFIELDS},
		{&MEDIASUBTYPE_RGB555, 1, 16, BI_BITFIELDS},
	};

	// this will make sure we won't connect to the old renderer in dvd mode
	// that renderer can't switch the format dynamically
	if(GetCLSID(m_pInput->GetConnected()) == CLSID_DVDNavigator)
		iPosition = iPosition*2;

	if(iPosition < 0) return E_INVALIDARG;
	if(iPosition >= 2*countof(fmts)) return VFW_S_NO_MORE_ITEMS;

	CMediaType& mt = m_pInput->CurrentMediaType();

	pmt->majortype = MEDIATYPE_Video;
	pmt->subtype = *fmts[iPosition/2].subtype;

	BITMAPINFOHEADER bihOut;
	memset(&bihOut, 0, sizeof(bihOut));
	bihOut.biSize = sizeof(bihOut);
	bihOut.biWidth = m_win;
	bihOut.biHeight = m_hin;
	bihOut.biPlanes = fmts[iPosition/2].biPlanes;
	bihOut.biBitCount = fmts[iPosition/2].biBitCount;
	bihOut.biCompression = fmts[iPosition/2].biCompression;
	bihOut.biSizeImage = m_win*m_hin*bihOut.biBitCount>>3;

	if(iPosition&1)
	{
		pmt->formattype = FORMAT_VideoInfo;
		VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
		memset(vih, 0, sizeof(VIDEOINFOHEADER));
		vih->bmiHeader = bihOut;
		vih->bmiHeader.biXPelsPerMeter = vih->bmiHeader.biWidth * m_aryin;
		vih->bmiHeader.biYPelsPerMeter = vih->bmiHeader.biHeight * m_arxin;
	}
	else
	{
		pmt->formattype = FORMAT_VideoInfo2;
		VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER2));
		memset(vih, 0, sizeof(VIDEOINFOHEADER2));
		vih->bmiHeader = bihOut;
		vih->dwPictAspectRatioX = m_arxin;
		vih->dwPictAspectRatioY = m_aryin;
	}

	// these fields have the same field offset in all four structs
	((VIDEOINFOHEADER*)pmt->Format())->AvgTimePerFrame = ((VIDEOINFOHEADER*)mt.Format())->AvgTimePerFrame;
	((VIDEOINFOHEADER*)pmt->Format())->dwBitRate = ((VIDEOINFOHEADER*)mt.Format())->dwBitRate;
	((VIDEOINFOHEADER*)pmt->Format())->dwBitErrorRate = ((VIDEOINFOHEADER*)mt.Format())->dwBitErrorRate;

	CorrectMediaType(pmt);

	return S_OK;
}

HRESULT CMpeg2DecFilter::SetMediaType(PIN_DIRECTION dir, const CMediaType* pmt)
{
	if(dir == PINDIR_INPUT)
	{
		m_win = m_hin = m_arxin = m_aryin = 0;
		ExtractDim(pmt, m_win, m_hin, m_arxin, m_aryin);
	}
	else if(dir == PINDIR_OUTPUT)
	{
		DWORD wout = 0, hout = 0, arxout = 0, aryout = 0;
		ExtractDim(pmt, wout, hout, arxout, aryout);
		if(m_win == wout && m_hin == hout && m_arxin == arxout && m_aryin == aryout)
		{
			m_wout = wout;
			m_hout = hout;
			m_arxout = arxout;
			m_aryout = aryout;
		}
	}

	return __super::SetMediaType(dir, pmt);
}

HRESULT CMpeg2DecFilter::StartStreaming()
{
	HRESULT hr = __super::StartStreaming();
	if(FAILED(hr)) return hr;

	m_dec.Attach(new CMpeg2Dec());
	if(!m_dec) return E_OUTOFMEMORY;

	ResetMpeg2Decoder();

	return S_OK;
}

HRESULT CMpeg2DecFilter::StopStreaming()
{
	m_dec.Free();

	return __super::StopStreaming();
}

HRESULT CMpeg2DecFilter::AlterQuality(Quality q)
{
	if(q.Late > 000*10000i64) 
		m_fDropFrames = true;
	else if(q.Late <= 0) 
		m_fDropFrames = false;

//	TRACE(_T("CMpeg2DecFilter::AlterQuality: Type=%d, Proportion=%d, Late=%I64d, TimeStamp=%I64d\n"), q.Type, q.Proportion, q.Late, q.TimeStamp);
	return S_OK;
}

// IMpeg2DecFilter

STDMETHODIMP CMpeg2DecFilter::SetDeinterlaceMethod(ditype di)
{
	CAutoLock cAutoLock(&m_csProps);
	m_di = di;
	return S_OK;
}

STDMETHODIMP_(ditype) CMpeg2DecFilter::GetDeinterlaceMethod()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_di;
}

void CMpeg2DecFilter::CalcBrCont(BYTE* YTbl, double bright, double cont)
{
	int Cont = (int)(cont * 512);
	int Bright = (int)bright;

	for(int i = 0; i < 256; i++)
	{
		int y = ((Cont * (i - 16)) >> 9) + Bright + 16;
		YTbl[i] = min(max(y, 0), 255);
//		YTbl[i] = min(max(y, 16), 235);
	}
}

void CMpeg2DecFilter::CalcHueSat(BYTE* UTbl, BYTE* VTbl, double hue, double sat)
{
	int Sat = (int)(sat * 512);
	double Hue = (hue * 3.1415926) / 180.0;
	int Sin = (int)(sin(Hue) * 4096);
	int Cos = (int)(cos(Hue) * 4096);

	for(int y = 0; y < 256; y++)
	{
		for(int x = 0; x < 256; x++)
		{
			int u = x - 128; 
			int v = y - 128;
			int ux = (u * Cos + v * Sin) >> 12;
			v = (v * Cos - u * Sin) >> 12;
			u = ((ux * Sat) >> 9) + 128;
			v = ((v * Sat) >> 9) + 128;
			u = min(max(u, 16), 235);
			v = min(max(v, 16), 235);
			UTbl[(y << 8) | x] = u;
			VTbl[(y << 8) | x] = v;
		}
	}
}

void CMpeg2DecFilter::ApplyBrContHueSat(BYTE* srcy, BYTE* srcu, BYTE* srcv, int w, int h, int pitch)
{
	CAutoLock cAutoLock(&m_csProps);

	double EPSILON = 1e-4;

	if(fabs(m_bright-0.0) > EPSILON || fabs(m_cont-1.0) > EPSILON)
	{
		for(int size = pitch*h; size > 0; size--)
		{
			*srcy++ = m_YTbl[*srcy];
		}
	}

	pitch /= 2;
	w /= 2;
	h /= 2;

	if(fabs(m_hue-0.0) > EPSILON || fabs(m_sat-1.0) > EPSILON)
	{
		for(int size = pitch*h; size > 0; size--)
		{
			WORD uv = (*srcv<<8)|*srcu;
			*srcu++ = m_UTbl[uv];
			*srcv++ = m_VTbl[uv];
		}
	}
}

STDMETHODIMP CMpeg2DecFilter::SetBrightness(double bright)
{
	CAutoLock cAutoLock(&m_csProps);
	CalcBrCont(m_YTbl, m_bright = bright, m_cont);
	return S_OK;
}

STDMETHODIMP CMpeg2DecFilter::SetContrast(double cont)
{
	CAutoLock cAutoLock(&m_csProps);
	CalcBrCont(m_YTbl, m_bright, m_cont = cont);
	return S_OK;
}

STDMETHODIMP CMpeg2DecFilter::SetHue(double hue)
{
	CAutoLock cAutoLock(&m_csProps);
	CalcHueSat(m_UTbl, m_VTbl, m_hue = hue, m_sat);
	return S_OK;
}

STDMETHODIMP CMpeg2DecFilter::SetSaturation(double sat)
{
	CAutoLock cAutoLock(&m_csProps);
	CalcHueSat(m_UTbl, m_VTbl, m_hue, m_sat = sat);
	return S_OK;
}

STDMETHODIMP_(double) CMpeg2DecFilter::GetBrightness()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_bright;
}

STDMETHODIMP_(double) CMpeg2DecFilter::GetContrast()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_cont;
}

STDMETHODIMP_(double) CMpeg2DecFilter::GetHue()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_hue;
}

STDMETHODIMP_(double) CMpeg2DecFilter::GetSaturation()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_sat;
}

STDMETHODIMP CMpeg2DecFilter::EnableForcedSubtitles(bool fEnable)
{
	CAutoLock cAutoLock(&m_csProps);
	m_fForcedSubs = fEnable;
	return S_OK;
}

STDMETHODIMP_(bool) CMpeg2DecFilter::IsForcedSubtitlesEnabled()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_fForcedSubs;
}

STDMETHODIMP CMpeg2DecFilter::EnablePlanarYUV(bool fEnable)
{
	CAutoLock cAutoLock(&m_csProps);
	m_fPlanarYUV = fEnable;
	return S_OK;
}

STDMETHODIMP_(bool) CMpeg2DecFilter::IsPlanarYUVEnabled()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_fPlanarYUV;
}

//
// CMpeg2DecInputPin
//

CMpeg2DecInputPin::CMpeg2DecInputPin(CTransformFilter* pFilter, HRESULT* phr, LPWSTR pName)
	: CDeCSSInputPin(NAME("CMpeg2DecInputPin"), pFilter, phr, pName)
{
	m_CorrectTS = 0;
	m_ratechange.Rate = 10000;
	m_ratechange.StartTime = _I64_MAX;
}

// IKsPropertySet

STDMETHODIMP CMpeg2DecInputPin::Set(REFGUID PropSet, ULONG Id, LPVOID pInstanceData, ULONG InstanceLength, LPVOID pPropertyData, ULONG DataLength)
{
	if(PropSet != AM_KSPROPSETID_TSRateChange /*&& PropSet != AM_KSPROPSETID_DVD_RateChange*/)
		return __super::Set(PropSet, Id, pInstanceData, InstanceLength, pPropertyData, DataLength);

	if(PropSet == AM_KSPROPSETID_TSRateChange)
	switch(Id)
	{
	case AM_RATE_SimpleRateChange:
		{
			AM_SimpleRateChange* p = (AM_SimpleRateChange*)pPropertyData;
			if(!m_CorrectTS) return E_PROP_ID_UNSUPPORTED;
			CAutoLock cAutoLock(&m_csRateLock);
			m_ratechange = *p;
			DbgLog((LOG_TRACE, 0, _T("StartTime=%I64d, Rate=%d"), p->StartTime, p->Rate));
		}
		break;
	case AM_RATE_UseRateVersion:
		{
			WORD* p = (WORD*)pPropertyData;
			if(*p > 0x0101) return E_PROP_ID_UNSUPPORTED;
		}
		break;
	case AM_RATE_CorrectTS:
		{
			LONG* p = (LONG*)pPropertyData;
			m_CorrectTS = *p;
		}
		break;
	default:
		return E_PROP_ID_UNSUPPORTED;
	}
/*
	if(PropSet == AM_KSPROPSETID_DVD_RateChange)
	switch(Id)
	{
	case AM_RATE_ChangeRate:
		{
			AM_DVD_ChangeRate* p = (AM_DVD_ChangeRate*)pPropertyData;
		}
		break;
	default:
		return E_PROP_ID_UNSUPPORTED;
	}
*/
	return S_OK;
}

STDMETHODIMP CMpeg2DecInputPin::Get(REFGUID PropSet, ULONG Id, LPVOID pInstanceData, ULONG InstanceLength, LPVOID pPropertyData, ULONG DataLength, ULONG* pBytesReturned)
{
	if(PropSet != AM_KSPROPSETID_TSRateChange /*&& PropSet != AM_KSPROPSETID_DVD_RateChange*/)
		return __super::Get(PropSet, Id, pInstanceData, InstanceLength, pPropertyData, DataLength, pBytesReturned);

	if(PropSet == AM_KSPROPSETID_TSRateChange)
	switch(Id)
	{
	case AM_RATE_SimpleRateChange:
		{
			AM_SimpleRateChange* p = (AM_SimpleRateChange*)pPropertyData;
			return E_PROP_ID_UNSUPPORTED;
		}
		break;
	case AM_RATE_MaxFullDataRate:
		{
			AM_MaxFullDataRate* p = (AM_MaxFullDataRate*)pPropertyData;
			*p = 8*10000;
			*pBytesReturned = sizeof(AM_MaxFullDataRate);
		}
		break;
	case AM_RATE_QueryFullFrameRate:
		{
			AM_QueryRate* p = (AM_QueryRate*)pPropertyData;
			p->lMaxForwardFullFrame = 8*10000;
			p->lMaxReverseFullFrame = 8*10000;
			*pBytesReturned = sizeof(AM_QueryRate);
		}
		break;
	case AM_RATE_QueryLastRateSegPTS:
		{
			REFERENCE_TIME* p = (REFERENCE_TIME*)pPropertyData;
			return E_PROP_ID_UNSUPPORTED;
		}
		break;
	default:
		return E_PROP_ID_UNSUPPORTED;
	}
/*
	if(PropSet == AM_KSPROPSETID_DVD_RateChange)
	switch(Id)
	{
	case AM_RATE_FullDataRateMax:
		{
			AM_MaxFullDataRate* p = (AM_MaxFullDataRate*)pPropertyData;
		}
		break;
	case AM_RATE_ReverseDecode:
		{
			LONG* p = (LONG*)pPropertyData;
		}
		break;
	case AM_RATE_DecoderPosition:
		{
			AM_DVD_DecoderPosition* p = (AM_DVD_DecoderPosition*)pPropertyData;
		}
		break;
	case AM_RATE_DecoderVersion:
		{
			LONG* p = (LONG*)pPropertyData;
		}
		break;
	default:
		return E_PROP_ID_UNSUPPORTED;
	}
*/
	return S_OK;
}

STDMETHODIMP CMpeg2DecInputPin::QuerySupported(REFGUID PropSet, ULONG Id, ULONG* pTypeSupport)
{
	if(PropSet != AM_KSPROPSETID_TSRateChange /*&& PropSet != AM_KSPROPSETID_DVD_RateChange*/)
		return __super::QuerySupported(PropSet, Id, pTypeSupport);

	if(PropSet == AM_KSPROPSETID_TSRateChange)
	switch(Id)
	{
	case AM_RATE_SimpleRateChange:
		*pTypeSupport = KSPROPERTY_SUPPORT_GET | KSPROPERTY_SUPPORT_SET;
		break;
	case AM_RATE_MaxFullDataRate:
		*pTypeSupport = KSPROPERTY_SUPPORT_GET;
		break;
	case AM_RATE_UseRateVersion:
		*pTypeSupport = KSPROPERTY_SUPPORT_SET;
		break;
	case AM_RATE_QueryFullFrameRate:
		*pTypeSupport = KSPROPERTY_SUPPORT_GET;
		break;
	case AM_RATE_QueryLastRateSegPTS:
		*pTypeSupport = KSPROPERTY_SUPPORT_GET;
		break;
	case AM_RATE_CorrectTS:
		*pTypeSupport = KSPROPERTY_SUPPORT_SET;
		break;
	default:
		return E_PROP_ID_UNSUPPORTED;
	}
/*
	if(PropSet == AM_KSPROPSETID_DVD_RateChange)
	switch(Id)
	{
	case AM_RATE_ChangeRate:
		*pTypeSupport = KSPROPERTY_SUPPORT_SET;
		break;
	case AM_RATE_FullDataRateMax:
		*pTypeSupport = KSPROPERTY_SUPPORT_GET;
		break;
	case AM_RATE_ReverseDecode:
		*pTypeSupport = KSPROPERTY_SUPPORT_GET;
		break;
	case AM_RATE_DecoderPosition:
		*pTypeSupport = KSPROPERTY_SUPPORT_GET;
		break;
	case AM_RATE_DecoderVersion:
		*pTypeSupport = KSPROPERTY_SUPPORT_GET;
		break;
	default:
		return E_PROP_ID_UNSUPPORTED;
	}
*/
	return S_OK;
}

//
// CSubpicInputPin
//

#define PTS2RT(pts) (10000i64*pts/90)

CSubpicInputPin::CSubpicInputPin(CTransformFilter* pFilter, HRESULT* phr) 
	: CMpeg2DecInputPin(pFilter, phr, L"SubPicture")
	, m_spon(TRUE)
	, m_fsppal(false)
{
	m_sppal[0].Y = 0x00;
	m_sppal[0].U = m_sppal[0].V = 0x80;
	m_sppal[1].Y = 0xe0;
	m_sppal[1].U = m_sppal[1].V = 0x80;
	m_sppal[2].Y = 0x80;
	m_sppal[2].U = m_sppal[2].V = 0x80;
	m_sppal[3].Y = 0x20;
	m_sppal[3].U = m_sppal[3].V = 0x80;
}

HRESULT CSubpicInputPin::CheckMediaType(const CMediaType* mtIn)
{
	return (mtIn->majortype == MEDIATYPE_DVD_ENCRYPTED_PACK
			|| mtIn->majortype == MEDIATYPE_MPEG2_PACK
			|| mtIn->majortype == MEDIATYPE_MPEG2_PES
			|| mtIn->majortype == MEDIATYPE_Video) 
		&& (mtIn->subtype == MEDIASUBTYPE_DVD_SUBPICTURE
			|| mtIn->subtype == MEDIASUBTYPE_CVD_SUBPICTURE
			|| mtIn->subtype == MEDIASUBTYPE_SVCD_SUBPICTURE)
		? S_OK
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CSubpicInputPin::SetMediaType(const CMediaType* mtIn)
{
	return CBasePin::SetMediaType(mtIn);
}

bool CSubpicInputPin::HasAnythingToRender(REFERENCE_TIME rt)
{
	if(!IsConnected()) return(false);

	CAutoLock cAutoLock(&m_csReceive);

	POSITION pos = m_sps.GetHeadPosition();
	while(pos)
	{
		spu* sp = m_sps.GetNext(pos);
		if(sp->m_rtStart <= rt && rt < sp->m_rtStop && (/*sp->m_psphli ||*/ sp->m_fForced || m_spon))
			return(true);
	}

	return(false);
}

void CSubpicInputPin::RenderSubpics(REFERENCE_TIME rt, BYTE** yuv, int w, int h)
{
	CAutoLock cAutoLock(&m_csReceive);

	POSITION pos;

	// remove no longer needed things first
	pos = m_sps.GetHeadPosition();
	while(pos)
	{
		POSITION cur = pos;
		spu* sp = m_sps.GetNext(pos);
		if(sp->m_rtStop <= rt) m_sps.RemoveAt(cur);
	}

	pos = m_sps.GetHeadPosition();
	while(pos)
	{
		spu* sp = m_sps.GetNext(pos);
		if(sp->m_rtStart <= rt && rt < sp->m_rtStop 
		&& (m_spon || sp->m_fForced && (((CMpeg2DecFilter*)m_pFilter)->IsForcedSubtitlesEnabled() || sp->m_psphli)))
			sp->Render(yuv, w, h, m_sppal, m_fsppal);
	}
}

HRESULT CSubpicInputPin::Transform(IMediaSample* pSample)
{
	HRESULT hr;

	AM_MEDIA_TYPE* pmt;
	if(SUCCEEDED(pSample->GetMediaType(&pmt)) && pmt)
	{
		CMediaType mt(*pmt);
		SetMediaType(&mt);
		DeleteMediaType(pmt);
	}

	BYTE* pDataIn = NULL;
	if(FAILED(hr = pSample->GetPointer(&pDataIn))) return hr;

	long len = pSample->GetActualDataLength();

	StripPacket(pDataIn, len);

	if(len <= 0) return S_FALSE;

	if(m_mt.subtype == MEDIASUBTYPE_SVCD_SUBPICTURE)
	{
		pDataIn += 4;
		len -= 4;
	}

	if(len <= 0) return S_FALSE;

	CAutoLock cAutoLock(&m_csReceive);

	REFERENCE_TIME rtStart = 0, rtStop = 0;
	hr = pSample->GetTime(&rtStart, &rtStop);

	bool fRefresh = false;

	if(FAILED(hr))
	{
		if(!m_sps.IsEmpty())
		{
			spu* sp = m_sps.GetTail();
			sp->m_pData.SetSize(sp->m_pData.GetSize() + len);
			memcpy(sp->m_pData.GetData() + sp->m_pData.GetSize() - len, pDataIn, len);
		}
	}
	else
	{
		POSITION pos = m_sps.GetTailPosition();
		while(pos)
		{
			POSITION cur = pos;
			spu* sp = m_sps.GetPrev(pos);
			if(sp->m_rtStop == _I64_MAX)
			{
				sp->m_rtStop = rtStart;
				break;
			}
		}

		CAutoPtr<spu> p;

		if(m_mt.subtype == MEDIASUBTYPE_DVD_SUBPICTURE) p.Attach(new dvdspu());
		else if(m_mt.subtype == MEDIASUBTYPE_CVD_SUBPICTURE) p.Attach(new cvdspu());
		else if(m_mt.subtype == MEDIASUBTYPE_SVCD_SUBPICTURE) p.Attach(new svcdspu());
		else return E_FAIL;

		p->m_rtStart = rtStart;
		p->m_rtStop = _I64_MAX;

		p->m_pData.SetSize(len);
		memcpy(p->m_pData.GetData(), pDataIn, len);

		if(m_sphli && p->m_rtStart == PTS2RT(m_sphli->StartPTM))
		{
			p->m_psphli = m_sphli;
			fRefresh = true;
		}

		m_sps.AddTail(p);
	}

	if(!m_sps.IsEmpty())
	{
		m_sps.GetTail()->Parse();
	}

	if(fRefresh)
	{
//		((CMpeg2DecFilter*)m_pFilter)->Deliver(true);
	}

	return S_FALSE;
}

STDMETHODIMP CSubpicInputPin::EndFlush()
{
	CAutoLock cAutoLock(&m_csReceive);
	m_sps.RemoveAll();
	return S_OK;
}

// IKsPropertySet

STDMETHODIMP CSubpicInputPin::Set(REFGUID PropSet, ULONG Id, LPVOID pInstanceData, ULONG InstanceLength, LPVOID pPropertyData, ULONG DataLength)
{
	if(PropSet != AM_KSPROPSETID_DvdSubPic)
		return __super::Set(PropSet, Id, pInstanceData, InstanceLength, pPropertyData, DataLength);

	bool fRefresh = false;

	switch(Id)
	{
	case AM_PROPERTY_DVDSUBPIC_PALETTE:
		{
			CAutoLock cAutoLock(&m_csReceive);
			AM_PROPERTY_SPPAL* pSPPAL = (AM_PROPERTY_SPPAL*)pPropertyData;
			memcpy(m_sppal, pSPPAL->sppal, sizeof(AM_PROPERTY_SPPAL));
			m_fsppal = true;

			DbgLog((LOG_TRACE, 0, _T("new palette")));
		}
		break;
	case AM_PROPERTY_DVDSUBPIC_HLI:
		{
			CAutoLock cAutoLock(&m_csReceive);

			AM_PROPERTY_SPHLI* pSPHLI = (AM_PROPERTY_SPHLI*)pPropertyData;

			m_sphli.Free();

			if(pSPHLI->HLISS)
			{
				POSITION pos = m_sps.GetHeadPosition();
				while(pos)
				{
					spu* sp = m_sps.GetNext(pos);
					if(sp->m_rtStart <= PTS2RT(pSPHLI->StartPTM) && PTS2RT(pSPHLI->StartPTM) < sp->m_rtStop)
					{
						fRefresh = true;
						sp->m_psphli.Free();
						sp->m_psphli.Attach(new AM_PROPERTY_SPHLI());
						memcpy((AM_PROPERTY_SPHLI*)sp->m_psphli, pSPHLI, sizeof(AM_PROPERTY_SPHLI));
					}
				}

				if(!fRefresh) // save it for later, a subpic might be late for this hli
				{
					m_sphli.Attach(new AM_PROPERTY_SPHLI());
					memcpy((AM_PROPERTY_SPHLI*)m_sphli, pSPHLI, sizeof(AM_PROPERTY_SPHLI));
				}
			}
			else
			{
				POSITION pos = m_sps.GetHeadPosition();
				while(pos)
				{
					spu* sp = m_sps.GetNext(pos);
					fRefresh |= !!sp->m_psphli;
					sp->m_psphli.Free();
				}
			}

			if(pSPHLI->HLISS)
			DbgLog((LOG_TRACE, 0, _T("hli: %I64d - %I64d, (%d,%d) - (%d,%d)"), 
				PTS2RT(pSPHLI->StartPTM)/10000, PTS2RT(pSPHLI->EndPTM)/10000,
				pSPHLI->StartX, pSPHLI->StartY, pSPHLI->StopX, pSPHLI->StopY));
		}
		break;
	case AM_PROPERTY_DVDSUBPIC_COMPOSIT_ON:
		{
			CAutoLock cAutoLock(&m_csReceive);
			AM_PROPERTY_COMPOSIT_ON* pCompositOn = (AM_PROPERTY_COMPOSIT_ON*)pPropertyData;
			m_spon = *pCompositOn;
		}
		break;
	default:
		return E_PROP_ID_UNSUPPORTED;
	}

	if(fRefresh)
	{
		((CMpeg2DecFilter*)m_pFilter)->Deliver(true);
	}

	return S_OK;
}

STDMETHODIMP CSubpicInputPin::QuerySupported(REFGUID PropSet, ULONG Id, ULONG* pTypeSupport)
{
	if(PropSet != AM_KSPROPSETID_DvdSubPic)
		return __super::QuerySupported(PropSet, Id, pTypeSupport);

	switch(Id)
	{
	case AM_PROPERTY_DVDSUBPIC_PALETTE:
		*pTypeSupport = KSPROPERTY_SUPPORT_SET;
		break;
	case AM_PROPERTY_DVDSUBPIC_HLI:
		*pTypeSupport = KSPROPERTY_SUPPORT_SET;
		break;
	case AM_PROPERTY_DVDSUBPIC_COMPOSIT_ON:
		*pTypeSupport = KSPROPERTY_SUPPORT_SET;
		break;
	default:
		return E_PROP_ID_UNSUPPORTED;
	}
	
	return S_OK;
}

// CSubpicInputPin::spu

static __inline BYTE GetNibble(BYTE* p, DWORD* offset, int& nField, int& fAligned)
{
	BYTE ret = (p[offset[nField]] >> (fAligned << 2)) & 0x0f;
	offset[nField] += 1-fAligned;
	fAligned = !fAligned;
    return ret;
}

static __inline BYTE GetHalfNibble(BYTE* p, DWORD* offset, int& nField, int& n)
{
	BYTE ret = (p[offset[nField]] >> (n << 1)) & 0x03;
	if(!n) offset[nField]++;
	n = (n-1+4)&3;
    return ret;
}

static __inline void DrawPixel(BYTE** yuv, CPoint pt, int pitch, AM_DVD_YUV& c)
{
	if(c.Reserved == 0) return;

	BYTE* p = &yuv[0][pt.y*pitch + pt.x];
//	*p = (*p*(15-contrast) + sppal[color].Y*contrast)>>4;
	*p -= (*p - c.Y) * c.Reserved >> 4;

	if(pt.y&1) return; // since U/V is half res there is no need to overwrite the same line again

	pt.x = (pt.x + 1) / 2;
	pt.y = (pt.y /*+ 1*/) / 2; // only paint the upper field always, don't round it
	pitch /= 2;

	// U/V is exchanged? wierd but looks true when comparing the outputted colors from other decoders

	p = &yuv[1][pt.y*pitch + pt.x];
//	*p = (BYTE)(((((int)*p-0x80)*(15-contrast) + ((int)sppal[color].V-0x80)*contrast) >> 4) + 0x80);
	*p -= (*p - c.V) * c.Reserved >> 4;

	p = &yuv[2][pt.y*pitch + pt.x];
//	*p = (BYTE)(((((int)*p-0x80)*(15-contrast) + ((int)sppal[color].U-0x80)*contrast) >> 4) + 0x80);
	*p -= (*p - c.U) * c.Reserved >> 4;

	// Neighter of the blending formulas are accurate (">>4" should be "/15").
	// Even though the second one is a bit worse, since we are scaling the difference only,
	// the error is still not noticable.
}

static __inline void DrawPixels(BYTE** yuv, int pitch, CPoint pt, int len, AM_DVD_YUV& c, CRect& rc)
{
    if(pt.y < rc.top || pt.y >= rc.bottom) return;
	if(pt.x < rc.left) {len -= rc.left - pt.x; pt.x = rc.left;}
	if(pt.x + len > rc.right) len = rc.right - pt.x;
	if(len <= 0 || pt.x >= rc.right) return;

	if(c.Reserved == 0)
	{
		if(rc.IsRectEmpty())
			return;

		if(pt.y < rc.top || pt.y >= rc.bottom 
		|| pt.x+len < rc.left || pt.x >= rc.right)
			return;
	}

	while(len-- > 0)
	{
		DrawPixel(yuv, pt, pitch, c);
		pt.x++;
	}
}

// CSubpicInputPin::dvdspu

bool CSubpicInputPin::dvdspu::Parse()
{
	BYTE* p = m_pData.GetData();

	WORD packetsize = (p[0]<<8)|p[1];
	WORD datasize = (p[2]<<8)|p[3];

    if(packetsize > m_pData.GetSize() || datasize > packetsize)
		return(false);

	int i, next = datasize;

	#define GetWORD (p[i]<<8)|p[i+1]; i += 2

	do
	{
		i = next;

		int pts = GetWORD;
		next = GetWORD;

		if(next > packetsize || next < datasize)
			return(false);

		for(bool fBreak = false; !fBreak; )
		{
			int len = 0;

			switch(p[i])
			{
				case 0x00: len = 0; break;
				case 0x01: len = 0; break;
				case 0x02: len = 0; break;
				case 0x03: len = 2; break;
				case 0x04: len = 2; break;
				case 0x05: len = 6; break;
				case 0x06: len = 4; break;
				default: len = 0; break;
			}

			if(i+len >= packetsize)
			{
				TRACE(_T("Warning: Wrong subpicture parameter block ending\n"));
				break;
			}

			switch(p[i++])
			{
				case 0x00: // forced start displaying
					m_fForced = true;
					break;
				case 0x01: // normal start displaying
					m_fForced = false;
					break;
				case 0x02: // stop displaying
					m_rtStop = m_rtStart + 1024*PTS2RT(pts);
					break;
				case 0x03:
					m_sphli.ColCon.emph2col = p[i]>>4;
					m_sphli.ColCon.emph1col = p[i]&0xf;
					m_sphli.ColCon.patcol = p[i+1]>>4;
					m_sphli.ColCon.backcol = p[i+1]&0xf;
					i += 2;
					break;
				case 0x04:
					m_sphli.ColCon.emph2con = p[i]>>4;
					m_sphli.ColCon.emph1con = p[i]&0xf;
					m_sphli.ColCon.patcon = p[i+1]>>4;
					m_sphli.ColCon.backcon = p[i+1]&0xf;
					i += 2;
					break;
				case 0x05:
					m_sphli.StartX = (p[i]<<4) + (p[i+1]>>4);
					m_sphli.StopX = ((p[i+1]&0x0f)<<8) + p[i+2]+1;
					m_sphli.StartY = (p[i+3]<<4) + (p[i+4]>>4);
					m_sphli.StopY = ((p[i+4]&0x0f)<<8) + p[i+5]+1;
					i += 6;
					break;
				case 0x06:
					m_offset[0] = GetWORD;
					m_offset[1] = GetWORD;
					break;
				case 0xff: // end of ctrlblk
					fBreak = true;
					continue;
				default: // skip this ctrlblk
					fBreak = true;
					break;
			}
		}
	}
	while(i <= next && i < packetsize);

	return(true);
}

void CSubpicInputPin::dvdspu::Render(BYTE** yuv, int w, int h, AM_DVD_YUV* sppal, bool fsppal)
{
	BYTE* p = m_pData.GetData();
	DWORD offset[2] = {m_offset[0], m_offset[1]};

	AM_PROPERTY_SPHLI sphli = m_sphli;
	CPoint pt(sphli.StartX, sphli.StartY);
	CRect rc(pt, CPoint(sphli.StopX, sphli.StopY));

	CRect rcclip(0, 0, w, h);
	rcclip &= rc;

	if(m_psphli)
	{
		rcclip &= CRect(m_psphli->StartX, m_psphli->StartY, m_psphli->StopX, m_psphli->StopY);
		sphli = *m_psphli;
	}

	AM_DVD_YUV pal[4];
	pal[0] = sppal[fsppal ? sphli.ColCon.backcol : 0];
	pal[0].Reserved = sphli.ColCon.backcon;
	pal[1] = sppal[fsppal ? sphli.ColCon.patcol : 1];
	pal[1].Reserved = sphli.ColCon.patcon;
	pal[2] = sppal[fsppal ? sphli.ColCon.emph1col : 2];
	pal[2].Reserved = sphli.ColCon.emph1con;
	pal[3] = sppal[fsppal ? sphli.ColCon.emph2col : 3];
	pal[3].Reserved = sphli.ColCon.emph2con;

	int nField = 0;
	int fAligned = 1;

	DWORD end[2] = {offset[1], (p[2]<<8)|p[3]};

	while((nField == 0 && offset[0] < end[0]) || (nField == 1 && offset[1] < end[1]))
	{
		DWORD code;

		if((code = GetNibble(p, offset, nField, fAligned)) >= 0x4
		|| (code = (code << 4) | GetNibble(p, offset, nField, fAligned)) >= 0x10
		|| (code = (code << 4) | GetNibble(p, offset, nField, fAligned)) >= 0x40
		|| (code = (code << 4) | GetNibble(p, offset, nField, fAligned)) >= 0x100)
		{
			DrawPixels(yuv, w, pt, code >> 2, pal[code&3], rcclip);
			if((pt.x += code >> 2) < rc.right) continue;
		}

		DrawPixels(yuv, w, pt, rc.right - pt.x, pal[code&3], rcclip);

		if(!fAligned) GetNibble(p, offset, nField, fAligned); // align to byte

		pt.x = rc.left;
		pt.y++;
		nField = 1 - nField;
	}
}

// CSubpicInputPin::cvdspu

bool CSubpicInputPin::cvdspu::Parse()
{
	BYTE* p = m_pData.GetData();

	WORD packetsize = (p[0]<<8)|p[1];
	WORD datasize = (p[2]<<8)|p[3];

    if(packetsize > m_pData.GetSize() || datasize > packetsize)
		return(false);

	p = m_pData.GetData() + datasize;

	for(int i = datasize, j = packetsize-4; i <= j; i+=4, p+=4)
	{
		switch(p[0])
		{
		case 0x0c: 
			break;
		case 0x04: 
			m_rtStop = m_rtStart + 10000i64*((p[1]<<16)|(p[2]<<8)|p[3])/90;
			break;
		case 0x17: 
			m_sphli.StartX = ((p[1]&0x0f)<<6) + (p[2]>>2);
			m_sphli.StartY = ((p[2]&0x03)<<8) + p[3];
			break;
		case 0x1f: 
			m_sphli.StopX = ((p[1]&0x0f)<<6) + (p[2]>>2);
			m_sphli.StopY = ((p[2]&0x03)<<8) + p[3];
			break;
		case 0x24: case 0x25: case 0x26: case 0x27: 
			m_sppal[0][p[0]-0x24].Y = p[1];
			m_sppal[0][p[0]-0x24].U = p[2];
			m_sppal[0][p[0]-0x24].V = p[3];
			break;
		case 0x2c: case 0x2d: case 0x2e: case 0x2f: 
			m_sppal[1][p[0]-0x2c].Y = p[1];
			m_sppal[1][p[0]-0x2c].U = p[2];
			m_sppal[1][p[0]-0x2c].V = p[3];
			break;
		case 0x37: 
			m_sppal[0][3].Reserved = p[2]>>4;
			m_sppal[0][2].Reserved = p[2]&0xf;
			m_sppal[0][1].Reserved = p[3]>>4;
			m_sppal[0][0].Reserved = p[3]&0xf;
			break;
		case 0x3f: 
			m_sppal[1][3].Reserved = p[2]>>4;
			m_sppal[1][2].Reserved = p[2]&0xf;
			m_sppal[1][1].Reserved = p[3]>>4;
			m_sppal[1][0].Reserved = p[3]&0xf;
			break;
		case 0x47: 
			m_offset[0] = (p[2]<<8)|p[3];
			break;
		case 0x4f: 
			m_offset[1] = (p[2]<<8)|p[3];
			break;
		default: 
			break;
		}
	}

	return(true);
}

void CSubpicInputPin::cvdspu::Render(BYTE** yuv, int w, int h, AM_DVD_YUV* sppal, bool fsppal)
{
	BYTE* p = m_pData.GetData();
	DWORD offset[2] = {m_offset[0], m_offset[1]};

	CRect rcclip(0, 0, w, h);

	/* FIXME: startx/y looks to be wrong in the sample I tested
	CPoint pt(m_sphli.StartX, m_sphli.StartY);
	CRect rc(pt, CPoint(m_sphli.StopX, m_sphli.StopY));
	*/

	CSize size(m_sphli.StopX - m_sphli.StartX, m_sphli.StopY - m_sphli.StartY);
	CPoint pt((rcclip.Width() - size.cx) / 2, (rcclip.Height()*3 - size.cy*1) / 4);
	CRect rc(pt, size);

	int nField = 0;
	int fAligned = 1;

	DWORD end[2] = {offset[1], (p[2]<<8)|p[3]};

	while((nField == 0 && offset[0] < end[0]) || (nField == 1 && offset[1] < end[1]))
	{
		BYTE code;

		if((code = GetNibble(p, offset, nField, fAligned)) >= 0x4)
		{
			DrawPixels(yuv, w, pt, code >> 2, m_sppal[0][code&3], rcclip);
			pt.x += code >> 2;
			continue;
		}

		code = GetNibble(p, offset, nField, fAligned);
		DrawPixels(yuv, w, pt, rc.right - pt.x, m_sppal[0][code&3], rcclip);

		if(!fAligned) GetNibble(p, offset, nField, fAligned); // align to byte

		pt.x = rc.left;
		pt.y++;
		nField = 1 - nField;
	}
}

// CSubpicInputPin::svcdspu

bool CSubpicInputPin::svcdspu::Parse()
{
	BYTE* p = m_pData.GetData();
	BYTE* p0 = p;

	if(m_pData.GetSize() < 2) 
		return(false);

	WORD packetsize = (p[0]<<8)|p[1]; p += 2;

    if(packetsize > m_pData.GetSize())
		return(false);

	bool duration = !!(*p++&0x04);

	*p++; // unknown

	if(duration)
	{
		m_rtStop = m_rtStart + 10000i64*((p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3])/90;
		p += 4;
	}

	m_sphli.StartX = m_sphli.StopX = (p[0]<<8)|p[1]; p += 2;
	m_sphli.StartY = m_sphli.StopY = (p[0]<<8)|p[1]; p += 2;
	m_sphli.StopX += (p[0]<<8)|p[1]; p += 2;
	m_sphli.StopY += (p[0]<<8)|p[1]; p += 2;

	for(int i = 0; i < 4; i++)
	{
		m_sppal[i].Y = *p++;
		m_sppal[i].U = *p++;
		m_sppal[i].V = *p++;
		m_sppal[i].Reserved = *p++ >> 4;
	}

	if(*p++&0xc0)
		p += 4; // duration of the shift operation should be here, but it is untested

	m_offset[1] = (p[0]<<8)|p[1]; p += 2;

	m_offset[0] = p - p0;
	m_offset[1] += m_offset[0];

	return(true);
}

void CSubpicInputPin::svcdspu::Render(BYTE** yuv, int w, int h, AM_DVD_YUV* sppal, bool fsppal)
{
	BYTE* p = m_pData.GetData();
	DWORD offset[2] = {m_offset[0], m_offset[1]};

	CRect rcclip(0, 0, w, h);

	/* FIXME: startx/y looks to be wrong in the sample I tested (yes, this one too!)
	CPoint pt(m_sphli.StartX, m_sphli.StartY);
	CRect rc(pt, CPoint(m_sphli.StopX, m_sphli.StopY));
	*/

	CSize size(m_sphli.StopX - m_sphli.StartX, m_sphli.StopY - m_sphli.StartY);
	CPoint pt((rcclip.Width() - size.cx) / 2, (rcclip.Height()*3 - size.cy*1) / 4);
	CRect rc(pt, size);

	int nField = 0;
	int n = 3;

	DWORD end[2] = {offset[1], (p[2]<<8)|p[3]};

	while((nField == 0 && offset[0] < end[0]) || (nField == 1 && offset[1] < end[1]))
	{
		BYTE code = GetHalfNibble(p, offset, nField, n);
		BYTE repeat = 1 + (code == 0 ? GetHalfNibble(p, offset, nField, n) : 0);

		DrawPixels(yuv, w, pt, repeat, m_sppal[code&3], rcclip);
		if((pt.x += repeat) < rc.right) continue;

		while(n != 3)
			GetHalfNibble(p, offset, nField, n); // align to byte

		pt.x = rc.left;
		pt.y++;
		nField = 1 - nField;
	}
}

//
// CMpeg2DecOutputPin
//

CMpeg2DecOutputPin::CMpeg2DecOutputPin(CTransformFilter* pTransformFilter, HRESULT* phr)
	: CTransformOutputPin(NAME("CMpeg2DecOutputPin"), pTransformFilter, phr, L"Output")
{
}

HRESULT CMpeg2DecOutputPin::CheckMediaType(const CMediaType* mtOut)
{
	HRESULT hr = ((CMpeg2DecFilter*)m_pFilter)->CheckOutputMediaType(*mtOut);
	if(FAILED(hr)) return hr;
	return __super::CheckMediaType(mtOut);
}

//
// CClosedCaptionOutputPin
//

CClosedCaptionOutputPin::CClosedCaptionOutputPin(CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseOutputPin(NAME("CClosedCaptionOutputPin"), pFilter, pLock, phr, L"~CC")
{
}

HRESULT CClosedCaptionOutputPin::CheckMediaType(const CMediaType* mtOut)
{
	return mtOut->majortype == MEDIATYPE_AUXLine21Data && mtOut->subtype == MEDIASUBTYPE_Line21_GOPPacket
		? S_OK
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CClosedCaptionOutputPin::GetMediaType(int iPosition, CMediaType* pmt)
{
	if(iPosition < 0) return E_INVALIDARG;
	if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;

	pmt->InitMediaType();
	pmt->majortype = MEDIATYPE_AUXLine21Data;
	pmt->subtype = MEDIASUBTYPE_Line21_GOPPacket;
	pmt->formattype = FORMAT_None;

	return S_OK;
}

HRESULT CClosedCaptionOutputPin::DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties)
{
	pProperties->cBuffers = 1;
	pProperties->cbBuffer = 2048;
	pProperties->cbAlign = 1;
	pProperties->cbPrefix = 0;

	HRESULT hr;
	ALLOCATOR_PROPERTIES Actual;
    if(FAILED(hr = pAllocator->SetProperties(pProperties, &Actual))) 
		return hr;

    return pProperties->cBuffers > Actual.cBuffers || pProperties->cbBuffer > Actual.cbBuffer
		? E_FAIL
		: NOERROR;
}