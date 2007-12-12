/* 
 *	Copyright (C) 2007 Gabest
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
#include "GSdx9.h"
#include "GSSettingsDlg.h"
#include <shlobj.h>

GSSetting g_renderers[] =
{
	{0, _T("Direct3D9"), NULL},
	{1, _T("Software"), NULL},
	// {2, _T("Software (fixed)"), NULL},
	{3, _T("Do not render"), NULL},
};

GSSetting g_psversion[] =
{
	{D3DPS_VERSION(3, 0), _T("Pixel Shader 3.0"), NULL},
	{D3DPS_VERSION(2, 0), _T("Pixel Shader 2.0"), NULL},
	//{D3DPS_VERSION(1, 4), _T("Pixel Shader 1.4"), NULL},
	//{D3DPS_VERSION(1, 1), _T("Pixel Shader 1.1"), NULL},
	//{D3DPS_VERSION(0, 0), _T("Fixed Pipeline (bogus)"), NULL},
};

GSSetting g_interlace[] =
{
	{0, _T("None"), NULL},
	{1, _T("Weave tff"), _T("saw-tooth")},
	{2, _T("Weave bff"), _T("saw-tooth")},
	{3, _T("Bob tff"), _T("use blend if shaking")},
	{4, _T("Bob bff"), _T("use blend if shaking")},
	{5, _T("Blend tff"), _T("slight blur, 1/2 fps")},
	{6, _T("Blend bff"), _T("slight blur, 1/2 fps")},
};

GSSetting g_aspectratio[] =
{
	{0, _T("Stretch"), NULL},
	{1, _T("4:3"), NULL},
	{2, _T("16:9"), NULL},
};

IMPLEMENT_DYNAMIC(GSSettingsDlg, CDialog)
GSSettingsDlg::GSSettingsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(GSSettingsDlg::IDD, pParent)
	, m_tvout(FALSE)
	, m_filter(1)
	, m_nloophack(2)
	, m_nativeres(FALSE)
	, m_vsync(FALSE)
	, m_logz(FALSE)
	, m_fba(TRUE)
{
}

GSSettingsDlg::~GSSettingsDlg()
{
}

void GSSettingsDlg::InitComboBox(CComboBox& combobox, const GSSetting* settings, int count, DWORD sel, DWORD maxid)
{
	for(int i = 0; i < count; i++)
	{
		if(settings[i].id <= maxid)
		{
			CString str = settings[i].name;
			if(settings[i].note != NULL) str = str + _T(" (") + settings[i].note + _T(")");
			int item = combobox.AddString(str);
			combobox.SetItemData(item, settings[i].id);
			if(settings[i].id == sel) combobox.SetCurSel(item);
		}
	}
}

void GSSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO3, m_resolution);
	DDX_Control(pDX, IDC_COMBO1, m_renderer);
	DDX_Control(pDX, IDC_COMBO4, m_psversion);
	DDX_Control(pDX, IDC_COMBO2, m_interlace);
	DDX_Control(pDX, IDC_COMBO5, m_aspectratio);
	DDX_Check(pDX, IDC_CHECK3, m_tvout);
	DDX_Check(pDX, IDC_CHECK4, m_filter);
	DDX_Check(pDX, IDC_CHECK6, m_nloophack);	
	DDX_Control(pDX, IDC_SPIN1, m_resx);
	DDX_Control(pDX, IDC_SPIN2, m_resy);
	DDX_Check(pDX, IDC_CHECK1, m_nativeres);
	DDX_Control(pDX, IDC_EDIT1, m_resxedit);
	DDX_Control(pDX, IDC_EDIT2, m_resyedit);
	DDX_Check(pDX, IDC_CHECK2, m_vsync);
	DDX_Check(pDX, IDC_CHECK5, m_logz);
	DDX_Check(pDX, IDC_CHECK7, m_fba);
}

BEGIN_MESSAGE_MAP(GSSettingsDlg, CDialog)
	ON_BN_CLICKED(IDC_CHECK1, &GSSettingsDlg::OnBnClickedCheck1)
END_MESSAGE_MAP()

BOOL GSSettingsDlg::OnInitDialog()
{
	__super::OnInitDialog();

    CWinApp* pApp = AfxGetApp();

	D3DCAPS9 caps;
	memset(&caps, 0, sizeof(caps));
	caps.PixelShaderVersion = D3DPS_VERSION(0, 0);

	m_modes.RemoveAll();

	// windowed

	{
		D3DDISPLAYMODE mode;
		memset(&mode, 0, sizeof(mode));
		m_modes.AddTail(mode);

		int iItem = m_resolution.AddString(_T("Windowed"));
		m_resolution.SetItemDataPtr(iItem, m_modes.GetTailPosition());
		m_resolution.SetCurSel(iItem);
	}

	// fullscreen

	if(CComPtr<IDirect3D9> pD3D = Direct3DCreate9(D3D_SDK_VERSION))
	{
		int ModeWidth = pApp->GetProfileInt(_T("Settings"), _T("ModeWidth"), 0);
		int ModeHeight = pApp->GetProfileInt(_T("Settings"), _T("ModeHeight"), 0);
		int ModeRefreshRate = pApp->GetProfileInt(_T("Settings"), _T("ModeRefreshRate"), 0);

		UINT nModes = pD3D->GetAdapterModeCount(D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8);

		for(UINT i = 0; i < nModes; i++)
		{
			D3DDISPLAYMODE mode;

			if(S_OK == pD3D->EnumAdapterModes(D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8, i, &mode))
			{
				CString str;
				str.Format(_T("%dx%d %dHz"), mode.Width, mode.Height, mode.RefreshRate);
				int iItem = m_resolution.AddString(str);

				m_modes.AddTail(mode);
				m_resolution.SetItemDataPtr(iItem, m_modes.GetTailPosition());

				if(ModeWidth == mode.Width && ModeHeight == mode.Height && ModeRefreshRate == mode.RefreshRate)
				{
					m_resolution.SetCurSel(iItem);
				}
			}
		}

		pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);
	}

	InitComboBox(m_renderer, g_renderers, countof(g_renderers), pApp->GetProfileInt(_T("Settings"), _T("Renderer"), 0));
	InitComboBox(m_psversion, g_psversion, countof(g_psversion), pApp->GetProfileInt(_T("Settings"), _T("PixelShaderVersion2"), D3DPS_VERSION(2, 0)), caps.PixelShaderVersion);
	InitComboBox(m_interlace, g_interlace, countof(g_interlace), pApp->GetProfileInt(_T("Settings"), _T("Interlace"), 0));
	InitComboBox(m_aspectratio, g_aspectratio, countof(g_aspectratio), pApp->GetProfileInt(_T("Settings"), _T("AspectRatio"), 1));

	//

	m_filter = pApp->GetProfileInt(_T("Settings"), _T("filter"), 1);
	m_tvout = pApp->GetProfileInt(_T("Settings"), _T("tvout"), FALSE);
	m_nloophack = pApp->GetProfileInt(_T("Settings"), _T("nloophack"), 2);
	m_vsync = !!pApp->GetProfileInt(_T("Settings"), _T("vsync"), FALSE);
	m_logz = !!pApp->GetProfileInt(_T("Settings"), _T("logz"), FALSE);
	m_fba = !!pApp->GetProfileInt(_T("Settings"), _T("fba"), TRUE);

	m_resx.SetRange(512, 4096);
	m_resy.SetRange(512, 4096);
	m_resx.SetPos(pApp->GetProfileInt(_T("Settings"), _T("resx"), 1024));
	m_resy.SetPos(pApp->GetProfileInt(_T("Settings"), _T("resy"), 1024));
	m_nativeres = !!pApp->GetProfileInt(_T("Settings"), _T("nativeres"), FALSE);

	m_resx.EnableWindow(!m_nativeres);
	m_resy.EnableWindow(!m_nativeres);
	m_resxedit.EnableWindow(!m_nativeres);
	m_resyedit.EnableWindow(!m_nativeres);

	//

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void GSSettingsDlg::OnOK()
{
	CWinApp* pApp = AfxGetApp();

	UpdateData();

	if(m_resolution.GetCurSel() >= 0)
	{
        D3DDISPLAYMODE& mode = m_modes.GetAt((POSITION)m_resolution.GetItemData(m_resolution.GetCurSel()));
		pApp->WriteProfileInt(_T("Settings"), _T("ModeWidth"), mode.Width);
		pApp->WriteProfileInt(_T("Settings"), _T("ModeHeight"), mode.Height);
		pApp->WriteProfileInt(_T("Settings"), _T("ModeRefreshRate"), mode.RefreshRate);
	}

	if(m_renderer.GetCurSel() >= 0)
	{
		pApp->WriteProfileInt(_T("Settings"), _T("Renderer"), (DWORD)m_renderer.GetItemData(m_renderer.GetCurSel()));
	}

	if(m_psversion.GetCurSel() >= 0)
	{
		pApp->WriteProfileInt(_T("Settings"), _T("PixelShaderVersion2"), (DWORD)m_psversion.GetItemData(m_psversion.GetCurSel()));
	}

	if(m_interlace.GetCurSel() >= 0)
	{
		pApp->WriteProfileInt(_T("Settings"), _T("Interlace"), (DWORD)m_interlace.GetItemData(m_interlace.GetCurSel()));
	}

	if(m_aspectratio.GetCurSel() >= 0)
	{
		pApp->WriteProfileInt(_T("Settings"), _T("AspectRatio"), (DWORD)m_aspectratio.GetItemData(m_aspectratio.GetCurSel()));
	}

	pApp->WriteProfileInt(_T("Settings"), _T("filter"), m_filter);
	pApp->WriteProfileInt(_T("Settings"), _T("tvout"), m_tvout);
	pApp->WriteProfileInt(_T("Settings"), _T("nloophack"), m_nloophack);
	pApp->WriteProfileInt(_T("Settings"), _T("vsync"), m_vsync);
	pApp->WriteProfileInt(_T("Settings"), _T("logz"), m_logz);
	pApp->WriteProfileInt(_T("Settings"), _T("fba"), m_fba);

	pApp->WriteProfileInt(_T("Settings"), _T("resx"), m_resx.GetPos());
	pApp->WriteProfileInt(_T("Settings"), _T("resy"), m_resy.GetPos());
	pApp->WriteProfileInt(_T("Settings"), _T("nativeres"), m_nativeres);

	__super::OnOK();
}

void GSSettingsDlg::OnBnClickedCheck1()
{
	UpdateData();

	m_resx.EnableWindow(!m_nativeres);
	m_resy.EnableWindow(!m_nativeres);
	m_resxedit.EnableWindow(!m_nativeres);
	m_resyedit.EnableWindow(!m_nativeres);
}
