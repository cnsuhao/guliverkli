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

#include "GS.h"
#include "GSWnd.h"
#include "GSLocalMemory.h"
#include "GSTextureCache.h"
#include "GSSoftVertex.h"
#include "GSVertexList.h"

// FIXME: sfex3

//
//#define DEBUG_SAVETEXTURES
//#define DEBUG_LOG
//#define DEBUG_LOG2
//#define DEBUG_LOGVERTICES
//#define DEBUG_RENDERTARGETS

// You don't want to enable this, short stips can actually slow rendering down 
// by adding too many drawprim calls. The ps2 bios is a good example where many rects are 
// drawn as a pair of triangle strips, it's simply a lot faster without it.

// #define ENABLE_STRIPFAN

struct GSDrawingContext
{
	struct GSDrawingContext() {memset(this, 0, sizeof(*this));}

	GIFRegXYOFFSET	XYOFFSET;
	GIFRegTEX0		TEX0;
	GIFRegTEX1		TEX1;
	GIFRegTEX2		TEX2;
	GIFRegCLAMP		CLAMP;
	GIFRegMIPTBP1	MIPTBP1;
	GIFRegMIPTBP2	MIPTBP2;
	GIFRegSCISSOR	SCISSOR;
	GIFRegALPHA		ALPHA;
	GIFRegTEST		TEST;
	GIFRegFBA		FBA;
	GIFRegFRAME		FRAME;
	GIFRegZBUF		ZBUF;

	GSLocalMemory::readTexel rp;
	GSLocalMemory::writeFrame wf;
	GSLocalMemory::readPixel rz;
	GSLocalMemory::writePixel wz;
	GSLocalMemory::readTexel rt;
};

struct GSDrawingEnvironment
{
	struct GSDrawingEnvironment() {memset(this, 0, sizeof(*this));}

	GIFRegPRIM			PRIM;
	GIFRegPRMODECONT	PRMODECONT;
	GIFRegTEXCLUT		TEXCLUT;
	GIFRegSCANMSK		SCANMSK;
	GIFRegTEXA			TEXA;
	GIFRegFOGCOL		FOGCOL;
	GIFRegDIMX			DIMX;
	GIFRegDTHE			DTHE;
	GIFRegCOLCLAMP		COLCLAMP;
	GIFRegPABE			PABE;
	GSDrawingContext	CTXT[2];
};

struct GSRegSet
{
	struct GSRegSet() {memset(this, 0, sizeof(*this));}

	CSize GetSize(int en)
	{
		ASSERT(en >= 0 && en < 2);
		CSize size;
		size.cx = DISPLAY[en].DW / (DISPLAY[en].MAGH+1) + 1;
		size.cy = DISPLAY[en].DH / (DISPLAY[en].MAGV+1) + 1;
		if(SMODE2.INT && SMODE2.FFMD && size.cy > 1) size.cy >>= 1;
		return size;
	}

	bool IsEnabled(int en)
	{
		ASSERT(en >= 0 && en < 2);
		if(en == 0 && PMODE.EN1) {return(DISPLAY[0].DW || DISPLAY[0].DH);}
		else if(en == 1 && PMODE.EN2) {return(DISPLAY[1].DW || DISPLAY[1].DH);}
		return(false);
	}

	GSRegBGCOLOR	BGCOLOR;
	GSRegBUSDIR		BUSDIR;
	GSRegCSR		CSRw, CSRr;
	GSRegDISPFB		DISPFB[2];
	GSRegDISPLAY	DISPLAY[2];
	GSRegEXTBUF		EXTBUF;
	GSRegEXTDATA	EXTDATA;
	GSRegEXTWRITE	EXTWRITE;
	GSRegIMR		IMR;
	GSRegPMODE		PMODE;
	GSRegSIGLBLID	SIGLBLID;
	GSRegSMODE1		SMODE1;
	GSRegSMODE2		SMODE2;

	GIFRegBITBLTBUF	BITBLTBUF;
	GIFRegTRXDIR	TRXDIR;
	GIFRegTRXPOS	TRXPOS;
	GIFRegTRXREG	TRXREG;
};

struct GSVertex
{
	GIFRegRGBAQ		RGBAQ;
	GIFRegST		ST;
	GIFRegUV		UV;
	GIFRegXYZ		XYZ;
	GIFRegFOG		FOG;
};

class GSStats
{
	int m_frame, m_prim;
	float m_fps, m_pps, m_ppf;
	CList<clock_t> m_clk;
	CList<int> m_prims;

public:
	GSStats() {Reset();}
	void Reset() {m_frame = m_prim = 0; m_fps = m_pps = m_ppf = 0;}
	void IncPrims(int n) {m_prim += n;}
	void VSync()
	{
		m_prims.AddTail(m_prim);
		if(m_clk.GetCount())
		{
			if(int dt = clock() - m_clk.GetHead())
			{
				m_fps = 1000.0f * m_clk.GetCount() / dt;

				int prims = 0;
				POSITION pos = m_prims.GetHeadPosition();
				while(pos) prims += m_prims.GetNext(pos);
				m_pps = 1000.0f * prims / dt;
				m_ppf = 1.0f * prims / m_clk.GetCount();
			}
			if(m_clk.GetCount() > 10)
			{
				m_clk.RemoveHead();
				m_prims.RemoveHead();
			}
		}
		m_clk.AddTail(clock());
		m_frame++;
		m_prim = 0;
	}
	int GetFrame() {return m_frame;}
	float GetFPS() {return m_fps;}
	float GetPPS() {return m_pps;}
	float GetPPF() {return m_ppf;}
	CString ToString(int fps)
	{
		CString str; 
		str.Format(_T("frame: %d | %.2f fps (%d%%) | %.0f ppf | %.0f pps"), 
			m_frame, m_fps, (int)(100*m_fps/fps), m_ppf, m_pps); 
		return str;
	}
};

class GSState
{
protected:
	static const int m_version = 1;

	GSLocalMemory m_lm;
	GSDrawingEnvironment m_de;
	GSDrawingContext* m_ctxt;
	GSRegSet m_rs;
	GSVertex m_v;
	GSStats m_stats;

	int m_x, m_y;
	void WriteStep();
	void ReadStep();
	void WriteTransfer(BYTE* pMem, int len);
	void ReadTransfer(BYTE* pMem, int len);
	void MoveTransfer();

	HWND m_hWnd;
	CComPtr<IDirect3D9> m_pD3D;
	CComPtr<IDirect3DDevice9> m_pD3DDev;
	CComPtr<IDirect3DSurface9> m_pOrgRenderTarget;
	CComPtr<IDirect3DSurface9> m_pOrgDepthStencil;
	CComPtr<IDirect3DPixelShader9> m_pPixelShaders[15];

	virtual void Reset();
	virtual void VertexKick(bool fSkip) = 0;
	virtual void DrawingKick(bool fSkip) = 0;
	virtual void NewPrim() = 0;
	virtual void FlushPrim() = 0;
	virtual void Flip();
	virtual void EndFrame() = 0;
	virtual void InvalidateTexture(DWORD TBP0) {}
	virtual void InvalidateTexture(DWORD TBP0, int x, int y) {InvalidateTexture(TBP0);}

	UINT32 m_PRIM;

	//

	GIFTag m_tag;
	int m_nreg;

	void (*m_fpGSirq)();

	typedef void (GSState::*GIFPackedRegHandler)(GIFPackedReg* r);
	GIFPackedRegHandler m_fpGIFPackedRegHandlers[16];

	void GIFPackedRegHandlerNull(GIFPackedReg* r);
	void GIFPackedRegHandlerPRIM(GIFPackedReg* r);
	void GIFPackedRegHandlerRGBAQ(GIFPackedReg* r);
	void GIFPackedRegHandlerST(GIFPackedReg* r);
	void GIFPackedRegHandlerUV(GIFPackedReg* r);
	void GIFPackedRegHandlerXYZF2(GIFPackedReg* r);
	void GIFPackedRegHandlerXYZ2(GIFPackedReg* r);
	void GIFPackedRegHandlerTEX0_1(GIFPackedReg* r);
	void GIFPackedRegHandlerTEX0_2(GIFPackedReg* r);
	void GIFPackedRegHandlerCLAMP_1(GIFPackedReg* r);
	void GIFPackedRegHandlerCLAMP_2(GIFPackedReg* r);
	void GIFPackedRegHandlerFOG(GIFPackedReg* r);
	void GIFPackedRegHandlerXYZF3(GIFPackedReg* r);
	void GIFPackedRegHandlerXYZ3(GIFPackedReg* r);
	void GIFPackedRegHandlerA_D(GIFPackedReg* r);
	void GIFPackedRegHandlerNOP(GIFPackedReg* r);

	typedef void (GSState::*GIFRegHandler)(GIFReg* r);
	GIFRegHandler m_fpGIFRegHandlers[256];

	void GIFRegHandlerNull(GIFReg* r);
	void GIFRegHandlerPRIM(GIFReg* r);
	void GIFRegHandlerRGBAQ(GIFReg* r);
	void GIFRegHandlerST(GIFReg* r);
	void GIFRegHandlerUV(GIFReg* r);
	void GIFRegHandlerXYZF2(GIFReg* r);
	void GIFRegHandlerXYZ2(GIFReg* r);
	void GIFRegHandlerTEX0_1(GIFReg* r);
	void GIFRegHandlerTEX0_2(GIFReg* r);
	void GIFRegHandlerCLAMP_1(GIFReg* r);
	void GIFRegHandlerCLAMP_2(GIFReg* r);
	void GIFRegHandlerFOG(GIFReg* r);
	void GIFRegHandlerXYZF3(GIFReg* r);
	void GIFRegHandlerXYZ3(GIFReg* r);
	void GIFRegHandlerNOP(GIFReg* r);
	void GIFRegHandlerTEX1_1(GIFReg* r);
	void GIFRegHandlerTEX1_2(GIFReg* r);
	void GIFRegHandlerTEX2_1(GIFReg* r);
	void GIFRegHandlerTEX2_2(GIFReg* r);
	void GIFRegHandlerXYOFFSET_1(GIFReg* r);
	void GIFRegHandlerXYOFFSET_2(GIFReg* r);
	void GIFRegHandlerPRMODECONT(GIFReg* r);
	void GIFRegHandlerPRMODE(GIFReg* r);
	void GIFRegHandlerTEXCLUT(GIFReg* r);
	void GIFRegHandlerSCANMSK(GIFReg* r);
	void GIFRegHandlerMIPTBP1_1(GIFReg* r);
	void GIFRegHandlerMIPTBP1_2(GIFReg* r);
	void GIFRegHandlerMIPTBP2_1(GIFReg* r);
	void GIFRegHandlerMIPTBP2_2(GIFReg* r);
	void GIFRegHandlerTEXA(GIFReg* r);
	void GIFRegHandlerFOGCOL(GIFReg* r);
	void GIFRegHandlerTEXFLUSH(GIFReg* r);
	void GIFRegHandlerSCISSOR_1(GIFReg* r);
	void GIFRegHandlerSCISSOR_2(GIFReg* r);
	void GIFRegHandlerALPHA_1(GIFReg* r);
	void GIFRegHandlerALPHA_2(GIFReg* r);
	void GIFRegHandlerDIMX(GIFReg* r);
	void GIFRegHandlerDTHE(GIFReg* r);
	void GIFRegHandlerCOLCLAMP(GIFReg* r);
	void GIFRegHandlerTEST_1(GIFReg* r);
	void GIFRegHandlerTEST_2(GIFReg* r);
	void GIFRegHandlerPABE(GIFReg* r);
	void GIFRegHandlerFBA_1(GIFReg* r);
	void GIFRegHandlerFBA_2(GIFReg* r);
	void GIFRegHandlerFRAME_1(GIFReg* r);
	void GIFRegHandlerFRAME_2(GIFReg* r);
	void GIFRegHandlerZBUF_1(GIFReg* r);
	void GIFRegHandlerZBUF_2(GIFReg* r);
	void GIFRegHandlerBITBLTBUF(GIFReg* r);
	void GIFRegHandlerTRXPOS(GIFReg* r);
	void GIFRegHandlerTRXREG(GIFReg* r);
	void GIFRegHandlerTRXDIR(GIFReg* r);
	void GIFRegHandlerHWREG(GIFReg* r);
	void GIFRegHandlerSIGNAL(GIFReg* r);
	void GIFRegHandlerFINISH(GIFReg* r);
	void GIFRegHandlerLABEL(GIFReg* r);

public:
	GSState(int w, int h, HWND hWnd, HRESULT& hr);
	virtual ~GSState();

	BOOL m_fDisableShaders;
	BOOL m_fHalfVRes;

	UINT32 Freeze(freezeData* fd);
	UINT32 Defrost(const freezeData* fd);
	void Write64(GS_REG mem, GSReg* r);
	UINT32 Read32(GS_REG mem);
	UINT64 Read64(GS_REG mem);
	void ReadFIFO(BYTE* pMem);
	void Transfer(BYTE* pMem);
	void Transfer(BYTE* pMem, UINT32 size);
	void VSync();

	void GSirq(void (*fpGSirq)()) {m_fpGSirq = fpGSirq;}

	UINT32 MakeSnapshot(char* path);

	FILE* m_fp;

#ifdef DEBUG_LOG
    #define LOG(_x_) LOGFILE _x_
#else
	#define LOG(_x_)
#endif

#ifdef DEBUG_LOG2
    #define LOG2(_x_) LOGFILE2 _x_
#else
	#define LOG2(_x_)
#endif

#ifdef DEBUG_LOGVERTICES
    #define LOGV(_x_) LOGVERTEX _x_
#else
    #define LOGV(_x_)
#endif

	void LOGFILE(LPCTSTR fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		/**/
		if(_tcsstr(fmt, _T("VSync")) 
		 || _tcsstr(fmt, _T("*** WARNING ***"))
		 || _tcsstr(fmt, _T("Flush"))
		// || _tcsstr(fmt, _T("CSR"))
		// || _tcsstr(fmt, _T("DISP"))
		// || _tcsstr(fmt, _T("FRAME"))
		// || _tcsstr(fmt, _T("ZBUF"))
		// || _tcsstr(fmt, _T("SMODE"))
		// || _tcsstr(fmt, _T("PMODE"))
		 || _tcsstr(fmt, _T("BITBLTBUF"))
		// || _tcsstr(fmt, _T("TRX"))
		// || _tcsstr(fmt, _T("PRIM"))
		// || _tcsstr(fmt, _T("RGB"))
		// || _tcsstr(fmt, _T("XYZ"))
		// || _tcsstr(fmt, _T("ST"))
		// || _tcsstr(fmt, _T("XYOFFSET"))
		// || _tcsstr(fmt, _T("TEX"))
		// || _tcsstr(fmt, _T("UV"))
		// || _tcsstr(fmt, _T("FOG"))
		// || _tcsstr(fmt, _T("TBP0")) == fmt
		// || _tcsstr(fmt, _T("CBP")) == fmt
		)
		if(m_fp)
		{
			TCHAR buff[2048];
			_stprintf(buff, _T("%d: "), clock());
			_vstprintf(buff + strlen(buff), fmt, args);
			_fputts(buff, m_fp); 
			fflush(m_fp);
		}
		va_end(args);
	}

	void LOGFILE2(LPCTSTR fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		if(m_fp)
		{
			TCHAR buff[2048];
			_stprintf(buff, _T("%d: "), clock());
			_vstprintf(buff + strlen(buff), fmt, args);
			_fputts(buff, m_fp); 
		}
		va_end(args);
	}
};