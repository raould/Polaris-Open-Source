// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once


// PolarizeExistingPetBox dialog

class PolarizeExistingPetBox : public CDialog
{
	DECLARE_DYNAMIC(PolarizeExistingPetBox)

public:
	PolarizeExistingPetBox(CWnd* pParent = NULL);   // standard constructor
	virtual ~PolarizeExistingPetBox();

// Dialog Data
	enum { IDD = IDD_DIALOG3 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedCancel();
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedButton1();
};
