// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "stdafx.h"
#include <Lm.h>
#include "Logger.h"

std::wstring chars = L"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_";

/**
 * RtlGenRandom is documented at 
 * http://msdn.microsoft.com/library/en-us/seccrypto/security/rtlgenrandom.asp. 
 **/
std::wstring strongPassword() {
    Logger logger(L"password generator");
    std::wstring result = L"";
    HMODULE hLib=LoadLibrary(L"ADVAPI32.DLL");
    if (hLib) {
        BOOLEAN (APIENTRY *pfn)(void*, ULONG) = 
            (BOOLEAN (APIENTRY *)(void*,ULONG))GetProcAddress(hLib,"SystemFunction036");
        if (pfn) {
            char buff[20];
            ULONG ulCbBuff = sizeof(buff);
            if(pfn(buff,ulCbBuff)) {
                for (int i = 0; i < sizeof(buff); ++i) {
                    size_t charIndex = abs((int)buff[i]) % chars.length();
                    result = result + chars.substr(charIndex,1);
                }
            }
        } else {logger.log(L"pfn for password failed");}

        FreeLibrary(hLib);
    } else {logger.log(L"failed to load advapi32.dll");}
    return result;
}
