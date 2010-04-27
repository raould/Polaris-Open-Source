#pragma once
#include "accesschecker.h"

class ImpersonationChecker : public AccessChecker
{
public:
	ImpersonationChecker(void);
	virtual ~ImpersonationChecker(void);
};
