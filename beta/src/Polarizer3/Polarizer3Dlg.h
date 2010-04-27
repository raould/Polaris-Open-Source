// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

// Polarizer3Dlg.h : header file
//

#pragma once
#include "afxwin.h"
#include "PetModel.h"


// CPolarizer3Dlg dialog
class CPolarizer3Dlg : public CDialog
{
// Construction
    std::vector<std::wstring> getFileExtensions();
    std::vector<PetModel*> knownAppModels;
    PetModel model;
    void polarizeFromScratch();
    void repolarize();
	void updateEndowments();
    void clearWindow();
    void loadDialogFromModel();
    /**
     * returns a status, 0 if success
     */
    int loadModelFromDialog();
public:
	CPolarizer3Dlg(CWnd* pParent = NULL);	// standard constructor
    ~CPolarizer3Dlg();

// Dialog Data
	enum { IDD = IDD_POLARIZER3_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
    CEdit m_path;
    CEdit m_petname;
    afx_msg void OnBnClickedButton2();
    afx_msg void OnBnClickedButton1();
    afx_msg void OnEnChangeEdit3();
    CEdit m_fileExtensionsText;
    afx_msg void OnBnClickedButton3();
    afx_msg void OnEnChangeEdit1();
    afx_msg void OnLbnSelchangeList1();
    CListBox ExistingPetsList;
    afx_msg void OnBnClickedButton5();
    afx_msg void OnBnClickedButton4();
    CListBox KnownAppsBox;
    afx_msg void OnLbnSelchangeList2();
	afx_msg void OnBnClickedButton6();
	afx_msg void OnBnClickedButton7();
};
