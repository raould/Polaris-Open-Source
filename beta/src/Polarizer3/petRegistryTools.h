// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once
#include <string>
#include "PetsVisitor.h"
#include "../polaris/RegKey.h"

void setDefaultUrlHandler(std::wstring urlProtocol, RegKey petUserRoot);

RegKey getUserRegistryRoot(std::wstring accountName, std::wstring password);

void makeAcrobatNotEmbedInBrowser(RegKey petKey);

void makeWordNotActiveXinBrowser(RegKey petKey);

void ensureFileExtensionsShown(RegKey petUser);

void makeEachAcrobatLaunchUseNewWindow(RegKey petKey);

void visitPets(PetsVisitor visitor);