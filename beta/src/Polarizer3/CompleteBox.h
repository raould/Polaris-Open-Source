// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once


// CompleteBox dialog

class CompleteBox : public CDialog
{
	DECLARE_DYNAMIC(CompleteBox)

public:
	CompleteBox(CWnd* pParent = NULL);   // standard constructor
	virtual ~CompleteBox();

// Dialog Data
	enum { IDD = IDD_PolarizationComplete };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedCancel();
};
