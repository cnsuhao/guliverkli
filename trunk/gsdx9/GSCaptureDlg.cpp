// GSCaptureDlg.cpp : implementation file
//

#include "stdafx.h"
#include <afxpriv.h>
#include "GSCaptureDlg.h"
#include ".\gscapturedlg.h"

// GSCaptureDlg dialog

IMPLEMENT_DYNAMIC(GSCaptureDlg, CDialog)
GSCaptureDlg::GSCaptureDlg(CWnd* pParent /*=NULL*/)
	: CDialog(GSCaptureDlg::IDD, pParent)
	, m_filename(_T(""))
{
}

GSCaptureDlg::~GSCaptureDlg()
{
}

int GSCaptureDlg::GetSelCodec(Codec& c)
{
	int iSel = m_codeclist.GetCurSel();
	if(iSel < 0) return 0;

	POSITION pos = (POSITION)m_codeclist.GetItemDataPtr(iSel);
	if(pos == NULL) return 2;

	c = m_codecs.GetAt(pos);

	if(!c.pBF)
	{
		c.pMoniker->BindToObject(NULL, NULL, __uuidof(IBaseFilter), (void**)&c.pBF);
		if(!c.pBF) return 0;
	}

	return 1;
}

LRESULT GSCaptureDlg::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT ret = __super::DefWindowProc(message, wParam, lParam);
	if(message == WM_INITDIALOG) SendMessage(WM_KICKIDLE);
	return(ret);
}

void GSCaptureDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_filename);
	DDX_Control(pDX, IDC_COMBO1, m_codeclist);
}

BOOL GSCaptureDlg::OnInitDialog()
{
	__super::OnInitDialog();

	m_codecs.RemoveAll();

	m_codeclist.ResetContent();
	m_codeclist.EnableWindow(FALSE);
	m_codeclist.SetItemDataPtr(m_codeclist.AddString(_T("Uncompressed")), NULL);

	BeginEnumSysDev(CLSID_VideoCompressorCategory, pMoniker)
	{
		Codec c;
		c.pMoniker = pMoniker;

		LPOLESTR strName = NULL;
		if(FAILED(pMoniker->GetDisplayName(NULL, NULL, &strName)))
			continue;

		c.DisplayName = strName;
		CoTaskMemFree(strName);

		CComPtr<IPropertyBag> pPB;
		pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPB);

		CComVariant var;
		if(FAILED(pPB->Read(CComBSTR(_T("FriendlyName")), &var, NULL)))
			continue;
		
		c.FriendlyName = var.bstrVal;

		CStringW str = CStringW(c.DisplayName).MakeLower();
		if(str.Find(L"@device:dmo:") == 0)
			c.FriendlyName = _T("(DMO) ") + c.FriendlyName;
		else if(str.Find(L"@device:sw:") == 0)
			c.FriendlyName = _T("(DS) ") + c.FriendlyName;
		else if(str.Find(L"@device:cm:") == 0)
			c.FriendlyName = _T("(VfW) ") + c.FriendlyName;

		m_codeclist.SetItemDataPtr(
			m_codeclist.AddString(c.FriendlyName),
			m_codecs.AddTail(c));
	}
	EndEnumSysDev

	m_codeclist.EnableWindow(m_codeclist.GetCount() > 1);

	// LoadDefaultCodec(m_codecs, m_codeclist, CLSID_VideoCompressorCategory);

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BEGIN_MESSAGE_MAP(GSCaptureDlg, CDialog)
	ON_MESSAGE_VOID(WM_KICKIDLE, OnKickIdle)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, OnBnClickedButton2)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON2, OnUpdateButton2)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_UPDATE_COMMAND_UI(IDOK, OnUpdateOK)
END_MESSAGE_MAP()

// GSCaptureDlg message handlers

void GSCaptureDlg::OnKickIdle()
{
	UpdateDialogControls(this, false);
}

void GSCaptureDlg::OnBnClickedButton1()
{
	UpdateData();

	CFileDialog fd(FALSE, _T("avi"), m_filename, 
		OFN_EXPLORER|OFN_ENABLESIZING|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST, 
		_T("Avi files (*.avi)|*.avi||"), this, 0);

	if(fd.DoModal() == IDOK)
	{
		m_filename = fd.GetPathName();
		UpdateData(FALSE);
	}
}

void GSCaptureDlg::OnBnClickedButton2()
{
	Codec c;
	if(GetSelCodec(c) != 1) return;

	if(CComQIPtr<ISpecifyPropertyPages> pSPP = c.pBF)
	{
		CAUUID caGUID;
		caGUID.pElems = NULL;
		if(SUCCEEDED(pSPP->GetPages(&caGUID)))
		{
			IUnknown* lpUnk = NULL;
			pSPP.QueryInterface(&lpUnk);
			OleCreatePropertyFrame(
				m_hWnd, 0, 0, CStringW(c.FriendlyName), 
				1, (IUnknown**)&lpUnk, 
				caGUID.cElems, caGUID.pElems, 
				0, 0, NULL);
			lpUnk->Release();

			if(caGUID.pElems) CoTaskMemFree(caGUID.pElems);
		}
	}
	else if(CComQIPtr<IAMVfwCompressDialogs> pAMVfWCD = c.pBF)
	{
		if(pAMVfWCD->ShowDialog(VfwCompressDialog_QueryConfig, NULL) == S_OK)
			pAMVfWCD->ShowDialog(VfwCompressDialog_Config, m_hWnd);		
	}
}

void GSCaptureDlg::OnUpdateButton2(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_codeclist.GetCurSel() >= 0 && m_codeclist.GetItemDataPtr(m_codeclist.GetCurSel()) != NULL);
}

void GSCaptureDlg::OnBnClickedOk()
{
	UpdateData();

	Codec c;
	if(GetSelCodec(c) == 0) return;

	m_pVidEnc = c.pBF;

	OnOK();
}

void GSCaptureDlg::OnUpdateOK(CCmdUI* pCmdUI)
{
	CString str;
	GetDlgItem(IDC_EDIT1)->GetWindowText(str);

	pCmdUI->Enable(!str.IsEmpty() && m_codeclist.GetCurSel() >= 0);
}