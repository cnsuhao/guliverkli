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

#include "StdAfx.h"
#include "GSStateSoft.h"

GSStateSoft::GSStateSoft(HWND hWnd, HRESULT& hr)
	: GSState(640, 512, hWnd, hr)
{
	m_pVertices = (GSSoftVertex*)_aligned_malloc(sizeof(GSSoftVertex) * (m_nMaxVertices = 256), 16);
	Reset();

	int i = -512, j = 0;
	for(; i < 0; i++, j++) m_clip[j] = 0, m_mask[j] = j&255;
	for(; i < 256; i++, j++) m_clip[j] = i, m_mask[j] = j&255;
	for(; i < 768; i++, j++) m_clip[j] = 255, m_mask[j] = j&255;
}

GSStateSoft::~GSStateSoft()
{
	_aligned_free(m_pVertices);
}

void GSStateSoft::Reset()
{
	m_primtype = PRIM_NONE;
	m_nVertices = m_nPrims = 0;
	m_tc.RemoveAll();
	m_pTexture = NULL;

	__super::Reset();
}

void GSStateSoft::NewPrim()
{
	m_vl.RemoveAll();
}

void GSStateSoft::VertexKick(bool fSkip)
{
	LOG((_T("VertexKick(%d)\n"), fSkip));

	GSDrawingContext* ctxt = &m_de.CTXT[m_de.PRIM.CTXT];

	GSSoftVertex v;
#ifdef FLOAT_SOFTVERTEX
	v.x = ((float)m_v.XYZ.X - ctxt->XYOFFSET.OFX) / 16;
	v.y = ((float)m_v.XYZ.Y - ctxt->XYOFFSET.OFY) / 16;
	// v.x = (float)(m_v.XYZ.X>>4) - (ctxt->XYOFFSET.OFX>>4);
	// v.y = (float)(m_v.XYZ.Y>>4) - (ctxt->XYOFFSET.OFY>>4);
	v.z = (float)m_v.XYZ.Z / UINT_MAX;
	v.q = m_v.RGBAQ.Q == 0 ? 1.0f : m_v.RGBAQ.Q;

	v.r = (float)m_v.RGBAQ.R;
	v.g = (float)m_v.RGBAQ.G;
	v.b = (float)m_v.RGBAQ.B;
	v.a = (float)m_v.RGBAQ.A;

	v.fog = (float)m_v.FOG.F;

	if(m_de.PRIM.TME)
	{
		if(m_de.PRIM.FST)
		{
			v.u = (float)m_v.UV.U / (16<<ctxt->TEX0.TW);
			v.v = (float)m_v.UV.V / (16<<ctxt->TEX0.TH);
			v.q = 1.0f;
		}
		else
		{
			v.u = m_v.ST.S;
			v.v = m_v.ST.T;
		}
	}
#else
	v.x = ((int)m_v.XYZ.X - ctxt->XYOFFSET.OFX) << 12;
	v.y = ((int)m_v.XYZ.Y - ctxt->XYOFFSET.OFY) << 12;
	v.z = (unsigned __int64)m_v.XYZ.Z << 32;
	v.q = m_v.RGBAQ.Q == 0 ? INT_MAX : (__int64)(m_v.RGBAQ.Q * INT_MAX);

	v.r = m_v.RGBAQ.R << 16;
	v.g = m_v.RGBAQ.G << 16;
	v.b = m_v.RGBAQ.B << 16;
	v.a = m_v.RGBAQ.A << 16;

	v.fog = m_v.FOG.F << 16;

	if(m_de.PRIM.TME)
	{
		if(m_de.PRIM.FST)
		{
			v.u = (__int64)(((float)m_v.UV.U / (16<<ctxt->TEX0.TW)) * INT_MAX);
			v.v = (__int64)(((float)m_v.UV.V / (16<<ctxt->TEX0.TH)) * INT_MAX);
			v.q = INT_MAX;
		}
		else
		{
			v.u = (__int64)(m_v.ST.S * INT_MAX);
			v.v = (__int64)(m_v.ST.T * INT_MAX);
		}
	}
#endif
	m_vl.AddTail(v);

	static const int vmin[8] = {1, 2, 2, 3, 3, 3, 2, 0};
	while(m_vl.GetCount() >= vmin[m_de.PRIM.PRIM])
	{
		if(m_nVertices+6 > m_nMaxVertices)
		{
			GSSoftVertex* pVertices = (GSSoftVertex*)_aligned_malloc(sizeof(GSSoftVertex) * (m_nMaxVertices <<= 1), 16);
			memcpy(pVertices, m_pVertices, m_nVertices*sizeof(GSSoftVertex));
			_aligned_free(m_pVertices);
			m_pVertices = pVertices;
		}

		DrawingKick(fSkip);
	}
}

void GSStateSoft::DrawingKick(bool fSkip)
{
	LOG((_T("DrawingKick %d\n"), m_de.PRIM.PRIM));

	if(m_PRIM != m_de.PRIM.PRIM && m_nVertices > 0) FlushPrim();
	m_PRIM = m_de.PRIM.PRIM;

	GSSoftVertex* pVertices = &m_pVertices[m_nVertices];
	int nVertices = 0;

	GSDrawingContext* ctxt = &m_de.CTXT[m_de.PRIM.CTXT];

	LOG2((_T("Prim %05x %05x %04x\n"), 
		ctxt->FRAME.Block(), m_de.PRIM.TME ? (UINT32)ctxt->TEX0.TBP0 : 0xfffff,
		(m_de.PRIM.ABE || (m_PRIM == 1 || m_PRIM == 2) && m_de.PRIM.AA1)
			? ((ctxt->ALPHA.A<<12)|(ctxt->ALPHA.B<<8)|(ctxt->ALPHA.C<<4)|ctxt->ALPHA.D) 
			: 0xffff));

	switch(m_PRIM)
	{
	case 3: // triangle list
		m_primtype = PRIM_TRIANGLE;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		LOGV((pVertices[0], _T("TriList")));
		LOGV((pVertices[1], _T("TriList")));
		LOGV((pVertices[2], _T("TriList")));
		break;
	case 4: // triangle strip
		m_primtype = PRIM_TRIANGLE;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.GetAt(0, pVertices[nVertices++]);
		m_vl.GetAt(1, pVertices[nVertices++]);
		LOGV((pVertices[0], _T("TriStrip")));
		LOGV((pVertices[1], _T("TriStrip")));
		LOGV((pVertices[2], _T("TriStrip")));
		break;
	case 5: // triangle fan
		m_primtype = PRIM_TRIANGLE;
		m_vl.GetAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(1, pVertices[nVertices++]);
		m_vl.GetAt(1, pVertices[nVertices++]);
		LOGV((pVertices[0], _T("TriFan")));
		LOGV((pVertices[1], _T("TriFan")));
		LOGV((pVertices[2], _T("TriFan")));
		break;
	case 6: // sprite
		m_primtype = PRIM_SPRITE;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		nVertices += 2;
		// ASSERT(pVertices[0].z == pVertices[1].z);
		pVertices[0].z = pVertices[1].z;
		pVertices[2] = pVertices[1];
		pVertices[3] = pVertices[1];
		pVertices[1].y = pVertices[0].y;
		pVertices[1].v = pVertices[0].v;
		pVertices[2].x = pVertices[0].x;
		pVertices[2].u = pVertices[0].u;
		LOGV((pVertices[0], _T("Sprite")));
		LOGV((pVertices[1], _T("Sprite")));
		LOGV((pVertices[2], _T("Sprite")));
		LOGV((pVertices[3], _T("Sprite")));
		break;
	case 1: // line
		m_primtype = PRIM_LINE;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		LOGV((pVertices[0], _T("LineList")));
		LOGV((pVertices[1], _T("LineList")));
		break;
	case 2: // line strip
		m_primtype = PRIM_LINE;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.GetAt(0, pVertices[nVertices++]);
		LOGV((pVertices[0], _T("LineStrip")));
		LOGV((pVertices[1], _T("LineStrip")));
		break;
	case 0: // point
		m_primtype = PRIM_POINT;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		LOGV((pVertices[0], _T("PointList")));
		break;
	default:
		ASSERT(0);
		return;
	}

	if(fSkip || !m_rs.IsEnabled(0) && !m_rs.IsEnabled(1))
	{
#ifdef ENABLE_STRIPFAN
		FlushPrim();
#endif
		return;
	}

	m_nVertices += nVertices;
#ifdef FLOAT_SOFTVERTEX
	if(!m_de.PRIM.IIP)
		for(int i = 4; i < 8; i++)
			pVertices[0].f[i] = pVertices[nVertices-1].f[i];
#else
	if(!m_de.PRIM.IIP)
		for(int i = 4; i < 8; i++)
			pVertices[0].dw[i] = pVertices[nVertices-1].dw[i];
#endif
/*
	if(::GetAsyncKeyState(VK_SPACE)&0x80000000)
	{
		FlushPrim();
		Flip();
	}
*/
}

void GSStateSoft::FlushPrim()
{
	if(m_nVertices > 0)
	{
		GSDrawingContext* ctxt = &m_de.CTXT[m_de.PRIM.CTXT];

		SetTexture();
#ifdef FLOAT_SOFTVERTEX
		CRect scissor(
			max(ctxt->SCISSOR.SCAX0, 0),
			max(ctxt->SCISSOR.SCAY0, 0),
			min(ctxt->SCISSOR.SCAX1+1, ctxt->FRAME.FBW * 64),
			min(ctxt->SCISSOR.SCAY1+1, 4096));
#else
		CRect scissor(
			max(ctxt->SCISSOR.SCAX0, 0)<<16,
			max(ctxt->SCISSOR.SCAY0, 0)<<16,
			min(ctxt->SCISSOR.SCAX1+1, ctxt->FRAME.FBW * 64)<<16,
			min(ctxt->SCISSOR.SCAY1+1, 4096)<<16);
#endif
		m_clamp = (m_de.COLCLAMP.CLAMP ? m_clip : m_mask) + 512;

		DWORD nPrims = 0;
		GSSoftVertex* pVertices = m_pVertices;

		switch(m_primtype)
		{
		case PRIM_SPRITE:
			ASSERT(!(m_nVertices&3));
			nPrims = m_nVertices / 4;
			LOG((_T("FlushPrim(pt=%d, nVertices=%d, nPrims=%d)\n"), m_primtype, m_nVertices, nPrims));
			for(int i = 0; i < nPrims; i++, pVertices += 4) DrawSprite(pVertices, scissor);
			break;
		case PRIM_TRIANGLE:
			ASSERT(!(m_nVertices%3));
			nPrims = m_nVertices / 3;
			LOG((_T("FlushPrim(pt=%d, nVertices=%d, nPrims=%d)\n"), m_primtype, m_nVertices, nPrims));
			for(int i = 0; i < nPrims; i++, pVertices += 3) DrawTriangle(pVertices, scissor);
			break;
		case PRIM_LINE: 
			ASSERT(!(m_nVertices&1));
			nPrims = m_nVertices / 2;
			LOG((_T("FlushPrim(pt=%d, nVertices=%d, nPrims=%d)\n"), m_primtype, m_nVertices, nPrims));
			for(int i = 0; i < nPrims; i++, pVertices += 2) DrawLine(pVertices, scissor);
			break;
		case PRIM_POINT:
			nPrims = m_nVertices;
			LOG((_T("FlushPrim(pt=%d, nVertices=%d, nPrims=%d)\n"), m_primtype, m_nVertices, nPrims));
			for(int i = 0; i < nPrims; i++, pVertices++) DrawPoint(pVertices, scissor);
			break;
		default:
			ASSERT(m_nVertices == 0);
			return;
		}

		InvalidateTexture(ctxt->FRAME.FBP<<5);

		m_stats.IncPrims(nPrims);
	}

	m_PRIM = 7;
	m_primtype = PRIM_NONE;
	m_nVertices = 0;
}

void GSStateSoft::Flip()
{
	__super::Flip();

	GSDrawingContext* ctxt = &m_de.CTXT[m_de.PRIM.CTXT];

	HRESULT hr;

	CComPtr<IDirect3DSurface9> pBackBuff;
	hr = m_pD3DDev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuff);

	D3DSURFACE_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	pBackBuff->GetDesc(&bd);

	CRect dst(0, 0, bd.Width, bd.Height);

	struct
	{
		CComPtr<IDirect3DTexture9> pRT;
		D3DSURFACE_DESC rd;
		scale_t scale;
		CRect src;
	} rt[2];

	for(int i = 0; i < countof(rt); i++)
	{
		if(m_rs.IsEnabled(i))
		{
			CSize size = m_rs.GetSize(i);

			int tw = size.cx;
			int th = size.cy;

			hr = m_pD3DDev->CreateTexture(tw, th, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &rt[i].pRT, NULL);
			if(S_OK != hr) continue;

			ZeroMemory(&rt[i].rd, sizeof(rt[i].rd));
			hr = rt[i].pRT->GetLevelDesc(0, &rt[i].rd);

			rt[i].scale = scale_t(1, 1);
			rt[i].src = CRect(0, 0, size.cx, size.cy);

			D3DLOCKED_RECT r;
			if(FAILED(hr = rt[i].pRT->LockRect(0, &r, NULL, 0)))
				continue;

			BYTE* dst = (BYTE*)r.pBits;

			GSLocalMemory::readTexel readTexel = m_lm.GetReadTexel(m_rs.DISPFB[i].PSM);
			GIFRegTEX0 TEX0;
			TEX0.TBP0 = m_rs.DISPFB[i].FBP<<5;
			TEX0.TBW = m_rs.DISPFB[i].FBW;
			TEX0.TCC = ctxt->TEX0.TCC;

			for(int y = 0, diff = r.Pitch - tw*4; y < th; y++, dst += diff)
				for(int x = 0; x < tw; x++, dst += 4)
					*(DWORD*)dst = (m_lm.*readTexel)(x, y, TEX0, m_de.TEXA);
/**/
			rt[i].pRT->UnlockRect(0);
		}
	}

	bool fShiftField = false;
		// m_rs.SMODE2.INT && !!(ctxt->XYOFFSET.OFY&0xf);
		// m_rs.CSRr.FIELD && m_rs.SMODE2.INT /*&& !m_rs.SMODE2.FFMD*/;

	struct
	{
		float x, y, z, rhw;
		float tu1, tv1;
		float tu2, tv2;
	}
	pVertices[] =
	{
		{(float)dst.left, (float)dst.top, 0.5f, 2.0f, 
			(float)rt[0].src.left / rt[0].rd.Width, (float)rt[0].src.top / rt[0].rd.Height, 
			(float)rt[1].src.left / rt[1].rd.Width, (float)rt[1].src.top / rt[1].rd.Height},
		{(float)dst.right, (float)dst.top, 0.5f, 2.0f, 
			(float)rt[0].src.right / rt[0].rd.Width, (float)rt[0].src.top / rt[0].rd.Height, 
			(float)rt[1].src.right / rt[1].rd.Width, (float)rt[1].src.top / rt[1].rd.Height},
		{(float)dst.left, (float)dst.bottom, 0.5f, 2.0f, 
			(float)rt[0].src.left / rt[0].rd.Width, (float)rt[0].src.bottom / rt[0].rd.Height, 
			(float)rt[1].src.left / rt[1].rd.Width, (float)rt[1].src.bottom / rt[1].rd.Height},
		{(float)dst.right, (float)dst.bottom, 0.5f, 2.0f, 
			(float)rt[0].src.right / rt[0].rd.Width, (float)rt[0].src.bottom / rt[0].rd.Height, 
			(float)rt[1].src.right / rt[1].rd.Width, (float)rt[1].src.bottom / rt[1].rd.Height},
	};

	for(int i = 0; i < countof(pVertices); i++)
	{
		pVertices[i].x -= 0.5;
		pVertices[i].y -= 0.5;

		if(fShiftField)
		{
			pVertices[i].tv1 += rt[0].scale.y*0.5f / rt[0].rd.Height;
			pVertices[i].tv2 += rt[1].scale.y*0.5f / rt[1].rd.Height;
		}
	}

	hr = m_pD3DDev->SetTexture(0, rt[0].pRT);
	hr = m_pD3DDev->SetTexture(1, rt[1].pRT);

	hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
	hr = m_pD3DDev->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);

	hr = m_pD3DDev->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX2);

	CComPtr<IDirect3DPixelShader9> pPixelShader;

	if(m_rs.IsEnabled(0) && m_rs.IsEnabled(1) && rt[0].pRT && rt[1].pRT) // RAO1 + RAO2
	{
		pPixelShader = m_pPixelShaders[11];
	}
	else if(m_rs.IsEnabled(0) && rt[0].pRT) // RAO1
	{
		pPixelShader = m_pPixelShaders[12];
	}
	else if(m_rs.IsEnabled(1) && rt[1].pRT) // RAO2
	{
		pPixelShader = m_pPixelShaders[13];
	}
	else
	{
		hr = m_pD3DDev->Present(NULL, NULL, NULL, NULL);
		return;
	}

	if(!pPixelShader)
	{
		int stage = 0;

		hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
		hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		stage++;

		if(m_rs.IsEnabled(0) && m_rs.IsEnabled(1) && rt[0].pRT && rt[1].pRT) // RAO1 + RAO2
		{
			if(m_rs.PMODE.ALP < 0xff)
			{
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLOROP, D3DTOP_LERP);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLORARG1, D3DTA_CURRENT);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLORARG2, m_rs.PMODE.SLBG ? D3DTA_CONSTANT : D3DTA_TEXTURE);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLORARG0, D3DTA_ALPHAREPLICATE|(m_rs.PMODE.MMOD ? D3DTA_CONSTANT : D3DTA_TEXTURE));
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_CONSTANT, D3DCOLOR_ARGB(m_rs.PMODE.ALP, m_rs.BGCOLOR.R, m_rs.BGCOLOR.G, m_rs.BGCOLOR.B));
				stage++;
			}
		}
		else if(m_rs.IsEnabled(0) && rt[0].pRT) // RAO1
		{
			if(m_rs.PMODE.ALP < 0xff)
			{
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLOROP, D3DTOP_MODULATE);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLORARG1, D3DTA_CURRENT);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLORARG2, D3DTA_ALPHAREPLICATE|(m_rs.PMODE.MMOD ? D3DTA_CONSTANT : D3DTA_TEXTURE));
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_CONSTANT, D3DCOLOR_ARGB(m_rs.PMODE.ALP, 0, 0, 0));
				stage++;
			}
		}
		else if(m_rs.IsEnabled(1) && rt[1].pRT) // RAO2
		{
			hr = m_pD3DDev->SetTexture(0, rt[1].pRT);
			hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 1);

			// FIXME
			hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
			for(int i = 0; i < countof(pVertices); i++)
			{
				pVertices[i].tu1 = pVertices[i].tu2;
				pVertices[i].tv1 = pVertices[i].tv2;
			}
		}

		hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLOROP, D3DTOP_DISABLE);
		hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	}

	hr = m_pD3DDev->BeginScene();
	hr = m_pD3DDev->SetPixelShader(pPixelShader);
	hr = m_pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pVertices, sizeof(pVertices[0]));
	hr = m_pD3DDev->SetPixelShader(NULL);
	hr = m_pD3DDev->EndScene();

	hr = m_pD3DDev->Present(NULL, NULL, NULL, NULL);
}

void GSStateSoft::EndFrame()
{
	POSITION pos = m_tc.GetHeadPosition();
	while(pos)
	{
		POSITION cur = pos;
		if(++m_tc.GetNext(pos)->m_age > 2)
			m_tc.RemoveAt(cur);
	}
}

void GSStateSoft::InvalidateTexture(DWORD TBP0)
{
	POSITION pos = m_tc.GetHeadPosition();
	while(pos)
	{
		POSITION cur = pos;
		CTexture* p = m_tc.GetNext(pos);
		if(p->m_TEX0.TBP0 == TBP0 || p->m_TEX0.CBP == TBP0)
			m_tc.RemoveAt(cur);
	}
}

void GSStateSoft::DrawPoint(GSSoftVertex* v, CRect& scissor)
{
	GSDrawingContext* ctxt = &m_de.CTXT[m_de.PRIM.CTXT];

	CPoint p(v->x, v->y);
	if(scissor.PtInRect(p))
		DrawVertex(p.x, p.y, *v);
}
#ifdef FLOAT_SOFTVERTEX
void GSStateSoft::DrawLine(GSSoftVertex* v, CRect& scissor)
{
	GSDrawingContext* ctxt = &m_de.CTXT[m_de.PRIM.CTXT];

	float dx = fabs(v[1].x - v[0].x);
	float dy = fabs(v[1].y - v[0].y);

	if(dx == 0 && dy == 0) return;

	int f = dx > dy ? 0 : 1;

	if(v[0].f[f] > v[1].f[f]) {GSSoftVertex tmp = v[0]; v[0] = v[1]; v[1] = tmp;}

	GSSoftVertex edge = v[0];
	GSSoftVertex dedge = (v[1] - v[0]) / (v[1].f[f] - v[0].f[f]);

	if(v[0].x < scissor.left) v[0] += dedge * ((float)scissor.left - v[0].x);
	if(v[1].x > scissor.right) v[1].x = (float)scissor.right;
	if(v[0].y < scissor.top) v[0] += dedge * ((float)scissor.top - v[0].y);
	if(v[1].y > scissor.bottom) v[1].y = (float)scissor.bottom;

	int start = int(v[0].f[f]), end = int(v[1].f[f]);

	for(; start < end; start++, edge += dedge)
	{
		DrawVertex(edge.x, edge.y, edge);
	}
}

void GSStateSoft::DrawTriangle(GSSoftVertex* v, CRect& scissor)
{
	GSDrawingContext* ctxt = &m_de.CTXT[m_de.PRIM.CTXT];

	if(v[1].y < v[0].y) {GSSoftVertex tmp = v[0]; v[0] = v[1]; v[1] = tmp;}
	if(v[2].y < v[0].y) {GSSoftVertex tmp = v[0]; v[0] = v[2]; v[2] = tmp;}
	if(v[2].y < v[1].y)  {GSSoftVertex tmp = v[1]; v[1] = v[2]; v[2] = tmp;}

	if(v[0].y >= v[2].y) return;

	float temp = (v[1].y - v[0].y) / (v[2].y - v[0].y);
	float longest = temp * (v[2].x - v[0].x) + (v[0].x - v[1].x);

	GSSoftVertex edge[2], dedge[2], scan, dscan;

	int ledge, redge;
	if(longest > 0) {ledge = 0; redge = 1;}
	else if(longest < 0) {ledge = 1; redge = 0;}
	else return;

	memset(dedge, 0, sizeof(dedge));
	if(v[1].y > v[0].y) dedge[ledge] = (v[1] - v[0]) / (v[1].y - v[0].y);
	if(v[2].y > v[0].y) dedge[redge] = (v[2] - v[0]) / (v[2].y - v[0].y);

	memset(&dscan, 0, sizeof(dscan));
	dscan = ((v[2] - v[0]) * temp + (v[0] - v[1])) / longest;

	edge[ledge] = edge[redge] = v[0];

	for(int i = 0; i < 2; i++, v++)
	{
 		int top = int(v[0].y), bottom = int(v[1].y);
//		float top = v[0].y, bottom = v[1].y;

		if(top < scissor.top)
		{
			for(int i = 0; i < 2; i++) edge[i] += dedge[i] * (min(scissor.top, bottom) - top);
			edge[0].y = edge[1].y = top = scissor.top;
		}

		if(bottom > scissor.bottom)
		{
			bottom = scissor.bottom;
		}

		for(; top < bottom; top++)
		{
			scan = edge[0];

			int left = int(edge[0].x), right = int(edge[1].x);
//			float left = edge[0].x, right = edge[1].x;

			if(left < scissor.left)
			{
				scan += dscan * (scissor.left - left);
				scan.x = left = scissor.left;
			}

			if(right > scissor.right)
			{
				right = scissor.right;
			}

			for(; left < right; left++)
			{
				DrawVertex(left, top, scan);
				scan += dscan;
			}

			edge[0] += dedge[0];
			edge[1].x += dedge[1].x;
		}

		if(v[2].y > v[1].y)
		{
			edge[ledge] = v[1];
			dedge[ledge] = (v[2] - v[1]) / (v[2].y - v[1].y);
		}
	}
}

void GSStateSoft::DrawSprite(GSSoftVertex* v, CRect& scissor)
{
	GSDrawingContext* ctxt = &m_de.CTXT[m_de.PRIM.CTXT];

	if(v[2].y < v[0].y) {GSSoftVertex tmp = v[0]; v[0] = v[2]; v[2] = tmp; tmp = v[1]; v[1] = v[3]; v[3] = tmp;}
	if(v[1].x < v[0].x) {GSSoftVertex tmp = v[0]; v[0] = v[1]; v[1] = tmp; tmp = v[2]; v[2] = v[3]; v[3] = tmp;}

	if(v[0].x == v[1].x || v[0].y == v[2].y) return;

	GSSoftVertex edge[2], dedge[2], scan, dscan;

	memset(dedge, 0, sizeof(dedge));
	for(int i = 0; i < 2; i++) dedge[i] = (v[i+2] - v[i]) / (v[i+2].y - v[i].y);

	memset(&dscan, 0, sizeof(dscan));
	dscan = (v[1] - v[0]) / (v[1].x - v[0].x);

	edge[0] = v[0];
	edge[1] = v[1];

	int top = int(v[0].y), bottom = int(v[2].y);
//	float top = v[0].y, bottom = v[2].y;

	if(top < scissor.top)
	{
		for(int i = 0; i < 2; i++) edge[i] += dedge[i] * (min(scissor.top, bottom) - top);
		edge[0].y = edge[1].y = top = scissor.top;
	}

	if(bottom > scissor.bottom)
	{
		bottom = scissor.bottom;
	}

	for(; top < bottom; top++)
	{
		scan = edge[0];

		int left = int(edge[0].x), right = int(edge[1].x);
//		float left = edge[0].x, right = edge[1].x;

		if(left < scissor.left)
		{
			scan += dscan * (scissor.left - left);
			scan.x = left = scissor.left;
		}

		if(right > scissor.right)
		{
			right = scissor.right;
		}

		for(; left < right; left++)
		{
			DrawVertex(left, top, scan);
			scan += dscan;
		}

		edge[0] += dedge[0];
		edge[1].x += dedge[1].x;
	}
}

void GSStateSoft::DrawVertex(int x, int y, GSSoftVertex& v)
{
	GSDrawingContext* ctxt = &m_de.CTXT[m_de.PRIM.CTXT];

//x = v.x; y = v.y;

	DWORD FBP = ctxt->FRAME.FBP<<5, FBW = ctxt->FRAME.FBW;

	if(ctxt->TEST.ZTE && ctxt->TEST.ZTST != 1)
	{
		if(ctxt->TEST.ZTST == 0)
			return;

		if(ctxt->rz)
		{
			float z = (float)(m_lm.*ctxt->rz)(x, y, ctxt->ZBUF.ZBP<<5, FBW) / UINT_MAX;
			if(ctxt->TEST.ZTST == 2 && v.z < z || ctxt->TEST.ZTST == 3 && v.z <= z)
				return;
		}
	}

	if(ctxt->TEST.DATE && ctxt->FRAME.PSM <= PSM_PSMCT16S)
	{
		GSLocalMemory::readPixel rp = m_lm.GetReadPixel(ctxt->FRAME.PSM);
		BYTE A = (m_lm.*rp)(x, y, FBP, FBW) >> (ctxt->FRAME.PSM == PSM_PSMCT32 ? 31 : 15);
		if(A ^ ctxt->TEST.DATM) return;
	}

	int Rf = (int)v.r;
	int Gf = (int)v.g;
	int Bf = (int)v.b;
	int Af = (int)v.a;

	if(m_de.PRIM.TME)
	{
		int tw = 1 << ctxt->TEX0.TW;
		int th = 1 << ctxt->TEX0.TH;

		float tu = v.u / v.q * tw;
		float tv = v.v / v.q * th;
		
		// TODO
		// float lod = ctxt->TEX1.K;
		// if(!ctxt->TEX1.LCM) lod += log2(1/v.q) << ctxt->TEX1.L;

		float ftu = modf(tu, &tu); // tu - 0.5f
		float ftv = modf(tv, &tv); // tv - 0.5f

		int itu[2] = {tu, tu+1};
		int itv[2] = {tv, tv+1};

		for(int i = 0; i < countof(itu); i++)
		{
			switch(ctxt->CLAMP.WMS)
			{
			case 0: itu[i] = itu[i] & (tw-1); break;
			case 1: itu[i] = itu[i] < 0 ? 0 : itu[i] >= tw ? itu[i] = tw-1 : itu[i]; break;
			case 2: itu[i] = itu[i] < ctxt->CLAMP.MINU ? ctxt->CLAMP.MINU : itu[i] > ctxt->CLAMP.MAXU ? ctxt->CLAMP.MAXU : itu[i]; break;
			case 3: itu[i] = (int(itu[i]) & ctxt->CLAMP.MINU) | ctxt->CLAMP.MAXU; break;
			}

			ASSERT(itu[i] >= 0 && itu[i] < tw);
		}

		for(int i = 0; i < countof(itv); i++)
		{
			switch(ctxt->CLAMP.WMT)
			{
			case 0: itv[i] = itv[i] & (th-1); break;
			case 1: itv[i] = itv[i] < 0 ? 0 : itv[i] >= th ? itv[i] = th-1 : itv[i]; break;
			case 2: itv[i] = itv[i] < ctxt->CLAMP.MINV ? ctxt->CLAMP.MINV : itv[i] > ctxt->CLAMP.MAXV ? ctxt->CLAMP.MAXV : itv[i]; break;
			case 3: itv[i] = (int(itv[i]) & ctxt->CLAMP.MINV) | ctxt->CLAMP.MAXV; break;
			}

			ASSERT(itv[i] >= 0 && itv[i] < th);
		}

		DWORD c[4];
		WORD Bt, Gt, Rt, At;

		// if(ctxt->TEX1.MMAG&1) // FIXME
		{
			if(ftu < 0) ftu += 1;
			if(ftv < 0) ftv += 1;
			float iftu = 1.0f - ftu;
			float iftv = 1.0f - ftv;

			if(m_pTexture)
			{
				c[0] = m_pTexture[(itv[0] << ctxt->TEX0.TW) + itu[0]];
				c[1] = m_pTexture[(itv[0] << ctxt->TEX0.TW) + itu[1]];
				c[2] = m_pTexture[(itv[1] << ctxt->TEX0.TW) + itu[0]];
				c[3] = m_pTexture[(itv[1] << ctxt->TEX0.TW) + itu[1]];
			}
			else
			{
				c[0] = (m_lm.*ctxt->rt)(itu[0], itv[0], ctxt->TEX0, m_de.TEXA);
				c[1] = (m_lm.*ctxt->rt)(itu[1], itv[0], ctxt->TEX0, m_de.TEXA);
				c[2] = (m_lm.*ctxt->rt)(itu[0], itv[1], ctxt->TEX0, m_de.TEXA);
				c[3] = (m_lm.*ctxt->rt)(itu[1], itv[1], ctxt->TEX0, m_de.TEXA);
			}

			float iuiv = iftu*iftv;
			float uiv = ftu*iftv;
			float iuv = iftu*ftv;
			float uv = ftu*ftv;

			Bt = (WORD)(iuiv*((c[0]>> 0)&0xff) + uiv*((c[1]>> 0)&0xff) + iuv*((c[2]>> 0)&0xff) + uv*((c[3]>> 0)&0xff) + 0.5f);
			Gt = (WORD)(iuiv*((c[0]>> 8)&0xff) + uiv*((c[1]>> 8)&0xff) + iuv*((c[2]>> 8)&0xff) + uv*((c[3]>> 8)&0xff) + 0.5f);
			Rt = (WORD)(iuiv*((c[0]>>16)&0xff) + uiv*((c[1]>>16)&0xff) + iuv*((c[2]>>16)&0xff) + uv*((c[3]>>16)&0xff) + 0.5f);
			At = (WORD)(iuiv*((c[0]>>24)&0xff) + uiv*((c[1]>>24)&0xff) + iuv*((c[2]>>24)&0xff) + uv*((c[3]>>24)&0xff) + 0.5f);
		}
//		else 
		if(0)
		{
			if(m_pTexture)
			{
				c[0] = m_pTexture[(itv[0] << ctxt->TEX0.TW) + itu[0]];
			}
			else
			{
				c[0] = (m_lm.*ctxt->rt)(itu[0], itv[0], ctxt->TEX0, m_de.TEXA);
			}

			Bt = (BYTE)((c[0]>>0)&0xff);
			Gt = (BYTE)((c[0]>>8)&0xff);
			Rt = (BYTE)((c[0]>>16)&0xff);
			At = (BYTE)((c[0]>>24)&0xff);
		}

		switch(ctxt->TEX0.TFX)
		{
		case 0:
			Rf = Rf * Rt >> 7;
			Gf = Gf * Gt >> 7;
			Bf = Bf * Bt >> 7;
			Af = ctxt->TEX0.TCC ? (Af * At >> 7) : Af;
			break;
		case 1:
			Rf = Rt;
			Gf = Gt;
			Bf = Bt;
			Af = At;
			break;
		case 2:
			Rf = (Rf * Rt >> 7) + Af;
			Gf = (Gf * Gt >> 7) + Af;
			Bf = (Bf * Bt >> 7) + Af;
			Af = ctxt->TEX0.TCC ? (Af + At) : Af;
			break;
		case 3:
			Rf = (Rf * Rt >> 7) + Af;
			Gf = (Gf * Gt >> 7) + Af;
			Bf = (Bf * Bt >> 7) + Af;
			Af = ctxt->TEX0.TCC ? At : Af;
			break;
		}

		Rf = m_clip[Rf+512];
		Gf = m_clip[Gf+512];
		Bf = m_clip[Bf+512];
		Af = m_clip[Af+512];
	}

	if(m_de.PRIM.FGE)
	{
		BYTE F = (BYTE)v.fog;
		Rf = (F * Rf + (255 - F) * m_de.FOGCOL.FCR) >> 8;
		Gf = (F * Gf + (255 - F) * m_de.FOGCOL.FCG) >> 8;
		Bf = (F * Bf + (255 - F) * m_de.FOGCOL.FCB) >> 8;
	}

	BOOL ZMSK = ctxt->ZBUF.ZMSK;
	DWORD FBMSK = ctxt->FRAME.FBMSK;

	if(ctxt->TEST.ATE)
	{
		bool fPass = true;

		switch(ctxt->TEST.ATST)
		{
		case 0: fPass = false; break;
		case 1: fPass = true; break;
		case 2: fPass = Af < ctxt->TEST.AREF; break;
		case 3: fPass = Af <= ctxt->TEST.AREF; break;
		case 4: fPass = Af == ctxt->TEST.AREF; break;
		case 5: fPass = Af >= ctxt->TEST.AREF; break;
		case 6: fPass = Af > ctxt->TEST.AREF; break;
		case 7: fPass = Af != ctxt->TEST.AREF; break;
		}

		if(!fPass)
		{
			switch(ctxt->TEST.AFAIL)
			{
			case 0: return;
			case 1: ZMSK = 1; break; // RGBA
			case 2: FBMSK = 0xffffffff; break; // Z
			case 3: FBMSK = 0xff000000; ZMSK = 1; break; // RGB
			}
		}
	}

	if(!ZMSK && ctxt->rz)
	{
		(m_lm.*ctxt->wz)(x, y, (DWORD)(v.z * UINT_MAX), ctxt->ZBUF.ZBP<<5, FBW);
	}

	if((m_de.PRIM.ABE || (m_de.PRIM.PRIM == 1 || m_de.PRIM.PRIM == 2) && m_de.PRIM.AA1) && (!m_de.PABE.PABE || (Af&0x80)))
	{
		GIFRegTEX0 TEX0;
		TEX0.TBP0 = FBP;
		TEX0.TBW = FBW;
		TEX0.TCC = ctxt->TEX0.TCC;
		DWORD Cd = (m_lm.*ctxt->rp)(x, y, TEX0, m_de.TEXA);

		BYTE R[3] = {Rf, (Cd>>16)&0xff, 0};
		BYTE G[3] = {Gf, (Cd>>8)&0xff, 0};
		BYTE B[3] = {Bf, (Cd>>0)&0xff, 0};
		BYTE A[3] = {Af, (Cd>>24)&0xff, ctxt->ALPHA.FIX};

		Rf = ((R[ctxt->ALPHA.A] - R[ctxt->ALPHA.B]) * A[ctxt->ALPHA.C] >> 7) + R[ctxt->ALPHA.D];
		Gf = ((G[ctxt->ALPHA.A] - G[ctxt->ALPHA.B]) * A[ctxt->ALPHA.C] >> 7) + G[ctxt->ALPHA.D];
		Bf = ((B[ctxt->ALPHA.A] - B[ctxt->ALPHA.B]) * A[ctxt->ALPHA.C] >> 7) + B[ctxt->ALPHA.D];
	}

	if(m_de.DTHE.DTHE)
	{
		WORD DMxy = (*((WORD*)&m_de.DIMX.i64 + (y&3)) >> ((x&3)<<2)) & 7;
		Rf += DMxy;
		Gf += DMxy;
		Bf += DMxy;
	}

	Rf = m_clamp[Rf];
	Gf = m_clamp[Gf];
	Bf = m_clamp[Bf];
	Af = m_clamp[Af]; // ?

	Af |= (ctxt->FBA.FBA << 7);

	DWORD color = ((Af << 24) | (Bf << 16) | (Gf << 8) | (Rf << 0)) & ~FBMSK;

	(m_lm.*ctxt->wf)(x, y, color, FBP, FBW);
}
#else
void GSStateSoft::DrawLine(GSSoftVertex* v, CRect& scissor)
{
	GSDrawingContext* ctxt = &m_de.CTXT[m_de.PRIM.CTXT];

	int dx = abs(v[1].x - v[0].x);
	int dy = abs(v[1].y - v[0].y); 

	if(dx == 0 && dy == 0) return;

	int i = dx > dy ? 0 : 1;

	if(v[0].dw[i] > v[1].dw[i]) {GSSoftVertex tmp = v[0]; v[0] = v[1]; v[1] = tmp;}

	GSSoftVertex edge = v[0];
	GSSoftVertex dedge = (v[1] - v[0]) / (v[1].dw[i] - v[0].dw[i]);

	if(v[0].x < scissor.left) v[0] += dedge * (scissor.left - v[0].x);
	if(v[1].x > scissor.right) v[1].x = scissor.right;
	if(v[0].y < scissor.top) v[0] += dedge * (scissor.top - v[0].y);
	if(v[1].y > scissor.bottom) v[1].y = scissor.bottom;

	int start = v[0].dw[i]>>16, end = v[1].dw[i]>>16;

	for(; start < end; start++, edge += dedge)
	{
		DrawVertex(edge.x, edge.y, edge);
	}
}

void GSStateSoft::DrawTriangle(GSSoftVertex* v, CRect& scissor)
{
	GSDrawingContext* ctxt = &m_de.CTXT[m_de.PRIM.CTXT];

	if(v[1].y < v[0].y) {GSSoftVertex tmp = v[0]; v[0] = v[1]; v[1] = tmp;}
	if(v[2].y < v[0].y) {GSSoftVertex tmp = v[0]; v[0] = v[2]; v[2] = tmp;}
	if(v[2].y < v[1].y)  {GSSoftVertex tmp = v[1]; v[1] = v[2]; v[2] = tmp;}

	if(v[0].y >= v[2].y) return;

	int temp = ((__int64)(v[1].y - v[0].y) << 16) / (v[2].y - v[0].y);
	int longest = ((__int64)temp * (v[2].x - v[0].x) >> 16) + (v[0].x - v[1].x);

	GSSoftVertex edge[2], dedge[2], scan, dscan;

	int ledge, redge;
	if(longest >= 0x10000) {ledge = 0; redge = 1;}
	else if(longest <= -0x10000) {ledge = 1; redge = 0;}
	else return;

	memset(dedge, 0, sizeof(dedge));
	if(v[1].y > v[0].y) dedge[ledge] = (v[1] - v[0]) / (v[1].y - v[0].y);
	if(v[2].y > v[0].y) dedge[redge] = (v[2] - v[0]) / (v[2].y - v[0].y);

	memset(&dscan, 0, sizeof(dscan));
	dscan = ((v[2] - v[0]) * temp + (v[0] - v[1])) / longest;

	edge[ledge] = edge[redge] = v[0];

	for(int i = 0; i < 2; i++, v++)
	{
 		int top = v[0].y, bottom = v[1].y;
//		float top = v[0].y, bottom = v[1].y;

		if(top < scissor.top)
		{
			for(int i = 0; i < 2; i++) edge[i] += dedge[i] * (min(scissor.top, bottom) - top);
			edge[0].y = edge[1].y = top = scissor.top;
		}

		if(bottom > scissor.bottom)
		{
			bottom = scissor.bottom;
		}

		for(top >>= 16, bottom >>= 16; top < bottom; top++)
		{
			scan = edge[0];

			int left = edge[0].x, right = edge[1].x;
//			float left = edge[0].x, right = edge[1].x;

			if(left < scissor.left)
			{
				scan += dscan * (scissor.left - left);
				scan.x = left = scissor.left;
			}

			if(right > scissor.right)
			{
				right = scissor.right;
			}

			for(left >>= 16, right >>= 16; left < right; left++)
			{
				DrawVertex(left<<16, top<<16, scan);
				scan += dscan;
			}

			edge[0] += dedge[0];
			edge[1].x += dedge[1].x;
		}

		if(v[2].y > v[1].y)
		{
			edge[ledge] = v[1];
			dedge[ledge] = (v[2] - v[1]) / (v[2].y - v[1].y);
		}
	}
}

void GSStateSoft::DrawSprite(GSSoftVertex* v, CRect& scissor)
{
	GSDrawingContext* ctxt = &m_de.CTXT[m_de.PRIM.CTXT];

	if(v[2].y < v[0].y) {GSSoftVertex tmp = v[0]; v[0] = v[2]; v[2] = tmp; tmp = v[1]; v[1] = v[3]; v[3] = tmp;}
	if(v[1].x < v[0].x) {GSSoftVertex tmp = v[0]; v[0] = v[1]; v[1] = tmp; tmp = v[2]; v[2] = v[3]; v[3] = tmp;}

	if(v[0].x == v[1].x || v[0].y == v[2].y) return;

	GSSoftVertex edge[2], dedge[2], scan, dscan;

	memset(dedge, 0, sizeof(dedge));
	for(int i = 0; i < 2; i++) dedge[i] = (v[i+2] - v[i]) / (v[i+2].y - v[i].y);

	memset(&dscan, 0, sizeof(dscan));
	dscan = (v[1] - v[0]) / (v[1].x - v[0].x);

	edge[0] = v[0];
	edge[1] = v[1];

	int top = v[0].y, bottom = v[2].y;
//	float top = v[0].y, bottom = v[2].y;

	if(top < scissor.top)
	{
		for(int i = 0; i < 2; i++) edge[i] += dedge[i] * (min(scissor.top, bottom) - top);
		edge[0].y = edge[1].y = top = scissor.top;
	}

	if(bottom > scissor.bottom)
	{
		bottom = scissor.bottom;
	}

	for(top >>= 16, bottom >>= 16; top < bottom; top++)
	{
		scan = edge[0];

		int left = edge[0].x, right = edge[1].x;
//		float left = edge[0].x, right = edge[1].x;

		if(left < scissor.left)
		{
			scan += dscan * (scissor.left - left);
			scan.x = left = scissor.left;
		}

		if(right > scissor.right)
		{
			right = scissor.right;
		}

		for(left >>= 16, right >>= 16; left < right; left++)
		{
			DrawVertex(left<<16, top<<16, scan);
			scan += dscan;
		}

		edge[0] += dedge[0];
		edge[1].x += dedge[1].x;
	}
}

void GSStateSoft::DrawVertex(int x, int y, GSSoftVertex& v)
{
	GSDrawingContext* ctxt = &m_de.CTXT[m_de.PRIM.CTXT];

	x >>= 16;
	y >>= 16;

	DWORD FBP = ctxt->FRAME.FBP<<5, FBW = ctxt->FRAME.FBW;

	if(ctxt->TEST.ZTE && ctxt->TEST.ZTST != 1)
	{
		if(ctxt->TEST.ZTST == 0)
			return;

		if(ctxt->rz)
		{
			DWORD z = (m_lm.*ctxt->rz)(x, y, ctxt->ZBUF.ZBP<<5, FBW);
			DWORD vz = v.z >> 32;
			if(ctxt->TEST.ZTST == 2 && vz < z || ctxt->TEST.ZTST == 3 && vz <= z)
				return;
		}
	}

	if(ctxt->TEST.DATE && ctxt->FRAME.PSM <= PSM_PSMCT16S)
	{
		GSLocalMemory::readPixel rp = m_lm.GetReadPixel(ctxt->FRAME.PSM);
		BYTE A = (m_lm.*rp)(x, y, FBP, FBW) >> (ctxt->FRAME.PSM == PSM_PSMCT32 ? 31 : 15);
		if(A ^ ctxt->TEST.DATM) return;
	}

	int Rf = v.r>>16;
	int Gf = v.g>>16;
	int Bf = v.b>>16;
	int Af = v.a>>16;

	if(m_de.PRIM.TME)
	{
		int tw = 1 << ctxt->TEX0.TW;
		int th = 1 << ctxt->TEX0.TH;

		// TODO: clean up these last remaining floats

		float tu = (float)v.u / v.q * tw;
		float tv = (float)v.v / v.q * th;

		float ftu = modf(tu, &tu);
		float ftv = modf(tv, &tv);

		int itu[2] = {tu, tu+1};
		int itv[2] = {tv, tv+1};

		for(int i = 0; i < countof(itu); i++)
		{
			switch(ctxt->CLAMP.WMS)
			{
			case 0: itu[i] = itu[i] & (tw-1); break;
			case 1: itu[i] = itu[i] < 0 ? 0 : itu[i] >= tw ? itu[i] = tw-1 : itu[i]; break;
			case 2: itu[i] = itu[i] < ctxt->CLAMP.MINU ? ctxt->CLAMP.MINU : itu[i] > ctxt->CLAMP.MAXU ? ctxt->CLAMP.MAXU : itu[i]; break;
			case 3: itu[i] = (int(itu[i]) & ctxt->CLAMP.MINU) | ctxt->CLAMP.MAXU; break;
			}

			ASSERT(itu[i] >= 0 && itu[i] < tw);
		}

		for(int i = 0; i < countof(itv); i++)
		{
			switch(ctxt->CLAMP.WMT)
			{
			case 0: itv[i] = itv[i] & (th-1); break;
			case 1: itv[i] = itv[i] < 0 ? 0 : itv[i] >= th ? itv[i] = th-1 : itv[i]; break;
			case 2: itv[i] = itv[i] < ctxt->CLAMP.MINV ? ctxt->CLAMP.MINV : itv[i] > ctxt->CLAMP.MAXV ? ctxt->CLAMP.MAXV : itv[i]; break;
			case 3: itv[i] = (int(itv[i]) & ctxt->CLAMP.MINV) | ctxt->CLAMP.MAXV; break;
			}

			ASSERT(itv[i] >= 0 && itv[i] < th);
		}

		DWORD c[4];
		WORD Bt, Gt, Rt, At;

		{
			if(ftu < 0) ftu += 1;
			if(ftv < 0) ftv += 1;
			float iftu = 1.0f - ftu;
			float iftv = 1.0f - ftv;

			if(m_pTexture)
			{
				c[0] = m_pTexture[(itv[0] << ctxt->TEX0.TW) + itu[0]];
				c[1] = m_pTexture[(itv[0] << ctxt->TEX0.TW) + itu[1]];
				c[2] = m_pTexture[(itv[1] << ctxt->TEX0.TW) + itu[0]];
				c[3] = m_pTexture[(itv[1] << ctxt->TEX0.TW) + itu[1]];
			}
			else
			{
				c[0] = (m_lm.*ctxt->rt)(itu[0], itv[0], ctxt->TEX0, m_de.TEXA);
				c[1] = (m_lm.*ctxt->rt)(itu[1], itv[0], ctxt->TEX0, m_de.TEXA);
				c[2] = (m_lm.*ctxt->rt)(itu[0], itv[1], ctxt->TEX0, m_de.TEXA);
				c[3] = (m_lm.*ctxt->rt)(itu[1], itv[1], ctxt->TEX0, m_de.TEXA);
			}

			float iuiv = iftu*iftv;
			float uiv = ftu*iftv;
			float iuv = iftu*ftv;
			float uv = ftu*ftv;

			Bt = (WORD)(iuiv*((c[0]>> 0)&0xff) + uiv*((c[1]>> 0)&0xff) + iuv*((c[2]>> 0)&0xff) + uv*((c[3]>> 0)&0xff) + 0.5f);
			Gt = (WORD)(iuiv*((c[0]>> 8)&0xff) + uiv*((c[1]>> 8)&0xff) + iuv*((c[2]>> 8)&0xff) + uv*((c[3]>> 8)&0xff) + 0.5f);
			Rt = (WORD)(iuiv*((c[0]>>16)&0xff) + uiv*((c[1]>>16)&0xff) + iuv*((c[2]>>16)&0xff) + uv*((c[3]>>16)&0xff) + 0.5f);
			At = (WORD)(iuiv*((c[0]>>24)&0xff) + uiv*((c[1]>>24)&0xff) + iuv*((c[2]>>24)&0xff) + uv*((c[3]>>24)&0xff) + 0.5f);
		}

		switch(ctxt->TEX0.TFX)
		{
		case 0:
			Rf = Rf * Rt >> 7;
			Gf = Gf * Gt >> 7;
			Bf = Bf * Bt >> 7;
			Af = ctxt->TEX0.TCC ? (Af * At >> 7) : Af;
			break;
		case 1:
			Rf = Rt;
			Gf = Gt;
			Bf = Bt;
			Af = At;
			break;
		case 2:
			Rf = (Rf * Rt >> 7) + Af;
			Gf = (Gf * Gt >> 7) + Af;
			Bf = (Bf * Bt >> 7) + Af;
			Af = ctxt->TEX0.TCC ? (Af + At) : Af;
			break;
		case 3:
			Rf = (Rf * Rt >> 7) + Af;
			Gf = (Gf * Gt >> 7) + Af;
			Bf = (Bf * Bt >> 7) + Af;
			Af = ctxt->TEX0.TCC ? At : Af;
			break;
		}

		Rf = m_clip[Rf+512];
		Gf = m_clip[Gf+512];
		Bf = m_clip[Bf+512];
		Af = m_clip[Af+512];
	}

	if(m_de.PRIM.FGE)
	{
		BYTE F = (BYTE)v.fog;
		Rf = (F * Rf + (255 - F) * m_de.FOGCOL.FCR) >> 8;
		Gf = (F * Gf + (255 - F) * m_de.FOGCOL.FCG) >> 8;
		Bf = (F * Bf + (255 - F) * m_de.FOGCOL.FCB) >> 8;
	}

	BOOL ZMSK = ctxt->ZBUF.ZMSK;
	DWORD FBMSK = ctxt->FRAME.FBMSK;

	if(ctxt->TEST.ATE)
	{
		bool fPass = true;

		switch(ctxt->TEST.ATST)
		{
		case 0: fPass = false; break;
		case 1: fPass = true; break;
		case 2: fPass = Af < ctxt->TEST.AREF; break;
		case 3: fPass = Af <= ctxt->TEST.AREF; break;
		case 4: fPass = Af == ctxt->TEST.AREF; break;
		case 5: fPass = Af >= ctxt->TEST.AREF; break;
		case 6: fPass = Af > ctxt->TEST.AREF; break;
		case 7: fPass = Af != ctxt->TEST.AREF; break;
		}

		if(!fPass)
		{
			switch(ctxt->TEST.AFAIL)
			{
			case 0: return;
			case 1: ZMSK = 1; break; // RGBA
			case 2: FBMSK = 0xffffffff; break; // Z
			case 3: FBMSK = 0xff000000; ZMSK = 1; break; // RGB
			}
		}
	}

	if(!ZMSK && ctxt->rz)
	{
		(m_lm.*ctxt->wz)(x, y, v.z >> 32, ctxt->ZBUF.ZBP<<5, FBW);
	}

	if((m_de.PRIM.ABE || (m_de.PRIM.PRIM == 1 || m_de.PRIM.PRIM == 2) && m_de.PRIM.AA1) && (!m_de.PABE.PABE || (Af&0x80)))
	{
		GIFRegTEX0 TEX0;
		TEX0.TBP0 = FBP;
		TEX0.TBW = FBW;
		TEX0.TCC = ctxt->TEX0.TCC;
		DWORD Cd = (m_lm.*ctxt->rp)(x, y, TEX0, m_de.TEXA);

		BYTE R[3] = {Rf, (Cd>>16)&0xff, 0};
		BYTE G[3] = {Gf, (Cd>>8)&0xff, 0};
		BYTE B[3] = {Bf, (Cd>>0)&0xff, 0};
		BYTE A[3] = {Af, (Cd>>24)&0xff, ctxt->ALPHA.FIX};

		Rf = ((R[ctxt->ALPHA.A] - R[ctxt->ALPHA.B]) * A[ctxt->ALPHA.C] >> 7) + R[ctxt->ALPHA.D];
		Gf = ((G[ctxt->ALPHA.A] - G[ctxt->ALPHA.B]) * A[ctxt->ALPHA.C] >> 7) + G[ctxt->ALPHA.D];
		Bf = ((B[ctxt->ALPHA.A] - B[ctxt->ALPHA.B]) * A[ctxt->ALPHA.C] >> 7) + B[ctxt->ALPHA.D];
	}

	Rf = m_clamp[Rf];
	Gf = m_clamp[Gf];
	Bf = m_clamp[Bf];
	Af = m_clamp[Af];

	Af |= (ctxt->FBA.FBA << 7);

	DWORD color = ((Af << 24) | (Bf << 16) | (Gf << 8) | (Rf << 0)) & ~FBMSK;

	(m_lm.*ctxt->wf)(x, y, color, FBP, FBW);
}
#endif
void GSStateSoft::SetTexture()
{
	GSDrawingContext* ctxt = &m_de.CTXT[m_de.PRIM.CTXT];

	if(!m_de.PRIM.TME || !ctxt->rt)
		return;

	if(CTexture* p = LookupTexture())
	{
		m_pTexture = p->m_pTexture;
		return;
	}

	InvalidateTexture(ctxt->TEX0.TBP0);

	m_lm.setupCLUT(ctxt->TEX0, m_de.TEXCLUT, m_de.TEXA);

	CAutoPtr<CTexture> p(new CTexture());

	p->m_TEX0 = ctxt->TEX0;
	p->m_TEXA = m_de.TEXA;
	p->m_TEXCLUT = m_de.TEXCLUT;

	int w = 1 << ctxt->TEX0.TW;
	int h = 1 << ctxt->TEX0.TH;

	p->m_pTexture = m_pTexture = new DWORD[w*h];

	DWORD* c = m_pTexture;
	for(int j = 0; j < h; j++)
		for(int i = 0; i < w; i++)
			*c++ = (m_lm.*ctxt->rt)(i, j, ctxt->TEX0, m_de.TEXA);

	m_tc.AddHead(p);
}

GSStateSoft::CTexture* GSStateSoft::LookupTexture()
{
	GSDrawingContext* ctxt = &m_de.CTXT[m_de.PRIM.CTXT];

	POSITION pos = m_tc.GetHeadPosition();
	while(pos)
	{
		CTexture* p = m_tc.GetNext(pos);
		if(p->m_TEX0.i64 == ctxt->TEX0.i64
		&& p->m_TEXA.i64 == m_de.TEXA.i64
		&& p->m_TEXCLUT.i64 == m_de.TEXCLUT.i64)
			return p;
	}

	return NULL;
}