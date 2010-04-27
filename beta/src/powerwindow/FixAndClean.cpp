// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "StdAfx.h"
#include ".\fixandclean.h"

#include <Ntsecapi.h>
#include "..\polaris\Constants.h"
#include "..\polaris\RegKey.h"
#include "..\polaris\StrongPassword.h"
#include <shlobj.h>
#include "..\polaris\Logger.h"
#include "..\polaris\makeAccount.h"
#include "..\polaris\RecursiveRemoveDirectoryWithPrejudice.h"
#include "..\polaris\FileACL.h"


void createShortcut(std::wstring shortcutPath, std::wstring executable) {
    IShellLink* link;
    CoInitialize(NULL);
    HRESULT hRes = ::CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                                      IID_IShellLink, (LPVOID*)&link);
    if (!SUCCEEDED(hRes)) {
        int blah = GetLastError();
        return;
    }
    hRes = link->SetPath(executable.c_str());
    if (!SUCCEEDED(hRes)) {
        return;
    }
    std::wstring args = L"" ;
    hRes = link->SetArguments(args.c_str());
    if (!SUCCEEDED(hRes)) {
        return;
    }
    hRes = link->SetIconLocation(executable.c_str(), 0);
    if (!SUCCEEDED(hRes)) {
        return;
    }
    IPersistFile* persist;
    hRes = link->QueryInterface(IID_IPersistFile, (LPVOID*)&persist);
    if (!SUCCEEDED(hRes)) {
        return;
    }
    persist->Save(shortcutPath.c_str(),false);
}

void configurePlacesBar() {
	RegKey policiesKey = RegKey::HKCU.create(L"software\\Microsoft\\Windows\\CurrentVersion\\Policies");
	RegKey comdlgKey = policiesKey.create(L"ComDlg32");
	RegKey placesKey = comdlgKey.create(L"PlacesBar");
	std::wstring home =  _wgetenv(L"USERPROFILE");
	placesKey.setValue(L"Place0", home + L"\\Recent");
	placesKey.setValue(L"Place1", home + L"\\Desktop");
	placesKey.setValue(L"Place2", home + L"\\My Documents");
	DWORD computer = CSIDL_DRIVES;
	placesKey.setBytesValue(L"Place3", REG_DWORD, (LPBYTE)&computer, 4);
	DWORD net = CSIDL_NETWORK;
	placesKey.setBytesValue(L"Place4", REG_DWORD, (LPBYTE)&net, 4);

}

FixAndClean::FixAndClean(void)
{
}

FixAndClean::~FixAndClean(void)
{
}

bool InitLsaString(
  PLSA_UNICODE_STRING pLsaString,
  LPCWSTR pwszString
)
{
  DWORD dwLen = 0;

  if (NULL == pLsaString)
      return FALSE;

  if (NULL != pwszString) 
  {
      dwLen = wcslen(pwszString);
      if (dwLen > 0x7ffe)   // String is too large
          return FALSE;
  }

  // Store the string.
  pLsaString->Buffer = (WCHAR *)pwszString;
  pLsaString->Length =  (USHORT)dwLen * sizeof(WCHAR);
  pLsaString->MaximumLength= (USHORT)(dwLen+1) * sizeof(WCHAR);

  return TRUE;
}

void FixAndClean::fix(){

    //Note: this logger cannot be used unless/until we are confident that 
    // the directory structure is in place, and the logger has a place to which to write,
    // which is to say, it is not safe to call the logger until after fix and clean
    // has checked the integrity of the polaris folder hierarchy
    Logger logger(L"FixAndClean");

    // make sure basic polaris folder hierarchy is present and cleanse old requests and responses
    // if this basic folder hierarchy is missing, assume this is the first time polaris has
    // been launched, i.e., we are installing, so put the powerwindow shortcut on the
    // startup folder and the polarizer on the desktop
    std::wstring requestFolder = Constants::requestsPath(Constants::polarisDataPath(
        _wgetenv(L"USERPROFILE")));
    if (CreateDirectory(requestFolder.c_str(), NULL) == 0 && GetLastError() == ERROR_ALREADY_EXISTS) {
        //cleanse
        WIN32_FIND_DATA findFileData;
        HANDLE hFind = FindFirstFile((requestFolder+L"\\*").c_str(), &findFileData);
        if (hFind != INVALID_HANDLE_VALUE) {
            while (true) {
                // only deleting request and response files. if there is anything else,
                // leave it so we can see it when debugging, since only request and response
                // files should be there.
                std::wstring filename = findFileData.cFileName;
                if (filename.find(L".response") != std::wstring.npos || 
                    filename.find(L".request") != std::wstring.npos) {
                    DeleteFile((requestFolder + L"\\" + filename).c_str());
                }
                if (FindNextFile(hFind,&findFileData) == 0) {break;}
            }
            FindClose(hFind);
        }
    } else {
        //create the polaris directories and the desktop, startup shortcuts
        SHCreateDirectoryEx(NULL, requestFolder.c_str(), NULL);
        logger.log(L"requests folder for main user did not exist, doing installation");

        std::wstring powerwindowPath = Constants::polarisExecutablesFolderPath() + L"\\powerwindow.exe";
        std::wstring profilePath = _wgetenv(L"USERPROFILE");
        std::wstring powerShortcutPath = profilePath + L"\\Start Menu\\Programs\\Startup\\powerwindow.lnk";
        createShortcut(powerShortcutPath, powerwindowPath);

        std::wstring polarizerPath = Constants::polarisExecutablesFolderPath() + L"\\polarizer3.exe";
        std::wstring polarizerShortcutPath = profilePath + L"\\Desktop\\Polarizer.lnk";
        createShortcut(polarizerShortcutPath, polarizerPath);
    }

    // basic folders now in position, the logger has a place to write to

    // make sure basic polaris keys are present
    if (!RegKey::HKCU.open(Constants::registryPets()).isGood()) {
        logger.log(L"Pets key in registry does not exist; fixing");
        RegKey hpKey = RegKey::HKCU.create(L"Software\\" + Constants::hp());
        RegKey polarisKey = RegKey::HKCU.create(Constants::registryBase());
        polarisKey.setValue(Constants::nextAccountToCreate(),L"0");
        polarisKey.setValue(Constants::nextAccountToUse(), L"0");
        std::wstring polaPrefix = L"pola";
        std::wstring randomPart = strongPassword();
        std::wstring petAccountBase = polaPrefix + randomPart.substr(0,10);
        polarisKey.setValue(Constants::petAccountNameBase(), petAccountBase);
        RegKey::HKCU.create(Constants::registryAccounts());
        RegKey::HKCU.create(Constants::registryPets());
        RegKey::HKCU.create(Constants::registrySuffixes());
        RegKey::HKCU.create(Constants::registryBootActions());
    }
    // configure the icebox. An icebox does need an editables subkey, but it does not need
    // an executables subkey
    std::wstring iceboxPath = Constants::registryPets() + L"\\" + Constants::icebox();
    if (!RegKey::HKCU.open(iceboxPath).isGood()) {
        RegKey icekey = RegKey::HKCU.create(iceboxPath);
        std::wstring iceAcct = makeAccount();
        icekey.setValue(Constants::accountName(), iceAcct);
        icekey.create(Constants::editables());
        SHCreateDirectoryEx(NULL, Constants::editablesPath(Constants::polarisDataPath(
            Constants::userProfilePath(iceAcct))).c_str(), NULL);
        SHCreateDirectoryEx(NULL, Constants::requestsPath(Constants::polarisDataPath(
            Constants::userProfilePath(iceAcct))).c_str(),NULL);
    }
	
    //make sure the mapi dll keys are set so you can send mail from pets
	RegKey hklmMail = RegKey::HKLM.create(L"Software").create(L"Clients").create(L"Mail");
    if (!hklmMail.open(L"Polaris").isGood()) {
        logger.log(L"mapi dll keys do not exist in registry");
        //setup the mapi dll so you can send mail from pets
        RegKey polarisKey = hklmMail.create(Constants::polarisBaseName());
        polarisKey.setValue(L"", L"Polaris MAPI");
        polarisKey.setValue(L"DLLPATH", Constants::polarisExecutablesFolderPath() + L"\\petmapi.dll");
    }
	std::wstring indirector = L"User";
	if (hklmMail.getValue(L"") != indirector) {
		RegKey hkcuMail= RegKey::HKCU.create(L"Software").create(L"Clients").create(L"Mail");
		hkcuMail.setValue(L"", hklmMail.getValue(L""));
		hklmMail.setValue(L"", L"User");
		RegKey userMail = hklmMail.create(indirector);
		userMail.setValue(L"", L"Check HKCU setting");
		userMail.setValue(L"DLLPath", Constants::polarisExecutablesFolderPath() + L"\\usermapi.dll");
	}

    // put out the pola.dat file (which stores password)
    std::wstring polaDatPath = Constants::passwordFilePath(Constants::polarisDataPath(_wgetenv(L"USERPROFILE")));
    FILE * polaDatFile = _wfopen(polaDatPath.c_str(), L"a+b");
    fclose(polaDatFile);

	// configuring the places bar is required because, at this time, the impersonation machinery causes the 
	// powerwindow to lose its correct user context for the file dialogs (a bug in Windows). All known fixes to this 
	// risk causing the entire powerwindow to crash when opening a file dialog
	configurePlacesBar();

    // destroy old synch files carefully
	RegKey petsKey = RegKey::HKCU.open(Constants::registryPets());
	for (int peti = 0; true; ++peti) {
		std::vector<std::wstring> keysToDelete;
		RegKey nextPet = petsKey.getSubKey(peti);
		if (!nextPet.isGood()) {break;}
		RegKey editables = nextPet.create(Constants::editables());
		for (int edj = 0; true; ++edj) { 
			RegKey nextEditable = editables.getSubKey(edj);
			if (!nextEditable.isGood()) {break;}
			keysToDelete.push_back(editables.getSubKeyName(edj));
			std::wstring original = nextEditable.getValue(L"originalPath");
			std::wstring editable = nextEditable.getValue(L"editablePath");			
			WIN32_FILE_ATTRIBUTE_DATA editable_attrs = {};
			if (GetFileAttributesEx(editable.c_str(), GetFileExInfoStandard, &editable_attrs)) {
				WIN32_FILE_ATTRIBUTE_DATA original_attrs = {};
				// if the original doc does not exist, or the editable file modify time does not match,
				// fail safe by copying the file to a folder on the desktop
				if (!GetFileAttributesEx(original.c_str(), GetFileExInfoStandard, &original_attrs) 
					|| (CompareFileTime(&original_attrs.ftLastWriteTime, &editable_attrs.ftLastWriteTime) != 0)) {
						std::wstring deskPath = _wgetenv(L"USERPROFILE");
						std::wstring recoverPath = deskPath + L"\\Desktop\\PolarisRecover";
						CreateDirectory(recoverPath.c_str(), NULL);
						size_t nameStartI = editable.rfind(L"\\") +1;
						size_t length = editable.length() - nameStartI;
						std::wstring filename = editable.substr(nameStartI, length );
						CopyFile(editable.c_str(), (recoverPath + L"\\" + filename).c_str(), false);
				}
			}
		}
		// all the editables recovered, now delete the regkeys
		for (size_t i = 0; i < keysToDelete.size(); ++i) {
			std::wstring next = keysToDelete[i];
			editables.deleteSubKey(keysToDelete[i]);
		}
	}

    // perform the intention list, including deletion of stubborn folders
    RegKey bootActionsKey = RegKey::HKCU.create(Constants::registryBootActions());
    std::vector<std::wstring> actionList;
    for (int i = 0; true; ++i) {
        std::wstring nextAction = bootActionsKey.getSubKeyName(i);
        if (nextAction.length() > 0) {
            actionList.push_back(nextAction);
        } else {
            break;
        }
    }
    for (size_t i = 0; i < actionList.size(); ++i) {
        RegKey nextActionKey = bootActionsKey.open(actionList[i]);
        if (!nextActionKey.isGood()) {
            break;
        } else {
            std::wstring pathToDeleteDir = nextActionKey.getValue(Constants::actionDeleteDir());
            if (pathToDeleteDir.length() > 0) {
                int zeroSucceed = RecursiveRemoveDirectoryWithPrejudice(pathToDeleteDir);
            }
            std::wstring fileWithAccountsToRevoke = nextActionKey.getValue(Constants::actionRevokeAccount());
            if (fileWithAccountsToRevoke.length() > 0) {
                std::wstring accountName = nextActionKey.getValue(Constants::accountName());
                FileACL acl(fileWithAccountsToRevoke);
                acl.revoke(accountName);
            }
            bootActionsKey.deleteSubKey(actionList[i]);
        }
    }

    // Grant needed privileges.
    BYTE admin_sid[256];
    DWORD admin_sid_size = sizeof admin_sid;
    wchar_t szAuthority[256];
    SID_NAME_USE snu;
    DWORD cchAuthority = sizeof szAuthority / sizeof *szAuthority;
    if (!LookupAccountName(NULL, L"Administrators", &admin_sid, &admin_sid_size,
                           szAuthority, &cchAuthority, &snu)) {
        logger.log(L"LookupAccountName(Administrators)", ::GetLastError());
    } else {
        LSA_HANDLE hPolicy;
        LSA_OBJECT_ATTRIBUTES oa = { sizeof oa };
        NTSTATUS s = ::LsaOpenPolicy(NULL, &oa, POLICY_LOOKUP_NAMES | POLICY_CREATE_ACCOUNT, &hPolicy);
        if (LSA_SUCCESS(s)) {
            LSA_UNICODE_STRING lucPrivilege;
            InitLsaString(&lucPrivilege, SE_ASSIGNPRIMARYTOKEN_NAME);
            s = LsaAddAccountRights(hPolicy, &admin_sid, &lucPrivilege, 1);
            if (!LSA_SUCCESS(s)) {
                logger.log(L"LsaAddAccountRights()", LsaNtStatusToWinError(s));
            } 
            LsaClose(hPolicy);
        } else {
            logger.log(L"LsaOpenPolicy()", LsaNtStatusToWinError(s));
        }
    }
}
