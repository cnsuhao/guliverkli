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

#include "PPageBase.h"

// CPPageFilters dialog

class CPPageFilters : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageFilters)

public:
	CPPageFilters();
	virtual ~CPPageFilters();

// Dialog Data
	enum { IDD = IDD_PPAGEFILTERS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

public:
	BOOL m_cdda;
	BOOL m_cdxa;
	BOOL m_vts;
	BOOL m_flic;
	BOOL m_dvd2avi;
	BOOL m_dtsac3;
	BOOL m_shoutcast;
	BOOL m_avi;
	BOOL m_matroska;
	BOOL m_realmedia;
	BOOL m_realvideo;
	BOOL m_realaudio;
	BOOL m_mpeg1;
	BOOL m_mpeg2;
	BOOL m_mpa;
	BOOL m_radgt;
	BOOL m_roq;
	BOOL m_ogg;
	BOOL m_lpcm;
	BOOL m_ac3;
	BOOL m_dts;
	BOOL m_nut;
};