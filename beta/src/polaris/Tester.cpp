// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "StdAfx.h"
#include "tester.h"
#include "Logger.h"

Tester::Tester(void)
{
}

Tester::~Tester(void)
{
}

int Tester::startTest() {
	int errorCount = 0;
	Logger logger(L"Test");
	int error = logger.log(L"This is a test from the Tester");
	if (error != 0) {
		errorCount++;
		printf("logger error %i", error);
	}
	printf("testing complete, num errors: %i", errorCount);
	return errorCount;
}
