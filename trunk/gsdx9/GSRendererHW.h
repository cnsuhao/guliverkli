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

#include "GSRenderer.h"

#pragma pack(push, 1)

__declspec(align(16)) union GSVertexHW
{
	struct
	{
		float x, y, z, w;
		union {struct {BYTE r, g, b, a;}; D3DCOLOR color;};
		D3DCOLOR fog;
		float tu, tv;
	};
	
	struct {__m128i xmm[2];};

#if _M_IX86_FP >= 2 || defined(_M_AMD64)
	GSVertexHW& operator = (GSVertexHW& v) {xmm[0] = v.xmm[0]; xmm[1] = v.xmm[1]; return *this;}
#endif
};

#pragma pack(pop)

class GSRendererHW : public GSRenderer<GSVertexHW>
{
protected:
	int m_width;
	int m_height;

	CComPtr<IDirect3DVertexShader9> m_pVertexShader;
	CComPtr<IDirect3DVertexDeclaration9> m_pVertexDeclaration;
	CComPtr<ID3DXConstantTable> m_pVertexShaderConstantTable;
	D3DXHANDLE m_hVertexShaderParams;
	CComPtr<IDirect3DPixelShader9> m_pPixelShader[5][4][2][2][2][2][2][2];

	GSTextureCache m_tc;

	void SetupVertexShader(const GSTextureCache::GSRenderTarget* rt);
	void SetupTexture(const GSTextureCache::GSTexture* t);
	void SetupAlphaBlend();
	void SetupColorMask();
	void SetupZBuffer();
	void SetupAlphaTest();
	void SetupDestinationAlphaTest(const GSTextureCache::GSRenderTarget* rt, const GSTextureCache::GSDepthStencil* ds);
	void SetupScissor(const GSScale& scale);
	void SetupFrameBufferAlpha();
	void UpdateFrameBufferAlpha(const GSTextureCache::GSRenderTarget* rt);

	void ResetState();
	void VertexKick(bool skip);
	void DrawingKick(bool skip);
	void FlushPrim();
	void Flip();
	void InvalidateTexture(const GIFRegBITBLTBUF& BITBLTBUF, CRect r);
	void InvalidateLocalMem(const GIFRegBITBLTBUF& BITBLTBUF, CRect r);
	void MinMaxUV(int w, int h, CRect& r);

public:
	GSRendererHW();
	virtual ~GSRendererHW();

	bool Create(LPCTSTR title);

	HRESULT ResetDevice(bool fForceWindowed = false);
};