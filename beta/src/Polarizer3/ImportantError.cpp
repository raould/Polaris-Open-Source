// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

// ImportantError.cpp : implementation file
//

#include "stdafx.h"
#include "Polarizer3.h"
#include "ImportantError.h"


// ImportantError dialog

IMPLEMENT_DYNAMIC(ImportantError, CDialog)
ImportantError::ImportantError(CWnd* pParent /*=NULL*/)
	: CDialog(ImportantError::IDD, pParent)
{
}

ImportantError::~ImportantError()
{
}

void ImportantError::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(ImportantError, CDialog)
END_MESSAGE_MAP()


// ImportantError message handlers
