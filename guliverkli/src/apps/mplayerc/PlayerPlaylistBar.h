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
#include <afxcoll.h>
#include "PlayerListCtrl.h"
#include "Playlist.h"

class CPlayerPlaylistBar : public CSizingControlBarG
{
	DECLARE_DYNAMIC(CPlayerPlaylistBar)

private:
	enum {COL_NAME, COL_TIME};

	CImageList m_fakeImageList;
	CPlayerListCtrl m_list;

	int m_nTimeColWidth;
	void ResizeListColumn();

	void AddItem(CString fn, CStringList* subs);
	void AddItem(CStringList& fns, CStringList* subs);
	void ParsePlayList(CString fn, CStringList* subs);
	void ParsePlayList(CStringList& fns, CStringList* subs);

	void SetupList();
	void EnsureVisible(POSITION pos);
	int FindItem(POSITION pos);
	POSITION FindPos(int i);

	CImageList* m_pDragImage;
	BOOL m_bDragging;
	int m_nDragIndex, m_nDropIndex;
	CPoint m_ptDropPoint;

	void DropItemOnList();

	CStringA m_strTipTextA;
	CStringW m_strTipTextW;

	CString StripPath(CString path);

public:
	CPlayerPlaylistBar();
	virtual ~CPlayerPlaylistBar();

	BOOL Create(CWnd* pParentWnd);

	CPlaylist m_pl;

	int GetCount();
	int GetSelIdx();
	void SetSelIdx(int i);
	bool IsAtEnd();
	bool GetCur(CPlaylistItem& pli);
	CString GetCur();
	void SetNext(), SetPrev(), SetFirst(), SetLast();
	void SetCurValid(bool fValid);

	void Empty();
	void Open(CStringList& fns, bool fMulti, CStringList* subs = NULL);
	void Append(CStringList& fns, bool fMulti, CStringList* subs = NULL);

	void SetTime(int nItem, REFERENCE_TIME rt);
	void SetCurTime(REFERENCE_TIME rt);

protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLvnKeyDown(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBeginlabeleditList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDolabeleditList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEndlabeleditList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMDblclkList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLvnKeydownList(NMHDR* pNMHDR, LRESULT* pResult);
//	afx_msg void OnCustomdrawList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg BOOL OnFileClosePlaylist(UINT nID);
	afx_msg BOOL OnPlayPlay(UINT nID);
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
};
