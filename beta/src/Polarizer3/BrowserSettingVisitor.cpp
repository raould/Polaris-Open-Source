// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "StdAfx.h"
#include ".\browsersettingvisitor.h"
#include "petRegistryTools.h"

BrowserSettingVisitor::BrowserSettingVisitor(void)
{
}

BrowserSettingVisitor::~BrowserSettingVisitor(void)
{
}

void BrowserSettingVisitor::visit(RegKey pet) {
	setDefaultUrlHandler(L"HTTP", pet);
	setDefaultUrlHandler(L"HTTPS", pet);
	setDefaultUrlHandler(L"ftp", pet);
	setDefaultUrlHandler(L"gopher", pet);
}