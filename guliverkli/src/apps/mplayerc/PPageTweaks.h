/* 
 *	Copyright (C) 2003-2005 Gabest
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
#include "PPageBase.h"

// CPPageTweaks dialog

class CPPageTweaks : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageTweaks)

private:
	bool m_fWMASFReader;

public:
	CPPageTweaks();
	virtual ~CPPageTweaks();

	BOOL m_fDisabeXPToolbars;
	CButton m_fDisabeXPToolbarsCtrl;
	BOOL m_fUseWMASFReader;
	CButton m_fUseWMASFReaderCtrl;

// Dialog Data
	enum { IDD = IDD_PPAGETWEAKS };
	int m_nJumpDistS;
	int m_nJumpDistM;
	int m_nJumpDistL;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnUpdateCheck3(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCheck2(CCmdUI* pCmdUI);
	afx_msg void OnBnClickedButton1();
	afx_msg void OnUpdateCheck4(CCmdUI* pCmdUI);
public:
	BOOL m_fFreeWindowResizing;
public:
	BOOL m_fNotifyMSN;
};
