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
#include <mmreg.h>
#include "MpaDecFilter.h"

#include "..\..\..\DSUtil\DSUtil.h"

#include <initguid.h>
#include "..\..\..\..\include\moreuuids.h"

#include "faad2\include\neaacdec.h"

const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] =
{
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_MP3},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG1AudioPayload},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG1Payload},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG1Packet},
	{&MEDIATYPE_DVD_ENCRYPTED_PACK, &MEDIASUBTYPE_MPEG2_AUDIO},
	{&MEDIATYPE_MPEG2_PACK, &MEDIASUBTYPE_MPEG2_AUDIO},
	{&MEDIATYPE_MPEG2_PES, &MEDIASUBTYPE_MPEG2_AUDIO},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG2_AUDIO},
	{&MEDIATYPE_DVD_ENCRYPTED_PACK, &MEDIASUBTYPE_DOLBY_AC3},
	{&MEDIATYPE_MPEG2_PACK, &MEDIASUBTYPE_DOLBY_AC3},
	{&MEDIATYPE_MPEG2_PES, &MEDIASUBTYPE_DOLBY_AC3},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_DOLBY_AC3},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_WAVE_DOLBY_AC3},
	{&MEDIATYPE_DVD_ENCRYPTED_PACK, &MEDIASUBTYPE_DTS},
	{&MEDIATYPE_MPEG2_PACK, &MEDIASUBTYPE_DTS},
	{&MEDIATYPE_MPEG2_PES, &MEDIASUBTYPE_DTS},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_DTS},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_WAVE_DTS},
	{&MEDIATYPE_DVD_ENCRYPTED_PACK, &MEDIASUBTYPE_DVD_LPCM_AUDIO},
	{&MEDIATYPE_MPEG2_PACK, &MEDIASUBTYPE_DVD_LPCM_AUDIO},
	{&MEDIATYPE_MPEG2_PES, &MEDIASUBTYPE_DVD_LPCM_AUDIO},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_DVD_LPCM_AUDIO},
	{&MEDIATYPE_DVD_ENCRYPTED_PACK, &MEDIASUBTYPE_AAC},
	{&MEDIATYPE_MPEG2_PACK, &MEDIASUBTYPE_AAC},
	{&MEDIATYPE_MPEG2_PES, &MEDIASUBTYPE_AAC},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_AAC},
	{&MEDIATYPE_DVD_ENCRYPTED_PACK, &MEDIASUBTYPE_MP4A},
	{&MEDIATYPE_MPEG2_PACK, &MEDIASUBTYPE_MP4A},
	{&MEDIATYPE_MPEG2_PES, &MEDIASUBTYPE_MP4A},
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_MP4A},
};

#ifdef REGISTER_FILTER

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] =
{
	{&MEDIATYPE_Audio, &MEDIASUBTYPE_PCM},
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
      sudPinTypesIn		// Pin information
    },
    { L"Output",            // Pins string name
      FALSE,                // Is it rendered
      TRUE,                 // Is it an output
      FALSE,                // Are we allowed none
      FALSE,                // And allowed many
      &CLSID_NULL,          // Connects to filter
      NULL,                 // Connects to pin
      countof(sudPinTypesOut), // Number of types
      sudPinTypesOut		// Pin information
    }
};

const AMOVIESETUP_FILTER sudFilter[] =
{
	{&__uuidof(CMpaDecFilter), L"MPEG/AC3/DTS/LPCM Audio Decoder", 0x40000001, countof(sudpPins), sudpPins},
};

CFactoryTemplate g_Templates[] =
{
    {L"MPEG/AC3/DTS/LPCM Audio Decoder", &__uuidof(CMpaDecFilter), CMpaDecFilter::CreateInstance, NULL, &sudFilter[0]},
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

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	return DllEntryPoint((HINSTANCE)hModule, ul_reason_for_call, 0); // "DllMain" of the dshow baseclasses;
}

//
// CMpaDecFilter
//

CUnknown* WINAPI CMpaDecFilter::CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
    CUnknown* punk = new CMpaDecFilter(lpunk, phr);
    if(punk == NULL) *phr = E_OUTOFMEMORY;
	return punk;
}

#endif

// dshow: left, right, center, LFE, left surround, right surround
// ac3: LFE, left, center, right, left surround, right surround
// dts: center, left, right, left surround, right surround, LFE

// lets see how we can map these things to dshow (oh the joy!)

static struct scmap_t
{
	WORD nChannels;
	BYTE ch[6];
	DWORD dwChannelMask;
}
s_scmap_ac3[2*11] = 
{
	{2, {0, 1,-1,-1,-1,-1}, 0},	// A52_CHANNEL
	{1, {0,-1,-1,-1,-1,-1}, 0}, // A52_MONO
	{2, {0, 1,-1,-1,-1,-1}, 0}, // A52_STEREO
	{3, {0, 2, 1,-1,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER}, // A52_3F
	{3, {0, 1, 2,-1,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_BACK_CENTER}, // A52_2F1R
	{4, {0, 2, 1, 3,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_BACK_CENTER}, // A52_3F1R
	{4, {0, 1, 2, 3,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT}, // A52_2F2R
	{5, {0, 2, 1, 3, 4,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT}, // A52_3F2R
	{1, {0,-1,-1,-1,-1,-1}, 0}, // A52_CHANNEL1
	{1, {0,-1,-1,-1,-1,-1}, 0}, // A52_CHANNEL2
	{2, {0, 1,-1,-1,-1,-1}, 0}, // A52_DOLBY

	{3, {1, 2, 0,-1,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_LOW_FREQUENCY},	// A52_CHANNEL|A52_LFE
	{2, {1, 0,-1,-1,-1,-1}, SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY}, // A52_MONO|A52_LFE
	{3, {1, 2, 0,-1,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_LOW_FREQUENCY}, // A52_STEREO|A52_LFE
	{4, {1, 3, 2, 0,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY}, // A52_3F|A52_LFE
	{4, {1, 2, 0, 3,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_CENTER}, // A52_2F1R|A52_LFE
	{5, {1, 3, 2, 0, 4,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_CENTER}, // A52_3F1R|A52_LFE
	{5, {1, 2, 0, 3, 4,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT}, // A52_2F2R|A52_LFE
	{6, {1, 3, 2, 0, 4, 5}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT}, // A52_3F2R|A52_LFE
	{2, {1, 0,-1,-1,-1,-1}, SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY}, // A52_CHANNEL1|A52_LFE
	{2, {1, 0,-1,-1,-1,-1}, SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY}, // A52_CHANNEL2|A52_LFE
	{3, {1, 2, 0,-1,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_LOW_FREQUENCY}, // A52_DOLBY|A52_LFE
},
s_scmap_dts[2*10] = 
{
	{1, {0,-1,-1,-1,-1,-1}, 0}, // DTS_MONO
	{2, {0, 1,-1,-1,-1,-1}, 0},	// DTS_CHANNEL
	{2, {0, 1,-1,-1,-1,-1}, 0}, // DTS_STEREO
	{2, {0, 1,-1,-1,-1,-1}, 0}, // DTS_STEREO_SUMDIFF
	{2, {0, 1,-1,-1,-1,-1}, 0}, // DTS_STEREO_TOTAL
	{3, {1, 2, 0,-1,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER}, // DTS_3F
	{3, {0, 1, 2,-1,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_BACK_CENTER}, // DTS_2F1R
	{4, {1, 2, 0, 3,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_BACK_CENTER}, // DTS_3F1R
	{4, {0, 1, 2, 3,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT}, // DTS_2F2R
	{5, {1, 2, 0, 3, 4,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT}, // DTS_3F2R

	{2, {0, 1,-1,-1,-1,-1}, SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY}, // DTS_MONO|DTS_LFE
	{3, {0, 1, 2,-1,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_LOW_FREQUENCY},	// DTS_CHANNEL|DTS_LFE
	{3, {0, 1, 2,-1,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_LOW_FREQUENCY}, // DTS_STEREO|DTS_LFE
	{3, {0, 1, 2,-1,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_LOW_FREQUENCY}, // DTS_STEREO_SUMDIFF|DTS_LFE
	{3, {0, 1, 2,-1,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_LOW_FREQUENCY}, // DTS_STEREO_TOTAL|DTS_LFE
	{4, {1, 2, 0, 3,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY}, // DTS_3F|DTS_LFE
	{4, {0, 1, 3, 2,-1,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_CENTER}, // DTS_2F1R|DTS_LFE
	{5, {1, 2, 0, 4, 3,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_CENTER}, // DTS_3F1R|DTS_LFE
	{5, {0, 1, 4, 2, 3,-1}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT}, // DTS_2F2R|DTS_LFE
	{6, {1, 2, 0, 5, 3, 4}, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT}, // DTS_3F2R|DTS_LFE
};

CMpaDecFilter::CMpaDecFilter(LPUNKNOWN lpunk, HRESULT* phr) 
	: CTransformFilter(NAME("CMpaDecFilter"), lpunk, __uuidof(this))
	, m_iSampleFormat(SF_PCM16)
	, m_fNormalize(false)
	, m_boost(1)
{
	if(phr) *phr = S_OK;

	if(!(m_pInput = new CMpaDecInputPin(this, phr, L"In"))) *phr = E_OUTOFMEMORY;
	if(FAILED(*phr)) return;

	if(!(m_pOutput = new CTransformOutputPin(NAME("CTransformOutputPin"), this, phr, L"Out"))) *phr = E_OUTOFMEMORY;
	if(FAILED(*phr))  {delete m_pInput, m_pInput = NULL; return;}

	m_iSpeakerConfig[ac3] = A52_STEREO;
	m_iSpeakerConfig[dts] = DTS_STEREO;
	m_iSpeakerConfig[aac] = AAC_STEREO;
	m_fDynamicRangeControl[ac3] = false;
	m_fDynamicRangeControl[dts] = false;
	m_fDynamicRangeControl[aac] = false;
}

CMpaDecFilter::~CMpaDecFilter()
{
}

STDMETHODIMP CMpaDecFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(IMpaDecFilter)
		 __super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CMpaDecFilter::EndOfStream()
{
	CAutoLock cAutoLock(&m_csReceive);
	return __super::EndOfStream();
}

HRESULT CMpaDecFilter::BeginFlush()
{
	return __super::BeginFlush();
}

HRESULT CMpaDecFilter::EndFlush()
{
	CAutoLock cAutoLock(&m_csReceive);
	m_buff.RemoveAll();
	m_sample_max = 0.1f;
	return __super::EndFlush();
}

HRESULT CMpaDecFilter::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	CAutoLock cAutoLock(&m_csReceive);
	m_buff.RemoveAll();
	m_sample_max = 0.1f;
	return __super::NewSegment(tStart, tStop, dRate);
}

HRESULT CMpaDecFilter::Receive(IMediaSample* pIn)
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
		pmt = NULL;
		m_sample_max = 0.1f;
		m_aac_state.init(mt);
	}

	BYTE* pDataIn = NULL;
	if(FAILED(hr = pIn->GetPointer(&pDataIn))) return hr;

	long len = pIn->GetActualDataLength();

	((CDeCSSInputPin*)m_pInput)->StripPacket(pDataIn, len);

	REFERENCE_TIME rtStart = _I64_MIN, rtStop = _I64_MIN;
	hr = pIn->GetTime(&rtStart, &rtStop);

	if(pIn->IsDiscontinuity() == S_OK)
	{
		m_fDiscontinuity = true;
		m_buff.RemoveAll();
		m_sample_max = 0.1f;
		// ASSERT(SUCCEEDED(hr)); // what to do if not?
		if(FAILED(hr)) {TRACE(_T("mpa: disc. w/o timestamp\n")); return S_OK;} // lets wait then...
		m_rtStart = rtStart;
	}

	if(SUCCEEDED(hr) && abs((int)(m_rtStart - rtStart)) > 1000000) // +-100ms jitter is allowed for now
	{
		m_buff.RemoveAll();
		m_rtStart = rtStart;
	}

	int tmp = m_buff.GetSize();
	m_buff.SetSize(m_buff.GetSize() + len);
	memcpy(m_buff.GetData() + tmp, pDataIn, len);
	len += tmp;

	const GUID& subtype = m_pInput->CurrentMediaType().subtype;

	if(subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO)
		hr = ProcessLPCM();
	else if(subtype == MEDIASUBTYPE_DOLBY_AC3 || subtype == MEDIASUBTYPE_WAVE_DOLBY_AC3)
		hr = ProcessAC3();
	else if(subtype == MEDIASUBTYPE_DTS || subtype == MEDIASUBTYPE_WAVE_DTS)
		hr = ProcessDTS();
	else if(subtype == MEDIASUBTYPE_AAC || subtype == MEDIASUBTYPE_MP4A)
		hr = ProcessAAC();
	else // if(.. the rest ..)
		hr = ProcessMPA();

	return hr;
}

HRESULT CMpaDecFilter::ProcessLPCM()
{
	WAVEFORMATEX* wfein = (WAVEFORMATEX*)m_pInput->CurrentMediaType().Format();

	ASSERT(wfein->nChannels == 2);
	ASSERT(wfein->wBitsPerSample == 16);

	BYTE* pDataIn = m_buff.GetData();
	int len = m_buff.GetSize() & ~(wfein->nChannels*wfein->wBitsPerSample/8-1);

	CArray<float> pBuff;
	pBuff.SetSize(len*8/wfein->wBitsPerSample);

	float* pDataOut = pBuff.GetData();
	for(int i = 0; i < len; i += 2, pDataIn += 2, pDataOut++)
		*pDataOut = (float)(short)((pDataIn[0]<<8)|pDataIn[1]) / 0x8000; // FIXME: care about 20/24 bps too

	memmove(m_buff.GetData(), pDataIn, m_buff.GetSize() - len);
	m_buff.SetSize(m_buff.GetSize() - len);

	return Deliver(pBuff, wfein->nSamplesPerSec, wfein->nChannels);
}

HRESULT CMpaDecFilter::ProcessAC3()
{
	BYTE* p = m_buff.GetData();
	BYTE* base = p;
	BYTE* end = p + m_buff.GetSize();

	while(end - p >= 7)
	{
		int size = 0, flags, sample_rate, bit_rate;

		if((size = a52_syncinfo(p, &flags, &sample_rate, &bit_rate)) > 0)
		{
//			TRACE(_T("ac3: size=%d, flags=%08x, sample_rate=%d, bit_rate=%d\n"), size, flags, sample_rate, bit_rate);

			bool fEnoughData = p + size <= end;

			if(fEnoughData)
			{
				int iSpeakerConfig = GetSpeakerConfig(ac3);

				if(iSpeakerConfig < 0)
				{
					HRESULT hr;
					if(S_OK != (hr = Deliver(p, size, bit_rate, 0x0001)))
						return hr;
				}
				else
				{
					flags = iSpeakerConfig&(A52_CHANNEL_MASK|A52_LFE);
					flags |= A52_ADJUST_LEVEL;

					sample_t level = 1, gain = 1, bias = 0;
					level *= gain;

					if(a52_frame(m_a52_state, p, &flags, &level, bias) == 0)
					{
						if(GetDynamicRangeControl(ac3))
							a52_dynrng(m_a52_state, NULL, NULL);

						int scmapidx = min(flags&A52_CHANNEL_MASK, countof(s_scmap_ac3)/2);
                        scmap_t& scmap = s_scmap_ac3[scmapidx + ((flags&A52_LFE)?(countof(s_scmap_ac3)/2):0)];

						CArray<float> pBuff;
						pBuff.SetSize(6*256*scmap.nChannels);
						float* p = pBuff.GetData();

						int i = 0;

						for(; i < 6 && a52_block(m_a52_state) == 0; i++)
						{
							sample_t* samples = a52_samples(m_a52_state);

							for(int j = 0; j < 256; j++, samples++)
							{
								for(int ch = 0; ch < scmap.nChannels; ch++)
								{
									ASSERT(scmap.ch[ch] != -1);
									*p++ = (float)(*(samples + 256*scmap.ch[ch]) / level);
								}
							}
						}

						if(i == 6)
						{
							HRESULT hr;
							if(S_OK != (hr = Deliver(pBuff, sample_rate, scmap.nChannels, scmap.dwChannelMask)))
								return hr;
						}
					}
				}

				p += size;
			}

			memmove(base, p, end - p);
			end = base + (end - p);
			p = base;

			if(!fEnoughData)
				break;
		}
		else
		{
			p++;
		}
	}

	m_buff.SetSize(end - p);

	return S_OK;
}

HRESULT CMpaDecFilter::ProcessDTS()
{
	BYTE* p = m_buff.GetData();
	BYTE* base = p;
	BYTE* end = p + m_buff.GetSize();

	while(end - p >= 14)
	{
		int size = 0, flags, sample_rate, bit_rate, frame_length;

		if((size = dts_syncinfo(m_dts_state, p, &flags, &sample_rate, &bit_rate, &frame_length)) > 0)
		{
//			TRACE(_T("dts: size=%d, flags=%08x, sample_rate=%d, bit_rate=%d, frame_length=%d\n"), size, flags, sample_rate, bit_rate, frame_length);

			bool fEnoughData = p + size <= end;

			if(fEnoughData)
			{
				int iSpeakerConfig = GetSpeakerConfig(dts);

				if(iSpeakerConfig < 0)
				{
					HRESULT hr;
					if(S_OK != (hr = Deliver(p, size, bit_rate, 0x000b)))
						return hr;
				}
				else
				{
					flags = iSpeakerConfig&(DTS_CHANNEL_MASK|DTS_LFE);
					flags |= DTS_ADJUST_LEVEL;

					sample_t level = 1, gain = 1, bias = 0;
					level *= gain;

					if(dts_frame(m_dts_state, p, &flags, &level, bias) == 0)
					{
						if(GetDynamicRangeControl(dts))
							dts_dynrng(m_dts_state, NULL, NULL);

						int scmapidx = min(flags&DTS_CHANNEL_MASK, countof(s_scmap_dts)/2);
                        scmap_t& scmap = s_scmap_dts[scmapidx + ((flags&DTS_LFE)?(countof(s_scmap_dts)/2):0)];

						int blocks = dts_blocks_num(m_dts_state);

						CArray<float> pBuff;
						pBuff.SetSize(blocks*256*scmap.nChannels);
						float* p = pBuff.GetData();

						int i = 0;

						for(; i < blocks && dts_block(m_dts_state) == 0; i++)
						{
							sample_t* samples = dts_samples(m_dts_state);

							for(int j = 0; j < 256; j++, samples++)
							{
								for(int ch = 0; ch < scmap.nChannels; ch++)
								{
									ASSERT(scmap.ch[ch] != -1);
									*p++ = (float)(*(samples + 256*scmap.ch[ch]) / level);
								}
							}
						}

						if(i == blocks)
						{
							HRESULT hr;
							if(S_OK != (hr = Deliver(pBuff, sample_rate, scmap.nChannels, scmap.dwChannelMask)))
								return hr;
						}
					}
				}

				p += size;
			}

			memmove(base, p, end - p);
			end = base + (end - p);
			p = base;

			if(!fEnoughData)
				break;
		}
		else
		{
			p++;
		}
	}

	m_buff.SetSize(end - p);

	return S_OK;
}

HRESULT CMpaDecFilter::ProcessAAC()
{
	int iSpeakerConfig = GetSpeakerConfig(aac);

	NeAACDecConfigurationPtr c = NeAACDecGetCurrentConfiguration(m_aac_state.h);
	c->downMatrix = iSpeakerConfig;
	NeAACDecSetConfiguration(m_aac_state.h, c);

	NeAACDecFrameInfo info;
	float* src = (float*)NeAACDecDecode(m_aac_state.h, &info, m_buff.GetData(), m_buff.GetSize());
	m_buff.SetSize(0);
	//if(!src) return E_FAIL;
	if(info.error) 	m_aac_state.init(m_pInput->CurrentMediaType());
	if(!src || info.samples == 0) return S_OK;

	// HACK: bug in faad2 with mono sources?
	if(info.channels == 2 && info.channel_position[1] == UNKNOWN_CHANNEL)
	{
		info.channel_position[0] = FRONT_CHANNEL_LEFT;
		info.channel_position[1] = FRONT_CHANNEL_RIGHT;
	}

	CArray<float> pBuff;
	pBuff.SetSize(info.samples);
	float* dst = pBuff.GetData();

	CMap<int,int,int,int> chmask;
	chmask[FRONT_CHANNEL_CENTER] = SPEAKER_FRONT_CENTER;
	chmask[FRONT_CHANNEL_LEFT] = SPEAKER_FRONT_LEFT;
	chmask[FRONT_CHANNEL_RIGHT] = SPEAKER_FRONT_RIGHT;
	chmask[SIDE_CHANNEL_LEFT] = SPEAKER_SIDE_LEFT;
	chmask[SIDE_CHANNEL_RIGHT] = SPEAKER_SIDE_RIGHT;
	chmask[BACK_CHANNEL_LEFT] = SPEAKER_BACK_LEFT;
	chmask[BACK_CHANNEL_RIGHT] = SPEAKER_BACK_RIGHT;
	chmask[BACK_CHANNEL_CENTER] = SPEAKER_BACK_CENTER;
	chmask[LFE_CHANNEL] = SPEAKER_LOW_FREQUENCY;

	DWORD dwChannelMask = 0;
	for(int i = 0; i < info.channels; i++)
	{
		if(info.channel_position[i] == UNKNOWN_CHANNEL) {ASSERT(0); return E_FAIL;}
		dwChannelMask |= chmask[info.channel_position[i]];
	}

	int chmap[countof(info.channel_position)];
	memset(chmap, 0, sizeof(chmap));

	for(int i = 0; i < info.channels; i++)
	{
		unsigned int ch = 0, mask = chmask[info.channel_position[i]];

		for(int j = 0; j < 32; j++)
		{
			if(dwChannelMask & (1 << j))
			{
				if((1 << j) == mask) {chmap[i] = ch; break;}
				ch++;
			}
		}
	}

	if(info.channels <= 2) dwChannelMask = 0;

	for(int j = 0; j < info.samples; j += info.channels, dst += info.channels)
		for(int i = 0; i < info.channels; i++)
			dst[chmap[i]] = *src++;

	HRESULT hr;
	if(S_OK != (hr = Deliver(pBuff, info.samplerate, info.channels, dwChannelMask)))
		return hr;

	return S_OK;
}

static inline float fscale(mad_fixed_t sample)
{
	if(sample >= MAD_F_ONE) sample = MAD_F_ONE - 1;
	else if(sample < -MAD_F_ONE) sample = -MAD_F_ONE;

	return (float)sample / (1 << MAD_F_FRACBITS);
}

HRESULT CMpaDecFilter::ProcessMPA()
{
	mad_stream_buffer(&m_stream, m_buff.GetData(), m_buff.GetSize());

	while(1)
	{
		if(mad_frame_decode(&m_frame, &m_stream) == -1)
		{
			if(m_stream.error == MAD_ERROR_BUFLEN)
			{
				memmove(m_buff.GetData(), m_stream.this_frame, m_stream.bufend - m_stream.this_frame);
				m_buff.SetSize(m_stream.bufend - m_stream.this_frame);
				break;
			}

			if(!MAD_RECOVERABLE(m_stream.error))
			{
				TRACE(_T("*m_stream.error == %d\n"), m_stream.error);
				return E_FAIL;
			}

			// FIXME: the renderer doesn't like this
			// m_fDiscontinuity = true;
			
			continue;
		}
/*
// TODO: needs to be tested... (has anybody got an external mpeg audio decoder?)
HRESULT hr;
if(S_OK != (hr = Deliver(
   (BYTE*)m_stream.this_frame, 
   m_stream.next_frame - m_stream.this_frame, 
   m_frame.header.bitrate, 
   m_frame.header.layer == 1 ? 0x0004 : 0x0005)))
	return hr;
continue;
*/
		mad_synth_frame(&m_synth, &m_frame);

		WAVEFORMATEX* wfein = (WAVEFORMATEX*)m_pInput->CurrentMediaType().Format();
		if(wfein->nChannels != m_synth.pcm.channels || wfein->nSamplesPerSec != m_synth.pcm.samplerate)
			continue;

		const mad_fixed_t* left_ch   = m_synth.pcm.samples[0];
		const mad_fixed_t* right_ch  = m_synth.pcm.samples[1];

		CArray<float> pBuff;
		pBuff.SetSize(m_synth.pcm.length*m_synth.pcm.channels);

		float* pDataOut = pBuff.GetData();
		for(unsigned short i = 0; i < m_synth.pcm.length; i++)
		{
			*pDataOut++ = fscale(*left_ch++);
			if(m_synth.pcm.channels == 2) *pDataOut++ = fscale(*right_ch++);
		}

		HRESULT hr;
		if(S_OK != (hr = Deliver(pBuff, m_synth.pcm.samplerate, m_synth.pcm.channels)))
			return hr;
	}

	return S_OK;
}

HRESULT CMpaDecFilter::GetDeliveryBuffer(IMediaSample** pSample, BYTE** pData)
{
	HRESULT hr;

	*pData = NULL;
	if(FAILED(hr = m_pOutput->GetDeliveryBuffer(pSample, NULL, NULL, 0))
	|| FAILED(hr = (*pSample)->GetPointer(pData)))
		return hr;

	AM_MEDIA_TYPE* pmt = NULL;
	if(SUCCEEDED((*pSample)->GetMediaType(&pmt)) && pmt)
	{
		CMediaType mt = *pmt;
		m_pOutput->SetMediaType(&mt);
		DeleteMediaType(pmt);
		pmt = NULL;
	}

	return S_OK;
}

HRESULT CMpaDecFilter::Deliver(CArray<float>& pBuff, DWORD nSamplesPerSec, WORD nChannels, DWORD dwChannelMask)
{
	HRESULT hr;

	SampleFormat sf = GetSampleFormat();

	CMediaType mt = CreateMediaType(sf, nSamplesPerSec, nChannels, dwChannelMask);
	WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.Format();

	int nSamples = pBuff.GetSize()/wfe->nChannels;

	if(FAILED(hr = ReconnectOutput(nSamples, mt)))
		return hr;

	CComPtr<IMediaSample> pOut;
	BYTE* pDataOut = NULL;
	if(FAILED(GetDeliveryBuffer(&pOut, &pDataOut)))
		return E_FAIL;

	REFERENCE_TIME rtDur = 10000000i64*nSamples/wfe->nSamplesPerSec;
	REFERENCE_TIME rtStart = m_rtStart, rtStop = m_rtStart + rtDur;
	m_rtStart += rtDur;
//TRACE(_T("CMpaDecFilter: %I64d - %I64d\n"), rtStart/10000, rtStop/10000);
	if(rtStart < 0 /*200000*/ /* < 0, FIXME: 0 makes strange noises */)
		return S_OK;

	if(hr == S_OK)
	{
		m_pOutput->SetMediaType(&mt);
		pOut->SetMediaType(&mt);
	}

	pOut->SetTime(&rtStart, &rtStop);
	pOut->SetMediaTime(NULL, NULL);

	pOut->SetPreroll(FALSE);
	pOut->SetDiscontinuity(m_fDiscontinuity); m_fDiscontinuity = false;
	pOut->SetSyncPoint(TRUE);

	pOut->SetActualDataLength(pBuff.GetSize()*wfe->wBitsPerSample/8);

WAVEFORMATEX* wfeout = (WAVEFORMATEX*)m_pOutput->CurrentMediaType().Format();
ASSERT(wfeout->nChannels == wfe->nChannels);
ASSERT(wfeout->nSamplesPerSec == wfe->nSamplesPerSec);

	float* pDataIn = pBuff.GetData();

	// TODO: move this into the audio switcher
	float sample_mul = 1;
	if(m_fNormalize)
	{
		for(int i = 0, len = pBuff.GetSize(); i < len; i++)
		{
			float f = *pDataIn++;
			if(f < 0) f = -f;
			if(m_sample_max < f) m_sample_max = f;
		}
		sample_mul = 1.0f / m_sample_max;
		pDataIn = pBuff.GetData();
	}

	bool fBoost = m_boost > 1;

	for(int i = 0, len = pBuff.GetSize(); i < len; i++)
	{
		float f = *pDataIn++;

		// TODO: move this into the audio switcher

		if(m_fNormalize) 
			f *= sample_mul;

		if(fBoost)
			f *= 1+log10(m_boost);

		if(f < -1) f = -1;
		else if(f > 1) f = 1;

		switch(sf)
		{
		default:
		case SF_PCM16:
			*(short*)pDataOut = (short)(f * SHRT_MAX);
			pDataOut += sizeof(short);
			break;
		case SF_PCM24:
			{DWORD i24 = (DWORD)(int)(f * ((1<<23)-1));
			*pDataOut++ = (BYTE)(i24);
			*pDataOut++ = (BYTE)(i24>>8);
			*pDataOut++ = (BYTE)(i24>>16);}
			break;
		case SF_PCM32:
			*(int*)pDataOut = (int)(f * INT_MAX);
			pDataOut += sizeof(int);
			break;
		case SF_FLOAT32:
			*(float*)pDataOut = f;
			pDataOut += sizeof(float);
			break;
		}
	}

	return m_pOutput->Deliver(pOut);
}

HRESULT CMpaDecFilter::Deliver(BYTE* pBuff, int size, int bit_rate, BYTE type)
{
	HRESULT hr;

	CMediaType mt = CreateMediaTypeSPDIF();
	WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.Format();

	int length = 0;
	while(length < size+sizeof(WORD)*4) length += 0x800;
	int size2 = 1i64 * wfe->nBlockAlign * wfe->nSamplesPerSec * size*8 / bit_rate;
	while(length < size2) length += 0x800;

	if(FAILED(hr = ReconnectOutput(length / wfe->nBlockAlign, mt)))
		return hr;

	CComPtr<IMediaSample> pOut;
	BYTE* pDataOut = NULL;
	if(FAILED(GetDeliveryBuffer(&pOut, &pDataOut)))
		return E_FAIL;

	REFERENCE_TIME rtDur = 10000000i64 * size*8 / bit_rate;
	REFERENCE_TIME rtStart = m_rtStart, rtStop = m_rtStart + rtDur;
	m_rtStart += rtDur;

	if(rtStart < 0)
		return S_OK;

	if(hr == S_OK)
	{
		m_pOutput->SetMediaType(&mt);
		pOut->SetMediaType(&mt);
	}

	pOut->SetTime(&rtStart, &rtStop);
	pOut->SetMediaTime(NULL, NULL);

	pOut->SetPreroll(FALSE);
	pOut->SetDiscontinuity(m_fDiscontinuity); m_fDiscontinuity = false;
	pOut->SetSyncPoint(TRUE);

	pOut->SetActualDataLength(length);

	WORD* pDataOutW = (WORD*)pDataOut;
	pDataOutW[0] = 0xf872;
	pDataOutW[1] = 0x4e1f;
	pDataOutW[2] = type;
	pDataOutW[3] = size*8;
	_swab((char*)pBuff, (char*)&pDataOutW[4], size);

	return m_pOutput->Deliver(pOut);
}

HRESULT CMpaDecFilter::ReconnectOutput(int nSamples, CMediaType& mt)
{
	HRESULT hr;

	CComQIPtr<IMemInputPin> pPin = m_pOutput->GetConnected();
	if(!pPin) return E_NOINTERFACE;

	CComPtr<IMemAllocator> pAllocator;
	if(FAILED(hr = pPin->GetAllocator(&pAllocator)) || !pAllocator) 
		return hr;

	ALLOCATOR_PROPERTIES props, actual;
	if(FAILED(hr = pAllocator->GetProperties(&props)))
		return hr;

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.Format();
	long cbBuffer = nSamples * wfe->nBlockAlign;

	if(mt != m_pOutput->CurrentMediaType() || cbBuffer > props.cbBuffer)
	{
		if(cbBuffer > props.cbBuffer)
		{
			props.cBuffers = 4;
			props.cbBuffer = cbBuffer*3/2;

			if(FAILED(hr = m_pOutput->DeliverBeginFlush())
			|| FAILED(hr = m_pOutput->DeliverEndFlush())
			|| FAILED(hr = pAllocator->Decommit())
			|| FAILED(hr = pAllocator->SetProperties(&props, &actual))
			|| FAILED(hr = pAllocator->Commit()))
				return hr;

			if(props.cBuffers > actual.cBuffers || props.cbBuffer > actual.cbBuffer)
			{
				NotifyEvent(EC_ERRORABORT, hr, 0);
				return E_FAIL;
			}
		}

		return S_OK;
	}

	return S_FALSE;
}

CMediaType CMpaDecFilter::CreateMediaType(SampleFormat sf, DWORD nSamplesPerSec, WORD nChannels, DWORD dwChannelMask)
{
	CMediaType mt;

	mt.majortype = MEDIATYPE_Audio;
	mt.subtype = sf == SF_FLOAT32 ? MEDIASUBTYPE_IEEE_FLOAT : MEDIASUBTYPE_PCM;
	mt.formattype = FORMAT_WaveFormatEx;

	WAVEFORMATEXTENSIBLE wfex;
	memset(&wfex, 0, sizeof(wfex));
	WAVEFORMATEX* wfe = &wfex.Format;
	wfe->wFormatTag = (WORD)mt.subtype.Data1;
	wfe->nChannels = nChannels;
	wfe->nSamplesPerSec = nSamplesPerSec;
	switch(sf)
	{
	default:
	case SF_PCM16: wfe->wBitsPerSample = 16; break;
	case SF_PCM24: wfe->wBitsPerSample = 24; break;
	case SF_PCM32: case SF_FLOAT32: wfe->wBitsPerSample = 32; break;
	}
	wfe->nBlockAlign = wfe->nChannels*wfe->wBitsPerSample/8;
	wfe->nAvgBytesPerSec = wfe->nSamplesPerSec*wfe->nBlockAlign;

	// FIXME: 24/32 bit only seems to work with WAVE_FORMAT_EXTENSIBLE
	if(dwChannelMask == 0 && (sf == SF_PCM24 || sf == SF_PCM32))
		dwChannelMask = nChannels == 2 ? (SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT) : SPEAKER_FRONT_CENTER;

	if(dwChannelMask)
	{
		wfex.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		wfex.Format.cbSize = sizeof(wfex) - sizeof(wfex.Format);
		wfex.dwChannelMask = dwChannelMask;
		wfex.Samples.wValidBitsPerSample = wfex.Format.wBitsPerSample;
		wfex.SubFormat = mt.subtype;
	}

	mt.SetFormat((BYTE*)&wfex, sizeof(wfex.Format) + wfex.Format.cbSize);

	return mt;
}

CMediaType CMpaDecFilter::CreateMediaTypeSPDIF()
{
	CMediaType mt = CreateMediaType(SF_PCM16, 48000, 2);
	((WAVEFORMATEX*)mt.pbFormat)->wFormatTag = WAVE_FORMAT_DOLBY_AC3_SPDIF;
	return mt;
}

HRESULT CMpaDecFilter::CheckInputType(const CMediaType* mtIn)
{
	// TODO: remove this limitation
	if(mtIn->subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO)
	{
		WAVEFORMATEX* wfe = (WAVEFORMATEX*)mtIn->Format();
		if(wfe->nChannels != 2 || wfe->wBitsPerSample != 16)
			return VFW_E_TYPE_NOT_ACCEPTED;
	}

	for(int i = 0; i < countof(sudPinTypesIn); i++)
	{
		if(*sudPinTypesIn[i].clsMajorType == mtIn->majortype
		&& *sudPinTypesIn[i].clsMinorType == mtIn->subtype)
			return S_OK;
	}

	return VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CMpaDecFilter::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	return SUCCEEDED(CheckInputType(mtIn))
		&& mtOut->majortype == MEDIATYPE_Audio && mtOut->subtype == MEDIASUBTYPE_PCM
		|| mtOut->majortype == MEDIATYPE_Audio && mtOut->subtype == MEDIASUBTYPE_IEEE_FLOAT
		? S_OK
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CMpaDecFilter::DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties)
{
	if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	CMediaType& mt = m_pInput->CurrentMediaType();
	WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.Format();

	pProperties->cBuffers = 4;
	// pProperties->cbBuffer = 1;
	pProperties->cbBuffer = 48000*6*(32/8)/10; // 48KHz 6ch 32bps 100ms
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

HRESULT CMpaDecFilter::GetMediaType(int iPosition, CMediaType* pmt)
{
    if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	if(iPosition < 0) return E_INVALIDARG;
	if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;
	
	CMediaType mt = m_pInput->CurrentMediaType();
	const GUID& subtype = mt.subtype;
	WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.Format();

	if(GetSpeakerConfig(ac3) < 0 && (subtype == MEDIASUBTYPE_DOLBY_AC3 || subtype == MEDIASUBTYPE_WAVE_DOLBY_AC3)
	|| GetSpeakerConfig(dts) < 0 && (subtype == MEDIASUBTYPE_DTS || subtype == MEDIASUBTYPE_WAVE_DTS))
	{
		*pmt = CreateMediaTypeSPDIF();
	}
	else
	{
		*pmt = CreateMediaType(GetSampleFormat(), wfe->nSamplesPerSec, min(2, wfe->nChannels));
	}

	return S_OK;
}

HRESULT CMpaDecFilter::StartStreaming()
{
	HRESULT hr = __super::StartStreaming();
	if(FAILED(hr)) return hr;

	m_a52_state = a52_init(0);

	m_dts_state = dts_init(0);

	m_aac_state.init(m_pInput->CurrentMediaType());

	mad_stream_init(&m_stream);
	mad_frame_init(&m_frame);
	mad_synth_init(&m_synth);
	mad_stream_options(&m_stream, 0/*options*/);

	m_fDiscontinuity = false;

	m_sample_max = 0.1f;

	return S_OK;
}

HRESULT CMpaDecFilter::StopStreaming()
{
	a52_free(m_a52_state);

	dts_free(m_dts_state);

	mad_synth_finish(&m_synth);
	mad_frame_finish(&m_frame);
	mad_stream_finish(&m_stream);

	return __super::StopStreaming();
}

// IMpaDecFilter

STDMETHODIMP CMpaDecFilter::SetSampleFormat(SampleFormat sf)
{
	CAutoLock cAutoLock(&m_csProps);
	m_iSampleFormat = sf;
	return S_OK;
}

STDMETHODIMP_(SampleFormat) CMpaDecFilter::GetSampleFormat()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_iSampleFormat;
}

STDMETHODIMP CMpaDecFilter::SetNormalize(bool fNormalize)
{
	CAutoLock cAutoLock(&m_csProps);
	if(m_fNormalize != fNormalize) m_sample_max = 0.1f;
	m_fNormalize = fNormalize;
	return S_OK;
}

STDMETHODIMP_(bool) CMpaDecFilter::GetNormalize()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_fNormalize;
}

STDMETHODIMP CMpaDecFilter::SetSpeakerConfig(enctype et, int sc)
{
	CAutoLock cAutoLock(&m_csProps);
	if(et >= 0 && et < etlast) m_iSpeakerConfig[et] = sc;
	return S_OK;
}

STDMETHODIMP_(int) CMpaDecFilter::GetSpeakerConfig(enctype et)
{
	CAutoLock cAutoLock(&m_csProps);
	if(et >= 0 && et < etlast) return m_iSpeakerConfig[et];
	return -1;
}

STDMETHODIMP CMpaDecFilter::SetDynamicRangeControl(enctype et, bool fDRC)
{
	CAutoLock cAutoLock(&m_csProps);
	if(et >= 0 && et < etlast) m_fDynamicRangeControl[et] = fDRC;
	else return E_INVALIDARG;
	return S_OK;
}

STDMETHODIMP_(bool) CMpaDecFilter::GetDynamicRangeControl(enctype et)
{
	CAutoLock cAutoLock(&m_csProps);
	if(et >= 0 && et < etlast) return m_fDynamicRangeControl[et];
	return false;
}

STDMETHODIMP CMpaDecFilter::SetBoost(float boost)
{
	CAutoLock cAutoLock(&m_csProps);
	m_boost = max(boost, 1);
	return S_OK;
}

STDMETHODIMP_(float) CMpaDecFilter::GetBoost()
{
	CAutoLock cAutoLock(&m_csProps);
	return m_boost;
}

//
// CMpaDecInputPin
//

CMpaDecInputPin::CMpaDecInputPin(CTransformFilter* pFilter, HRESULT* phr, LPWSTR pName)
	: CDeCSSInputPin(NAME("CMpaDecInputPin"), pFilter, phr, pName)
{
}

//
// aac_state_t
//

aac_state_t::aac_state_t() : h(NULL), freq(0), channels(0) {open();}
aac_state_t::~aac_state_t() {close();}

bool aac_state_t::open()
{
	close();
	if(!(h = NeAACDecOpen())) return false;
	NeAACDecConfigurationPtr c = NeAACDecGetCurrentConfiguration(h);
	c->outputFormat = FAAD_FMT_FLOAT;
	NeAACDecSetConfiguration(h, c);
	return true;
}

void aac_state_t::close()
{
	if(h) NeAACDecClose(h);
	h = NULL;
}

bool aac_state_t::init(CMediaType& mt)
{
	if(mt.subtype != MEDIASUBTYPE_AAC && mt.subtype != MEDIASUBTYPE_MP4A) return(true); // nothing to do
	open();
	WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.Format();
	return !NeAACDecInit2(h, (BYTE*)(wfe+1), wfe->cbSize, &freq, &channels);
}