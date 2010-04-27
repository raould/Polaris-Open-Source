// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "stdafx.h"
#include "Constants.h"
#include <string>
#include <Lm.h>
#include <shellapi.h>

// XXX this computation of userprofile path should be upgraded, machines 
// could be configured so user homes are on d not c for example. Looks like we should link
// Netapi32.dll (include Lm.h) NetUserGetInfo(NULL, username, level1, user_info_1), extract the
// usri1_home_dir
std::wstring Constants::userProfilePath(std::wstring accountName){
    
    return L"c:\\Documents and Settings\\" + accountName;
    //LPUSER_INFO_1 info = NULL;
    //char blah[1000];
    //wchar_t wblah[2000];
    //mbstowcs(wblah, accountName.c_str(), accountName.length() + 1);
    //NET_API_STATUS err = NetUserGetInfo(NULL, (LPCWSTR)&wblah,1, (LPBYTE*)&info);
    //if (err == NERR_Success) {
    //    if (info != NULL) {
    //        wcstombs(blah, info->usri1_home_dir,999);
    //        std::string path = blah;
    //        NetApiBufferFree(&info);
    //        return path;
    //    } else {return "";}
    //} else {return "";}
}


std::wstring Constants::polarisExecutablesFolderPath(){
    int numArgs = 0;
    LPWSTR * argList = (CommandLineToArgvW(GetCommandLine(), &numArgs));
    std::wstring currentExecutablePath = argList[0];
    LocalFree(argList);
    size_t lastSlash = currentExecutablePath.find_last_of(L"\\");
    std::wstring path = currentExecutablePath.substr(0,lastSlash);
    return path;
}

