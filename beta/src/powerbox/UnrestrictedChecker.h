#pragma once
#include "AccessChecker.h"

class UnrestrictedChecker : public AccessChecker
{
public:
	UnrestrictedChecker();
	virtual ~UnrestrictedChecker();

	virtual bool canWriteFile(const char* path);
};
