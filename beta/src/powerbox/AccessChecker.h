#pragma once

struct AccessChecker
{
	virtual ~AccessChecker() {}

	virtual bool canWriteFile(const char* path) = 0;
};