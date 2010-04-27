// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "stdafx.h"
#include "accountCanOpenFile.h"
#include "Constants.h"
#include "RegKey.h"

/**
 * the dword rights are the list of rights for a createfile open operation, such as FILE_EXECUTE and/or FILE_READ_DATA
 **/
bool accountCanOpenFile(std::wstring accountName, std::wstring password, std::wstring filePath, DWORD rights) {
    bool canOpen;
    HANDLE htok;
    if (!LogonUser(accountName.c_str(), NULL, password.c_str(),LOGON32_LOGON_INTERACTIVE, 0, &htok)) {
        DWORD err = GetLastError();
        printf("accountCanOpenFile could not logon user");
        return false;
    }
    ImpersonateLoggedOnUser(htok);
    //HANDLE hfile = CreateFile(filePath.c_str(), FILE_EXECUTE | FILE_READ_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
	HANDLE hfile = CreateFile(filePath.c_str(), rights, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    if (hfile == INVALID_HANDLE_VALUE) {
        canOpen = false;
    } else {
        canOpen = true;
        CloseHandle(hfile);
    }
    RevertToSelf();
    CloseHandle(htok);
    return canOpen;
}

bool petCanReadFile(std::wstring petname, std::wstring filePath) {
    RegKey petKey = RegKey::HKCU.open(Constants::registryPets() + L"\\" + petname);
    std::wstring account = petKey.getValue(Constants::accountName());
    RegKey accountKey = RegKey::HKCU.open(Constants::registryAccounts() + L"\\" + account);
    std::wstring pass = accountKey.getValue(Constants::password());
    return accountCanOpenFile(account, pass, filePath, FILE_READ_DATA);
}
