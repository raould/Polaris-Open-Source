// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "StdAfx.h"
#include ".\getsetfiledialogpath.h"
#include "Constants.h"
#include "RegKey.h"

GetSetFileDialogPath::GetSetFileDialogPath(std::wstring petname)
{
	m_petname = petname;
}

GetSetFileDialogPath::~GetSetFileDialogPath(void)
{
}

std::wstring GetSetFileDialogPath::getFileDialogPath(){
	return RegKey::HKCU.open(Constants::registryPets() + L"\\" + m_petname).getValue(Constants::fileDialogPath());
}

void GetSetFileDialogPath::setFileDialogPath(std::wstring newPath) {
	RegKey::HKCU.open(Constants::registryPets() + L"\\" + m_petname).setValue(Constants::fileDialogPath(),newPath);
}
