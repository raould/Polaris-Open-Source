// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

// CompleteBox.cpp : implementation file
//

#include "stdafx.h"
#include "Polarizer3.h"
#include "CompleteBox.h"
#include ".\completebox.h"
#include <windows.h>


// CompleteBox dialog

IMPLEMENT_DYNAMIC(CompleteBox, CDialog)
CompleteBox::CompleteBox(CWnd* pParent /*=NULL*/)
	: CDialog(CompleteBox::IDD, pParent)
{
}

CompleteBox::~CompleteBox()
{
}

void CompleteBox::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CompleteBox, CDialog)
    ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
END_MESSAGE_MAP()


// CompleteBox message handlers

void CompleteBox::OnBnClickedCancel()
{
    // TODO: Add your control notification handler code here

    ExitProcess(0);
}
