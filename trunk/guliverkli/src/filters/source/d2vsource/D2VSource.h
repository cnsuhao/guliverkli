/* 
 *	Copyright (C) Gabest - November 2002
 *
 *  D2VSource.ax is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  D2VSource.ax is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 */

#pragma once
#include <atlbase.h>
#include "..\BaseSource\BaseSource.h"
#include "mpeg2dec.h"

class CD2VStream;

[uuid("47CE0591-C4D5-4b41-BED7-28F59AD76228")]
class CD2VSource : public CBaseSource<CD2VStream>
{
public:
	CD2VSource(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CD2VSource();

#ifdef REGISTER_FILTER
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);
#endif
};

class CD2VStream : public CBaseStream
{
private:
	CAutoPtr<CMPEG2Dec> m_pDecoder;
	CAutoVectorPtr<BYTE> m_pFrameBuffer;

	bool GetDim(int& w, int& h, int& bpp);

public:
    CD2VStream(const WCHAR* fn, CSource* pParent, HRESULT* phr);
	virtual ~CD2VStream();

    HRESULT FillBuffer(IMediaSample* pSample, int nFrame, BYTE* pOut, long& len /*in+out*/);

    HRESULT DecideBufferSize(IMemAllocator* pIMemAlloc, ALLOCATOR_PROPERTIES* pProperties);
    HRESULT CheckMediaType(const CMediaType* pMediaType);
    HRESULT GetMediaType(int iPosition, CMediaType* pmt);
    HRESULT SetMediaType(const CMediaType* pmt);

	STDMETHODIMP Notify(IBaseFilter* pSender, Quality q);
};
