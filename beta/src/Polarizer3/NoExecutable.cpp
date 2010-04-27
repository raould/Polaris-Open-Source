// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

// NoExecutable.cpp : implementation file
//

#include "stdafx.h"
#include "Polarizer3.h"
#include "NoExecutable.h"


// NoExecutable dialog

IMPLEMENT_DYNAMIC(NoExecutable, CDialog)
NoExecutable::NoExecutable(CWnd* pParent /*=NULL*/)
	: CDialog(NoExecutable::IDD, pParent)
{
}

NoExecutable::~NoExecutable()
{
}

void NoExecutable::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(NoExecutable, CDialog)
END_MESSAGE_MAP()


// NoExecutable message handlers
