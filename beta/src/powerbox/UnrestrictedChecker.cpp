#include "StdAfx.h"
#include "UnrestrictedChecker.h"

UnrestrictedChecker::UnrestrictedChecker()
{
}

UnrestrictedChecker::~UnrestrictedChecker()
{
}

bool UnrestrictedChecker::canWriteFile(const char* path) {
	return true;
}