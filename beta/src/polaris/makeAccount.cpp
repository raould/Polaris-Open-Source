// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "stdafx.h"
#include "makeAccount.h"
#include "Constants.h"
#include <Lm.h>
#include "RegKey.h"
#include "auto_array.h"
#include "auto_handle.h"
#include "auto_buffer.h"
#include "StrongPassword.h"
#include "Logger.h"


std::wstring incNumberString(std::wstring numberString) {
	int num = _wtoi(numberString.c_str());
	++num;
	wchar_t numString[20];
	_itow(num, numString, 10);
	return numString;
}	

/**
 * makeAccount is designed to be upgraded relatively straightforwardly to have predefined 
 * accounts that can be put to use quickly, while additional accounts are created eventually
 * in the background. hence the distinct next to create number and next to use number.
 * XXX TODO must create the accounts as part of a new Polaris group, not as part of user
 * group. TODO must ensure the polaris group includes the privilege that is needed to 
 * enable LogonUser.
 *
 * This function does create the polaris folders in the new directory for the new account
 *
 * If the Polaris regkey does not yet have a petAccountNameBase, this will create one. The
 * petAccountNameBase is the base name to which an integer is suffixed to create the next account name. 
 * Each user on the machine gets his own petAccountNameBase when he starts polarizing apps. Hence
 * there is no overlap or conflict among pet accounts among multiple users of a single machine.
 **/
std::wstring makeAccount() {
    Logger logger(L"makeAccount");
    RegKey polarisKey = RegKey::HKCU.open(Constants::registryBase());
    std::wstring nextAccountToUseNumber = polarisKey.getValue(Constants::nextAccountToUse());
    std::wstring nextAccountToCreateNumber = polarisKey.getValue(Constants::nextAccountToCreate());
    std::wstring petAccountBase = polarisKey.getValue(Constants::petAccountNameBase());
    if (petAccountBase.length() == 0) {
        std::wstring polaPrefix = L"pola";
        std::wstring randomPart = strongPassword();
        petAccountBase = polaPrefix + randomPart.substr(0,10);
        polarisKey.setValue(Constants::petAccountNameBase(), petAccountBase);
    }
    wchar_t nextAccountName[32];
    wcsncpy (nextAccountName, petAccountBase.c_str(), 32);
    wcsncat(nextAccountName, nextAccountToUseNumber.c_str(), 32 - petAccountBase.length());
    if (nextAccountToUseNumber >= nextAccountToCreateNumber) {
		if (nextAccountToUseNumber > nextAccountToCreateNumber) {logger.log(L"!!!AccountToUse > AccountToCreate");}
        //create the new account, no spares lying around)
        USER_INFO_1 info = {}; 
        info.usri1_name = nextAccountName;
        wchar_t password[40] = {};
        wcsncpy(password, strongPassword().c_str(), 40);
        info.usri1_password = password;
        info.usri1_priv = USER_PRIV_USER;
        info.usri1_flags = UF_SCRIPT | UF_DONT_EXPIRE_PASSWD | UF_PASSWD_CANT_CHANGE;        
        DWORD err = 0;
        NET_API_STATUS status = NetUserAdd(NULL, 1, (LPBYTE)&info, &err);
        if (status != NERR_Success) {
			if (status == NERR_UserExists) {
				logger.log(L"!user exists error");
				polarisKey.setValue(Constants::nextAccountToCreate(), incNumberString(nextAccountToCreateNumber));
				polarisKey.setValue(Constants::nextAccountToUse(), incNumberString(nextAccountToUseNumber));
			}
			logger.log(L"failed on netUserAdd",status);
			return L"";
        }

        // Get the length of the account's SID.
        DWORD sid_size = 0;
        DWORD domain_length = 0;
        SID_NAME_USE account_type = SidTypeUnknown;
        LookupAccountName(NULL, nextAccountName, NULL, &sid_size, NULL, &domain_length, &account_type);
        auto_buffer<PSID> sid(sid_size);
        auto_array<wchar_t> domain(domain_length);
        if (!LookupAccountName(NULL, nextAccountName, sid.get(), &sid_size, domain.get(), &domain_length, &account_type)) {
            DWORD error = GetLastError();
			logger.log(L"lookupaccountname failed", error);
            return L"";
        }
        LOCALGROUP_MEMBERS_INFO_0 group_info = {};
        group_info.lgrmi0_sid = sid.get();
        status = NetLocalGroupAddMembers(NULL, L"Users", 0, (LPBYTE)&group_info, 1);
        if (NERR_Success != status) {
			logger.log (L"netlocalgroupaddmembers failed", status);
            return L"";
        }
        //do logon session, see if desktop created
        auto_handle session(INVALID_HANDLE_VALUE);
	    if (!LogonUser(nextAccountName, NULL, password,
				    LOGON32_LOGON_INTERACTIVE,
				    LOGON32_PROVIDER_DEFAULT,
				    &session)) {
            DWORD error = GetLastError();
			logger.log(L"logonuser failed", error);
            return L"";
	    }
        // Load the pet account's registry hive.
	    PROFILEINFO profile = { sizeof(PROFILEINFO), 0, nextAccountName };
	    if (!LoadUserProfile(session.get(), &profile)) {
			logger.log(L"loaduserprofile failed", GetLastError());
		    return L"";
	    }

        //int nextCreationNumber = _wtoi(nextAccountToCreateNumber.c_str()) + 1;
        //wchar_t numString[20];
        //_itow(nextCreationNumber, numString, 10);
        //polarisKey.setValue(Constants::nextAccountToCreate(),numString); 
		polarisKey.setValue(Constants::nextAccountToCreate(), incNumberString(nextAccountToCreateNumber));
        RegKey accountsKey = RegKey::HKCU.open(Constants::registryAccounts());
        RegKey nextAccountKey = accountsKey.create(nextAccountName);
        nextAccountKey.setValue(Constants::password(), password);
        CreateDirectory((Constants::userProfilePath(nextAccountName) + L"\\Application Data\\" + Constants::hp()).c_str(), NULL);
        CreateDirectory(Constants::polarisDataPath(Constants::userProfilePath(nextAccountName)).c_str(), NULL);
        CreateDirectory(Constants::requestsPath(Constants::polarisDataPath(Constants::userProfilePath(nextAccountName))).c_str(), NULL);
        CreateDirectory(Constants::editablesPath(Constants::polarisDataPath(Constants::userProfilePath(nextAccountName))).c_str(), NULL);

    }
    //int nextUseNumber = _wtoi(nextAccountToUseNumber.c_str()) + 1;
    //wchar_t numString[20];
    //_itow(nextUseNumber, numString, 10);
    //polarisKey.setValue(Constants::nextAccountToUse(),numString); 
	polarisKey.setValue(Constants::nextAccountToUse(),incNumberString(nextAccountToUseNumber));
    return nextAccountName;
}