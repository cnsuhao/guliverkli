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

#include "StdAfx.h"
#include "GSRendererHW.h"
#include "GSUtil.h"
#include "resource.h"

GSRendererHW::GSRendererHW()
	: m_tc(this)
	, m_width(1024)
	, m_height(1024)
	, m_skip(0)
{
	if(!AfxGetApp()->GetProfileInt(_T("Settings"), _T("nativeres"), FALSE))
	{
		m_width = AfxGetApp()->GetProfileInt(_T("Settings"), _T("resx"), 1024);
		m_height = AfxGetApp()->GetProfileInt(_T("Settings"), _T("resy"), 1024);
	}
}

GSRendererHW::~GSRendererHW()
{
}

bool GSRendererHW::Create(LPCTSTR title)
{
	if(!__super::Create(title))
		return false;

	if(!m_tfx.Create(&m_dev))
		return false;

	D3D10_DEPTH_STENCIL_DESC dsd;

	memset(&dsd, 0, sizeof(dsd));

	dsd.DepthEnable = true;
	dsd.DepthWriteMask = D3D10_DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = D3D10_COMPARISON_GREATER; // shader will output depth <= 0 if it wants put zero into stencil
	dsd.StencilEnable = true;
	dsd.StencilReadMask = 1;
	dsd.StencilWriteMask = 1;
	dsd.FrontFace.StencilFunc = D3D10_COMPARISON_ALWAYS;
	dsd.FrontFace.StencilPassOp = D3D10_STENCIL_OP_REPLACE;
	dsd.FrontFace.StencilFailOp = D3D10_STENCIL_OP_KEEP;
	dsd.FrontFace.StencilDepthFailOp = D3D10_STENCIL_OP_ZERO;
	dsd.BackFace.StencilFunc = D3D10_COMPARISON_ALWAYS;
	dsd.BackFace.StencilPassOp = D3D10_STENCIL_OP_REPLACE;
	dsd.BackFace.StencilFailOp = D3D10_STENCIL_OP_KEEP;
	dsd.BackFace.StencilDepthFailOp = D3D10_STENCIL_OP_ZERO;

	m_dev->CreateDepthStencilState(&dsd, &m_date.dss);

	D3D10_BLEND_DESC bd;

	memset(&bd, 0, sizeof(bd));

	m_dev->CreateBlendState(&bd, &m_date.bs);

	return true;
}

static const float s_one_over_log_2pow32 = 1.0f / (log(2.0f)*32);

void GSRendererHW::VertexKick(bool skip)
{
	GSVertexHW& v = m_vl.AddTail();

	v.x = (float)m_v.XYZ.X;
	v.y = (float)m_v.XYZ.Y;
	v.z = (float)m_v.XYZ.Z;

	v.c = m_v.RGBAQ.ai32[0];

	v.f = m_v.FOG.ai32[1];

	if(m_pPRIM->TME)
	{
		if(m_pPRIM->FST)
		{
			v.w = 1.0f;
			v.u = (float)(int)m_v.UV.U;
			v.v = (float)(int)m_v.UV.V;
		}
		else
		{
			v.w = m_v.RGBAQ.Q;
			v.u = m_v.ST.S;
			v.v = m_v.ST.T;
		}
	}
	else
	{
		v.w = 1.0f;
		v.u = 0;
		v.v = 0;
	}

	__super::VertexKick(skip);
}

void GSRendererHW::DrawingKick(bool skip)
{
	GSVertexHW* v = &m_pVertices[m_nVertices];
	int nv = 0;

	switch(m_pPRIM->PRIM)
	{
	case GS_POINTLIST:
		m_vl.RemoveAt(0, v[0]);
		nv = 1;
		break;
	case GS_LINELIST:
		m_vl.RemoveAt(0, v[0]);
		m_vl.RemoveAt(0, v[1]);
		nv = 2;
		break;
	case GS_LINESTRIP:
		m_vl.RemoveAt(0, v[0]);
		m_vl.GetAt(0, v[1]);
		nv = 2;
		break;
	case GS_TRIANGLELIST:
		m_vl.RemoveAt(0, v[0]);
		m_vl.RemoveAt(0, v[1]);
		m_vl.RemoveAt(0, v[2]);
		nv = 3;
		break;
	case GS_TRIANGLESTRIP:
		m_vl.RemoveAt(0, v[0]);
		m_vl.GetAt(0, v[1]);
		m_vl.GetAt(1, v[2]);
		nv = 3;
		break;
	case GS_TRIANGLEFAN:
		m_vl.GetAt(0, v[0]);
		m_vl.RemoveAt(1, v[1]);
		m_vl.GetAt(1, v[2]);
		nv = 3;
		break;
	case GS_SPRITE:
		m_vl.RemoveAt(0, v[0]);
		m_vl.RemoveAt(0, v[1]);
		nv = 2;
		break;
	default:
		//m_vl.RemoveAll();
		ASSERT(0);
		return;
	}

	if(skip)
	{
		return;
	}

	float sx0 = m_context->scissor.x0;
	float sy0 = m_context->scissor.y0;
	float sx1 = m_context->scissor.x1;
	float sy1 = m_context->scissor.y1;

	switch(nv)
	{
	case 1:
		if(v[0].x < sx0
		|| v[0].x > sx1
		|| v[0].y < sy0
		|| v[0].y > sy1)
			return;
		break;
	case 2:
		if(v[0].x < sx0 && v[1].x < sx0
		|| v[0].x > sx1 && v[1].x > sx1
		|| v[0].y < sy0 && v[1].y < sy0
		|| v[0].y > sy1 && v[1].y > sy1)
			return;
		break;
	case 3:
		if(v[0].x < sx0 && v[1].x < sx0 && v[2].x < sx0
		|| v[0].x > sx1 && v[1].x > sx1 && v[2].x > sx1
		|| v[0].y < sy0 && v[1].y < sy0 && v[2].y < sy0
		|| v[0].y > sy1 && v[1].y > sy1 && v[2].y > sy1)
			return;
		break;
	default:
		__assume(0);
	}

	m_nVertices += nv;

	// costs a few fps, but fixes RR's shadows (or anything which paints overlapping shapes with date)
/*
	if(m_context->TEST.DATE)
	{
		Flush();
	}
*/
}

int s_n = 0;
bool s_dump = false;
bool s_save = false;
bool s_savez = false;

void GSRendererHW::FlushPrim()
{
	if(m_nVertices > 0)
	{
		FlushPrimInternal();
	}

	__super::FlushPrim();
}

void GSRendererHW::FlushPrimInternal()
{
/*
TRACE(_T("[%d] FlushPrim f %05x (%d) z %05x (%d %d %d %d) t %05x %05x (%d)\n"), 
	  (int)m_perfmon.GetFrame(), 
	  (int)m_context->FRAME.Block(), 
	  (int)m_context->FRAME.PSM, 
	  (int)m_context->ZBUF.Block(), 
	  (int)m_context->ZBUF.PSM, 
	  m_context->TEST.ZTE, 
	  m_context->TEST.ZTST, 
	  m_context->ZBUF.ZMSK, 
	  m_pPRIM->TME ? (int)m_context->TEX0.TBP0 : 0xfffff, 
	  m_pPRIM->TME && m_context->TEX0.PSM > PSM_PSMCT16S ? (int)m_context->TEX0.CBP : 0xfffff, 
	  m_pPRIM->TME ? (int)m_context->TEX0.PSM : 0xff);
*/
	//

	if(DetectBadFrame())
	{
		return;
	}

if(s_n >= 1074)
{
	s_save = true;
}

	//

	GIFRegTEX0 TEX0;

	// rt

	TEX0.TBP0 = m_context->FRAME.Block();
	TEX0.TBW = m_context->FRAME.FBW;
	TEX0.PSM = m_context->FRAME.PSM;

	GSTextureCache::GSRenderTarget* rt = m_tc.GetRenderTarget(TEX0, m_width, m_height);

	// ds

	TEX0.TBP0 = m_context->ZBUF.Block();
	TEX0.TBW = m_context->FRAME.FBW;
	TEX0.PSM = m_context->ZBUF.PSM;

	GSTextureCache::GSDepthStencil* ds = m_tc.GetDepthStencil(TEX0, m_width, m_height);

	// tex

	GSTextureCache::GSTexture* tex = NULL;

	if(m_pPRIM->TME && !(tex = m_tc.GetTexture()))
	{
		return;
	}

if(s_dump)
{
	CString str;
	str.Format(_T("c:\\temp2\\_%05d_f%I64d_tex_%05x_%d.dds"), s_n++, m_perfmon.GetFrame(), (int)m_context->TEX0.TBP0, (int)m_context->TEX0.PSM);
	if(m_pPRIM->TME) if(s_save) D3DX10SaveTextureToFile(tex->m_texture, D3DX10_IFF_DDS, str);
	str.Format(_T("c:\\temp2\\_%05d_f%I64d_rt0_%05x_%d.bmp"), s_n++, m_perfmon.GetFrame(), m_context->FRAME.Block(), m_context->FRAME.PSM);
	if(s_save) D3DX10SaveTextureToFile(rt->m_texture, D3DX10_IFF_BMP, str);
	str.Format(_T("c:\\temp2\\_%05d_f%I64d_rz0_%05x_%d.bmp"), s_n-1, m_perfmon.GetFrame(), m_context->ZBUF.Block(), m_context->ZBUF.PSM);
	if(s_savez) m_dev.SaveToFileD32S8X24(ds->m_texture, str);
}

	//

	int prim = m_pPRIM->PRIM;

	if(!OverrideInput(prim, m_nVertices, tex))
	{
		return;
	}

	D3D10_PRIMITIVE_TOPOLOGY topology;

	switch(prim)
	{
	case GS_POINTLIST:
		topology = D3D10_PRIMITIVE_TOPOLOGY_POINTLIST;
		m_perfmon.Put(GSPerfMon::Prim, m_nVertices);
		break;
	case GS_LINELIST: 
	case GS_LINESTRIP:
	case GS_SPRITE:
		topology = D3D10_PRIMITIVE_TOPOLOGY_LINELIST;
		m_perfmon.Put(GSPerfMon::Prim, m_nVertices / 2);
		break;
	case GS_TRIANGLELIST: 
	case GS_TRIANGLESTRIP: 
	case GS_TRIANGLEFAN: 
		topology = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		m_perfmon.Put(GSPerfMon::Prim, m_nVertices / 3);
		break;
	default:
		__assume(0);
	}

	m_perfmon.Put(GSPerfMon::Draw, 1);

	// date

	SetupDestinationAlphaTest(rt, ds);

	// om

	GSTextureFX::OMDepthStencilSelector om_dssel;

	om_dssel.zte = m_context->TEST.ZTE;
	om_dssel.ztst = m_context->TEST.ZTST;
	om_dssel.zwe = !m_context->ZBUF.ZMSK;
	om_dssel.date = m_context->TEST.DATE;

	GSTextureFX::OMBlendSelector om_bsel;

	om_bsel.abe = m_pPRIM->ABE || (m_pPRIM->PRIM == 1 || m_pPRIM->PRIM == 2) && m_pPRIM->AA1;
	om_bsel.a = m_context->ALPHA.A;
	om_bsel.b = m_context->ALPHA.B;
	om_bsel.c = m_context->ALPHA.C;
	om_bsel.d = m_context->ALPHA.D;
	om_bsel.wr = (m_context->FRAME.FBMSK & 0x000000ff) != 0x000000ff;
	om_bsel.wg = (m_context->FRAME.FBMSK & 0x0000ff00) != 0x0000ff00;
	om_bsel.wb = (m_context->FRAME.FBMSK & 0x00ff0000) != 0x00ff0000;
	om_bsel.wa = (m_context->FRAME.FBMSK & 0xff000000) != 0xff000000;

	float factor = (float)(int)m_context->ALPHA.FIX / 0x80;

	m_tfx.SetupOM(om_dssel, om_bsel, factor, rt->m_texture, ds->m_texture);

	// ia

	m_tfx.SetupIA(m_pVertices, m_nVertices, topology);

	// vs

	GSTextureFX::VSConstantBuffer vs_cb;

	float sx = 2.0f * rt->m_scale.x / (rt->m_texture.m_desc.Width * 16);
	float sy = 2.0f * rt->m_scale.y / (rt->m_texture.m_desc.Height * 16);
	float ox = (float)(int)m_context->XYOFFSET.OFX;
	float oy = (float)(int)m_context->XYOFFSET.OFY;

	vs_cb.VertexScale = D3DXVECTOR4(sx, -sy, 1.0f / UINT_MAX, 0);
	vs_cb.VertexOffset = D3DXVECTOR4(ox * sx + 1, -(oy * sy + 1), 0, -1);
	vs_cb.TextureScale = D3DXVECTOR2(1.0f, 1.0f);

	if(m_pPRIM->TME && m_pPRIM->FST)
	{
		vs_cb.TextureScale.x = 1.0f / (16 << m_context->TEX0.TW);
		vs_cb.TextureScale.y = 1.0f / (16 << m_context->TEX0.TH);
	}

	m_tfx.SetupVS(&vs_cb);

	// gs

	GSTextureFX::GSSelector gs_sel;

	gs_sel.iip = m_pPRIM->IIP;

	switch(prim)
	{
	case GS_POINTLIST:
		gs_sel.prim = 0;
		break;
	case GS_LINELIST: 
	case GS_LINESTRIP:
		gs_sel.prim = 1;
		break;
	case GS_TRIANGLELIST: 
	case GS_TRIANGLESTRIP: 
	case GS_TRIANGLEFAN: 
		gs_sel.prim = 2;
		break;
	case GS_SPRITE:
		gs_sel.prim = 3;
		break;
	default:
		__assume(0);
	}	

	m_tfx.SetupGS(gs_sel);

	// ps

	GSTextureFX::PSSelector ps_sel;

	ps_sel.fst = m_pPRIM->FST;
	ps_sel.clamp = 0;
	ps_sel.bpp = 0;
	ps_sel.aem = m_env.TEXA.AEM;
	ps_sel.tfx = m_context->TEX0.TFX;
	ps_sel.tcc = m_context->TEX0.TCC;
	ps_sel.ate = m_context->TEST.ATE;
	ps_sel.atst = m_context->TEST.ATST;
	ps_sel.fog = m_pPRIM->FGE;
	ps_sel.clr1 = om_bsel.abe && om_bsel.a == 1 && om_bsel.b == 2 && om_bsel.d == 1;
	ps_sel.fba = m_context->FBA.FBA;
	ps_sel.aout = m_context->FRAME.PSM == PSM_PSMCT16 || m_context->FRAME.PSM == PSM_PSMCT16S ? 1 : 0;

	GSTextureFX::PSSamplerSelector ps_ssel;

	ps_ssel.min = m_filter == 2 ? (m_context->TEX1.MMIN & 1) : m_filter;
	ps_ssel.mag = m_filter == 2 ? (m_context->TEX1.MMAG & 1) : m_filter;
	ps_ssel.tau = 0;
	ps_ssel.tav = 0;

	GSTextureFX::PSConstantBuffer ps_cb;

	ps_cb.FogColor = D3DXVECTOR4((float)(int)m_env.FOGCOL.FCR / 255, (float)(int)m_env.FOGCOL.FCG / 255, (float)(int)m_env.FOGCOL.FCB / 255, 0);
	ps_cb.ClampMin = D3DXVECTOR2(-4096, -4096);
	ps_cb.ClampMax = D3DXVECTOR2(+4096, +4096);
	ps_cb.TA0 = (float)(int)m_env.TEXA.TA0 / 255;
	ps_cb.TA1 = (float)(int)m_env.TEXA.TA1 / 255;
	ps_cb.AREF = (float)(int)m_context->TEST.AREF / 255;

	ID3D10ShaderResourceView* tex_view = NULL;
	ID3D10ShaderResourceView* pal_view = NULL;

	if(tex)
	{
		ps_sel.bpp = tex->m_bpp2;

		switch(m_context->CLAMP.WMS)
		{
		case 0: case 3: ps_ssel.tau = 1; break;
		case 1: case 2: ps_ssel.tau = 0; break;
		default: __assume(0);
		}

		switch(m_context->CLAMP.WMT)
		{
		case 0: case 3: ps_ssel.tav = 1; break;
		case 1: case 2: ps_ssel.tav = 0; break;
		default: __assume(0);
		}

		if(m_context->CLAMP.WMS == 2)
		{
			ps_cb.ClampMin.x = (float)(int)m_context->CLAMP.MINU / (1 << m_context->TEX0.TW);
			ps_cb.ClampMax.x = (float)(int)m_context->CLAMP.MAXU / (1 << m_context->TEX0.TW);
			ps_sel.clamp = 1;
		}

		if(m_context->CLAMP.WMT == 2)
		{
			ps_cb.ClampMin.y = (float)(int)m_context->CLAMP.MINV / (1 << m_context->TEX0.TH);
			ps_cb.ClampMax.y = (float)(int)m_context->CLAMP.MAXV / (1 << m_context->TEX0.TH);
			ps_sel.clamp = 1;
		}

		float w = (float)(int)tex->m_texture.m_desc.Width;
		float h = (float)(int)tex->m_texture.m_desc.Height;

		ps_cb.WH = D3DXVECTOR2(w, h);
		ps_cb.rWrH = D3DXVECTOR2(1.0f / w, 1.0f / h);
		ps_cb.rWZ = D3DXVECTOR2(1.0f / w, 0);
		ps_cb.ZrH = D3DXVECTOR2(0, 1.0f / h);

		tex_view = tex->m_texture;
		pal_view = tex->m_palette;
	}
	else
	{
		ps_sel.tfx = 4;
	}

	m_tfx.SetupPS(ps_sel, &ps_cb, ps_ssel, tex_view, pal_view);

	// rs

	UINT w = rt->m_texture.m_desc.Width;
	UINT h = rt->m_texture.m_desc.Height;

	CRect scissor(
		(int)(rt->m_scale.x * (m_context->SCISSOR.SCAX0)),
		(int)(rt->m_scale.y * (m_context->SCISSOR.SCAY0)), 
		(int)(rt->m_scale.x * (m_context->SCISSOR.SCAX1 + 1)),
		(int)(rt->m_scale.y * (m_context->SCISSOR.SCAY1 + 1)));

	scissor &= CRect(0, 0, w, h);

	m_tfx.SetupRS(w, h, scissor);

	// draw

	if(!m_context->TEST.ATE || m_context->TEST.ATST != 0)
	{
		m_dev->Draw(m_nVertices, 0);
	}

	if(m_context->TEST.ATE && m_context->TEST.ATST != 1 && m_context->TEST.AFAIL)
	{
		ASSERT(!m_env.PABE.PABE);

		static const DWORD iatst[] = {1, 0, 5, 6, 7, 2, 3, 4};

		ps_sel.atst = iatst[ps_sel.atst];

		m_tfx.UpdatePS(ps_sel, ps_ssel);

		bool z = om_dssel.zwe;
		bool r = om_bsel.wr;
		bool g = om_bsel.wg;
		bool b = om_bsel.wb;
		bool a = om_bsel.wa;

		switch(m_context->TEST.AFAIL)
		{
		case 0: z = r = g = b = a = false; break; // none
		case 1: z = false; break; // rgba
		case 2: r = g = b = a = false; break; // z
		case 3: z = a = false; break; // rgb
		default: __assume(0);
		}

		if(z || r || g || b || a)
		{
			om_dssel.zwe = z;
			om_bsel.wr = r;
			om_bsel.wg = g;
			om_bsel.wb = b;
			om_bsel.wa = a;

			m_tfx.UpdateOM(om_dssel, om_bsel, factor);

			m_dev->Draw(m_nVertices, 0);
		}
	}

	m_dev.EndScene();


	if(m_env.COLCLAMP.CLAMP == 0) m_perfmon.Put(GSPerfMon::COLCLAMP);
	if(m_env.PABE.PABE) m_perfmon.Put(GSPerfMon::PABE);
	if(m_context->TEST.DATE) m_perfmon.Put(GSPerfMon::DATE);
	if(om_bsel.abe && om_bsel.a == om_bsel.d && om_bsel.a != om_bsel.b && om_bsel.a != 1 && om_bsel.b != 2) m_perfmon.Put(GSPerfMon::ABE);

if(s_dump)
{
	CString str;
	str.Format(_T("c:\\temp2\\_%05d_f%I64d_rt1_%05x_%d.bmp"), s_n++, m_perfmon.GetFrame(), m_context->FRAME.Block(), m_context->FRAME.PSM);
	if(s_save) D3DX10SaveTextureToFile(rt->m_texture, D3DX10_IFF_BMP, str);
	str.Format(_T("c:\\temp2\\_%05d_f%I64d_rz1_%05x_%d.bmp"), s_n-1, m_perfmon.GetFrame(), m_context->ZBUF.Block(), m_context->ZBUF.PSM);
	if(s_savez) m_dev.SaveToFileD32S8X24(ds->m_texture, str);
}

}

void GSRendererHW::Flip()
{
	FlipInfo src[2];

	for(int i = 0; i < countof(src); i++)
	{
		if(!m_regs.IsEnabled(i))
		{
			continue;
		}

		GIFRegTEX0 TEX0;

		TEX0.TBP0 = m_regs.pDISPFB[i]->Block();
		TEX0.TBW = m_regs.pDISPFB[i]->FBW;
		TEX0.PSM = m_regs.pDISPFB[i]->PSM;

		if(GSTextureCache::GSRenderTarget* rt = m_tc.GetRenderTarget(TEX0, m_width, m_height, true))
		{
			src[i].t = rt->m_texture;
			src[i].s = rt->m_scale;

if(s_dump)
{
	CString str;
	str.Format(_T("c:\\temp2\\_%05d_f%I64d_fr%d_%05x_%d.bmp"), s_n++, m_perfmon.GetFrame(), i, (int)TEX0.TBP0, (int)TEX0.PSM);
	if(s_save) ::D3DX10SaveTextureToFile(rt->m_texture, D3DX10_IFF_BMP, str);
}

//s_dump = m_perfmon.GetFrame() >= 5000;
//if(m_perfmon.GetFrame() == 5000) m_tc.RemoveAll();
		}
	}

	FinishFlip(src);

	m_tc.IncAge();

	m_skip = 0;
}

void GSRendererHW::InvalidateTexture(const GIFRegBITBLTBUF& BITBLTBUF, CRect r)
{
	TRACE(_T("[%d] InvalidateTexture %d,%d - %d,%d %05x\n"), (int)m_perfmon.GetFrame(), r.left, r.top, r.right, r.bottom, (int)BITBLTBUF.DBP);

	m_tc.InvalidateTexture(BITBLTBUF, &r);
}

void GSRendererHW::InvalidateLocalMem(const GIFRegBITBLTBUF& BITBLTBUF, CRect r)
{
	TRACE(_T("[%d] InvalidateLocalMem %d,%d - %d,%d %05x\n"), (int)m_perfmon.GetFrame(), r.left, r.top, r.right, r.bottom, (int)BITBLTBUF.SBP);

	m_tc.InvalidateLocalMem(BITBLTBUF, &r);
}

void GSRendererHW::MinMaxUV(int w, int h, CRect& r)
{
	r.SetRect(0, 0, w, h);

	uvmm_t uv;
	CSize bsm;

	if(m_context->CLAMP.WMS < 3 || m_context->CLAMP.WMT < 3)
	{
		uv.umin = uv.vmin = 0;
		uv.umax = uv.vmax = 1;

		if(m_pPRIM->FST)
		{
			UVMinMax(m_nVertices, (vertex_t*)m_pVertices, &uv);

			uv.umin *= 1.0f / (16 << m_context->TEX0.TW);
			uv.umax *= 1.0f / (16 << m_context->TEX0.TW);
			uv.vmin *= 1.0f / (16 << m_context->TEX0.TH);
			uv.vmax *= 1.0f / (16 << m_context->TEX0.TH);
		}
		else
		{
			// FIXME

			if(m_nVertices > 0)// && m_nVertices < 100)
			{
				uv.umin = uv.vmin = +1e10;
				uv.umax = uv.vmax = -1e10;

				for(int i = 0, j = m_nVertices; i < j; i++)
				{
					float w = 1.0f / m_pVertices[i].w;
					float u = m_pVertices[i].u * w;
					if(uv.umax < u) uv.umax = u;
					if(uv.umin > u) uv.umin = u;
					float v = m_pVertices[i].v * w;
					if(uv.vmax < v) uv.vmax = v;
					if(uv.vmin > v) uv.vmin = v;
				}
			}
		}

		CSize bs = GSLocalMemory::m_psmtbl[m_context->TEX0.PSM].bs;

		bsm.SetSize(bs.cx-1, bs.cy-1);
	}

	if(m_context->CLAMP.WMS < 3)
	{
		if(m_context->CLAMP.WMS == 0)
		{
			float fmin = floor(uv.umin);
			float fmax = floor(uv.umax);

			if(fmin != fmax) {uv.umin = 0; uv.umax = 1.0f;}
			else {uv.umin -= fmin; uv.umax -= fmax;}

			// FIXME: 
			if(uv.umin == 0 && uv.umax != 1.0f) uv.umax = 1.0f;
		}
		else if(m_context->CLAMP.WMS == 1)
		{
			if(uv.umin < 0) uv.umin = 0;
			else if(uv.umin > 1.0f) uv.umin = 1.0f;
			if(uv.umax < 0) uv.umax = 0;
			else if(uv.umax > 1.0f) uv.umax = 1.0f;
			if(uv.umin > uv.umax) uv.umin = uv.umax;
		}
		else if(m_context->CLAMP.WMS == 2)
		{
			float minu = 1.0f * m_context->CLAMP.MINU / w;
			float maxu = 1.0f * m_context->CLAMP.MAXU / w;
			if(uv.umin < minu) uv.umin = minu;
			else if(uv.umin > maxu) uv.umin = maxu;
			if(uv.umax < minu) uv.umax = minu;
			else if(uv.umax > maxu) uv.umax = maxu;
			if(uv.umin > uv.umax) uv.umin = uv.umax;
		}

		r.left = max((int)(uv.umin * w) & ~bsm.cx, 0);
		r.right = min(((int)(uv.umax * w) + bsm.cx + 1) & ~bsm.cx, w);
	}

	if(m_context->CLAMP.WMT < 3)
	{
		if(m_context->CLAMP.WMT == 0)
		{
			float fmin = floor(uv.vmin);
			float fmax = floor(uv.vmax);

			if(fmin != fmax) {uv.vmin = 0; uv.vmax = 1.0f;}
			else {uv.vmin -= fmin; uv.vmax -= fmax;}

			// FIXME: 
			if(uv.vmin == 0 && uv.vmax != 1.0f) uv.vmax = 1.0f;
		}
		else if(m_context->CLAMP.WMT == 1)
		{
			if(uv.vmin < 0) uv.vmin = 0;
			else if(uv.vmin > 1.0f) uv.vmin = 1.0f;
			if(uv.vmax < 0) uv.vmax = 0;
			else if(uv.vmax > 1.0f) uv.vmax = 1.0f;
			if(uv.vmin > uv.vmax) uv.vmin = uv.vmax;
		}
		else if(m_context->CLAMP.WMT == 2)
		{
			float minv = 1.0f * m_context->CLAMP.MINV / h;
			float maxv = 1.0f * m_context->CLAMP.MAXV / h;
			if(uv.vmin < minv) uv.vmin = minv;
			else if(uv.vmin > maxv) uv.vmin = maxv;
			if(uv.vmax < minv) uv.vmax = minv;
			else if(uv.vmax > maxv) uv.vmax = maxv;
			if(uv.vmin > uv.vmax) uv.vmin = uv.vmax;
		}
		
		r.top = max((int)(uv.vmin * h) & ~bsm.cy, 0);
		r.bottom = min(((int)(uv.vmax * h) + bsm.cy + 1) & ~bsm.cy, h);
	}

	//ASSERT(r.left <= r.right);
	//ASSERT(r.top <= r.bottom);
}

void GSRendererHW::SetupDestinationAlphaTest(GSTextureCache::GSRenderTarget* rt, GSTextureCache::GSDepthStencil* ds)
{
	if(!m_context->TEST.DATE) return; // || (::GetAsyncKeyState(VK_CONTROL)&0x80000000)

	// sfex3 (after the capcom logo), vf4 (first menu fading in), ffxii shadows, rumble roses shadows

	float xmin = -1, xmax = +1;
	float ymin = -1, ymax = +1;

	float umin = 0, umax = 1;
	float vmin = 0, vmax = 1;

	// if(m_nVertices < 1000) {

#if _M_IX86_FP >= 2 || defined(_M_AMD64)
		
	__m128 xymin = _mm_set1_ps(+1e10);
	__m128 xymax = _mm_set1_ps(-1e10);

	for(int i = 0, j = m_nVertices; i < j; i++)
	{
		xymin = _mm_min_ps(m_pVertices[i].m128[0], xymin);
		xymax = _mm_max_ps(m_pVertices[i].m128[0], xymax);
	}

	xmin = xymin.m128_f32[0];
	ymin = xymin.m128_f32[1];
	xmax = xymax.m128_f32[0];
	ymax = xymax.m128_f32[1];

#else	

	xmin = ymin = +1e10;
	xmax = ymax = -1e10;

	for(int i = 0, j = m_nVertices; i < j; i++)
	{
		float x = m_pVertices[i].x;

		if(x < xmin) xmin = x;
		if(x > xmax) xmax = x;
		
		float y = m_pVertices[i].y;

		if(y < ymin) ymin = y;
		if(y > ymax) ymax = y;
	}

#endif

	float sx = 2.0f * rt->m_scale.x / (rt->m_texture.m_desc.Width * 16);
	float sy = 2.0f * rt->m_scale.y / (rt->m_texture.m_desc.Height * 16);
	float ox = (float)(int)m_context->XYOFFSET.OFX;
	float oy = (float)(int)m_context->XYOFFSET.OFY;

	xmin = xmin * sx - (ox * sx + 1);
	xmax = xmax * sx - (ox * sx + 1);
	ymin = ymin * sy - (oy * sy + 1);
	ymax = ymax * sy - (oy * sy + 1);

	if(xmin < -1) xmin = -1;
	if(xmax > +1) xmax = +1;
	if(ymin < -1) ymin = -1;
	if(ymax > +1) ymax = +1;
	
	umin = (xmin + 1) / 2;
	umax = (xmax + 1) / 2;
	vmin = (ymin + 1) / 2;
	vmax = (ymax + 1) / 2;

	TRACE(_T("[%I64d] %.2f %.2f %.2f %.2f | %.2f %.2f %.2f %.2f\n"), 
		m_perfmon.GetFrame(), 
		xmin, xmax, ymin, ymax,
		umin, umax, vmin, vmax);

	// }

	// om

	GSTexture2D tmp;

	m_dev.CreateRenderTarget(tmp, rt->m_texture.m_desc.Width, rt->m_texture.m_desc.Height);

	// m_dev->ClearDepthStencilView(ds->m_texture, D3D10_CLEAR_STENCIL, 0, 0);

	m_dev.OMSet(tmp, ds->m_texture);

	m_dev.OMSet(m_date.dss, 1, m_date.bs, 0);

	// ia

	VertexPT1 vertices[] =
	{
		{xmin, -ymin, 0.5f, 1.0f, umin, vmin},
		{xmax, -ymin, 0.5f, 1.0f, umax, vmin},
		{xmin, -ymax, 0.5f, 1.0f, umin, vmax},
		{xmax, -ymax, 0.5f, 1.0f, umax, vmax},
	};

	m_dev.IASet(m_dev.m_convert.vb, 4, vertices, m_dev.m_convert.il, D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// vs

	m_dev.VSSet(m_dev.m_convert.vs, NULL);

	// gs

	m_dev.GSSet(NULL);

	// ps

	ID3D10ShaderResourceView* srvs[] = {rt->m_texture};

	m_dev->PSSetShaderResources(0, 1, srvs);

	m_dev.PSSet(m_dev.m_convert.ps[m_context->TEST.DATM ? 2 : 3], m_dev.m_ss_point);

	// rs

	m_dev.RSSet(tmp.m_desc.Width, tmp.m_desc.Height);

	// set

	m_dev->Draw(4, 0);

	//

	m_dev.EndScene();

	m_dev.Recycle(tmp);
}

bool GSRendererHW::DetectBadFrame()
{
	DWORD FBP = m_context->FRAME.Block();
	DWORD FPSM = m_context->FRAME.PSM;

	bool TME = m_pPRIM->TME;
	DWORD TBP0 = m_context->TEX0.TBP0;
	DWORD TPSM = m_context->TEX0.PSM;

	switch(m_crc)
	{
	case 0x21068223: // okami ntsc/us

		if(m_skip == 0)
		{
			//if(TME && FBP == 0x02c00 && FPSM == PSM_PSMCT16 && TBP0 == 0x02800 && TPSM == PSM_PSMCT16)
			if(TME && FBP == 0x00e00 && FPSM == PSM_PSMCT32 && TBP0 == 0x00000 && TPSM == PSM_PSMCT32)
			{
				m_skip = 1000; // 240;
			}
		}
		else
		{
			if(TME && FBP == 0x00e00 && FPSM == PSM_PSMCT32 && TBP0 == 0x03800 && TPSM == PSM_PSMT4)
			{
				m_skip = 0;
			}
		}

		break;

	case 0x053D2239: // mgs3s1 ntsc/us

		if(m_skip == 0)
		{
			if(TME && FBP == 0x02000 && FPSM == PSM_PSMCT32 && (TBP0 == 0x00000 || TBP0 == 0x01000) && TPSM == PSM_PSMCT24)
			{
				m_skip = 1000; // 76, 79
			}
			else if(TME && FBP == 0x02800 && FPSM == PSM_PSMCT24 && (TBP0 == 0x00000 || TBP0 == 0x01000) && TPSM == PSM_PSMCT32)
			{
				m_skip = 1000; // 69
			}
		}
		else 
		{
			if(!TME && (FBP == 0x00000 || FBP == 0x01000) && FPSM == PSM_PSMCT32)
			{
				m_skip = 0;
			}
		}

		break;

	case 0x278722BF: // dbz bt2 ntsc/us

		if(m_skip == 0)
		{
			if(TME && /*FBP == 0x00000 && FPSM == PSM_PSMCT16 &&*/ TBP0 == 0x02000 && TPSM == PSM_PSMZ16)
			{
				m_skip = 27;
			}
		}

		break;

	case 0x72B3802A: // sfex3 ntsc/us

		if(m_skip == 0)
		{
			if(TME && FBP == 0x00f00 && FPSM == PSM_PSMCT16 && (TBP0 == 0x00500 || TBP0 == 0x00000) && TPSM == PSM_PSMCT32)
			{
				m_skip = 4;
			}
		}

		break;
	}

	if(m_skip == 0)
	{
		if(TME)
		{
			if(HasSharedBits(FBP, FPSM, TBP0, TPSM))
			{
				m_skip = 1;
			}

			// depth textures (bully, mgs3s1 intro)

			if(TPSM == PSM_PSMZ32 || TPSM == PSM_PSMZ24 || TPSM == PSM_PSMZ16 || TPSM == PSM_PSMZ16S)
			{
				m_perfmon.Put(GSPerfMon::DepthTexture);
				m_skip = 1;
			}
		}
	}

	if(m_skip > 0)
	{
		m_skip--;

		return true;
	}

	return false;
}

bool GSRendererHW::OverrideInput(int& prim, int& count, GSTextureCache::GSTexture* tex)
{
	#pragma region ffxii pal video conversion

	if(m_crc == 0x78da0252 || m_crc == 0xc1274668 || m_crc == 0xdc2a467e || m_crc == 0xca284668)
	{
		static DWORD* video = NULL;
		static bool ok = false;

		if(prim == GS_POINTLIST && count >= 448*448 && count <= 448*512)
		{
			// incoming pixels are stored in columns, one column is 16x512, total res 448x512 or 448x454

			if(!video) video = new DWORD[512*512];

			for(int x = 0, i = 0, rows = count / 448; x < 448; x += 16)
			{
				DWORD* dst = &video[x];

				for(int y = 0; y < rows; y++, dst += 512)
				{
					for(int j = 0; j < 16; j++, i++)
					{
						dst[j] = m_pVertices[i].c;
					}
				}
			}

			ok = true;

			return false;
		}
		else if(prim == GS_LINELIST && count == 512*2 && ok)
		{
			// normally, this step would copy the video onto screen with 512 texture mapped horizontal lines,
			// but we use the stored video data to create a new texture, and replace the lines with two triangles

			ok = false;

			m_dev.Recycle(tex->m_texture);
			m_dev.Recycle(tex->m_palette);

			m_dev.CreateTexture(tex->m_texture, 512, 512);

			D3D10_BOX box = {0, 0, 0, 448, 512, 1};

			m_dev->UpdateSubresource(tex->m_texture, 0, &box, video, 512*4, 0);

			m_pVertices[0] = m_pVertices[0];
			m_pVertices[1] = m_pVertices[m_nVertices - 1];

			prim = GS_SPRITE;
			count = 2;

			return true;
		}
	}

	#pragma endregion

	return true;
}

