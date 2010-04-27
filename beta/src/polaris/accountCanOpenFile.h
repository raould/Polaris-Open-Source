// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html


#pragma once

#include <string>

bool accountCanOpenFile(std::wstring accountName, 
                        std::wstring password, 
                        std::wstring filePath,
						DWORD rights) ;

bool petCanReadFile(std::wstring petname, std::wstring filePath);