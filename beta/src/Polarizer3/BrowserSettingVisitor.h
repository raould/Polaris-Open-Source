// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once
#include "petsvisitor.h"

class BrowserSettingVisitor :
	public PetsVisitor
{
public:
	BrowserSettingVisitor(void);
	virtual ~BrowserSettingVisitor(void);
	virtual void visit(RegKey pet);
};
