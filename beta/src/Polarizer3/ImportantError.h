// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once


// ImportantError dialog

class ImportantError : public CDialog
{
	DECLARE_DYNAMIC(ImportantError)

public:
	ImportantError(CWnd* pParent = NULL);   // standard constructor
	virtual ~ImportantError();

// Dialog Data
	enum { IDD = IDD_DIALOG7 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
