// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "stdafx.h"
#include "CopyDirectory.h"

int copyDirectory(std::wstring fromPath, std::wstring toPath){
    
    SHFILEOPSTRUCT fileOp;
    fileOp.hwnd = NULL;
    fileOp.wFunc = FO_COPY;
    // gotta have a double null at end of frompath and topath
    wchar_t fromPathChars[1000] = {};
    std::wstring fromWildCarded = fromPath + L"\\*";
    wcsncpy(fromPathChars,fromWildCarded.c_str(), fromWildCarded.length()); 
    fileOp.pFrom = fromPathChars;
    wchar_t toPathChars[1000] = {};
    wcsncpy(toPathChars,toPath.c_str(), toPath.length()); 
    fileOp.pTo = toPathChars;
    fileOp.fFlags = FOF_NOCONFIRMMKDIR | FOF_NOCOPYSECURITYATTRIBS | FOF_NOERRORUI | 
        FOF_SIMPLEPROGRESS | FOF_NOCONFIRMATION;
    fileOp.lpszProgressTitle = L"Copying Underway";
    int status = SHFileOperation(&fileOp);
    return status;
}