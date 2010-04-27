// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

// PolarizeExistingPetBox.cpp : implementation file
//

#include "stdafx.h"
#include "Polarizer3.h"
#include "PolarizeExistingPetBox.h"
#include "..\polaris\Constants.h"


// PolarizeExistingPetBox dialog

IMPLEMENT_DYNAMIC(PolarizeExistingPetBox, CDialog)
PolarizeExistingPetBox::PolarizeExistingPetBox(CWnd* pParent /*=NULL*/)
	: CDialog(PolarizeExistingPetBox::IDD, pParent)
{
}

PolarizeExistingPetBox::~PolarizeExistingPetBox()
{
}

void PolarizeExistingPetBox::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(PolarizeExistingPetBox, CDialog)
    ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
    ON_BN_CLICKED(IDOK, OnBnClickedOk)
    ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
END_MESSAGE_MAP()


// PolarizeExistingPetBox message handlers

void PolarizeExistingPetBox::OnBnClickedCancel()
{
    // TODO: Add your control notification handler code here
    OnCancel();
}

void PolarizeExistingPetBox::OnBnClickedOk()
{
    // TODO: Add your control notification handler code here
    OnOK();
}

void PolarizeExistingPetBox::OnBnClickedButton1()
{
    EndDialog(Constants::repolarizeRequested());
}
