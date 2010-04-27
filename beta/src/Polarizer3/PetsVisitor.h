// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once
#include "../polaris/RegKey.h"

class PetsVisitor
{
public:
	PetsVisitor(void);
	virtual ~PetsVisitor(void);
	virtual void visit(RegKey pet);

};
