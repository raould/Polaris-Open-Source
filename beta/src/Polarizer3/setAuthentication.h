// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once
#include "afxwin.h"


// SetAuthentication dialog

class SetAuthentication : public CDialog
{
	DECLARE_DYNAMIC(SetAuthentication)

public:
	SetAuthentication(CWnd* pParent = NULL);   // standard constructor
	virtual ~SetAuthentication();

// Dialog Data
	enum { IDD = IDD_DIALOG1 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	BOOL SetAuthentication::OnInitDialog();
    afx_msg void OnBnClickedRadio1();
    int noAuthenticationButton;
    CButton setPasswordAtBoot;
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedRadio3();
    CEdit password;
	CEdit password2;
};
