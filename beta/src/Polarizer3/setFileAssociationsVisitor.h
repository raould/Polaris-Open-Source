// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once
#include "stdafx.h"
#include "petsvisitor.h"

class SetFileAssociationsVisitor :
	public PetsVisitor
{
private:
	std::wstring m_suffix;
	std::wstring m_appPath;
	std::wstring m_petname;
	std::wstring m_progIDstring;
public:
	SetFileAssociationsVisitor(std::wstring uncheckedSuffix, 
		std::wstring appPath, 
		std::wstring petname);
	virtual ~SetFileAssociationsVisitor(void);
	virtual void visit(RegKey pet);
};
