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

#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "..\..\filters\transform\bufferfilter\bufferfilter.h"
#include "FloatEdit.h"

//

template<class T>
class CFormatElem
{
public:
	AM_MEDIA_TYPE* pmt;
	T caps;
	CFormatElem() {pmt = NULL;}
	virtual ~CFormatElem() {if(pmt) DeleteMediaType(pmt);}
};

template<class T>
class CFormat : public CAutoPtrArray<CFormatElem<T> >
{
public:
	CString name;
	CFormat(CString name = _T("")) {this->name = name;}
	virtual ~CFormat() {}
};

template<class T>
class CFormatArray : public CAutoPtrArray<CFormat<T> >
{
public:
	virtual ~CFormatArray() {}

	CFormat<T>* Find(CString name, bool fCreate = false)
	{
		for(int i = 0; i < (int)GetCount(); i++)
		{
			if(GetAt(i)->name == name)
				return(GetAt(i));
		}

		if(fCreate)
		{
			CAutoPtr<CFormat<T> > pf(new CFormat<T>(name));
			CFormat<T>* tmp = pf;
			Add(pf);
			return(tmp);
		}

		return(NULL);
	}

	bool FindFormat(AM_MEDIA_TYPE* pmt, CFormat<T>** ppf)
	{
		if(!pmt) return(false);

		for(int i = 0; i < (int)GetCount(); i++)
		{
			CFormat<T>* pf = GetAt(i);
			for(int j = 0; j < (int)pf->GetCount(); j++)
			{
				CFormatElem<T>* pfe = pf->GetAt(j);
				if(!pmt || (pfe->pmt->majortype == pmt->majortype && pfe->pmt->subtype == pmt->subtype))
				{
                    if(ppf) *ppf = pf;
					return(true);
				}
			}
		}

		return(false);
	}

	bool FindFormat(AM_MEDIA_TYPE* pmt, T* pcaps, CFormat<T>** ppf, CFormatElem<T>** ppfe)
	{
		if(!pmt && !pcaps) return(false);

		for(int i = 0; i < (int)GetCount(); i++)
		{
			CFormat<T>* pf = GetAt(i);
			for(int j = 0; j < (int)pf->GetCount(); j++)
			{
				CFormatElem<T>* pfe = pf->GetAt(j);
				if((!pmt || CMediaType(*pfe->pmt) == *pmt)
				&& (!pcaps || !memcmp(pcaps, &pfe->caps, sizeof(T))))
				{
                    if(ppf) *ppf = pf;
                    if(ppfe) *ppfe = pfe;
					return(true);
				}
			}
		}

		return(false);
	}

	bool AddFormat(AM_MEDIA_TYPE* pmt, T caps)
	{
		if(!pmt) return(false);

		if(FindFormat(pmt, NULL, NULL, NULL)) {DeleteMediaType(pmt); return(false);}
//		if(pmt->formattype == FORMAT_VideoInfo2) {DeleteMediaType(pmt); return(false);} // TODO

		CFormat<T>* pf = Find(MakeFormatName(pmt), true);
		if(!pf) {DeleteMediaType(pmt); return(false);}

		CAutoPtr<CFormatElem<T> > pfe(new CFormatElem<T>());
		pfe->pmt = pmt;
		pfe->caps = caps;
		pf->Add(pfe);

		return(true);
	}

	virtual CString MakeFormatName(AM_MEDIA_TYPE* pmt) = 0;
	virtual CString MakeDimensionName(CFormatElem<T>* pfe) = 0;
};

typedef CFormatElem<VIDEO_STREAM_CONFIG_CAPS> CVidFormatElem;
typedef CFormat<VIDEO_STREAM_CONFIG_CAPS> CVidFormat;

class CVidFormatArray : public CFormatArray<VIDEO_STREAM_CONFIG_CAPS>
{
public:
	CString MakeFormatName(AM_MEDIA_TYPE* pmt)
	{
		CString str(_T("Default"));

		if(!pmt) return(str);

		BITMAPINFOHEADER* bih = (pmt->formattype == FORMAT_VideoInfo)
			? &((VIDEOINFOHEADER*)pmt->pbFormat)->bmiHeader
			: (pmt->formattype == FORMAT_VideoInfo2)
			? &((VIDEOINFOHEADER2*)pmt->pbFormat)->bmiHeader
			: NULL;

		if(!bih) 
		{
			// it may have a fourcc in the mediasubtype, let's check that

			WCHAR guid[100];
			memset(guid, 0, 100*sizeof(WCHAR));
			StringFromGUID2(pmt->subtype, guid, 100);

			if(CStringW(guid).MakeUpper().Find(L"0000-0010-8000-00AA00389B71") >= 0)
			{
				str.Format(_T("%c%c%c%c"), 
					(TCHAR)((pmt->subtype.Data1>>0)&0xff), (TCHAR)((pmt->subtype.Data1>>8)&0xff), 
					(TCHAR)((pmt->subtype.Data1>>16)&0xff), (TCHAR)((pmt->subtype.Data1>>24)&0xff));
			}

			return(str);
		}

		switch(bih->biCompression)
		{
		case BI_RGB: str.Format(_T("RGB%d"), bih->biBitCount); break;
		case BI_RLE8: str = _T("RLE8"); break;
		case BI_RLE4: str = _T("RLE4"); break;
		case BI_BITFIELDS: str.Format(_T("BITF%d"), bih->biBitCount); break;
		case BI_JPEG: str = _T("JPEG"); break;
		case BI_PNG: str = _T("PNG"); break;
		default:  
			str.Format(_T("%c%c%c%c"), 
				(TCHAR)((bih->biCompression>>0)&0xff), (TCHAR)((bih->biCompression>>8)&0xff), 
				(TCHAR)((bih->biCompression>>16)&0xff), (TCHAR)((bih->biCompression>>24)&0xff));
			break;
		}

		return(str);
	}

	CString MakeDimensionName(CVidFormatElem* pfe)
	{
		CString str(_T("Default"));

		if(!pfe) return(str);

		BITMAPINFOHEADER* bih = (pfe->pmt->formattype == FORMAT_VideoInfo)
			? &((VIDEOINFOHEADER*)pfe->pmt->pbFormat)->bmiHeader
			: (pfe->pmt->formattype == FORMAT_VideoInfo2)
			? &((VIDEOINFOHEADER2*)pfe->pmt->pbFormat)->bmiHeader
			: NULL;

		if(bih == NULL) return(str);

		str.Format(_T("%dx%d %.2f"), bih->biWidth, bih->biHeight, (float)10000000/((VIDEOINFOHEADER*)pfe->pmt->pbFormat)->AvgTimePerFrame);

		if(pfe->pmt->formattype == FORMAT_VideoInfo2)
		{
			VIDEOINFOHEADER2* vih2 = (VIDEOINFOHEADER2*)pfe->pmt->pbFormat;
			CString str2;
			str2.Format(_T(" i%02x %d:%d"), vih2->dwInterlaceFlags, vih2->dwPictAspectRatioX, vih2->dwPictAspectRatioY);
			str += str2;
		}

		return(str);
	}
};

typedef CFormatElem<AUDIO_STREAM_CONFIG_CAPS> CAudFormatElem;
typedef CFormat<AUDIO_STREAM_CONFIG_CAPS> CAudFormat;

class CAudFormatArray : public CFormatArray<AUDIO_STREAM_CONFIG_CAPS>
{
public:
	CString MakeFormatName(AM_MEDIA_TYPE* pmt)
	{
		CString str(_T("Unknown"));

		if(!pmt) return(str);

		WAVEFORMATEX* wfe = (pmt->formattype == FORMAT_WaveFormatEx)
			? (WAVEFORMATEX*)pmt->pbFormat
			: NULL;

		if(!wfe)
		{
			WCHAR guid[100];
			memset(guid, 0, 100*sizeof(WCHAR));
			StringFromGUID2(pmt->subtype, guid, 100);

			if(CStringW(guid).MakeUpper().Find(L"0000-0010-8000-00AA00389B71") >= 0)
			{
				str.Format(_T("0x%04x"), pmt->subtype.Data1);
			}

			return(str);
		}

		switch(wfe->wFormatTag)
		{
		case 1: str = _T("PCM "); break;
		default: str.Format(_T("0x%03x "), wfe->wFormatTag); break;
		}

		return(str);
	}

	CString MakeDimensionName(CAudFormatElem* pfe)
	{
		CString str(_T("Unknown"));

		if(!pfe) return(str);

		WAVEFORMATEX* wfe = (pfe->pmt->formattype == FORMAT_WaveFormatEx)
			? (WAVEFORMATEX*)pfe->pmt->pbFormat
			: NULL;

		if(!wfe) return(str);

		str.Empty();
		CString str2;

		str2.Format(_T("%6dKHz "), wfe->nSamplesPerSec); str += str2;

		str2.Format(_T("%dbps "), wfe->wBitsPerSample); str += str2;

		switch(wfe->nChannels)
		{
		case 1: str += _T("mono "); break;
		case 2: str += _T("stereo "); break;
		default: str2.Format(_T("%d channels "), wfe->nChannels); str += str2; break;
		}

		str2.Format(_T("%3dkbps "), wfe->nAvgBytesPerSec*8/1000); str += str2;

		return(str);
	}
};

//

typedef struct
{
	CComPtr<IMoniker> pMoniker;
	CComPtr<IBaseFilter> pBF;
	CString FriendlyName;
	CComBSTR DisplayName;
} Codec;

typedef CArray<Codec> CCodecArray;

// CPlayerCaptureDialog dialog

class CPlayerCaptureDialog : public CDialog
{
	DECLARE_DYNAMIC(CPlayerCaptureDialog)

	// video input
	CStringW m_VidDisplayName;
	CComPtr<IAMStreamConfig> m_pAMVSC;
	CComPtr<IAMCrossbar> m_pAMXB;
	CComPtr<IAMTVTuner> m_pAMTuner;
	CComPtr<IAMVfwCaptureDialogs> m_pAMVfwCD;
	CVidFormatArray m_vfa;

	// audio input
	CStringW m_AudDisplayName;
	CComPtr<IAMStreamConfig> m_pAMASC;
	CInterfaceArray<IAMAudioInputMixer> m_pAMAIM;
	CAudFormatArray m_afa;

	// video codec
	CCodecArray m_pVidEncArray;
	CVidFormatArray m_vcfa;

	// audio codec
	CCodecArray m_pAudEncArray;
	CAudFormatArray m_acfa;

	void EmptyVideo();
	void EmptyAudio();

	void UpdateMediaTypes();
	void UpdateUserDefinableControls();
	void UpdateVideoCodec();
	void UpdateAudioCodec();
	void UpdateMuxer();
	void UpdateOutputControls();

	void UpdateGraph();

	CMap<HWND, HWND&, BOOL, BOOL&> m_wndenabledmap;
	void EnableControls(CWnd* pWnd, bool fEnable);

public:
	CPlayerCaptureDialog();   // standard constructor
	virtual ~CPlayerCaptureDialog();

	BOOL Create(CWnd* pParent = NULL);

// Dialog Data
	enum { IDD = IDD_CAPTUREDIALOG };

	CComboBox m_vidinput;
	CComboBox m_vidtype;
	CComboBox m_viddimension;
	CSpinButtonCtrl m_vidhor;
	CSpinButtonCtrl m_vidver;
	CFloatEdit m_vidfpsedit;
	float m_vidfps;
	CButton m_vidsetres;
	CComboBox m_audinput;
	CComboBox m_audtype;
	CComboBox m_auddimension;
	CComboBox m_vidcodec;
	CComboBox m_vidcodectype;
	CComboBox m_vidcodecdimension;
	BOOL m_fVidOutput;
	CButton m_vidoutput;
	BOOL m_fVidPreview;
	CButton m_vidpreview;
	CComboBox m_audcodec;
	CComboBox m_audcodectype;
	CComboBox m_audcodecdimension;
	BOOL m_fAudOutput;
	CButton m_audoutput;
	BOOL m_fAudPreview;
	CButton m_audpreview;
	int m_nVidBuffers;
	int m_nAudBuffers;
	CString m_file;
	int m_muxtype;
	CButton m_recordbtn;
	BOOL m_fSepAudio;

	CMediaType m_mtv, m_mta, m_mtcv, m_mtca;
	CComPtr<IBaseFilter> m_pVidEnc, m_pAudEnc, m_pMux, m_pDst, m_pAudMux, m_pAudDst;
	CComPtr<IMoniker> m_pVidEncMoniker, m_pAudEncMoniker;
	CComPtr<IBaseFilter> m_pVidBuffer, m_pAudBuffer;

public:
	void SetupVideoControls(CStringW DisplayName, CComPtr<IAMStreamConfig> pAMSC, CComPtr<IAMCrossbar> pAMXB, CComPtr<IAMTVTuner> pAMTuner);
	void SetupVideoControls(CStringW DisplayName, CComPtr<IAMStreamConfig> pAMSC, CComPtr<IAMVfwCaptureDialogs> pAMVfwCD);
	void SetupAudioControls(CStringW DisplayName, CComPtr<IAMStreamConfig> pAMSC, CInterfaceArray<IAMAudioInputMixer>& pAMAIM);

	bool IsTunerActive();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnInitDialog();
	virtual void OnOK() {}
	virtual void OnCancel() {}

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnDestroy();
	afx_msg void OnVideoInput();
	afx_msg void OnVideoType();
	afx_msg void OnVideoDimension();
	afx_msg void OnOverrideVideoDimension();
	afx_msg void OnAudioInput();
	afx_msg void OnAudioType();
	afx_msg void OnAudioDimension();
	afx_msg void OnRecordVideo();
	afx_msg void OnVideoCodec();
	afx_msg void OnVideoCodecType();
	afx_msg void OnVideoCodecDimension();
	afx_msg void OnRecordAudio();
	afx_msg void OnAudioCodec();
	afx_msg void OnAudioCodecType();
	afx_msg void OnAudioCodecDimension();
	afx_msg void OnOpenFile();
	afx_msg void OnMuxerType();
	afx_msg void OnRecord();
	afx_msg void OnEnChangeEdit9();
	afx_msg void OnEnChangeEdit12();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnBnClickedVidAudPreview();
	afx_msg void OnBnClickedCheck7();
};
