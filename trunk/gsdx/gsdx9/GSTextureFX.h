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

#include "GSVertexHW.h"
#include "GSDevice.h"

#define PS_ALPHA_TEST

class GSTextureFX
{
public:
	#pragma pack(push, 1)

	struct VSConstantBuffer
	{
		D3DXVECTOR4 VertexScale;
		D3DXVECTOR4 VertexOffset;
		D3DXVECTOR2 TextureScale;
		float _pad[2];
	};

	struct PSConstantBuffer
	{
		D3DXVECTOR4 FogColor;
		D3DXVECTOR2 ClampMin;
		D3DXVECTOR2 ClampMax;
		float TA0;
		float TA1;
		float AREF;
		float _pad[1];
		D3DXVECTOR2 WH;
		D3DXVECTOR2 rWrH;
		D3DXVECTOR2 rWZ;
		D3DXVECTOR2 ZrH;
	};

	union PSSelector
	{
		struct
		{
			DWORD fst:1;
			DWORD clamp:1;
			DWORD bpp:3;
			DWORD aem:1;
			DWORD tfx:3;
			DWORD tcc:1;
			DWORD ate:1;
			DWORD atst:3;
			DWORD fog:1;
			DWORD clr1:1;
			DWORD rt:1;
		};

		DWORD dw;

		operator DWORD() {return dw & 0x1ffff;}
	};

	union PSSamplerSelector
	{
		struct
		{
			DWORD tau:1;
			DWORD tav:1;
			DWORD min:1;
			DWORD mag:1;
		};

		DWORD dw;

		operator DWORD() {return dw & 0xf;}
	};

	union OMDepthStencilSelector
	{
		struct
		{
			DWORD zte:1;
			DWORD ztst:2;
			DWORD zwe:1;
			DWORD date:1;
		};

		DWORD dw;

		operator DWORD() {return dw & 0x1f;}
	};

	union OMBlendSelector
	{
		struct
		{
			DWORD abe:1;
			DWORD a:2;
			DWORD b:2;
			DWORD c:2;
			DWORD d:2;
			DWORD wr:1;
			DWORD wg:1;
			DWORD wb:1;
			DWORD wa:1;
		};

		DWORD dw;

		operator DWORD() {return dw & 0x1fff;}
	};

	#pragma pack(pop)

private:
	GSDevice* m_dev;
	CComPtr<IDirect3DVertexDeclaration9> m_vd;
	CComPtr<IDirect3DVertexShader9> m_vs;
	CComPtr<ID3DXConstantTable> m_vs_ct;
	D3DXHANDLE m_vs_params;
	CSimpleMap<DWORD, CComPtr<IDirect3DPixelShader9> > m_ps;

public:
	GSTextureFX();

	bool Create(GSDevice* dev);
	
	bool SetupVS(const VSConstantBuffer* cb);
	bool SetupPS(PSSelector sel, const PSConstantBuffer* cb, PSSamplerSelector ssel, IDirect3DTexture9* tex, IDirect3DTexture9* pal);
	void UpdatePS(PSSelector sel, PSSamplerSelector ssel);
	void SetupRS(const RECT& scissor);
	void SetupOM(OMDepthStencilSelector dssel, OMBlendSelector bsel, BYTE bf, IDirect3DSurface9* rt, IDirect3DSurface9* ds);
	void UpdateOM(OMDepthStencilSelector dssel, OMBlendSelector bsel, BYTE bf);
};
