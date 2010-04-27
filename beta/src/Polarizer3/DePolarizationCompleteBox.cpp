// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

// DePolarizationCompleteBox.cpp : implementation file
//

#include "stdafx.h"
#include "Polarizer3.h"
#include "DePolarizationCompleteBox.h"
#include ".\depolarizationcompletebox.h"


// DePolarizationCompleteBox dialog

IMPLEMENT_DYNAMIC(DePolarizationCompleteBox, CDialog)
DePolarizationCompleteBox::DePolarizationCompleteBox(CWnd* pParent /*=NULL*/)
	: CDialog(DePolarizationCompleteBox::IDD, pParent)
{
}

DePolarizationCompleteBox::~DePolarizationCompleteBox()
{
}

void DePolarizationCompleteBox::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(DePolarizationCompleteBox, CDialog)
    ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
END_MESSAGE_MAP()


// DePolarizationCompleteBox message handlers

void DePolarizationCompleteBox::OnBnClickedCancel()
{
    
    ExitProcess(0);
}
