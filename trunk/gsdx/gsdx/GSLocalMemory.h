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

#pragma warning(disable: 4100) // warning C4100: 'TEXA' : unreferenced formal parameter

#include "GS.h"
#include "GSTables.h"
#include "GSVector.h"

class GSLocalMemory
{
public:
	typedef DWORD (*pixelAddress)(int x, int y, DWORD bp, DWORD bw);
	typedef void (GSLocalMemory::*writePixel)(int x, int y, DWORD c, DWORD bp, DWORD bw);
	typedef void (GSLocalMemory::*writeFrame)(int x, int y, DWORD c, DWORD bp, DWORD bw);
	typedef DWORD (GSLocalMemory::*readPixel)(int x, int y, DWORD bp, DWORD bw);
	typedef DWORD (GSLocalMemory::*readTexel)(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	typedef void (GSLocalMemory::*writePixelAddr)(DWORD addr, DWORD c);
	typedef void (GSLocalMemory::*writeFrameAddr)(DWORD addr, DWORD c);
	typedef DWORD (GSLocalMemory::*readPixelAddr)(DWORD addr);
	typedef DWORD (GSLocalMemory::*readTexelAddr)(DWORD addr, GIFRegTEXA& TEXA);
	typedef void (GSLocalMemory::*SwizzleTexture)(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	typedef void (GSLocalMemory::*unSwizzleTexture)(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	typedef void (GSLocalMemory::*readTexture)(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA, GIFRegCLAMP& CLAMP);

	typedef union 
	{
		struct
		{
			pixelAddress pa, ba, pga;
			readPixel rp;
			readPixelAddr rpa;
			writePixel wp;
			writePixelAddr wpa;
			readTexel rt, rtNP;
			readTexelAddr rta;
			writeFrameAddr wfa;
			SwizzleTexture st;
			unSwizzleTexture ust, ustNP;
			DWORD bpp, pal, trbpp; 
			CSize bs, pgs;
			int* rowOffset[8];
		};
		BYTE dummy[128];
	} psm_t;

	static psm_t m_psm[64];

protected:
	static DWORD pageOffset32[32][32][64];
	static DWORD pageOffset32Z[32][32][64];
	static DWORD pageOffset16[32][64][64];
	static DWORD pageOffset16S[32][64][64];
	static DWORD pageOffset16Z[32][64][64];
	static DWORD pageOffset16SZ[32][64][64];
	static DWORD pageOffset8[32][64][128];
	static DWORD pageOffset4[32][128][128];

	static int rowOffset32[2048];
	static int rowOffset32Z[2048];
	static int rowOffset16[2048];
	static int rowOffset16S[2048];
	static int rowOffset16Z[2048];
	static int rowOffset16SZ[2048];
	static int rowOffset8[2][2048];
	static int rowOffset4[2][2048];

	union {BYTE* m_vm8; WORD* m_vm16; DWORD* m_vm32;};

	DWORD m_CBP[2];
	WORD* m_pCLUT;
	DWORD* m_pCLUT32;
	UINT64* m_pCLUT64;

	GIFRegTEX0 m_prevTEX0;
	GIFRegTEXCLUT m_prevTEXCLUT;
	bool m_fCLUTMayBeDirty;

	__forceinline static void unSwizzleBlock32(BYTE* src, BYTE* dst, int dstpitch);
	__forceinline static void unSwizzleBlock16(BYTE* src, BYTE* dst, int dstpitch);
	__forceinline static void unSwizzleBlock8(BYTE* src, BYTE* dst, int dstpitch);
	__forceinline static void unSwizzleBlock4(BYTE* src, BYTE* dst, int dstpitch);
	__forceinline static void SwizzleBlock32(BYTE* dst, BYTE* src, int srcpitch);
	__forceinline static void SwizzleBlock32(BYTE* dst, BYTE* src, int srcpitch, DWORD mask);
	__forceinline static void SwizzleBlock32u(BYTE* dst, BYTE* src, int srcpitch);
	__forceinline static void SwizzleBlock32u(BYTE* dst, BYTE* src, int srcpitch, DWORD mask);
	__forceinline static void SwizzleBlock16(BYTE* dst, BYTE* src, int srcpitch);
	__forceinline static void SwizzleBlock16u(BYTE* dst, BYTE* src, int srcpitch);
	__forceinline static void SwizzleBlock8(BYTE* dst, BYTE* src, int srcpitch);
	__forceinline static void SwizzleBlock8u(BYTE* dst, BYTE* src, int srcpitch);
	__forceinline static void SwizzleBlock4(BYTE* dst, BYTE* src, int srcpitch);
	__forceinline static void SwizzleBlock4u(BYTE* dst, BYTE* src, int srcpitch);
	__forceinline static void SwizzleColumn32(int y, BYTE* dst, BYTE* src, int srcpitch);
	__forceinline static void SwizzleColumn32(int y, BYTE* dst, BYTE* src, int srcpitch, DWORD mask);
	__forceinline static void SwizzleColumn16(int y, BYTE* dst, BYTE* src, int srcpitch);
	__forceinline static void SwizzleColumn8(int y, BYTE* dst, BYTE* src, int srcpitch);
	__forceinline static void SwizzleColumn4(int y, BYTE* dst, BYTE* src, int srcpitch);

	static void WriteCLUT_T32_I8_CSM1(DWORD* vm, WORD* clut);
	static void WriteCLUT_T32_I4_CSM1(DWORD* vm, WORD* clut);
	static void WriteCLUT_T16_I8_CSM1(WORD* vm, WORD* clut);
	static void WriteCLUT_T16_I4_CSM1(WORD* vm, WORD* clut);
	static void ReadCLUT32_T32_I8(WORD* src, DWORD* dst);
	static void ReadCLUT32_T32_I4(WORD* src, DWORD* dst);
	static void ReadCLUT32_T16_I8(WORD* src, DWORD* dst);
	static void ReadCLUT32_T16_I4(WORD* src, DWORD* dst);

	static void ExpandBlock24(BYTE* src, int srcpitch, DWORD* dst);
	static void ExpandBlock8H(BYTE* src, int srcpitch, DWORD* dst);
	static void ExpandBlock4HL(BYTE* src, int srcpitch, DWORD* dst);
	static void ExpandBlock4HH(BYTE* src, int srcpitch, DWORD* dst);
	static void ExpandBlock24(DWORD* src, DWORD* dst, int dstpitch, GIFRegTEXA* TEXA);
	static void ExpandBlock16(WORD* src, DWORD* dst, int dstpitch, GIFRegTEXA* TEXA);
	static void Expand16(WORD* src, DWORD* dst, int w, GIFRegTEXA* TEXA);

	__forceinline static DWORD Expand24To32(DWORD c, GIFRegTEXA& TEXA)
	{
		return (((!TEXA.AEM | (c & 0xffffff)) ? TEXA.TA0 : 0) << 24) | (c & 0xffffff);
	}

	__forceinline static DWORD Expand16To32(WORD c, GIFRegTEXA& TEXA)
	{
		return (((c & 0x8000) ? TEXA.TA1 : (!TEXA.AEM | c) ? TEXA.TA0 : 0) << 24) | ((c & 0x7c00) << 9) | ((c & 0x03e0) << 6) | ((c & 0x001f) << 3);
	}

public:
	GSLocalMemory();
	virtual ~GSLocalMemory();

	BYTE* GetVM() 
	{
		return m_vm8;
	}

	//

	static int EncodeFPSM(int psm)
	{
		switch(psm)
		{
		case PSM_PSMCT32: return 0;
		case PSM_PSMCT24: return 1;
		case PSM_PSMCT16: return 2;
		case PSM_PSMCT16S: return 3;
		case PSM_PSMZ32: return 4;
		case PSM_PSMZ24: return 5;
		case PSM_PSMZ16: return 6;
		case PSM_PSMZ16S: return 7;
		}

		return -1;
	}

	static int DecodeFPSM(int index)
	{
		switch(index)
		{
		case 0: return PSM_PSMCT32;
		case 1: return PSM_PSMCT24;
		case 2: return PSM_PSMCT16;
		case 3: return PSM_PSMCT16S;
		case 4: return PSM_PSMZ32;
		case 5: return PSM_PSMZ24;
		case 6: return PSM_PSMZ16;
		case 7: return PSM_PSMZ16S;
		}

		return -1;
	}

	static int EncodeZPSM(int psm)
	{
		switch(psm)
		{
		case PSM_PSMZ32: return 0;
		case PSM_PSMZ24: return 1;
		case PSM_PSMZ16: return 2;
		case PSM_PSMZ16S: return 3;
		}

		return -1;
	}

	static int DecodeZPSM(int index)
	{
		switch(index)
		{
		case 0: return PSM_PSMZ32;
		case 1: return PSM_PSMZ24;
		case 2: return PSM_PSMZ16;
		case 3: return PSM_PSMZ16S;
		}

		return -1;
	}

	// address

	static DWORD pageAddress32(int x, int y, DWORD bp, DWORD bw)
	{
		return ((bp >> 5) + (y >> 5) * bw + (x >> 6)) << 11; 
	}

	static DWORD pageAddress16(int x, int y, DWORD bp, DWORD bw)
	{
		return ((bp >> 5) + (y >> 6) * bw + (x >> 6)) << 12;
	}

	static DWORD pageAddress8(int x, int y, DWORD bp, DWORD bw)
	{
		return ((bp >> 5) + (y >> 6) * ((bw + 1) >> 1) + (x >> 7)) << 13; 
	}

	static DWORD pageAddress4(int x, int y, DWORD bp, DWORD bw)
	{
		return ((bp >> 5) + (y >> 7) * ((bw + 1) >> 1) + (x >> 7)) << 14;
	}

	static DWORD blockAddress32(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = bp + (y & ~0x1f) * bw + ((x >> 1) & ~0x1f);
		DWORD block = blockTable32[(y >> 3) & 3][(x >> 3) & 7];
		return (page + block) << 6;
	}

	static DWORD blockAddress16(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
		DWORD block = blockTable16[(y >> 3) & 7][(x >> 4) & 3];
		return (page + block) << 7;
	}

	static DWORD blockAddress16S(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
		DWORD block = blockTable16S[(y >> 3) & 7][(x >> 4) & 3];
		return (page + block) << 7;
	}

	static DWORD blockAddress8(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = bp + ((y >> 1) & ~0x1f) * ((bw+1)>>1) + ((x >> 2) & ~0x1f); 
		DWORD block = blockTable8[(y >> 4) & 3][(x >> 4) & 7];
		return (page + block) << 8;
	}

	static DWORD blockAddress4(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = bp + ((y >> 2) & ~0x1f) * ((bw+1)>>1) + ((x >> 2) & ~0x1f); 
		DWORD block = blockTable4[(y >> 4) & 7][(x >> 5) & 3];
		return (page + block) << 9;
	}

	static DWORD blockAddress32Z(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = bp + (y & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
		DWORD block = blockTable32Z[(y >> 3) & 3][(x >> 3) & 7];
		return (page + block) << 6;
	}

	static DWORD blockAddress16Z(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
		DWORD block = blockTable16Z[(y >> 3) & 7][(x >> 4) & 3];
		return (page + block) << 7;
	}

	static DWORD blockAddress16SZ(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
		DWORD block = blockTable16SZ[(y >> 3) & 7][(x >> 4) & 3];
		return (page + block) << 7;
	}

	static DWORD pixelAddressOrg32(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = bp + (y & ~0x1f) * bw + ((x >> 1) & ~0x1f);
		DWORD block = blockTable32[(y >> 3) & 3][(x >> 3) & 7];
		DWORD word = ((page + block) << 6) + columnTable32[y & 7][x & 7];
		ASSERT(word < 1024*1024);
		return word;
	}

	static DWORD pixelAddressOrg16(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
		DWORD block = blockTable16[(y >> 3) & 7][(x >> 4) & 3];
		DWORD word = ((page + block) << 7) + columnTable16[y & 7][x & 15];
		ASSERT(word < 1024*1024*2);
		return word;
	}

	static DWORD pixelAddressOrg16S(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
		DWORD block = blockTable16S[(y >> 3) & 7][(x >> 4) & 3];
		DWORD word = ((page + block) << 7) + columnTable16[y & 7][x & 15];
		ASSERT(word < 1024*1024*2);
		return word;
	}

	static DWORD pixelAddressOrg8(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = bp + ((y >> 1) & ~0x1f) * ((bw + 1)>>1) + ((x >> 2) & ~0x1f); 
		DWORD block = blockTable8[(y >> 4) & 3][(x >> 4) & 7];
		DWORD word = ((page + block) << 8) + columnTable8[y & 15][x & 15];
	//	ASSERT(word < 1024*1024*4);
		return word;
	}

	static DWORD pixelAddressOrg4(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = bp + ((y >> 2) & ~0x1f) * ((bw + 1)>>1) + ((x >> 2) & ~0x1f); 
		DWORD block = blockTable4[(y >> 4) & 7][(x >> 5) & 3];
		DWORD word = ((page + block) << 9) + columnTable4[y & 15][x & 31];
		ASSERT(word < 1024*1024*8);
		return word;
	}

	static DWORD pixelAddressOrg32Z(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = bp + (y & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
		DWORD block = blockTable32Z[(y >> 3) & 3][(x >> 3) & 7];
		DWORD word = ((page + block) << 6) + columnTable32[y & 7][x & 7];
		ASSERT(word < 1024*1024);
		return word;
	}

	static DWORD pixelAddressOrg16Z(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
		DWORD block = blockTable16Z[(y >> 3) & 7][(x >> 4) & 3];
		DWORD word = ((page + block) << 7) + columnTable16[y & 7][x & 15];
		ASSERT(word < 1024*1024*2);
		return word;
	}

	static DWORD pixelAddressOrg16SZ(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
		DWORD block = blockTable16SZ[(y >> 3) & 7][(x >> 4) & 3];
		DWORD word = ((page + block) << 7) + columnTable16[y & 7][x & 15];
		ASSERT(word < 1024*1024*2);
		return word;
	}

	static __forceinline DWORD pixelAddress32(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = (bp >> 5) + (y >> 5) * bw + (x >> 6); 
		DWORD word = (page << 11) + pageOffset32[bp & 0x1f][y & 0x1f][x & 0x3f];
		return word;
	}

	static __forceinline DWORD pixelAddress16(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
		DWORD word = (page << 12) + pageOffset16[bp & 0x1f][y & 0x3f][x & 0x3f];
		return word;
	}

	static __forceinline DWORD pixelAddress16S(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
		DWORD word = (page << 12) + pageOffset16S[bp & 0x1f][y & 0x3f][x & 0x3f];
		return word;
	}

	static __forceinline DWORD pixelAddress8(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = (bp >> 5) + (y >> 6) * ((bw + 1)>>1) + (x >> 7); 
		DWORD word = (page << 13) + pageOffset8[bp & 0x1f][y & 0x3f][x & 0x7f];
		return word;
	}

	static __forceinline DWORD pixelAddress4(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = (bp >> 5) + (y >> 7) * ((bw + 1)>>1) + (x >> 7);
		DWORD word = (page << 14) + pageOffset4[bp & 0x1f][y & 0x7f][x & 0x7f];
		return word;
	}

	static __forceinline DWORD pixelAddress32Z(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = (bp >> 5) + (y >> 5) * bw + (x >> 6); 
		DWORD word = (page << 11) + pageOffset32Z[bp & 0x1f][y & 0x1f][x & 0x3f];
		return word;
	}

	static __forceinline DWORD pixelAddress16Z(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
		DWORD word = (page << 12) + pageOffset16Z[bp & 0x1f][y & 0x3f][x & 0x3f];
		return word;
	}

	static __forceinline DWORD pixelAddress16SZ(int x, int y, DWORD bp, DWORD bw)
	{
		DWORD page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
		DWORD word = (page << 12) + pageOffset16SZ[bp & 0x1f][y & 0x3f][x & 0x3f];
		return word;
	}

	// pixel R/W

	__forceinline DWORD readPixel32(DWORD addr) 
	{
		return m_vm32[addr];
	}

	__forceinline DWORD readPixel24(DWORD addr) 
	{
		return m_vm32[addr] & 0x00ffffff;
	}

	__forceinline DWORD readPixel16(DWORD addr) 
	{
		return (DWORD)m_vm16[addr];
	}

	__forceinline DWORD readPixel8(DWORD addr) 
	{
		return (DWORD)m_vm8[addr];
	}

	__forceinline DWORD readPixel4(DWORD addr) 
	{
		return (m_vm8[addr >> 1] >> ((addr & 1) << 2)) & 0x0f;
	}

	__forceinline DWORD readPixel8H(DWORD addr) 
	{
		return m_vm32[addr] >> 24;
	}

	__forceinline DWORD readPixel4HL(DWORD addr) 
	{
		return (m_vm32[addr] >> 24) & 0x0f;
	}

	__forceinline DWORD readPixel4HH(DWORD addr) 
	{
		return (m_vm32[addr] >> 28) & 0x0f;
	}

	__forceinline DWORD readFrame24(DWORD addr) 
	{
		return 0x80000000 | (m_vm32[addr] & 0xffffff);
	}

	__forceinline DWORD readFrame16(DWORD addr) 
	{
		DWORD c = (DWORD)m_vm16[addr];

		return ((c & 0x8000) << 16) | ((c & 0x7c00) << 9) | ((c & 0x03e0) << 6) | ((c & 0x001f) << 3);
	}

	__forceinline DWORD readPixel32(int x, int y, DWORD bp, DWORD bw)
	{
		return readPixel32(pixelAddress32(x, y, bp, bw));
	}

	__forceinline DWORD readPixel24(int x, int y, DWORD bp, DWORD bw)
	{
		return readPixel24(pixelAddress32(x, y, bp, bw));
	}

	__forceinline DWORD readPixel16(int x, int y, DWORD bp, DWORD bw)
	{
		return readPixel16(pixelAddress16(x, y, bp, bw));
	}

	__forceinline DWORD readPixel16S(int x, int y, DWORD bp, DWORD bw)
	{
		return readPixel16(pixelAddress16S(x, y, bp, bw));
	}

	__forceinline DWORD readPixel8(int x, int y, DWORD bp, DWORD bw)
	{
		return readPixel8(pixelAddress8(x, y, bp, bw));
	}

	__forceinline DWORD readPixel4(int x, int y, DWORD bp, DWORD bw)
	{
		return readPixel4(pixelAddress4(x, y, bp, bw));
	}

	__forceinline DWORD readPixel8H(int x, int y, DWORD bp, DWORD bw)
	{
		return readPixel8H(pixelAddress32(x, y, bp, bw));
	}

	__forceinline DWORD readPixel4HL(int x, int y, DWORD bp, DWORD bw)
	{
		return readPixel4HL(pixelAddress32(x, y, bp, bw));
	}

	__forceinline DWORD readPixel4HH(int x, int y, DWORD bp, DWORD bw)
	{
		return readPixel4HH(pixelAddress32(x, y, bp, bw));
	}

	__forceinline DWORD readPixel32Z(int x, int y, DWORD bp, DWORD bw)
	{
		return readPixel32(pixelAddress32Z(x, y, bp, bw));
	}

	__forceinline DWORD readPixel24Z(int x, int y, DWORD bp, DWORD bw)
	{
		return readPixel24(pixelAddress32Z(x, y, bp, bw));
	}

	__forceinline DWORD readPixel16Z(int x, int y, DWORD bp, DWORD bw)
	{
		return readPixel16(pixelAddress16Z(x, y, bp, bw));
	}

	__forceinline DWORD readPixel16SZ(int x, int y, DWORD bp, DWORD bw)
	{
		return readPixel16(pixelAddress16SZ(x, y, bp, bw));
	}

	__forceinline DWORD readFrame24(int x, int y, DWORD bp, DWORD bw)
	{
		return readFrame24(pixelAddress32(x, y, bp, bw));
	}

	__forceinline DWORD readFrame16(int x, int y, DWORD bp, DWORD bw)
	{
		return readFrame16(pixelAddress16(x, y, bp, bw));
	}

	__forceinline DWORD readFrame16S(int x, int y, DWORD bp, DWORD bw)
	{
		return readFrame16(pixelAddress16S(x, y, bp, bw));
	}

	__forceinline DWORD readFrame24Z(int x, int y, DWORD bp, DWORD bw)
	{
		return readFrame24(pixelAddress32Z(x, y, bp, bw));
	}

	__forceinline DWORD readFrame16Z(int x, int y, DWORD bp, DWORD bw)
	{
		return readFrame16(pixelAddress16Z(x, y, bp, bw));
	}

	__forceinline DWORD readFrame16SZ(int x, int y, DWORD bp, DWORD bw)
	{
		return readFrame16(pixelAddress16SZ(x, y, bp, bw));
	}

	__forceinline void writePixel32(DWORD addr, DWORD c) 
	{
		m_vm32[addr] = c;
	}

	__forceinline void writePixel24(DWORD addr, DWORD c) 
	{
		m_vm32[addr] = (m_vm32[addr] & 0xff000000) | (c & 0x00ffffff);
	}

	__forceinline void writePixel16(DWORD addr, DWORD c) 
	{
		m_vm16[addr] = (WORD)c;
	}

	__forceinline void writePixel8(DWORD addr, DWORD c)
	{
		m_vm8[addr] = (BYTE)c;
	}

	__forceinline void writePixel4(DWORD addr, DWORD c) 
	{
		int shift = (addr & 1) << 2; addr >>= 1; 

		m_vm8[addr] = (BYTE)((m_vm8[addr] & (0xf0 >> shift)) | ((c & 0x0f) << shift));
	}

	__forceinline void writePixel8H(DWORD addr, DWORD c)
	{
		m_vm32[addr] = (m_vm32[addr] & 0x00ffffff) | (c << 24);
	}

	__forceinline void writePixel4HL(DWORD addr, DWORD c) 
	{
		m_vm32[addr] = (m_vm32[addr] & 0xf0ffffff) | ((c & 0x0f) << 24);
	}

	__forceinline void writePixel4HH(DWORD addr, DWORD c)
	{
		m_vm32[addr] = (m_vm32[addr] & 0x0fffffff) | ((c & 0x0f) << 28);
	}

	__forceinline void writeFrame16(DWORD addr, DWORD c) 
	{
		writePixel16(addr, ((c >> 16) & 0x8000) | ((c >> 9) & 0x7c00) | ((c >> 6) & 0x03e0) | ((c >> 3) & 0x001f));
	}

	__forceinline void writePixel32(int x, int y, DWORD c, DWORD bp, DWORD bw)
	{
		writePixel32(pixelAddress32(x, y, bp, bw), c);
	}

	__forceinline void writePixel24(int x, int y, DWORD c, DWORD bp, DWORD bw)
	{
		writePixel24(pixelAddress32(x, y, bp, bw), c);
	}

	__forceinline void writePixel16(int x, int y, DWORD c, DWORD bp, DWORD bw)
	{
		writePixel16(pixelAddress16(x, y, bp, bw), c);
	}

	__forceinline void writePixel16S(int x, int y, DWORD c, DWORD bp, DWORD bw)
	{
		writePixel16(pixelAddress16S(x, y, bp, bw), c);
	}

	__forceinline void writePixel8(int x, int y, DWORD c, DWORD bp, DWORD bw)
	{
		writePixel8(pixelAddress8(x, y, bp, bw), c);
	}

	__forceinline void writePixel4(int x, int y, DWORD c, DWORD bp, DWORD bw)
	{
		writePixel4(pixelAddress4(x, y, bp, bw), c);
	}

	__forceinline void writePixel8H(int x, int y, DWORD c, DWORD bp, DWORD bw)
	{
		writePixel8H(pixelAddress32(x, y, bp, bw), c);
	}

    __forceinline void writePixel4HL(int x, int y, DWORD c, DWORD bp, DWORD bw)
	{
		writePixel4HL(pixelAddress32(x, y, bp, bw), c);
	}

	__forceinline void writePixel4HH(int x, int y, DWORD c, DWORD bp, DWORD bw)
	{
		writePixel4HH(pixelAddress32(x, y, bp, bw), c);
	}

	__forceinline void writePixel32Z(int x, int y, DWORD c, DWORD bp, DWORD bw)
	{
		writePixel32(pixelAddress32Z(x, y, bp, bw), c);
	}

	__forceinline void writePixel24Z(int x, int y, DWORD c, DWORD bp, DWORD bw)
	{
		writePixel24(pixelAddress32Z(x, y, bp, bw), c);
	}

	__forceinline void writePixel16Z(int x, int y, DWORD c, DWORD bp, DWORD bw)
	{
		writePixel16(pixelAddress16Z(x, y, bp, bw), c);
	}

	__forceinline void writePixel16SZ(int x, int y, DWORD c, DWORD bp, DWORD bw)
	{
		writePixel16(pixelAddress16SZ(x, y, bp, bw), c);
	}

	__forceinline void writeFrame16(int x, int y, DWORD c, DWORD bp, DWORD bw)
	{
		writeFrame16(pixelAddress16(x, y, bp, bw), c);
	}

	__forceinline void writeFrame16S(int x, int y, DWORD c, DWORD bp, DWORD bw)
	{
		writeFrame16(pixelAddress16S(x, y, bp, bw), c);
	}

	__forceinline void writeFrame16Z(int x, int y, DWORD c, DWORD bp, DWORD bw)
	{
		writeFrame16(pixelAddress16Z(x, y, bp, bw), c);
	}

	__forceinline void writeFrame16SZ(int x, int y, DWORD c, DWORD bp, DWORD bw)
	{
		writeFrame16(pixelAddress16SZ(x, y, bp, bw), c);
	}

	__forceinline DWORD readTexel32(DWORD addr, GIFRegTEXA& TEXA) 
	{
		return m_vm32[addr];
	}

	__forceinline DWORD readTexel24(DWORD addr, GIFRegTEXA& TEXA) 
	{
		return Expand24To32(m_vm32[addr], TEXA);
	}

	__forceinline DWORD readTexel16(DWORD addr, GIFRegTEXA& TEXA) 
	{
		return Expand16To32(m_vm16[addr], TEXA);
	}

	__forceinline DWORD readTexel8(DWORD addr, GIFRegTEXA& TEXA) 
	{
		return m_pCLUT32[readPixel8(addr)];
	}

	__forceinline DWORD readTexel4(DWORD addr, GIFRegTEXA& TEXA) 
	{
		return m_pCLUT32[readPixel4(addr)];
	}

	__forceinline DWORD readTexel8H(DWORD addr, GIFRegTEXA& TEXA) 
	{
		return m_pCLUT32[readPixel8H(addr)];
	}

	__forceinline DWORD readTexel4HL(DWORD addr, GIFRegTEXA& TEXA)
	{
		return m_pCLUT32[readPixel4HL(addr)];
	}

	__forceinline DWORD readTexel4HH(DWORD addr, GIFRegTEXA& TEXA) 
	{
		return m_pCLUT32[readPixel4HH(addr)];
	}

	__forceinline DWORD readTexel32(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
	{
		return readTexel32(pixelAddress32(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline DWORD readTexel24(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
	{
		return readTexel24(pixelAddress32(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline DWORD readTexel16(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
	{
		return readTexel16(pixelAddress16(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline DWORD readTexel16S(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
	{
		return readTexel16(pixelAddress16S(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline DWORD readTexel8(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
	{
		return readTexel8(pixelAddress8(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline DWORD readTexel4(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
	{
		return readTexel4(pixelAddress4(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline DWORD readTexel8H(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
	{
		return readTexel8H(pixelAddress32(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline DWORD readTexel4HL(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
	{
		return readTexel4HL(pixelAddress32(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline DWORD readTexel4HH(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
	{
		return readTexel4HH(pixelAddress32(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline DWORD readTexel32Z(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
	{
		return readTexel32(pixelAddress32Z(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline DWORD readTexel24Z(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
	{
		return readTexel24(pixelAddress32Z(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline DWORD readTexel16Z(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
	{
		return readTexel16(pixelAddress16Z(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline DWORD readTexel16SZ(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
	{
		return readTexel16(pixelAddress16SZ(x, y, TEX0.TBP0, TEX0.TBW), TEXA);
	}

	__forceinline DWORD readTexel16NP(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
	{
		return readPixel16(x, y, TEX0.TBP0, TEX0.TBW);
	}

	__forceinline DWORD readTexel16SNP(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
	{
		return readPixel16S(x, y, TEX0.TBP0, TEX0.TBW);
	}

	__forceinline DWORD readTexel16ZNP(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
	{
		return readPixel16Z(x, y, TEX0.TBP0, TEX0.TBW);
	}

	__forceinline DWORD readTexel16SZNP(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
	{
		return readPixel16SZ(x, y, TEX0.TBP0, TEX0.TBW);
	}

	//

	__forceinline DWORD pixelAddressX(int PSM, int x, int y, DWORD bp, DWORD bw)
	{
		switch(PSM)
		{
		case PSM_PSMCT32: return pixelAddress32(x, y, bp, bw); 
		case PSM_PSMCT24: return pixelAddress32(x, y, bp, bw); 
		case PSM_PSMCT16: return pixelAddress16(x, y, bp, bw);
		case PSM_PSMCT16S: return pixelAddress16S(x, y, bp, bw);
		case PSM_PSMT8: return pixelAddress8(x, y, bp, bw);
		case PSM_PSMT4: return pixelAddress4(x, y, bp, bw);
		case PSM_PSMT8H: return pixelAddress32(x, y, bp, bw);
		case PSM_PSMT4HL: return pixelAddress32(x, y, bp, bw);
		case PSM_PSMT4HH: return pixelAddress32(x, y, bp, bw);
		case PSM_PSMZ32: return pixelAddress32Z(x, y, bp, bw);
		case PSM_PSMZ24: return pixelAddress32Z(x, y, bp, bw);
		case PSM_PSMZ16: return pixelAddress16Z(x, y, bp, bw);
		case PSM_PSMZ16S: return pixelAddress16SZ(x, y, bp, bw);
		default: ASSERT(0); return pixelAddress32(x, y, bp, bw);
		}
	}

	__forceinline DWORD readPixelX(int PSM, DWORD addr)
	{
		switch(PSM)
		{
		case PSM_PSMCT32: return readPixel32(addr); 
		case PSM_PSMCT24: return readPixel24(addr); 
		case PSM_PSMCT16: return readPixel16(addr);
		case PSM_PSMCT16S: return readPixel16(addr);
		case PSM_PSMT8: return readPixel8(addr);
		case PSM_PSMT4: return readPixel4(addr);
		case PSM_PSMT8H: return readPixel8H(addr);
		case PSM_PSMT4HL: return readPixel4HL(addr);
		case PSM_PSMT4HH: return readPixel4HH(addr);
		case PSM_PSMZ32: return readPixel32(addr);
		case PSM_PSMZ24: return readPixel24(addr);
		case PSM_PSMZ16: return readPixel16(addr);
		case PSM_PSMZ16S: return readPixel16(addr);
		default: ASSERT(0); return readPixel32(addr);
		}
	}
	
	__forceinline DWORD readFrameX(int PSM, DWORD addr)
	{
		switch(PSM)
		{
		case PSM_PSMCT32: return readPixel32(addr);
		case PSM_PSMCT24: return readFrame24(addr);
		case PSM_PSMCT16: return readFrame16(addr);
		case PSM_PSMCT16S: return readFrame16(addr);
		case PSM_PSMZ32: return readPixel32(addr);
		case PSM_PSMZ24: return readFrame24(addr);
		case PSM_PSMZ16: return readFrame16(addr);
		case PSM_PSMZ16S: return readFrame16(addr);
		default: ASSERT(0); return readPixel32(addr);
		}
	}

	__forceinline DWORD readTexelX(int PSM, DWORD addr, GIFRegTEXA& TEXA)
	{
		switch(PSM)
		{
		case PSM_PSMCT32: return readTexel32(addr, TEXA);
		case PSM_PSMCT24: return readTexel24(addr, TEXA);
		case PSM_PSMCT16: return readTexel16(addr, TEXA);
		case PSM_PSMCT16S: return readTexel16(addr, TEXA);
		case PSM_PSMT8: return readTexel8(addr, TEXA);
		case PSM_PSMT4: return readTexel4(addr, TEXA);
		case PSM_PSMT8H: return readTexel8H(addr, TEXA);
		case PSM_PSMT4HL: return readTexel4HL(addr, TEXA);
		case PSM_PSMT4HH: return readTexel4HH(addr, TEXA);
		case PSM_PSMZ32: return readTexel32(addr, TEXA);
		case PSM_PSMZ24: return readTexel24(addr, TEXA);
		case PSM_PSMZ16: return readTexel16(addr, TEXA);
		case PSM_PSMZ16S: return readTexel16(addr, TEXA);
		default: ASSERT(0); return readTexel32(addr, TEXA);
		}
	}

	__forceinline DWORD readTexelX(int PSM, int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
	{
		switch(PSM)
		{
		case PSM_PSMCT32: return readTexel32(x, y, TEX0, TEXA);
		case PSM_PSMCT24: return readTexel24(x, y, TEX0, TEXA);
		case PSM_PSMCT16: return readTexel16(x, y, TEX0, TEXA);
		case PSM_PSMCT16S: return readTexel16(x, y, TEX0, TEXA);
		case PSM_PSMT8: return readTexel8(x, y, TEX0, TEXA);
		case PSM_PSMT4: return readTexel4(x, y, TEX0, TEXA);
		case PSM_PSMT8H: return readTexel8H(x, y, TEX0, TEXA);
		case PSM_PSMT4HL: return readTexel4HL(x, y, TEX0, TEXA);
		case PSM_PSMT4HH: return readTexel4HH(x, y, TEX0, TEXA);
		case PSM_PSMZ32: return readTexel32Z(x, y, TEX0, TEXA);
		case PSM_PSMZ24: return readTexel24Z(x, y, TEX0, TEXA);
		case PSM_PSMZ16: return readTexel16Z(x, y, TEX0, TEXA);
		case PSM_PSMZ16S: return readTexel16Z(x, y, TEX0, TEXA);
		default: ASSERT(0); return readTexel32(x, y, TEX0, TEXA);
		}
	}

	__forceinline void writePixelX(int PSM, DWORD addr, DWORD c)
	{
		switch(PSM)
		{
		case PSM_PSMCT32: writePixel32(addr, c); break; 
		case PSM_PSMCT24: writePixel24(addr, c); break; 
		case PSM_PSMCT16: writePixel16(addr, c); break;
		case PSM_PSMCT16S: writePixel16(addr, c); break;
		case PSM_PSMT8: writePixel8(addr, c); break;
		case PSM_PSMT4: writePixel4(addr, c); break;
		case PSM_PSMT8H: writePixel8H(addr, c); break;
		case PSM_PSMT4HL: writePixel4HL(addr, c); break;
		case PSM_PSMT4HH: writePixel4HH(addr, c); break;
		case PSM_PSMZ32: writePixel32(addr, c); break;
		case PSM_PSMZ24: writePixel24(addr, c); break;
		case PSM_PSMZ16: writePixel16(addr, c); break;
		case PSM_PSMZ16S: writePixel16(addr, c); break;
		default: ASSERT(0); writePixel32(addr, c); break;
		}
	}

	__forceinline void writeFrameX(int PSM, DWORD addr, DWORD c)
	{
		switch(PSM)
		{
		case PSM_PSMCT32: writePixel32(addr, c); break; 
		case PSM_PSMCT24: writePixel24(addr, c); break; 
		case PSM_PSMCT16: writeFrame16(addr, c); break;
		case PSM_PSMCT16S: writeFrame16(addr, c); break;
		case PSM_PSMZ32: writePixel32(addr, c); break; 
		case PSM_PSMZ24: writePixel24(addr, c); break; 
		case PSM_PSMZ16: writeFrame16(addr, c); break;
		case PSM_PSMZ16S: writeFrame16(addr, c); break;
		default: ASSERT(0); writePixel32(addr, c); break;
		}
	}

	__forceinline GSVector4i readFrameX(int PSM, const GSVector4i& addr)
	{
		GSVector4i c, r, g, b, a;

		switch(PSM)
		{
		case PSM_PSMCT32: case PSM_PSMZ32: 
			c.u32[0] = readPixel32(addr.u32[0]);
			c.u32[1] = readPixel32(addr.u32[1]);
			c.u32[2] = readPixel32(addr.u32[2]);
			c.u32[3] = readPixel32(addr.u32[3]);
			break;
		case PSM_PSMCT24: case PSM_PSMZ24: 
			c.u32[0] = readPixel32(addr.u32[0]);
			c.u32[1] = readPixel32(addr.u32[1]);
			c.u32[2] = readPixel32(addr.u32[2]);
			c.u32[3] = readPixel32(addr.u32[3]);
			c = (c & 0x00ffffff) | 0x80000000;
			break;
		case PSM_PSMCT16: case PSM_PSMCT16S: 
		case PSM_PSMZ16: case PSM_PSMZ16S: 
			c.u32[0] = readPixel16(addr.u32[0]);
			c.u32[1] = readPixel16(addr.u32[1]);
			c.u32[2] = readPixel16(addr.u32[2]);
			c.u32[3] = readPixel16(addr.u32[3]);
			c = ((c & 0x001f) << 3) | ((c & 0x03e0) << 6) | ((c & 0x7c00) << 9) | ((c & 0x8000) << 16); 
			break;
		default: 
			ASSERT(0); 
			c = GSVector4i::zero();
		}
		
		return c;
	}

	__forceinline void writeFrameX(int PSM, const GSVector4i& addr, const GSVector4i& c, const GSVector4i& mask, int pixels)
	{
		GSVector4i rb, ga, tmp;

		switch(PSM)
		{
		case PSM_PSMCT32: case PSM_PSMZ32: 
			for(int i = 0; i < pixels; i++)
				if(mask.u32[i] != 0xffffffff)
					writePixel32(addr.u32[i], c.u32[i]); 
			break; 
		case PSM_PSMCT24: case PSM_PSMZ24: 
			for(int i = 0; i < pixels; i++)
				if(mask.u32[i] != 0xffffffff)
					writePixel24(addr.u32[i], c.u32[i]); 
			break; 
		case PSM_PSMCT16: case PSM_PSMCT16S: 
		case PSM_PSMZ16: case PSM_PSMZ16S: 
			rb = c & 0x00f800f8;
			ga = c & 0x8000f800;
			tmp = (rb >> 3) | (ga >> 6) | (rb >> 9) | (ga >> 16);
			for(int i = 0; i < pixels; i++)
				if(mask.u32[i] != 0xffffffff)
					writePixel16(addr.u32[i], tmp.u16[i*2]); 
			break;
		default: 
			ASSERT(0); 
			break;
		}
	}

	// FillRect

	bool FillRect(const CRect& r, DWORD c, DWORD psm, DWORD fbp, DWORD fbw);

	// CLUT

	void InvalidateCLUT() {m_fCLUTMayBeDirty = true;}
	bool IsCLUTDirty(GIFRegTEX0 TEX0, GIFRegTEXCLUT TEXCLUT);
	bool IsCLUTUpdating(GIFRegTEX0 TEX0, GIFRegTEXCLUT TEXCLUT);
	bool WriteCLUT(GIFRegTEX0 TEX0, GIFRegTEXCLUT TEXCLUT);

	void ReadCLUT(GIFRegTEX0 TEX0, DWORD* pCLUT32);
	void SetupCLUT(GIFRegTEX0 TEX0);

	// expands 16->32

	void ReadCLUT32(GIFRegTEX0 TEX0, GIFRegTEXA TEXA, DWORD* pCLUT32);
	void SetupCLUT32(GIFRegTEX0 TEX0, GIFRegTEXA TEXA);
	void CopyCLUT32(DWORD* pCLUT32, int n);

	// 

	void SwizzleTexture32(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void SwizzleTexture24(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void SwizzleTexture16(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void SwizzleTexture16S(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void SwizzleTexture8(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void SwizzleTexture8H(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void SwizzleTexture4(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void SwizzleTexture4HL(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void SwizzleTexture4HH(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void SwizzleTexture32Z(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void SwizzleTexture24Z(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void SwizzleTexture16Z(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void SwizzleTexture16SZ(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);
	void SwizzleTextureX(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG);

	//

	void unSwizzleTexture32(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture24(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture16(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture16S(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture8(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture8H(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture4(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture4HL(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture4HH(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture32Z(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture24Z(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture16Z(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture16SZ(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);

	void ReadTexture(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA, GIFRegCLAMP& CLAMP);
	void ReadTextureNC(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA, GIFRegCLAMP& CLAMP);

	// 32/16

	void unSwizzleTexture16NP(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture16SNP(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture8NP(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture8HNP(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture4NP(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture4HLNP(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture4HHNP(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture16ZNP(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);
	void unSwizzleTexture16SZNP(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA);

	void ReadTextureNP(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA, GIFRegCLAMP& CLAMP);
	void ReadTextureNPNC(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA, GIFRegCLAMP& CLAMP);

	//

	static DWORD m_xtbl[1024], m_ytbl[1024]; 

	template<typename T> void ReadTexture(CRect r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA, GIFRegCLAMP& CLAMP, readTexel rt, unSwizzleTexture st);
	template<typename T> void ReadTextureNC(CRect r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA, readTexel rt, unSwizzleTexture st);

	//

	HRESULT SaveBMP(LPCTSTR fn, DWORD bp, DWORD bw, DWORD psm, int w, int h);
};

#pragma warning(default: 4244)