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

#include "GSTextureCache.h"
#include "GSTextureFX.h"

class GSRendererHWDX10 : public GSRendererHW<GSDeviceDX10>
{
	friend class GSTextureCache;

protected:
	int m_width;
	int m_height;
	int m_skip;

	GSTextureCache m_tc;
	GSTextureFX m_tfx;

	void VSync(int field);
	bool GetOutput(int i, GSTextureDX10& t, GSVector2& s);
	void DrawingKick(bool skip);
	void Draw();
	void InvalidateVideoMem(const GIFRegBITBLTBUF& BITBLTBUF, CRect r);
	void InvalidateLocalMem(const GIFRegBITBLTBUF& BITBLTBUF, CRect r);
	void MinMaxUV(int w, int h, CRect& r);

	struct
	{
		CComPtr<ID3D10DepthStencilState> dss;
		CComPtr<ID3D10BlendState> bs;
	} m_date;

	void SetupDATE(GSTextureCache::GSRenderTarget* rt, GSTextureCache::GSDepthStencil* ds);
	bool OverrideInput(int& prim, GSTextureCache::GSTexture* tex);	

public:
	GSRendererHWDX10(BYTE* base, bool mt, void (*irq)(), int nloophack, int interlace, int aspectratio, int filter, bool vsync);

	bool Create(LPCTSTR title);
};