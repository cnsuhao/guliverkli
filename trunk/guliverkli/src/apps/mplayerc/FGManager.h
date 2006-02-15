/* 
 *	Copyright (C) 2003-2006 Gabest
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

#include "FGFilter.h"
#include "IGraphBuilder2.h"

class CFGManager
	: public CUnknown
	, public IGraphBuilder2
	, public IGraphBuilderDeadEnd
	, public CCritSec
{
public:
	struct path_t {CLSID clsid; CString filter, pin;};

	class CStreamPath : public CAtlList<path_t> 
	{
	public: 
		void Append(IBaseFilter* pBF, IPin* pPin); 
		bool Compare(const CStreamPath& path);
	};

	class CStreamDeadEnd : public CStreamPath 
	{
	public: 
		CAtlList<CMediaType> mts;
	};

private:
	CComPtr<IUnknown> m_pUnkInner;
	DWORD m_dwRegister;

	CStreamPath m_streampath;
	CAutoPtrArray<CStreamDeadEnd> m_deadends;

protected:
	CComPtr<IFilterMapper2> m_pFM;
	CInterfaceList<IUnknown, &IID_IUnknown> m_pUnks;
	CAtlList<CFGFilter*> m_source, m_transform, m_override;

	static bool CheckBytes(HANDLE hFile, CString chkbytes);

	HRESULT AddSourceFilter(CFGFilter* pFGF, LPCWSTR lpcwstrFileName, LPCWSTR lpcwstrFilterName, IBaseFilter** ppBF);

	// IFilterGraph

	STDMETHODIMP AddFilter(IBaseFilter* pFilter, LPCWSTR pName);
	STDMETHODIMP RemoveFilter(IBaseFilter* pFilter);
	STDMETHODIMP EnumFilters(IEnumFilters** ppEnum);
	STDMETHODIMP FindFilterByName(LPCWSTR pName, IBaseFilter** ppFilter);
	STDMETHODIMP ConnectDirect(IPin* pPinOut, IPin* pPinIn, const AM_MEDIA_TYPE* pmt);
	STDMETHODIMP Reconnect(IPin* ppin);
	STDMETHODIMP Disconnect(IPin* ppin);
	STDMETHODIMP SetDefaultSyncSource();

	// IGraphBuilder

	STDMETHODIMP Connect(IPin* pPinOut, IPin* pPinIn);
	STDMETHODIMP Render(IPin* pPinOut);
	STDMETHODIMP RenderFile(LPCWSTR lpcwstrFile, LPCWSTR lpcwstrPlayList);
	STDMETHODIMP AddSourceFilter(LPCWSTR lpcwstrFileName, LPCWSTR lpcwstrFilterName, IBaseFilter** ppFilter);
	STDMETHODIMP SetLogFile(DWORD_PTR hFile);
	STDMETHODIMP Abort();
	STDMETHODIMP ShouldOperationContinue();

	// IFilterGraph2

	STDMETHODIMP AddSourceFilterForMoniker(IMoniker* pMoniker, IBindCtx* pCtx, LPCWSTR lpcwstrFilterName, IBaseFilter** ppFilter);
	STDMETHODIMP ReconnectEx(IPin* ppin, const AM_MEDIA_TYPE* pmt);
	STDMETHODIMP RenderEx(IPin* pPinOut, DWORD dwFlags, DWORD* pvContext);

	// IGraphBuilder2

	STDMETHODIMP ConnectFilter(IBaseFilter* pBF, IPin* pPinIn);
	STDMETHODIMP ConnectFilter(IPin* pPinOut, IBaseFilter* pBF);
	STDMETHODIMP ConnectFilterDirect(IPin* pPinOut, IBaseFilter* pBF, const AM_MEDIA_TYPE* pmt);
	STDMETHODIMP FindInterface(REFIID iid, void** ppv, BOOL bRemove);
	STDMETHODIMP AddToROT();
	STDMETHODIMP RemoveFromROT();

	// IGraphBuilderDeadEnd

	STDMETHODIMP_(size_t) GetCount();
	STDMETHODIMP GetDeadEnd(int iIndex, CAtlList<CStringW>& path, CAtlList<CMediaType>& mts);

public:
	CFGManager(LPCTSTR pName, LPUNKNOWN pUnk);
	virtual ~CFGManager();

    DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);
};

class CFGManagerCustom : public CFGManager
{
public:
	// IFilterGraph

	STDMETHODIMP AddFilter(IBaseFilter* pFilter, LPCWSTR pName);

public:
	CFGManagerCustom(LPCTSTR pName, LPUNKNOWN pUnk, UINT src, UINT tra);
};

class CFGManagerPlayer : public CFGManagerCustom
{
protected:
	HWND m_hWnd;
	UINT64 m_vrmerit, m_armerit;

	// IFilterGraph

	STDMETHODIMP ConnectDirect(IPin* pPinOut, IPin* pPinIn, const AM_MEDIA_TYPE* pmt);

public:
	CFGManagerPlayer(LPCTSTR pName, LPUNKNOWN pUnk, UINT src, UINT tra, HWND hWnd);
};

class CFGManagerDVD : public CFGManagerPlayer
{
protected:
	// IGraphBuilder

	STDMETHODIMP AddSourceFilter(LPCWSTR lpcwstrFileName, LPCWSTR lpcwstrFilterName, IBaseFilter** ppFilter);

public:
	CFGManagerDVD(LPCTSTR pName, LPUNKNOWN pUnk, UINT src, UINT tra, HWND hWnd);
};

class CFGManagerCapture : public CFGManagerPlayer
{
public:
	CFGManagerCapture(LPCTSTR pName, LPUNKNOWN pUnk, UINT src, UINT tra, HWND hWnd);
};

class CFGManagerMuxer : public CFGManagerCustom
{
public:
	CFGManagerMuxer(LPCTSTR pName, LPUNKNOWN pUnk);
};

