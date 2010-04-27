// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once


// DePolarizationCompleteBox dialog

class DePolarizationCompleteBox : public CDialog
{
	DECLARE_DYNAMIC(DePolarizationCompleteBox)

public:
	DePolarizationCompleteBox(CWnd* pParent = NULL);   // standard constructor
	virtual ~DePolarizationCompleteBox();

// Dialog Data
	enum { IDD = IDD_DIALOG2 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedCancel();
};
