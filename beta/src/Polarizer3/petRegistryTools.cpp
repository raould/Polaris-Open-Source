// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "StdAfx.h"
#include "../polaris/Logger.h"
#include "../polaris/Constants.h"
#include "../polaris/RegKey.h"
#include <Sddl.h>
#include <Lm.h>
#include <windows.h>
#include <userenv.h>
#include "petRegistryTools.h"
#include "PetsVisitor.h"

Logger logtools(L"petRegistryTools");

/**
 * Set Default Url Handler looks to see if a default browser pet has been set. If so,
 * it builds the regkey substructure to coax Windows to call the openbrowser.exe
 * that will send a message to the powerwindow, which will then look up the default
 * browser in the polaris registry key area and launch it with the url on the command line.
 * The structure of the regkey area for this phenomenon is:

    HKU/petAccount/software/classes/urlProtocol ->
        shell
            open
                command
                    @=openBrowserPath "%1"
                ddeexec
                    "NoActivateHandler"=""
                    application
                        @=openBrowserPath

 * Some peculiarities: windows is manic about using ddeexec to launch the browser. Since openbrowser
 * does not use dde, you have got to kill this thing dead.
 *
 */
void setDefaultUrlHandler(std::wstring urlProtocol, RegKey petUserRoot) {
    RegKey classesKey = petUserRoot.open(L"software\\classes");
    RegKey urlKey = classesKey.create(urlProtocol);
	RegKey iconKey = urlKey.create(L"DefaultIcon");
	iconKey.setValue(L"",L"c:\\windows\\system32\\shell32.dll,13");
    RegKey shellKey = urlKey.create(L"shell");
    RegKey openKey = shellKey.create(L"open");
    RegKey commandKey = openKey.create(L"command");
	commandKey.setValue(L"",
        L"\"" + Constants::polarisExecutablesFolderPath() + L"\\powercmd.exe\" openurl \"%1\"");
	openKey.deleteSubKey(L"ddeexec");   // This is just paranoia. There should not be a ddeexec key.

    // This one is not paranoia.
    RegKey lm_open_key = RegKey::HKLM.open(L"Software\\Classes\\" + urlProtocol + L"\\shell\\open");
    lm_open_key.deleteSubKey(L"ddeexec");
}

void makeAcrobatNotEmbedInBrowser(RegKey petKey) {
	std::wstring adobePath = L"Software\\adobe\\acrobat reader\\7.0\\originals";
	DWORD noAdobeEmbedded = 0;
	RegKey adobeKey = petKey.create(adobePath);
	adobeKey.setBytesValue(L"bBrowserIntegration", REG_DWORD, (LPBYTE)&noAdobeEmbedded, 4);
}


void makeWordNotActiveXinBrowser(RegKey petKey) {
    std::wstring wordPath = L"Software\\Classes\\Word.Document.8";
    DWORD noActiveX = 8;
    RegKey temp = petKey.create(wordPath);
    RegKey wordKey = petKey.open(wordPath);
    wordKey.setBytesValue(L"BrowserFlags",REG_DWORD, (LPBYTE)&noActiveX, 4);
}

// if file extensions not shown in pet, then the dialog boxes from the pet will not show the
// extensions, which means that the dialog watcher will not be able to ascertaion the file
// extensions by parsing the data in the dialog
void ensureFileExtensionsShown(RegKey petUser) {
    std::wstring advancedPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced";
    BYTE zeroes[] = {0,0,0,0};
    std::wstring hideFileExt = L"HideFileExt";
    RegKey advanced = petUser.open(advancedPath);
    if (advanced.isGood()) {
        advanced.setBytesValue(hideFileExt, REG_DWORD, zeroes, 4);
    } else {
        logtools.log(L"explorer/advanced key does not exist for showing file extension");
        RegKey newAdvanced = petUser.create(advancedPath);
        if (newAdvanced.isGood()) {
            newAdvanced.setBytesValue(hideFileExt, REG_DWORD, zeroes, 4);
        } else {logtools.log(L"cannot even create advanced key");}

    }
}

void makeEachAcrobatLaunchUseNewWindow(RegKey petKey) {
	//TODO: in theory, but not in practice, you can fix acrobat by modifying the hkcu...localserver32 key,
	//    rather than going nuclear with the hklm hive. test this again some day
	std::wstring clsid = RegKey::HKR.open(L"AcroExch.Document.7\\CLSID").getValue(L"");
	RegKey serverKey = RegKey::HKLM.open(L"software\\classes\\clsid\\" + clsid + L"\\LocalServer32");
	serverKey.setValue(L"", L"\"c:\\program files\\adobe\\reader\\AcroRd32.exe\" /n");
	//std::wstring acroPath = RegKey::HKLM.open(L"software\\classes\\clsid\\" + clsid + L"\\LocalServer32").getValue(L"");
	//if (acroPath == L"") {
	//	logtools.log(L"no acropath in hklm");
	//	return;
	//}
	//RegKey clsidSuperKey = RegKey::HKCU.create(L"software\\classes\\clsid");
	//RegKey clsidKey = clsidSuperKey.create(clsid);
	//RegKey serverKey = clsidKey.create(L"LocalServer32");
	//serverKey.setValue(L"", acroPath + L" /n");
}

/**
 * Does not closeHandle on the session handle, nor does it perform UnloadUserProfile
 * on the profile handle. The profile handle cannot be unloaded, that would unload
 * the registry keys that we just got the root to. These cleanup flaws are probably acceptable
 * for the Polarizer, which runs for a few minutes before shutting down.
 **/
RegKey getUserRegistryRoot(std::wstring accountName, std::wstring password) {
    HANDLE session;
	if (!LogonUser(accountName.c_str(), NULL, password.c_str(),
				LOGON32_LOGON_INTERACTIVE,
				LOGON32_PROVIDER_DEFAULT,
				&session)) {
        DWORD error = GetLastError();
        printf("LogonUser() failed: %d", error);
        logtools.log(L"LogonUser() failed", error);
        return RegKey::INVALID;
	}
    // Load the pet account's registry hive.
    wchar_t account[128] = {};
    wcsncat(account, accountName.c_str(), 128);
	PROFILEINFO profile = { sizeof(PROFILEINFO), 0, account };
	if (!LoadUserProfile(session, &profile)) {
        printf("LoadUserProfile() failed: %d", GetLastError());
        logtools.log(L"LoadUserProfile() failed", GetLastError());
        return RegKey::INVALID;
	}
    // now the pet hive is loaded, compute the SID so I
    // can get a name for the key that I can use to make a RegKey
    // Can't use the session handle I just got back because it
    // can't be converted to a regkey, either get the name or 
    // give up on using our regkey abstraction.
    BYTE sid[10000];
    DWORD sidSize = 10000;
    DWORD domainSize = 1000;
    wchar_t domain[1000];
    SID_NAME_USE peUse;
    BOOL lookupWorked = LookupAccountName(NULL,accountName.c_str(), sid, 
        &sidSize, domain, &domainSize, &peUse);
    if (lookupWorked) {

        LPTSTR sidNameChars;
        BOOL converted = ConvertSidToStringSidW(sid, &sidNameChars);
        if (converted) {
            std::wstring sidName = sidNameChars;
            RegKey key = RegKey::HKU.open(sidName);
            LocalFree(sidNameChars);
            return key;
        } else {
            logtools.log(L"convert sid to string failed", GetLastError());
            return RegKey::INVALID;
        }
    } else {
        int err = GetLastError();
        logtools.log(L"lookupAccountName failed", err);
        return RegKey::INVALID;
    }
    
    return RegKey::INVALID;
}

/*
std::vector<RegKey> getPetRoots() {
	std::vector<RegKey> results;
	RegKey petsKey = RegKey::HKCU.open(Constants::registryPets());
	RegKey accountsKey = RegKey::HKCU.open(Constants::registryAccounts());
	for (int i = 0; true; ++i) {
		RegKey nextPet = petsKey.getSubKey(i);
		if (!nextPet.isGood()) {break;}
		std::wstring accountName = nextPet.getValue(Constants::accountName());
		std::wstring password = accountsKey.open(accountName).getValue(Constants::password());
		results.push_back(getUserRegistryRoot(accountName,password));
	}
	return results;
}
*/

/**
 * You can't make a vector full of RegKeys, because regkeys are incompatible (private equality operator)
 * So instead of making a list of pet key roots, you make a visitor that will be invoked for each
 * pet key root. 
 * TODO: with this design, it is possible to unload each pet registry after the change is applied. Not
 * quite possible because the getUserRoot doesn't give you back enough handles to toss the hive,
 * but possible if it gets important. All the pet hives at once will be huge quantities of RAM.
 **/
void visitPets(PetsVisitor visitor) {
	RegKey petsKey = RegKey::HKCU.open(Constants::registryPets());
	RegKey accountsKey = RegKey::HKCU.open(Constants::registryAccounts());
	for (int i = 0; true; ++i) {
		RegKey nextPet = petsKey.getSubKey(i);
		if (!nextPet.isGood()) {break;}
		std::wstring accountName = nextPet.getValue(Constants::accountName());
		std::wstring password = accountsKey.open(accountName).getValue(Constants::password());
		visitor.visit(getUserRegistryRoot(accountName,password));
	}
}