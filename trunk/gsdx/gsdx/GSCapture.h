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

#pragma once

#include "GSCaptureDlg.h"

class GSCapture : protected CCritSec
{
	bool m_capturing;
	CSize m_size;
	CComPtr<IGraphBuilder> m_graph;
	CComPtr<IBaseFilter> m_src;

public:
	GSCapture();
	virtual ~GSCapture();

	bool BeginCapture(int fps);
	bool DeliverFrame(const void* bits, int pitch);
	bool EndCapture();

	bool IsCapturing() {return m_capturing;}
	CSize GetSize() {return m_size;}
};
