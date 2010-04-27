// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once


// CannotPolarizeActivePet dialog

class CannotPolarizeActivePet : public CDialog
{
	DECLARE_DYNAMIC(CannotPolarizeActivePet)

public:
	CannotPolarizeActivePet(CWnd* pParent = NULL);   // standard constructor
	virtual ~CannotPolarizeActivePet();

// Dialog Data
	enum { IDD = IDD_DIALOG4 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
