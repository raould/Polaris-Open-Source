// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

// annotPolarizeActivePet.cpp : implementation file
//

#include "stdafx.h"
#include "Polarizer3.h"
#include "CannotPolarizeActivePet.h"


// CannotPolarizeActivePet dialog

IMPLEMENT_DYNAMIC(CannotPolarizeActivePet, CDialog)
CannotPolarizeActivePet::CannotPolarizeActivePet(CWnd* pParent /*=NULL*/)
	: CDialog(CannotPolarizeActivePet::IDD, pParent)
{
}

CannotPolarizeActivePet::~CannotPolarizeActivePet()
{
}

void CannotPolarizeActivePet::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CannotPolarizeActivePet, CDialog)
END_MESSAGE_MAP()


// CannotPolarizeActivePet message handlers
