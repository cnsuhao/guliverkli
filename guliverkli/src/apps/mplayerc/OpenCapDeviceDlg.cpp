// OpenCapDeviceDlg.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include "OpenCapDeviceDlg.h"
#include "..\..\DSUtil\DSUtil.h"

// COpenCapDeviceDlg dialog

IMPLEMENT_DYNAMIC(COpenCapDeviceDlg, CDialog)
COpenCapDeviceDlg::COpenCapDeviceDlg(CWnd* pParent /*=NULL*/)
	: CDialog(COpenCapDeviceDlg::IDD, pParent)
	, m_vidstr(_T(""))
	, m_audstr(_T(""))
{
}

COpenCapDeviceDlg::~COpenCapDeviceDlg()
{
}

void COpenCapDeviceDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_vidctrl);
	DDX_Control(pDX, IDC_COMBO2, m_audctrl);
}


BEGIN_MESSAGE_MAP(COpenCapDeviceDlg, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


// COpenCapDeviceDlg message handlers

BOOL COpenCapDeviceDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	BeginEnumSysDev(CLSID_VideoInputDeviceCategory, pMoniker)
	{
//		m_vidmonikers.Add(pMoniker);

		CComPtr<IPropertyBag> pPB;
		pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPB);

		CComVariant var;
		pPB->Read(CComBSTR(_T("FriendlyName")), &var, NULL);
		m_vidctrl.AddString(CString(var.bstrVal));

		LPOLESTR strName = NULL;
		if(SUCCEEDED(pMoniker->GetDisplayName(NULL, NULL, &strName)))
		{
			m_vidnames.Add(CString(strName));
			CoTaskMemFree(strName);
		}
	}
	EndEnumSysDev

	if(m_vidctrl.GetCount())
		m_vidctrl.SetCurSel(0);

	BeginEnumSysDev(CLSID_AudioInputDeviceCategory, pMoniker)
	{
//		m_audmonikers.Add(pMoniker);

		CComPtr<IPropertyBag> pPB;
		pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPB);

		CComVariant var;
		pPB->Read(CComBSTR(_T("FriendlyName")), &var, NULL);
		m_audctrl.AddString(CString(var.bstrVal));

		LPOLESTR strName = NULL;
		if(SUCCEEDED(pMoniker->GetDisplayName(NULL, NULL, &strName)))
		{
			m_audnames.Add(CString(strName));
			CoTaskMemFree(strName);
		}
	}
	EndEnumSysDev

	if(m_audctrl.GetCount())
		m_audctrl.SetCurSel(0);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void COpenCapDeviceDlg::OnBnClickedOk()
{
	UpdateData();

	if(m_vidctrl.GetCurSel() >= 0) 
	{
//		m_vidctrl.GetLBText(m_vidctrl.GetCurSel(), m_vidfrstr);
		m_vidstr = m_vidnames[m_vidctrl.GetCurSel()];
/*
		m_pVidCap = NULL;
		m_vidmonikers[m_vidctrl.GetCurSel()]->BindToObject(0, 0, IID_IBaseFilter, (void**)&m_pVidCap);
*/
	}

	if(m_audctrl.GetCurSel() >= 0)
	{
//		m_audctrl.GetLBText(m_audctrl.GetCurSel(), m_audfrstr);
		m_audstr = m_audnames[m_audctrl.GetCurSel()];
/*
		m_pAudCap = NULL;
		m_audmonikers[m_audctrl.GetCurSel()]->BindToObject(0, 0, IID_IBaseFilter, (void**)&m_pAudCap);
*/
	}

	OnOK();
}
