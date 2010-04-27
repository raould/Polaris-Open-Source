// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once


// NoExecutable dialog

class NoExecutable : public CDialog
{
	DECLARE_DYNAMIC(NoExecutable)

public:
	NoExecutable(CWnd* pParent = NULL);   // standard constructor
	virtual ~NoExecutable();

// Dialog Data
	enum { IDD = IDD_DIALOG6 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
