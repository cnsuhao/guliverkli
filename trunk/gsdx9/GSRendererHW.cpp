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
#include "GSRendererHW.h"

#define INTERNALRES 1024

inline BYTE SCALE_ALPHA(BYTE a) 
{
	return (((a)&0x80)?0xff:((a)<<1));
}

static const double log_2pow32 = log(2.0)*32;

//

GSRendererHW::GSRendererHW(HWND hWnd, HRESULT& hr)
	: GSRenderer<HWVERTEX>(INTERNALRES, INTERNALRES, hWnd, hr)
{
	Reset();

	if(m_pD3DDev)
	{
		hr = m_pD3DDev->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
		hr = m_pD3DDev->SetRenderState(D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD);
		hr = m_pD3DDev->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
		hr = m_pD3DDev->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO);
	}

	m_fHalfVRes = !!AfxGetApp()->GetProfileInt(_T("Settings"), _T("HalfVRes"), FALSE);
}

GSRendererHW::~GSRendererHW()
{
}

void GSRendererHW::Reset()
{
	m_primtype = D3DPT_FORCE_DWORD;
	
	m_tc.RemoveAll();

	m_pRenderTargets.RemoveAll();

	POSITION pos = m_pRenderWnds.GetStartPosition();
	while(pos)
	{
		DWORD FBP;
		CGSWnd* pWnd = NULL;
		m_pRenderWnds.GetNextAssoc(pos, FBP, pWnd);
		pWnd->DestroyWindow();
		delete pWnd;
	}
	m_pRenderWnds.RemoveAll();

	__super::Reset();
}

void GSRendererHW::VertexKick(bool fSkip)
{
	HWVERTEX v;

	v.x = ((float)m_v.XYZ.X - m_ctxt->XYOFFSET.OFX)/16;
	v.y = ((float)m_v.XYZ.Y - m_ctxt->XYOFFSET.OFY)/16;
//	v.x = (float)m_v.XYZ.X/16 - (m_ctxt->XYOFFSET.OFX>>4);
//	v.y = (float)m_v.XYZ.Y/16 - (m_ctxt->XYOFFSET.OFY>>4);
	//if(m_v.XYZ.Z && m_v.XYZ.Z < 0x100) m_v.XYZ.Z = 0x100;
	//v.z = 1.0f * (m_v.XYZ.Z>>8)/(UINT_MAX>>8);
	v.z = log(1.0 + m_v.XYZ.Z)/log_2pow32;
	//v.z = (float)m_v.XYZ.Z / UINT_MAX;
	v.rhw = v.z ? 1.0f/v.z : 1.0f; //m_v.RGBAQ.Q;

	BYTE R = m_v.RGBAQ.R;
	BYTE G = m_v.RGBAQ.G;
	BYTE B = m_v.RGBAQ.B;
	BYTE A = SCALE_ALPHA(m_v.RGBAQ.A);
/*
	if(m_de.PRIM.FGE)
	{
		R = (m_v.FOG.F * R + (0xff - m_v.FOG.F) * m_de.FOGCOL.FCR + 127) >> 8;
		G = (m_v.FOG.F * G + (0xff - m_v.FOG.F) * m_de.FOGCOL.FCG + 127) >> 8;
		B = (m_v.FOG.F * B + (0xff - m_v.FOG.F) * m_de.FOGCOL.FCB + 127) >> 8;
	}
*/
	if(m_de.pPRIM->TME)
	{
		A = m_v.RGBAQ.A;

		if(m_de.pPRIM->FST)
		{
			v.tu = (float)m_v.UV.U / (16<<m_ctxt->TEX0.TW);
			v.tv = (float)m_v.UV.V / (16<<m_ctxt->TEX0.TH);
			//v.rhw = 1.0f; // ???
		}
		else if(m_v.RGBAQ.Q != 0)
		{
			v.tu = m_v.ST.S / m_v.RGBAQ.Q;
			v.tv = m_v.ST.T / m_v.RGBAQ.Q;
		}
		else
		{
			v.tu = m_v.ST.S;
			v.tv = m_v.ST.T;
		}
	}

	v.color = D3DCOLOR_ARGB(A, B, G, R);
	v.fog = (m_de.pPRIM->FGE ? m_v.FOG.F : 0xff) << 24;

	m_vl.AddTail(v);

	if(v.x == 496.00 && v.y == 71.50)
	{
		int i = 0;
	}

	__super::VertexKick(fSkip);
}

int GSRendererHW::DrawingKick(bool fSkip)
{
	HWVERTEX* pVertices = &m_pVertices[m_nVertices];
	int nVertices = 0;

	CSize size(m_ctxt->FRAME.FBW*64, m_rs.GetSize(m_rs.IsEnabled(1)?1:0).cy*2);

	switch(m_PRIM)
	{
	case 3: // triangle list
		m_primtype = D3DPT_TRIANGLELIST;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		if(pVertices[nVertices-1].x < 0 && pVertices[nVertices-2].x < 0 && pVertices[nVertices-3].x < 0
		|| pVertices[nVertices-1].x < 0 && pVertices[nVertices-2].x < 0 && pVertices[nVertices-3].x < 0
		|| pVertices[nVertices-1].x >= size.cx && pVertices[nVertices-2].x >= size.cx && pVertices[nVertices-3].x >= size.cx
		|| pVertices[nVertices-1].y >= size.cy && pVertices[nVertices-2].y >= size.cy && pVertices[nVertices-3].y >= size.cy)
			{nVertices -= 3; break;}
		LOGV((pVertices[0], _T("TriList")));
		LOGV((pVertices[1], _T("TriList")));
		LOGV((pVertices[2], _T("TriList")));
		break;
	case 4: // triangle strip
		m_primtype = D3DPT_TRIANGLELIST;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.GetAt(0, pVertices[nVertices++]);
		m_vl.GetAt(1, pVertices[nVertices++]);
		if(pVertices[nVertices-1].x < 0 && pVertices[nVertices-2].x < 0 && pVertices[nVertices-3].x < 0
		|| pVertices[nVertices-1].y < 0 && pVertices[nVertices-2].y < 0 && pVertices[nVertices-3].y < 0
		|| pVertices[nVertices-1].x >= size.cx && pVertices[nVertices-2].x >= size.cx && pVertices[nVertices-3].x >= size.cx
		|| pVertices[nVertices-1].y >= size.cy && pVertices[nVertices-2].y >= size.cy && pVertices[nVertices-3].y >= size.cy)
			{nVertices -= 3; break;}
		LOGV((pVertices[0], _T("TriStrip")));
		LOGV((pVertices[1], _T("TriStrip")));
		LOGV((pVertices[2], _T("TriStrip")));
		break;
	case 5: // triangle fan
		m_primtype = D3DPT_TRIANGLELIST;
		m_vl.GetAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(1, pVertices[nVertices++]);
		m_vl.GetAt(1, pVertices[nVertices++]);
		if(pVertices[nVertices-1].x < 0 && pVertices[nVertices-2].x < 0 && pVertices[nVertices-3].x < 0
		|| pVertices[nVertices-1].y < 0 && pVertices[nVertices-2].y < 0 && pVertices[nVertices-3].y < 0
		|| pVertices[nVertices-1].x >= size.cx && pVertices[nVertices-2].x >= size.cx && pVertices[nVertices-3].x >= size.cx
		|| pVertices[nVertices-1].y >= size.cy && pVertices[nVertices-2].y >= size.cy && pVertices[nVertices-3].y >= size.cy)
			{nVertices -= 3; break;}
		LOGV((pVertices[0], _T("TriFan")));
		LOGV((pVertices[1], _T("TriFan")));
		LOGV((pVertices[2], _T("TriFan")));
		break;
	case 6: // sprite
		m_primtype = D3DPT_TRIANGLELIST;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		if(pVertices[nVertices-1].x < 0 && pVertices[nVertices-2].x < 0
		|| pVertices[nVertices-1].y < 0 && pVertices[nVertices-2].y < 0
		|| pVertices[nVertices-1].x >= size.cx && pVertices[nVertices-2].x >= size.cx
		|| pVertices[nVertices-1].y >= size.cy && pVertices[nVertices-2].y >= size.cy)
			{nVertices -= 2; break;}
		nVertices += 2;

/*
		float lod;
		if(m_ctxt->TEX1.LCM) lod = -log(pVertices[nVertices-1].rhw)/log(2.0f) * (1 << m_ctxt->TEX1.L) + m_ctxt->TEX1.K;
		else lod = m_ctxt->TEX1.K;

		int filter;
		if(lod < 0) filter = m_ctxt->TEX1.MMAG&1;
		else filter = m_ctxt->TEX1.MMIN&1;

//		if(!filter)
		{
			pVertices[nVertices-2].x -= 0.5f;
			pVertices[nVertices-2].y -= 0.5f;
			pVertices[nVertices-1].x -= 0.5f;
			pVertices[nVertices-1].y -= 0.5f;
		}
*/
		// ASSERT(pVertices[0].z == pVertices[1].z);
		pVertices[0].z = pVertices[1].z;
		pVertices[2] = pVertices[1];
		pVertices[3] = pVertices[1];
		pVertices[1].y = pVertices[0].y;
		pVertices[1].tv = pVertices[0].tv;
		pVertices[2].x = pVertices[0].x;
		pVertices[2].tu = pVertices[0].tu;
		LOGV((pVertices[0], _T("Sprite")));
		LOGV((pVertices[1], _T("Sprite")));
		LOGV((pVertices[2], _T("Sprite")));
		LOGV((pVertices[3], _T("Sprite")));
		nVertices += 2;
		pVertices[5] = pVertices[3];
		pVertices[3] = pVertices[1];
		pVertices[4] = pVertices[2];
		break;
	case 1: // line
		m_primtype = D3DPT_LINELIST;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		if(pVertices[nVertices-1].x < 0 && pVertices[nVertices-2].x < 0
		|| pVertices[nVertices-1].y < 0 && pVertices[nVertices-2].y < 0
		|| pVertices[nVertices-1].x >= size.cx && pVertices[nVertices-2].x >= size.cx
		|| pVertices[nVertices-1].y >= size.cy && pVertices[nVertices-2].y >= size.cy)
			{nVertices -= 2; break;}
		LOGV((pVertices[0], _T("LineList")));
		LOGV((pVertices[1], _T("LineList")));
		break;
	case 2: // line strip
		m_primtype = D3DPT_LINELIST;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		m_vl.GetAt(0, pVertices[nVertices++]);
		if(pVertices[nVertices-1].x < 0 && pVertices[nVertices-2].x < 0
		|| pVertices[nVertices-1].y < 0 && pVertices[nVertices-2].y < 0
		|| pVertices[nVertices-1].x >= size.cx && pVertices[nVertices-2].x >= size.cx
		|| pVertices[nVertices-1].y >= size.cy && pVertices[nVertices-2].y >= size.cy)
			{nVertices -= 2; break;}
		LOGV((pVertices[0], _T("LineStrip")));
		LOGV((pVertices[1], _T("LineStrip")));
		break;
	case 0: // point
		m_primtype = D3DPT_POINTLIST;
		m_vl.RemoveAt(0, pVertices[nVertices++]);
		if(pVertices[nVertices-1].x < 0
		|| pVertices[nVertices-1].y < 0
		|| pVertices[nVertices-1].x >= size.cx
		|| pVertices[nVertices-1].y >= size.cy)
			{nVertices--; break;}
		LOGV((pVertices[0], _T("PointList")));
		break;
	default:
		ASSERT(0);
		m_vl.RemoveAll();
		return 0;
	}

	if(fSkip || !m_rs.IsEnabled(0) && !m_rs.IsEnabled(1))
		return 0;

	if(!m_de.pPRIM->IIP)
	{
		pVertices[0].color = pVertices[nVertices-1].color;
		if(m_PRIM == 6) pVertices[3].color = pVertices[5].color;
		/*for(int i = nVertices-1; i > 0; i--)
			pVertices[i-1].color = pVertices[i].color;*/
	}

	return nVertices;
}

void GSRendererHW::FlushPrim()
{
	if(m_nVertices > 0 && (!m_de.pPRIM->TME || m_ctxt->TEX0.TBP0 != m_ctxt->FRAME.Block()))
	{
		int nPrims = 0;

		switch(m_primtype)
		{
		case D3DPT_TRIANGLELIST: ASSERT(!(m_nVertices%3)); nPrims = m_nVertices/3; break;
		case D3DPT_TRIANGLESTRIP: ASSERT(m_nVertices > 2); nPrims = m_nVertices-2; break;
		case D3DPT_TRIANGLEFAN: ASSERT(m_nVertices > 2); nPrims = m_nVertices-2; break;
		case D3DPT_LINELIST: ASSERT(!(m_nVertices&1)); nPrims = m_nVertices/2; break;
		case D3DPT_LINESTRIP: ASSERT(m_nVertices > 1); nPrims = m_nVertices-1; break;
		case D3DPT_POINTLIST: nPrims = m_nVertices; break;
		default: ASSERT(0); return;
		}

		LOG(_T("FlushPrim(pt=%d, nVertices=%d, nPrims=%d)\n"), m_primtype, m_nVertices, nPrims);

		m_stats.IncPrims(nPrims);

		//////////////////////

		HRESULT hr;

		scale_t scale((float)INTERNALRES / (m_ctxt->FRAME.FBW*64), (float)INTERNALRES / m_rs.GetSize(m_rs.IsEnabled(1)?1:0).cy);

// FIXME!!!!!!!!!!!!!!! damnit
if(m_fHalfVRes) 
	scale.y /= 2;
/*
//scale.x = 1;
scale.y = 1;
*/
		//////////////////////

		CComPtr<IDirect3DTexture9> pRT;
		CComPtr<IDirect3DSurface9> pDS;

		bool fClearRT = false;
		bool fClearDS = false;

		if(!m_pRenderTargets.Lookup(m_ctxt->FRAME.Block(), pRT))
		{
			hr = m_pD3DDev->CreateTexture(INTERNALRES, INTERNALRES, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pRT, NULL);
			if(S_OK != hr) {ASSERT(0); return;}
			m_pRenderTargets[m_ctxt->FRAME.Block()] = pRT;
#ifdef DEBUG_RENDERTARGETS
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
			CGSWnd* pWnd = new CGSWnd();
			CString str; str.Format(_T("%05x"), m_ctxt->FRAME.Block());
			pWnd->Create(str);
			m_pRenderWnds[m_ctxt->FRAME.Block()] = pWnd;
			pWnd->Show();
#endif
			fClearRT = true;
		}

		if(!m_pDepthStencils.Lookup(m_ctxt->ZBUF.ZBP, pDS))
		{
			hr = m_pD3DDev->CreateDepthStencilSurface(INTERNALRES, INTERNALRES, m_fmtDepthStencil, D3DMULTISAMPLE_NONE, 0, FALSE, &pDS, NULL);
			if(S_OK != hr) {ASSERT(0); return;}
			m_pDepthStencils[m_ctxt->ZBUF.ZBP] = pDS;
			fClearDS = true;
		}

		if(!pRT || !pDS) {ASSERT(0); return;}

		scale.Set(pRT);

		//////////////////////

		GSTexture t;
		D3DSURFACE_DESC td;
		ZeroMemory(&td, sizeof(td));

		if(m_de.pPRIM->TME)
		{
			if(m_caps.PixelShaderVersion >= D3DVS_VERSION(2, 0))
			{
				if(m_tc.FetchPal(this, t))
				{
					hr = t.m_pTexture->GetLevelDesc(0, &td);

					if(nPrims > 100 && t.m_pPalette) // TODO: find the optimal value for nPrims > ?
					{
						CComPtr<IDirect3DTexture9> pRT;
						hr = m_pD3DDev->CreateTexture(td.Width, td.Height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pRT, NULL);

						CComPtr<IDirect3DSurface9> pRTSurf;
						hr = pRT->GetSurfaceLevel(0, &pRTSurf);
						hr = m_pD3DDev->SetRenderTarget(0, pRTSurf);
						hr = m_pD3DDev->SetDepthStencilSurface(NULL);

						hr = m_pD3DDev->SetTexture(0, t.m_pTexture);
						hr = m_pD3DDev->SetTexture(1, t.m_pPalette);
						hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
						hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
						hr = m_pD3DDev->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
						hr = m_pD3DDev->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_POINT);
						hr = m_pD3DDev->SetRenderState(D3DRS_ZENABLE, FALSE);
						hr = m_pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
						hr = m_pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
						hr = m_pD3DDev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
						hr = m_pD3DDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
						hr = m_pD3DDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
						hr = m_pD3DDev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
						hr = m_pD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, 15);

						hr = m_pD3DDev->SetPixelShader(m_pPixelShaderTFX[13]);

						struct
						{
							float x, y, z, rhw;
							float tu, tv;
						}
						pVertices[] =
						{
							{0, 0, 0.5f, 2.0f, 0, 0},
							{td.Width, 0, 0.5f, 2.0f, 1, 0},
							{0, td.Height, 0.5f, 2.0f, 0, 1},
							{td.Width, td.Height, 0.5f, 2.0f, 1, 1},
						};

						hr = m_pD3DDev->BeginScene();
						hr = m_pD3DDev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
						hr = m_pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pVertices, sizeof(pVertices[0]));
						hr = m_pD3DDev->EndScene();

						t.m_pTexture = pRT;
						t.m_pPalette = NULL;
/*
if(m_stats.GetFrame() > 1000)
{
		CString fn;
		fn.Format(_T("c:\\%08I64x_%I64d_%I64d_%I64d_%I64d_%I64d_%I64d_%I64d-%I64d_%I64d-%I64d.bmp"), 
			t.m_TEX0.TBP0, t.m_TEX0.PSM, t.m_TEX0.TBW, 
			t.m_TEX0.TW, t.m_TEX0.TH,
			t.m_CLAMP.WMS, t.m_CLAMP.WMT, t.m_CLAMP.MINU, t.m_CLAMP.MAXU, t.m_CLAMP.MINV, t.m_CLAMP.MAXV);
		D3DXSaveTextureToFile(fn, D3DXIFF_BMP, pRT, NULL);
}
*/
					}
				}
			}
			else
			{
				if(m_tc.Fetch(this, t))
				{
					hr = t.m_pTexture->GetLevelDesc(0, &td);
				}
			}
		}

		//////////////////////

		hr = m_pD3DDev->BeginScene();

		//////////////////////

		CComPtr<IDirect3DSurface9> pSurf;
		hr = pRT->GetSurfaceLevel(0, &pSurf);
		hr = m_pD3DDev->SetRenderTarget(0, pSurf);
		hr = m_pD3DDev->SetDepthStencilSurface(pDS);
		if(fClearRT) hr = m_pD3DDev->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
		if(fClearDS) hr = m_pD3DDev->Clear(0, NULL, D3DCLEAR_ZBUFFER, 0, 1.0f, 0);

		D3DSURFACE_DESC rd;
		ZeroMemory(&rd, sizeof(rd));
		pRT->GetLevelDesc(0, &rd);

		//////////////////////

		hr = m_pD3DDev->SetRenderState(D3DRS_SHADEMODE, m_de.pPRIM->IIP ? D3DSHADE_GOURAUD : D3DSHADE_FLAT);

		//////////////////////

		float tsx = 1.0f, tsy = 1.0f;

		if(m_de.pPRIM->TME)
		{
			tsx = 1.0f * (1<<m_ctxt->TEX0.TW) / td.Width * t.m_scale.x;
			tsy = 1.0f * (1<<m_ctxt->TEX0.TH) / td.Height * t.m_scale.y;
		}

		SetupTexture(t, tsx, tsy);

		//////////////////////

		SetupAlphaBlend();

		//////////////////////

		// close approx., to be tested...
		int mask = D3DCOLORWRITEENABLE_ALPHA|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED;
		if((m_ctxt->FRAME.FBMSK&0xff000000) == 0xff000000) mask &= ~D3DCOLORWRITEENABLE_ALPHA;
		if((m_ctxt->FRAME.FBMSK&0x00ff0000) == 0x00ff0000) mask &= ~D3DCOLORWRITEENABLE_BLUE;
		if((m_ctxt->FRAME.FBMSK&0x0000ff00) == 0x0000ff00) mask &= ~D3DCOLORWRITEENABLE_GREEN;
		if((m_ctxt->FRAME.FBMSK&0x000000ff) == 0x000000ff) mask &= ~D3DCOLORWRITEENABLE_RED;
		//if(m_ctxt->FRAME.PSM == PSM_PSMCT24) mask &= ~D3DCOLORWRITEENABLE_ALPHA;
		hr = m_pD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, mask);

		// ASSERT(m_ctxt->FRAME.FBMSK == 0); // wild arms (also 8H+pal on RT...)

		//////////////////////

		hr = m_pD3DDev->SetRenderState(D3DRS_ZENABLE, m_ctxt->TEST.ZTE);
		hr = m_pD3DDev->SetRenderState(D3DRS_ZWRITEENABLE, !m_ctxt->ZBUF.ZMSK);
		if(m_ctxt->TEST.ZTE)
		{
			DWORD zfunc[] = {D3DCMP_NEVER, D3DCMP_ALWAYS, D3DCMP_GREATEREQUAL, D3DCMP_GREATER};
			hr = m_pD3DDev->SetRenderState(D3DRS_ZFUNC, zfunc[m_ctxt->TEST.ZTST]);
//			hr = m_pD3DDev->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);

			// FIXME
			if(m_ctxt->ZBUF.ZMSK && m_ctxt->TEST.ZTST == 1)
			{
				HWVERTEX* pVertices = m_pVertices;
				for(int i = m_nVertices; i-- > 0; pVertices++)
					pVertices->z = 0;
			}
		}

		//////////////////////

		hr = m_pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, m_ctxt->TEST.ATE); 
		if(m_ctxt->TEST.ATE)
		{
			DWORD afunc[] =
			{
				D3DCMP_NEVER, D3DCMP_ALWAYS, D3DCMP_LESS, D3DCMP_LESSEQUAL, 
				D3DCMP_EQUAL, D3DCMP_GREATEREQUAL, D3DCMP_GREATER, D3DCMP_NOTEQUAL
			};

			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAFUNC, afunc[m_ctxt->TEST.ATST]);
			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAREF, (DWORD)SCALE_ALPHA(m_ctxt->TEST.AREF));
		}

		//////////////////////

		hr = m_pD3DDev->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
		CRect scissor(scale.x * m_ctxt->SCISSOR.SCAX0, scale.y * m_ctxt->SCISSOR.SCAY0, scale.x * (m_ctxt->SCISSOR.SCAX1+1), scale.y * (m_ctxt->SCISSOR.SCAY1+1));
		scissor.IntersectRect(scissor, CRect(0, 0, INTERNALRES, INTERNALRES));
		hr = m_pD3DDev->SetScissorRect(scissor);

		//////////////////////

		// ASSERT(!m_de.PABE.PABE); // bios
		// ASSERT(!m_ctxt->FBA.FBA); // bios
		// ASSERT(!m_ctxt->TEST.DATE); // sfex3 (after the capcom logo), vf4 (first menu fading in)

		//////////////////////

if(m_de.pPRIM->TME && (m_ctxt->FRAME.Block()) == 0x01e00 && m_ctxt->TEX0.TBP0 == 0x00000)
{
	if(m_stats.GetFrame() > 1200)
	{
		//hr = D3DXSaveTextureToFile(_T("c:\\rtbefore.bmp"), D3DXIFF_BMP, pRT, NULL);
	}
}

		{
			// hr = m_pD3DDev->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
			// hr = m_pD3DDev->SetTexture(1, pRT);

			HWVERTEX* pVertices = m_pVertices;
			for(int i = m_nVertices; i-- > 0; pVertices++)
			{
				pVertices->x *= scale.x;
				pVertices->y *= scale.y;

				// pVertices->tu2 = pVertices->x / rd.Width;
				// pVertices->tv2 = pVertices->y / rd.Height;

				if(m_de.pPRIM->TME)
				{
					float base, fract;
					fract = modf(pVertices->tu, &base);
					fract *= tsx;
					ASSERT(-1 <= fract && fract <= 1.01);
					pVertices->tu = base + fract;
					fract = modf(pVertices->tv, &base);
					fract *= tsy;
					//ASSERT(-1 <= fract && fract <= 1.01);
					pVertices->tv = base + fract;
				}
/*
				if(m_de.pPRIM->TME)
				{
					pVertices->tu *= tsx;
					pVertices->tv *= tsy;
				}
*/
				if(m_de.pPRIM->FGE)
				{
					pVertices->fog = D3DCOLOR_ARGB(pVertices->fog>>24, m_de.FOGCOL.FCB, m_de.FOGCOL.FCG, m_de.FOGCOL.FCR);
				}
			}
		}

		hr = m_pD3DDev->SetFVF(D3DFVF_HWVERTEX);

		if(1)//!m_de.PABE.PABE)
		{
			hr = m_pD3DDev->DrawPrimitiveUP(m_primtype, nPrims, m_pVertices, sizeof(HWVERTEX));
		}
/*		else
		{
			ASSERT(!m_ctxt->TEST.ATE); // TODO

			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE); 
			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAREF, 0xfe);

			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			hr = m_pD3DDev->DrawPrimitive(m_primtype, 0, nPrims);

			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_LESS);
			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
			hr = m_pD3DDev->DrawPrimitive(m_primtype, 0, nPrims);
		}
*/
		if(m_ctxt->TEST.ATE && m_ctxt->TEST.AFAIL && m_ctxt->TEST.ATST != 1)
		{
			ASSERT(!m_de.PABE.PABE);

			DWORD iafunc[] =
			{
				D3DCMP_ALWAYS, D3DCMP_NEVER, D3DCMP_GREATEREQUAL, D3DCMP_GREATER, 
				D3DCMP_NOTEQUAL, D3DCMP_LESS, D3DCMP_LESSEQUAL, D3DCMP_EQUAL
			};

			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAFUNC, iafunc[m_ctxt->TEST.ATST]);
			hr = m_pD3DDev->SetRenderState(D3DRS_ALPHAREF, (DWORD)SCALE_ALPHA(m_ctxt->TEST.AREF));

			int mask = 0;
			bool zwrite = false;

			switch(m_ctxt->TEST.AFAIL)
			{
			case 0: break; // keep
			case 1: mask = D3DCOLORWRITEENABLE_ALPHA|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED; break; // fbuf
			case 2: zwrite = true; break; // zbuf
			case 3: mask = D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED; break; // fbuf w/o alpha
			}

			hr = m_pD3DDev->SetRenderState(D3DRS_ZWRITEENABLE, zwrite);
			hr = m_pD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, mask);

			if(mask || zwrite)
				hr = m_pD3DDev->DrawPrimitiveUP(m_primtype, nPrims, m_pVertices, sizeof(HWVERTEX));
		}

if(m_de.pPRIM->TME && /*(m_ctxt->FRAME.Block()) == 0x00000 &&*/ m_ctxt->TEX0.TBP0 == 0x02320)
{
	if(m_stats.GetFrame() > 1200)
	{
		//hr = D3DXSaveTextureToFile(_T("c:\\rtafter.bmp"), D3DXIFF_BMP, pRT, NULL);
		//if(t.m_pTexture) hr = D3DXSaveTextureToFile(_T("c:\\tx.bmp"), D3DXIFF_BMP, t.m_pTexture, NULL);
	}
}
		hr = m_pD3DDev->EndScene();

		//////////////////////

		m_tc.AddRT(m_ctxt->FRAME.Block(), pRT, scale);
	}

	m_primtype = D3DPT_FORCE_DWORD;

	__super::FlushPrim();
}

void GSRendererHW::Flip()
{
	HRESULT hr;

	FlipSrc rt[2];

	for(int i = 0; i < countof(rt); i++)
	{
		if(!m_rs.IsEnabled(i)) continue;

		DWORD FBP = m_rs.DISPFB[i].FBP<<5;

#ifdef DEBUG_RENDERTARGETS
		if(::GetAsyncKeyState(VK_SPACE)&0x80000000) FBP = m_ctxt->FRAME.Block();
#endif

		CSurfMap<IDirect3DTexture9>::CPair* pPair = m_pRenderTargets.PLookup(FBP);

		if(!pPair)
		{
			for(CSurfMap<IDirect3DTexture9>::CPair* pPair2 = m_pRenderTargets.PGetFirstAssoc(); 
				pPair2; 
				pPair2 = m_pRenderTargets.PGetNextAssoc(pPair2))
			{
				if(pPair2->key <= FBP && (!pPair || pPair2->key >= pPair->key))
				{
					pPair = pPair2;
				}
			}
		}

		if(pPair)
		{
			m_tc.ResetAge(pPair->key);

			rt[i].pRT = pPair->value;

			ZeroMemory(&rt[i].rd, sizeof(rt[i].rd));
			hr = rt[i].pRT->GetLevelDesc(0, &rt[i].rd);

			rt[i].scale.Get(rt[i].pRT);

			CSize size = m_rs.GetSize(i);
			rt[i].src = CRect(0, 0, rt[i].scale.x*size.cx, rt[i].scale.y*size.cy);
		}
	}

	bool fShiftField = m_rs.SMODE2.INT && !!(m_ctxt->XYOFFSET.OFY&0xf);
		// m_pCSRr->FIELD && m_rs.SMODE2.INT /*&& !m_rs.SMODE2.FFMD*/;

	FinishFlip(rt, fShiftField);

#ifdef DEBUG_RENDERTARGETS
	CRect dst(0, 0, m_bd.Width, m_bd.Height);

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if(!m_pRenderWnds.IsEmpty())
	{
		POSITION pos = m_pRenderWnds.GetStartPosition();
		while(pos)
		{
			DWORD fbp;
			CGSWnd* pWnd = NULL;
			m_pRenderWnds.GetNextAssoc(pos, fbp, pWnd);

			CComPtr<IDirect3DTexture9> pRT;
			if(m_pRenderTargets.Lookup(fbp, pRT))
			{
				D3DSURFACE_DESC rd;
				ZeroMemory(&rd, sizeof(rd));
				hr = pRT->GetLevelDesc(0, &rd);

				hr = m_pD3DDev->SetTexture(0, pRT);
				hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
				hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
				hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);

				scale_t scale(pRT);

				CSize size = m_rs.GetSize(m_rs.IsEnabled(1)?1:0);
				CRect src = CRect(0, 0, scale.x*size.cx, scale.y*size.cy);

				struct
				{
					float x, y, z, rhw;
					float tu, tv;
				}
				pVertices[] =
				{
					{(float)dst.left, (float)dst.top, 0.5f, 2.0f, (float)src.left / rd.Width, (float)src.top / rd.Height},
					{(float)dst.right, (float)dst.top, 0.5f, 2.0f, (float)src.right / rd.Width, (float)src.top / rd.Height},
					{(float)dst.left, (float)dst.bottom, 0.5f, 2.0f, (float)src.left / rd.Width, (float)src.bottom / rd.Height},
					{(float)dst.right, (float)dst.bottom, 0.5f, 2.0f, (float)src.right / rd.Width, (float)src.bottom / rd.Height},
				};

				for(int i = 0; i < countof(pVertices); i++)
				{
					pVertices[i].x -= 0.5;
					pVertices[i].y -= 0.5;
				}

				hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
				hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
				hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

				hr = m_pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

				hr = m_pD3DDev->SetPixelShader(NULL);

				hr = m_pD3DDev->BeginScene();
				hr = m_pD3DDev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
				hr = m_pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pVertices, sizeof(pVertices[0]));
				hr = m_pD3DDev->EndScene();

				hr = m_pD3DDev->Present(NULL, NULL, pWnd->m_hWnd, NULL);

				hr = m_pD3DDev->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);

				CString str;
				str.Format(_T("PCSX2 - %05x - %05x"), fbp, m_ctxt->TEX0.TBP0);
				if(fbp == (m_ctxt->FRAME.Block()))
				{
					// pWnd->SetFocus();
					str += _T(" - Drawing");
				}
				pWnd->SetWindowText(str);

				MSG msg;
				ZeroMemory(&msg, sizeof(msg));
				while(msg.message != WM_QUIT)
				{
					if(PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
					{
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
#ifdef DEBUG_RENDERTARGETS
					else if(!(::GetAsyncKeyState(VK_RCONTROL)&0x80000000))
					{
						break;
					}
#endif
				}

#ifdef DEBUG_RENDERTARGETS
				if(::GetAsyncKeyState(VK_LCONTROL)&0x80000000)
					Sleep(500);
#endif
			}
			else
			{
				if(IsWindow(pWnd->m_hWnd)) pWnd->DestroyWindow();
				m_pRenderWnds.RemoveKey(fbp);
			}

			CString str;
			str.Format(_T("PCSX2 - %05x - %05x"), m_ctxt->FRAME.Block(), m_ctxt->TEX0.TBP0);
			SetWindowText(m_hWnd, str);
		}
	}
	else
	{
		SetWindowText(m_hWnd, _T("PCSX2"));
	}
#endif
}

void GSRendererHW::EndFrame()
{
	m_tc.IncAge(m_pRenderTargets);
}

void GSRendererHW::InvalidateTexture(DWORD TBP0, DWORD PSM, CRect r)
{
	m_tc.Invalidate(this, TBP0, PSM, &r);

	//CRect r(m_rs.TRXPOS.DSAX, y, m_rs.TRXREG.RRW, min(m_x == m_rs.TRXPOS.DSAX ? m_y : m_y+1, m_rs.TRXREG.RRH));

/*
	GSTexture t;
	if(m_tc.LookupByTBP(TBP0, t))
	{
		HRESULT hr;
		D3DLOCKED_RECT lr;

		D3DSURFACE_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		t.m_pTexture->GetLevelDesc(0, &desc);

		if(!t.m_fRT)
		{
			int w = r.right, h = r.bottom;

			if(m_rs.TRXREG.RRW <= 16 && m_rs.TRXREG.RRH <= 16 // allowing larger isn't worth, most of the time the size of the texture changes too
			&& !(t.m_tex.CLAMP.WMS&2) && !(t.m_tex.CLAMP.WMT&2)
			&& t.m_scale.x == 1.0f && t.m_scale.y == 1.0f
			&& desc.Width >= w && desc.Height >= h
			&& S_OK == t.m_pTexture->LockRect(0, &lr, r, 0))
			{
				m_lm.setupCLUT(m_ctxt->TEX0, m_de.TEXA);

				m_lm.ReadTexture(w, h, (BYTE*)lr.pBits, lr.Pitch, m_ctxt->TEX0, m_de.TEXA, t.m_tex.CLAMP);

				t.m_pTexture->UnlockRect(0);

				m_stats.IncReads(w*h*4);
			}
			else
			{
				m_tc.InvalidateByTBP(TBP0);
			}
		}
		else if(m_rs.BITBLTBUF.DPSM == PSM_PSMCT32 || m_rs.BITBLTBUF.DPSM == PSM_PSMCT16 || m_rs.BITBLTBUF.DPSM == PSM_PSMCT16S)
		{
			CComPtr<IDirect3DTexture9> pTexture;
			hr = m_pD3DDev->CreateTexture(r.Width(), r.Height(), 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pTexture, NULL);

			if(pTexture && S_OK == pTexture->LockRect(0, &lr, NULL, 0))
			{
				GIFRegTEX0 TEX0;
				TEX0.TBP0 = m_rs.BITBLTBUF.DBP;
				TEX0.TBW = m_rs.BITBLTBUF.DBW;
				TEX0.PSM = m_rs.BITBLTBUF.DPSM;
				TEX0.TCC = 0;

				GIFRegTEXA TEXA;
				TEXA.TA1 = 0x80;
				TEXA.TA0 = 0;
				TEXA.AEM = 1;

				GSLocalMemory::readTexel rt = m_lm.GetReadTexel(m_rs.BITBLTBUF.DBP);

				if(r.top == 0 && r.left == 0)
				{
					GSLocalMemory::unSwizzleTexture st = m_lm.GetUnSwizzleTexture(m_rs.BITBLTBUF.DPSM);
					(m_lm.*st)(r.Width(), r.Height(), (BYTE*)lr.pBits, lr.Pitch, TEX0, TEXA);
				}
				else // FIXME
				{
					BYTE* dst = (BYTE*)lr.pBits;

					for(int y = r.top, diff = lr.Pitch - r.Width()*4; y < r.bottom; y++, dst += diff)
						for(int x = r.left; x < r.right; x++, dst += 4)
							*(DWORD*)dst = (m_lm.*rt)(x, y, TEX0, TEXA);
				}

				pTexture->UnlockRect(0);

				m_stats.IncReads(r.Width()*r.Height()*4);

				struct
				{
					float x, y, z, rhw;
					float tu, tv;
				}
				pVertices[] =
				{
					{(float)r.left, (float)r.top, 0.5f, 2.0f, 0, 0},
					{(float)r.right, (float)r.top, 0.5f, 2.0f, 1.0f, 0},
					{(float)r.left, (float)r.bottom, 0.5f, 2.0f, 0, 1.0f},
					{(float)r.right, (float)r.bottom, 0.5f, 2.0f, 1.0f, 1.0f},
				};

				scale_t scale(t.m_pTexture);

				for(int i = 0; i < countof(pVertices); i++)
				{
					pVertices[i].x = pVertices[i].x * scale.x - 0.5;
					pVertices[i].y = pVertices[i].y * scale.y - 0.5;
				}

				CComPtr<IDirect3DSurface9> pSurf;
				hr = t.m_pTexture->GetSurfaceLevel(0, &pSurf);
				hr = m_pD3DDev->SetRenderTarget(0, pSurf);

			    hr = m_pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
				hr = m_pD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED);
				hr = m_pD3DDev->SetRenderState(D3DRS_ZENABLE, FALSE);
				hr = m_pD3DDev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
				hr = m_pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE); 
				hr = m_pD3DDev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

				hr = m_pD3DDev->SetTexture(0, pTexture);
				hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
				hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
				hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
				hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
				hr = m_pD3DDev->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
				hr = m_pD3DDev->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
				hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
				hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
				hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
				hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);

				hr = m_pD3DDev->BeginScene();
				hr = m_pD3DDev->SetPixelShader(NULL);
				hr = m_pD3DDev->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);
				hr = m_pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pVertices, sizeof(pVertices[0]));
				hr = m_pD3DDev->EndScene();
			}
		}
		else
		{
			m_tc.InvalidateByTBP(TBP0);
		}
	}

	m_tc.InvalidateByCBP(TBP0);
*/
}

void GSRendererHW::MaxTexUV(int& tw, int& th)
{
	if(m_ctxt->CLAMP.WMS < 3 && m_ctxt->CLAMP.WMT < 3)
	{
		uvmm_t uv;
		UVMinMax(m_nVertices, (vertex_t*)m_pVertices, &uv);

		if(m_ctxt->CLAMP.WMS == 0)
		{
			float fmin = floor(uv.umin);
			float fmax = floor(uv.umax);

			if(fmin != fmax) {uv.umin = 0; uv.umax = 1.0f;}
			else {uv.umin -= fmin; uv.umax -= fmax;}

			// FIXME
			if(uv.umin == 0 && uv.umax != 1.0f) uv.umax = 1.0f;
		}
		else if(m_ctxt->CLAMP.WMS == 1)
		{
			if(uv.umin < 0) uv.umin = 0;
			if(uv.umax > 1.0f) uv.umax = 1.0f;
		}
		else if(m_ctxt->CLAMP.WMS == 2)
		{
			float minu = 1.0f * m_ctxt->CLAMP.MINU / (1<<m_ctxt->TEX0.TW);
			float maxu = 1.0f * m_ctxt->CLAMP.MAXU / (1<<m_ctxt->TEX0.TW);
			if(uv.umin < minu) uv.umin = minu;
			if(uv.umax > maxu) uv.umax = maxu;
		}

		if(m_ctxt->CLAMP.WMT == 0)
		{
			float fmin = floor(uv.vmin);
			float fmax = floor(uv.vmax);

			if(fmin != fmax) {uv.vmin = 0; uv.vmax = 1.0f;}
			else {uv.vmin -= fmin; uv.vmax -= fmax;}

			// FIXME
			if(uv.vmin == 0 && uv.vmax != 1.0f) uv.vmax = 1.0f;
		}
		else if(m_ctxt->CLAMP.WMT == 1)
		{
			if(uv.vmin < 0) uv.vmin = 0;
			if(uv.vmax > 1.0f) uv.vmax = 1.0f;
		}
		else if(m_ctxt->CLAMP.WMT == 2)
		{
			float minv = 1.0f * m_ctxt->CLAMP.MINV / (1<<m_ctxt->TEX0.TH);
			float maxv = 1.0f * m_ctxt->CLAMP.MAXV / (1<<m_ctxt->TEX0.TH);
			if(uv.vmin < minv) uv.vmin = minv;
			if(uv.vmax > maxv) uv.vmax = maxv;
		}

		uv.umin *= tw;
		uv.umax *= tw;
		uv.vmin *= th;
		uv.vmax *= th;
/*
		// TODO
		// tx = ;
		// ty = ;
		tw = min(((int)uv.umax + 1 + 31) & ~31, tw);
		th = min(((int)uv.vmax + 1 + 15) & ~15, th);
*/
		CSize size = GSLocalMemory::GetBlockSize(m_ctxt->TEX0.PSM);
		tw = min(((int)uv.umax + (size.cx-1) + 1) & ~(size.cx-1), tw);
		th = min(((int)uv.vmax + (size.cy-1) + 1) & ~(size.cy-1), th);
	}
}

void GSRendererHW::SetupTexture(const GSTexture& t, float tsx, float tsy)
{
	HRESULT hr;

	// FIXME
	int tw = 1 << t.m_TEX0.TW;
	int th = 1 << t.m_TEX0.TH;
	float rw = 1.0f / tw;
	float rh = 1.0f / th;

	CComPtr<IDirect3DPixelShader9> pPixelShader;

	if(m_de.pPRIM->TME && t.m_pTexture)
	{
		hr = m_pD3DDev->SetTexture(0, t.m_pTexture);
		hr = m_pD3DDev->SetTexture(1, t.m_pPalette);

		D3DTEXTUREADDRESS u, v;

		switch(m_ctxt->CLAMP.WMS)
		{
		case 0: case 3: u = D3DTADDRESS_WRAP; break; // repeat
		case 1: case 2: u = D3DTADDRESS_CLAMP; break; // clamp
		}

		switch(m_ctxt->CLAMP.WMT)
		{
		case 0: case 3: v = D3DTADDRESS_WRAP; break; // repeat
		case 1: case 2: v = D3DTADDRESS_CLAMP; break; // clamp
		}

		hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, u);
		hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, v);

		hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_MAGFILTER, t.m_pPalette ? D3DTEXF_POINT : D3DTEXF_LINEAR);
		hr = m_pD3DDev->SetSamplerState(0, D3DSAMP_MINFILTER, t.m_pPalette ? D3DTEXF_POINT : D3DTEXF_LINEAR);
		hr = m_pD3DDev->SetSamplerState(1, D3DSAMP_MAGFILTER, t.m_pPalette ? D3DTEXF_POINT : D3DTEXF_LINEAR);
		hr = m_pD3DDev->SetSamplerState(1, D3DSAMP_MINFILTER, t.m_pPalette ? D3DTEXF_POINT : D3DTEXF_LINEAR);

		if(!pPixelShader && m_caps.PixelShaderVersion >= D3DVS_VERSION(2, 0) && m_pPixelShaderTFX[m_ctxt->TEX0.TFX])
		{
			int i = m_ctxt->TEX0.TFX;
			if(t.m_pPalette) i += 4;
			pPixelShader = m_pPixelShaderTFX[i];
		}

		if(!pPixelShader && m_caps.PixelShaderVersion >= D3DVS_VERSION(1, 1))
		{
			switch(m_ctxt->TEX0.TFX)
			{
			case 0:
				if(!m_ctxt->TEX0.TCC) pPixelShader = m_pPixelShaders[0];
				else if(!t.m_fRT) pPixelShader = m_pPixelShaders[1];
				else pPixelShader = m_pPixelShaders[2];
				break;
			case 1:
				if(!t.m_fRT) pPixelShader = m_pPixelShaders[3];
				else pPixelShader = m_pPixelShaders[4];
				break;
			case 2:
				if(!m_ctxt->TEX0.TCC) pPixelShader = m_pPixelShaders[5];
				else if(!t.m_fRT) pPixelShader = m_pPixelShaders[6];
				else pPixelShader = m_pPixelShaders[7];
				break;
			case 3:
				if(!m_ctxt->TEX0.TCC) pPixelShader = m_pPixelShaders[8];
				else if(!t.m_fRT) pPixelShader = m_pPixelShaders[9];
				else pPixelShader = m_pPixelShaders[10];
				break;
			}
		}

		if(!pPixelShader)
		{
			int stage = 0;

			hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
			hr = m_pD3DDev->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 0);

			switch(m_ctxt->TEX0.TFX)
			{
			case 0:
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLOROP, D3DTOP_MODULATE2X);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLORARG1, D3DTA_TEXTURE);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
				if(m_ctxt->TEX0.TCC)
				{
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAOP, t.m_fRT ? D3DTOP_MODULATE2X : D3DTOP_MODULATE4X);
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
				}
				else
				{
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAOP, D3DTOP_ADD);
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
				}
				stage++;
				break;
			case 1:
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLORARG1, D3DTA_TEXTURE);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAOP, t.m_fRT ? D3DTOP_SELECTARG1 : D3DTOP_ADD);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAARG2, D3DTA_TEXTURE);
				stage++;
				break;
			case 2:
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLOROP, D3DTOP_MODULATE2X);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLORARG1, D3DTA_TEXTURE);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
				if(m_ctxt->TEX0.TCC)
				{
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAOP, D3DTOP_ADD);
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAARG2, D3DTA_TEXTURE);
					ASSERT(!t.m_fRT); // FIXME
				}
				stage++;
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLOROP, D3DTOP_ADD);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLORARG1, D3DTA_CURRENT);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLORARG2, D3DTA_ALPHAREPLICATE|D3DTA_DIFFUSE);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAOP, D3DTOP_ADD);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAARG1, D3DTA_CURRENT);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
				stage++;
				break;
			case 3:
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLOROP, D3DTOP_MODULATE2X);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLORARG1, D3DTA_TEXTURE);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
				if(m_ctxt->TEX0.TCC)
				{
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
					hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
					ASSERT(!t.m_fRT); // FIXME
				}
				stage++;
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLOROP, D3DTOP_ADD);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLORARG1, D3DTA_CURRENT);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLORARG2, D3DTA_ALPHAREPLICATE|D3DTA_DIFFUSE);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAOP, D3DTOP_ADD);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAARG1, D3DTA_CURRENT);
				hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
				stage++;
				break;
			}

			hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_COLOROP, D3DTOP_DISABLE);
			hr = m_pD3DDev->SetTextureStageState(stage, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		}
	}
	else
	{
		hr = m_pD3DDev->SetTexture(0, NULL);
		hr = m_pD3DDev->SetTexture(1, NULL);

		if(!pPixelShader && m_caps.PixelShaderVersion >= D3DVS_VERSION(2, 0) && m_pPixelShaderTFX[12])
		{
			pPixelShader = m_pPixelShaderTFX[12];
		}

		if(!pPixelShader && m_caps.PixelShaderVersion >= D3DVS_VERSION(1, 1) && m_pPixelShaders[11])
		{
			pPixelShader = m_pPixelShaders[11];
		}

		if(!pPixelShader)
		{
			hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_DISABLE);
			hr = m_pD3DDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		}
	}

	float fConstData[][4] = 
	{
		{(float)m_ctxt->TEX0.TFX, (float)m_ctxt->TEX0.TCC, (float)t.m_fRT, (float)!!(m_de.pPRIM->TME && t.m_pTexture)},
		{(float)m_ctxt->TEX0.PSM, (float)m_de.TEXA.AEM, (float)m_de.TEXA.TA0 / 255, (float)m_de.TEXA.TA1 / 255},
		{(float)tw, (float)th, 0, 0},
		{rw, rh, 0, 0},
		{rw, 0, 0, 0},
		{0, rh, 0, 0},
	};

	hr = m_pD3DDev->SetPixelShaderConstantF(0, (float*)fConstData, countof(fConstData));

	hr = m_pD3DDev->SetPixelShader(pPixelShader);
}

void GSRendererHW::SetupAlphaBlend()
{
	HRESULT hr;

	DWORD ABE = FALSE;
	hr = m_pD3DDev->GetRenderState(D3DRS_ALPHABLENDENABLE, &ABE);

	bool fABE = m_de.pPRIM->ABE || (m_primtype == D3DPT_LINELIST || m_primtype == D3DPT_LINESTRIP) && m_de.pPRIM->AA1; // FIXME
	if(fABE != !!ABE)
		hr = m_pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, fABE);
	if(fABE)
	{
		// (A:Cs/Cd/0 - B:Cs/Cd/0) * C:As/Ad/FIX + D:Cs/Cd/0

		BYTE FIX = SCALE_ALPHA(m_ctxt->ALPHA.FIX);

		hr = m_pD3DDev->SetRenderState(D3DRS_BLENDFACTOR, (0x010101*FIX)|(FIX<<24));

		D3DBLENDOP op = D3DBLENDOP_ADD;
		D3DBLEND src = D3DBLEND_SRCALPHA, dst = D3DBLEND_INVSRCALPHA;

		static const struct {bool bogus; D3DBLENDOP op; D3DBLEND src, dst;} blendmap[3*3*3*3] =
		{
			{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 0000: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cs ==> Cs
			{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 0001: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cd ==> Cd
			{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 0002: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + 0 ==> 0
			{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 0010: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cs ==> Cs
			{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 0011: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cd ==> Cd
			{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 0012: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + 0 ==> 0
			{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 0020: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cs ==> Cs
			{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 0021: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cd ==> Cd
			{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 0022: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + 0 ==> 0
			{1, D3DBLENDOP_SUBTRACT, D3DBLEND_SRCALPHA, D3DBLEND_SRCALPHA},			// * 0100: (Cs - Cd)*As + Cs ==> Cs*(As + 1) - Cd*As
			{0, D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA},			// 0101: (Cs - Cd)*As + Cd ==> Cs*As + Cd*(1 - As)
			{0, D3DBLENDOP_SUBTRACT, D3DBLEND_SRCALPHA, D3DBLEND_SRCALPHA},			// 0102: (Cs - Cd)*As + 0 ==> Cs*As - Cd*As
			{1, D3DBLENDOP_SUBTRACT, D3DBLEND_DESTALPHA, D3DBLEND_DESTALPHA},		// * 0110: (Cs - Cd)*Ad + Cs ==> Cs*(Ad + 1) - Cd*Ad
			{0, D3DBLENDOP_ADD, D3DBLEND_DESTALPHA, D3DBLEND_INVDESTALPHA},			// 0111: (Cs - Cd)*Ad + Cd ==> Cs*Ad + Cd*(1 - Ad)
			{0, D3DBLENDOP_ADD, D3DBLEND_DESTALPHA, D3DBLEND_DESTALPHA},			// 0112: (Cs - Cd)*Ad + 0 ==> Cs*Ad - Cd*Ad
			{1, D3DBLENDOP_SUBTRACT, D3DBLEND_BLENDFACTOR, D3DBLEND_BLENDFACTOR},	// * 0120: (Cs - Cd)*F + Cs ==> Cs*(F + 1) - Cd*F
			{0, D3DBLENDOP_ADD, D3DBLEND_BLENDFACTOR, D3DBLEND_INVBLENDFACTOR},		// 0121: (Cs - Cd)*F + Cd ==> Cs*F + Cd*(1 - F)
			{0, D3DBLENDOP_SUBTRACT, D3DBLEND_BLENDFACTOR, D3DBLEND_BLENDFACTOR},	// 0122: (Cs - Cd)*F + 0 ==> Cs*F - Cd*F
			{1, D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_ZERO},					// * 0200: (Cs - 0)*As + Cs ==> Cs*(As + 1)
			{0, D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_ONE},					// 0201: (Cs - 0)*As + Cd ==> Cs*As + Cd
			{0, D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_ZERO},					// 0202: (Cs - 0)*As + 0 ==> Cs*As
			{1, D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_ZERO},					// * 0210: (Cs - 0)*Ad + Cs ==> Cs*(As + 1)
			{0, D3DBLENDOP_ADD, D3DBLEND_DESTALPHA, D3DBLEND_ONE},					// 0211: (Cs - 0)*Ad + Cd ==> Cs*Ad + Cd
			{0, D3DBLENDOP_ADD, D3DBLEND_DESTALPHA, D3DBLEND_ZERO},					// 0212: (Cs - 0)*Ad + 0 ==> Cs*Ad
			{1, D3DBLENDOP_ADD, D3DBLEND_BLENDFACTOR, D3DBLEND_ZERO},				// * 0220: (Cs - 0)*F + Cs ==> Cs*(F + 1)
			{0, D3DBLENDOP_ADD, D3DBLEND_BLENDFACTOR, D3DBLEND_ONE},				// 0221: (Cs - 0)*F + Cd ==> Cs*F + Cd
			{0, D3DBLENDOP_ADD, D3DBLEND_BLENDFACTOR, D3DBLEND_ZERO},				// 0222: (Cs - 0)*F + 0 ==> Cs*F
			{0, D3DBLENDOP_ADD, D3DBLEND_INVSRCALPHA, D3DBLEND_SRCALPHA},			// 1000: (Cd - Cs)*As + Cs ==> Cd*As + Cs*(1 - As)
			{1, D3DBLENDOP_REVSUBTRACT, D3DBLEND_SRCALPHA, D3DBLEND_SRCALPHA},		// * 1001: (Cd - Cs)*As + Cd ==> Cd*(As + 1) - Cs*As
			{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_SRCALPHA, D3DBLEND_SRCALPHA},		// 1002: (Cd - Cs)*As + 0 ==> Cd*As - Cs*As
			{0, D3DBLENDOP_ADD, D3DBLEND_INVDESTALPHA, D3DBLEND_DESTALPHA},			// 1010: (Cd - Cs)*Ad + Cs ==> Cd*Ad + Cs*(1 - Ad)
			{1, D3DBLENDOP_REVSUBTRACT, D3DBLEND_DESTALPHA, D3DBLEND_DESTALPHA},	// * 1011: (Cd - Cs)*Ad + Cd ==> Cd*(Ad + 1) - Cs*Ad
			{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_DESTALPHA, D3DBLEND_DESTALPHA},	// 1012: (Cd - Cs)*Ad + 0 ==> Cd*Ad - Cs*Ad
			{0, D3DBLENDOP_ADD, D3DBLEND_INVBLENDFACTOR, D3DBLEND_BLENDFACTOR},		// 1020: (Cd - Cs)*F + Cs ==> Cd*F + Cs*(1 - F)
			{1, D3DBLENDOP_REVSUBTRACT, D3DBLEND_BLENDFACTOR, D3DBLEND_BLENDFACTOR},// * 1021: (Cd - Cs)*F + Cd ==> Cd*(F + 1) - Cs*F
			{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_BLENDFACTOR, D3DBLEND_BLENDFACTOR},// 1022: (Cd - Cs)*F + 0 ==> Cd*F - Cs*F
			{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 1100: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cs ==> Cs
			{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 1101: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cd ==> Cd
			{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 1102: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + 0 ==> 0
			{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 1110: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cs ==> Cs
			{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 1111: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cd ==> Cd
			{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 1112: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + 0 ==> 0
			{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 1120: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cs ==> Cs
			{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 1121: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cd ==> Cd
			{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 1122: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + 0 ==> 0
			{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_SRCALPHA},					// 1200: (Cd - 0)*As + Cs ==> Cs + Cd*As
			{1, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_SRCALPHA},					// * 1201: (Cd - 0)*As + Cd ==> Cd*(1 + As)
			{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_SRCALPHA},					// 1202: (Cd - 0)*As + 0 ==> Cd*As
			{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_DESTALPHA},					// 1210: (Cd - 0)*Ad + Cs ==> Cs + Cd*Ad
			{1, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_DESTALPHA},					// * 1211: (Cd - 0)*Ad + Cd ==> Cd*(1 + Ad)
			{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_DESTALPHA},					// 1212: (Cd - 0)*Ad + 0 ==> Cd*Ad
			{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_BLENDFACTOR},				// 1220: (Cd - 0)*F + Cs ==> Cs + Cd*F
			{1, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_BLENDFACTOR},				// * 1221: (Cd - 0)*F + Cd ==> Cd*(1 + F)
			{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_BLENDFACTOR},				// 1222: (Cd - 0)*F + 0 ==> Cd*F
			{0, D3DBLENDOP_ADD, D3DBLEND_INVSRCALPHA, D3DBLEND_ZERO},				// 2000: (0 - Cs)*As + Cs ==> Cs*(1 - As)
			{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_SRCALPHA, D3DBLEND_ONE},			// 2001: (0 - Cs)*As + Cd ==> Cd - Cs*As
			{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_SRCALPHA, D3DBLEND_ZERO},			// 2002: (0 - Cs)*As + 0 ==> 0 - Cs*As
			{0, D3DBLENDOP_ADD, D3DBLEND_INVDESTALPHA, D3DBLEND_ZERO},				// 2010: (0 - Cs)*Ad + Cs ==> Cs*(1 - Ad)
			{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_DESTALPHA, D3DBLEND_ONE},			// 2011: (0 - Cs)*Ad + Cd ==> Cd - Cs*Ad
			{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_DESTALPHA, D3DBLEND_ZERO},			// 2012: (0 - Cs)*Ad + 0 ==> 0 - Cs*Ad
			{0, D3DBLENDOP_ADD, D3DBLEND_INVBLENDFACTOR, D3DBLEND_ZERO},			// 2020: (0 - Cs)*F + Cs ==> Cs*(1 - F)
			{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_BLENDFACTOR, D3DBLEND_ONE},		// 2021: (0 - Cs)*F + Cd ==> Cd - Cs*F
			{0, D3DBLENDOP_REVSUBTRACT, D3DBLEND_BLENDFACTOR, D3DBLEND_ZERO},		// 2022: (0 - Cs)*F + 0 ==> 0 - Cs*F
			{0, D3DBLENDOP_SUBTRACT, D3DBLEND_ONE, D3DBLEND_SRCALPHA},				// 2100: (0 - Cd)*As + Cs ==> Cs - Cd*As
			{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_INVSRCALPHA},				// 2101: (0 - Cd)*As + Cd ==> Cd*(1 - As)
			{0, D3DBLENDOP_SUBTRACT, D3DBLEND_ZERO, D3DBLEND_SRCALPHA},				// 2102: (0 - Cd)*As + 0 ==> 0 - Cd*As
			{0, D3DBLENDOP_SUBTRACT, D3DBLEND_ONE, D3DBLEND_DESTALPHA},				// 2110: (0 - Cd)*Ad + Cs ==> Cs - Cd*Ad
			{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_INVDESTALPHA},				// 2111: (0 - Cd)*Ad + Cd ==> Cd*(1 - Ad)
			{0, D3DBLENDOP_SUBTRACT, D3DBLEND_ONE, D3DBLEND_DESTALPHA},				// 2112: (0 - Cd)*Ad + 0 ==> 0 - Cd*Ad
			{0, D3DBLENDOP_SUBTRACT, D3DBLEND_ONE, D3DBLEND_BLENDFACTOR},			// 2120: (0 - Cd)*F + Cs ==> Cs - Cd*F
			{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_INVBLENDFACTOR},			// 2121: (0 - Cd)*F + Cd ==> Cd*(1 - F)
			{0, D3DBLENDOP_SUBTRACT, D3DBLEND_ONE, D3DBLEND_BLENDFACTOR},			// 2122: (0 - Cd)*F + 0 ==> 0 - Cd*F
			{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 2200: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cs ==> Cs
			{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 2201: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cd ==> Cd
			{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 2202: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + 0 ==> 0
			{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 2210: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cs ==> Cs
			{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 2211: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cd ==> Cd
			{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 2212: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + 0 ==> 0
			{0, D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ZERO},						// 2220: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cs ==> Cs
			{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE},						// 2221: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + Cd ==> Cd
			{0, D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO},						// 2222: (Cs/Cd/0 - Cs/Cd/0)*As/Ad/F + 0 ==> 0
		};

		// bogus: 0100, 0110, 0120, 0200, 0210, 0220, 1001, 1011, 1021, 1201, 1211, 1221

		int i = (((m_ctxt->ALPHA.A&3)*3+(m_ctxt->ALPHA.B&3))*3+(m_ctxt->ALPHA.C&3))*3+(m_ctxt->ALPHA.D&3);

		ASSERT(m_ctxt->ALPHA.A != 3);
		ASSERT(m_ctxt->ALPHA.B != 3);
		ASSERT(m_ctxt->ALPHA.C != 3);
		ASSERT(m_ctxt->ALPHA.D != 3);
		ASSERT(!blendmap[i].bogus);

		hr = m_pD3DDev->SetRenderState(D3DRS_BLENDOP, blendmap[i].op);
		hr = m_pD3DDev->SetRenderState(D3DRS_SRCBLEND, blendmap[i].src);
		hr = m_pD3DDev->SetRenderState(D3DRS_DESTBLEND, blendmap[i].dst);
	}
}
