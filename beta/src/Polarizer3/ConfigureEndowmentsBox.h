// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once

#include "PetModel.h"
#include "afxwin.h"

// ConfigureEndowmentsBox dialog

class ConfigureEndowmentsBox : public CDialog
{
	DECLARE_DYNAMIC(ConfigureEndowmentsBox)
    PetModel* m_model;

public:
	ConfigureEndowmentsBox(CWnd* pParent = NULL);   // standard constructor
	virtual ~ConfigureEndowmentsBox();
    /**
     * Must set the petmodel before starting the dialog!
     */
    void setPetModel(PetModel* petModel);

// Dialog Data
	enum { IDD = IDD_DIALOG5 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnEnChangeEdit1();
    CEdit m_AuthenticatingServerSitesField;
    afx_msg void OnBnClickedOk();
	virtual BOOL OnInitDialog();
    CEdit m_additionalApps;
    CEdit m_editableFolders;
    CEdit m_readonlyFolders;
    CButton m_readOnlyWithLinks;
    CButton m_allowTempEdit;
    CButton m_allowAppFolderEdit;
    afx_msg void OnBnClickedCheck4();
    CButton m_setAsDefaultBrowserButton;
    afx_msg void OnBnClickedCheck5();
    CButton m_setAsDefaultMailClientCheck;
    CEdit m_additionalCommandLineArgsBox;
	CButton fullNetworkCredentialsButton;
	afx_msg void OnBnClickedCheck7();
	afx_msg void OnBnClickedCheck1();
	CButton m_allowOpAfterWindowClosed;
};
