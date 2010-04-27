// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

// Messager.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols


// CMessagerApp:
// See Messager.cpp for the implementation of this class
//

class CMessagerApp : public CWinApp
{
public:
	CMessagerApp();

// Overrides
	public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CMessagerApp theApp;