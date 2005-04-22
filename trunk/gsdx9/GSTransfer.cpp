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

#include "stdafx.h"
#include "GSState.h"

void GSState::WriteStep()
{
//	if(m_y == m_rs.TRXREG.RRH && m_x == m_rs.TRXPOS.DSAX) ASSERT(0);

	if(++m_x == m_rs.TRXREG.RRW)
	{
		m_x = m_rs.TRXPOS.DSAX;
		m_y++;
	}
}

void GSState::ReadStep()
{
//	if(m_y == m_rs.TRXREG.RRH && m_x == m_rs.TRXPOS.SSAX) ASSERT(0);

	if(++m_x == m_rs.TRXREG.RRW)
	{
		m_x = m_rs.TRXPOS.SSAX;
		m_y++;
	}
}

void GSState::WriteTransfer(BYTE* pMem, int len)
{
	if(len == 0) return;

	if(m_de.pPRIM->TME && (m_rs.BITBLTBUF.DBP == m_ctxt->TEX0.TBP0 || m_rs.BITBLTBUF.DBP == m_ctxt->TEX0.CBP))
		FlushPrim();

	int x = m_x, y = m_y;

	m_stats.IncWrites(len);

	GSLocalMemory::SwizzleTexture st = m_lm.GetSwizzleTexture(m_rs.BITBLTBUF.DPSM);
	(m_lm.*st)(m_x, m_y, pMem, len, m_rs.BITBLTBUF, m_rs.TRXPOS, m_rs.TRXREG);

	//ASSERT(m_rs.TRXREG.RRH >= m_y - y);
LOG(_T("*TC2 WriteTransfer %dx%d - %dx%d (psm=%d rr=%dx%d len=%d)\n"), x, y, m_x, m_y, m_rs.BITBLTBUF.DPSM, m_rs.TRXREG.RRW, m_rs.TRXREG.RRH, len);

	CRect r(m_rs.TRXPOS.DSAX, y, m_rs.TRXREG.RRW, min(m_x == m_rs.TRXPOS.DSAX ? m_y : m_y+1, m_rs.TRXREG.RRH));

	InvalidateTexture(m_rs.BITBLTBUF.DBP, m_rs.BITBLTBUF.DPSM, r);

	m_lm.invalidateCLUT();
}

void GSState::ReadTransfer(BYTE* pMem, int len)
{
	BYTE* pb = (BYTE*)pMem;
	WORD* pw = (WORD*)pMem;
	DWORD* pd = (DWORD*)pMem;

	if(m_y >= (int)m_rs.TRXREG.RRH) {ASSERT(0); return;}

	switch(m_rs.BITBLTBUF.SPSM)
	{
	case PSM_PSMCT32:
		for(len /= 4; len-- > 0; ReadStep(), pd++)
			*pd = m_lm.readPixel32(m_x, m_y, m_rs.BITBLTBUF.SBP, m_rs.BITBLTBUF.SBW);
		break;
	case PSM_PSMCT24:
		for(len /= 3; len-- > 0; ReadStep(), pb+=3)
		{
			DWORD dw = m_lm.readPixel24(m_x, m_y, m_rs.BITBLTBUF.SBP, m_rs.BITBLTBUF.SBW);
			pb[0] = ((BYTE*)&dw)[0]; pb[1] = ((BYTE*)&dw)[1]; pb[2] = ((BYTE*)&dw)[2];
		}
		break;
	case PSM_PSMCT16:
		for(len /= 2; len-- > 0; ReadStep(), pw++)
			*pw = (WORD)m_lm.readPixel16(m_x, m_y, m_rs.BITBLTBUF.SBP, m_rs.BITBLTBUF.SBW);
		break;
	case PSM_PSMCT16S:
		for(len /= 2; len-- > 0; ReadStep(), pw++)
			*pw = (WORD)m_lm.readPixel16S(m_x, m_y, m_rs.BITBLTBUF.SBP, m_rs.BITBLTBUF.SBW);
		break;
	case PSM_PSMT8:
		for(; len-- > 0; ReadStep(), pb++)
			*pb = (BYTE)m_lm.readPixel8(m_x, m_y, m_rs.BITBLTBUF.SBP, m_rs.BITBLTBUF.SBW);
		break;
	case PSM_PSMT4:
		for(; len-- > 0; ReadStep(), ReadStep(), pb++)
			*pb = (BYTE)(m_lm.readPixel4(m_x, m_y, m_rs.BITBLTBUF.SBP, m_rs.BITBLTBUF.SBW)&0x0f)
				| (BYTE)(m_lm.readPixel4(m_x+1, m_y, m_rs.BITBLTBUF.SBP, m_rs.BITBLTBUF.SBW)<<4);
		break;
	case PSM_PSMT8H:
		for(; len-- > 0; ReadStep(), pb++)
			*pb = (BYTE)m_lm.readPixel8H(m_x, m_y, m_rs.BITBLTBUF.SBP, m_rs.BITBLTBUF.SBW);
		break;
	case PSM_PSMT4HL:
		for(; len-- > 0; ReadStep(), ReadStep(), pb++)
			*pb = (BYTE)(m_lm.readPixel4HL(m_x, m_y, m_rs.BITBLTBUF.SBP, m_rs.BITBLTBUF.SBW)&0x0f)
				| (BYTE)(m_lm.readPixel4HL(m_x+1, m_y, m_rs.BITBLTBUF.SBP, m_rs.BITBLTBUF.SBW)<<4);
		break;
	case PSM_PSMT4HH:
		for(; len-- > 0; ReadStep(), ReadStep(), pb++)
			*pb = (BYTE)(m_lm.readPixel4HH(m_x, m_y, m_rs.BITBLTBUF.SBP, m_rs.BITBLTBUF.SBW)&0x0f)
				| (BYTE)(m_lm.readPixel4HH(m_x+1, m_y, m_rs.BITBLTBUF.SBP, m_rs.BITBLTBUF.SBW)<<4);
		break;
	case PSM_PSMZ32:
		for(len /= 4; len-- > 0; ReadStep(), pd++)
			*pd = m_lm.readPixel32Z(m_x, m_y, m_rs.BITBLTBUF.SBP, m_rs.BITBLTBUF.SBW);
		break;
	case PSM_PSMZ24:
		for(len /= 3; len-- > 0; ReadStep(), pb+=3)
		{
			DWORD dw = m_lm.readPixel24Z(m_x, m_y, m_rs.BITBLTBUF.SBP, m_rs.BITBLTBUF.SBW);
			pb[0] = ((BYTE*)&dw)[0]; pb[1] = ((BYTE*)&dw)[1]; pb[2] = ((BYTE*)&dw)[2];
		}
		break;
	case PSM_PSMZ16:
		for(len /= 2; len-- > 0; ReadStep(), pw++)
			*pw = (WORD)m_lm.readPixel16Z(m_x, m_y, m_rs.BITBLTBUF.SBP, m_rs.BITBLTBUF.SBW);
		break;
	case PSM_PSMZ16S:
		for(len /= 2; len-- > 0; ReadStep(), pw++)
			*pw = (WORD)m_lm.readPixel16SZ(m_x, m_y, m_rs.BITBLTBUF.SBP, m_rs.BITBLTBUF.SBW);
		break;
	}
}

void GSState::MoveTransfer()
{
	if(m_rs.TRXPOS.SSAX == m_rs.TRXPOS.DSAX && m_rs.TRXPOS.SSAY == m_rs.TRXPOS.DSAY)
		return;

	GSLocalMemory::readPixel rp = m_lm.GetReadPixel(m_rs.BITBLTBUF.SPSM);
	GSLocalMemory::writePixel wp = m_lm.GetWritePixel(m_rs.BITBLTBUF.DPSM);

	int sx = m_rs.TRXPOS.SSAX;
	int dx = m_rs.TRXPOS.DSAX;
	int sy = m_rs.TRXPOS.SSAY;
	int dy = m_rs.TRXPOS.DSAY;
	int w = m_rs.TRXREG.RRW;
	int h = m_rs.TRXREG.RRH;
	int xinc = 1;
	int yinc = 1;

	if(sx < dx) sx += w-1, dx += w-1, xinc = -1;
	if(sy < dy) sy += h-1, dy += h-1, yinc = -1;

	for(int y = 0; y < h; y++, sy += yinc, dy += yinc, sx -= xinc*w, dx -= xinc*w)
		for(int x = 0; x < w; x++, sx += xinc, dx += xinc)
			(m_lm.*wp)(dx, dy, (m_lm.*rp)(sx, sy, m_rs.BITBLTBUF.SBP, m_rs.BITBLTBUF.SBW), m_rs.BITBLTBUF.DBP, m_rs.BITBLTBUF.DBW);
}
