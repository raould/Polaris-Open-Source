// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once

class GetSetFileDialogPath
{
	std::wstring m_petname;
public:
	GetSetFileDialogPath(std::wstring petname);
	virtual ~GetSetFileDialogPath(void);
	std::wstring getFileDialogPath();
	void setFileDialogPath(std::wstring newPath);
};
