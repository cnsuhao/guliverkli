// subresync.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols


// CsubresyncApp:
// See subresync.cpp for the implementation of this class
//

class CSubresyncApp : public CWinApp
{
public:
	CSubresyncApp();

// Overrides
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CSubresyncApp theApp;