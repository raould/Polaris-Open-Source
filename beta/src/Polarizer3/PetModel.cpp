// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "StdAfx.h"
#include ".\petmodel.h"
#include "..\polaris\RegKey.h"
#include "..\polaris\Constants.h"
#include <shlobj.h>
#include <intshcut.h>
#include "..\polaris\StrongPassword.h"
#include "..\polaris\FileACL.h"
#include "..\polaris\accountCanOpenFile.h"
#include "petRegistryTools.h"
#include "BrowserSettingVisitor.h"
#include "SetFileAssociationsVisitor.h"


PetModel::PetModel(void)
{
    clear();
}

PetModel::~PetModel(void)
{
}

std::wstring enquote(std::wstring text) {return L"\"" + text + L"\"";}

void createShortcut(std::wstring shortcutBaseName, std::wstring petname, std::wstring executable) {
    IShellLink* link;
    HRESULT hRes = ::CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                                      IID_IShellLink, (LPVOID*)&link);
    if (!SUCCEEDED(hRes)) {
        return;
    }
    std::wstring openSafePath = Constants::polarisExecutablesFolderPath() + L"\\powercmd.exe";
    hRes = link->SetPath(openSafePath.c_str());
    if (!SUCCEEDED(hRes)) {
        return;
    }
    std::wstring args = L"launch " +  enquote(petname) + L" " + enquote(executable) ;
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
    std::wstring fromHomeToFile = L"\\Desktop\\";
    fromHomeToFile += shortcutBaseName + L"Safe.lnk";
    std::wstring pathToFile = _wgetenv(L"USERPROFILE") + fromHomeToFile;
    persist->Save(pathToFile.c_str(),false);
}

/*
 * given the root of a user account, create a default launch behavior for a particular 
 * suffix: the progIDstring is the "type" represented by a particular suffix that
 * must be ascertained by looking in the HKLM registry before coming here
 */
//void setFileProgID(RegKey user, std::wstring suffix, std::wstring progIDstring, 
//                   std::wstring appPath, std::wstring petname) {
//                       
//    std::wstring softwareClasses = L"Software\\Classes\\";
//    RegKey testProgIDkey = user.open(softwareClasses + progIDstring);
//    if (!testProgIDkey.isGood()) {
//        RegKey tempProgIDkey = user.create(softwareClasses + progIDstring);
//        tempProgIDkey.setValue(L"", suffix + L" File");
//    }
//    RegKey progIDkey = RegKey::HKCU.open(softwareClasses + progIDstring);
//    RegKey shellKey = progIDkey.create(L"shell");
//    shellKey.setValue(L"", L"OpenSafe");
//    RegKey openSafeKey = shellKey.create(L"OpenSafe");
//    RegKey commandKey = openSafeKey.create(L"command");
//    commandKey.setValue(L"", enquote(Constants::polarisExecutablesFolderPath() + 
//        L"\\powercmd.exe") + L" launch " +  enquote(petname) + L" " + enquote(appPath) + L" " + enquote(L"%1"));
//    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
//}


/*
 * perform registry magic to force the local default behavior to be to use opensafe,
 * and also enter the association in the polaris suffix entries
 * Details of algorithm:
 *
 * For file extension .abc, for pet "dog"
 *
 *   Look to see if HKLM/Software/Classes/.abc exists. If not, 
 *       create it, 
 *       set the defaultvalue to polaris.abc.1, which will be the new progid
 *   else (hkr/.abc does exist)
 *       retrieve the default value as the candidate progid
 *       if the hklm progid key has a CurVer subkey, (as Acrobat does),
 *           get the default value from the CurVer subkey, that is the real progid
 *       else the candidate progid is the real progid
 *
 *   At this point we have the progid, henceforth called marcs.dog.abc.
 *   Look to see if this progid exists in hkcu/software/classes/polaris.abc.1. If not,
 *       create it
 *       set the defaultvalue to "abc File"
 *
 *   create if necessary hkcu/software/classes/polaris.abc.1/shell
 *   set default value in shell to OpenSafe
 *   create polaris.abc.1/shell/Opensafe
 *   create polaris.abc.1/shell/Opensafe/command
 *   set command default to "pathToOpenSafe" /default "%1"
 *
 *   Having set the file association, we now put an entry in polaris/suffixes
 *
 */
void setFileAssociation(std::wstring uncheckedSuffix, std::wstring appPath, std::wstring petname) {
    //std::wstring softwareClasses = L"Software\\Classes\\";
    //std::wstring suffix;
    //std::wstring progIDstring;
    //if (uncheckedSuffix[0] == L'.') {
    //    suffix = uncheckedSuffix.substr(1, uncheckedSuffix.length() - 1);
    //} else {suffix = uncheckedSuffix;}
    //RegKey testSuffixKey = RegKey::HKLM.open(softwareClasses + L"." + suffix);
    //if (!testSuffixKey.isGood()) {
    //    RegKey tempSuffixKey = RegKey::HKLM.create(softwareClasses + L"." + suffix);
    //    progIDstring = Constants::polarisBaseName() + L"." + suffix + L".1";
    //    tempSuffixKey.setValue(L"", progIDstring);
    //} else {
    //    std::wstring candidateProgIDstring = testSuffixKey.getValue(L"");
    //    RegKey hklmProgID = RegKey::HKLM.open(softwareClasses + candidateProgIDstring);
    //    RegKey curVerKey = hklmProgID.open(L"CurVer");
    //    if (curVerKey.isGood()) {
    //        progIDstring = curVerKey.getValue(L"");
    //    } else {
    //        progIDstring = candidateProgIDstring;
    //    }
    //}
    //setFileProgID(RegKey::HKCU, suffix, progIDstring, appPath, petname);

	std::wstring suffix;
    if (uncheckedSuffix[0] == L'.') {
        suffix = uncheckedSuffix.substr(1, uncheckedSuffix.length() - 1);
    } else {suffix = uncheckedSuffix;}
	SetFileAssociationsVisitor visitor(suffix, appPath, petname);
	visitor.visit(RegKey::HKCU);
	// TODO: turn on the following commented-out visit of all pets so that all pets
	//     use this pet for launching docs with this suffix. Cannot turn it on until
	//     we also have the code to turn it off in unsetFileAssociations when
	//     we depolarize
	// visitPets(visitor);
    
    RegKey suffixesKey = RegKey::HKCU.create(Constants::registrySuffixes());
    RegKey suffixKey = suffixesKey.create(suffix);
    suffixKey.setValue(Constants::registrySuffixAppName(), appPath);
    suffixKey.setValue(Constants::registrySuffixPetname(), petname);

}

/**
 * removes the progID association in the users software classes, removes the association
 * in the polaris area. 
 *    suffix must not have a preceding "."
 */
void unsetFileAssociation(std::wstring suffix) {
    std::wstring softwareClasses = L"Software\\Classes\\";
    RegKey HKLMsuffixKey = RegKey::HKLM.open(softwareClasses + L"." + suffix);
    std::wstring progIDstring = HKLMsuffixKey.getValue(L"");
    RegKey classesKey = RegKey::HKCU.open(softwareClasses);
    classesKey.deleteSubKey(progIDstring);
    
    RegKey suffixesKey = RegKey::HKCU.open(Constants::registrySuffixes());
    suffixesKey.deleteSubKey(suffix);
}

std::vector<std::wstring> getFileExtensionsForPet(std::wstring petname) {
    std::vector<std::wstring> resultList;
    RegKey suffixesKey = RegKey::HKCU.open(Constants::registrySuffixes());
    for (int i = 0; true; ++i) {
        RegKey suffixKey = suffixesKey.getSubKey(i);
        if (!suffixKey.isGood()) {break;}
        if (suffixKey.getValue(Constants::registrySuffixPetname()) == petname) {
            resultList.push_back(suffixesKey.getSubKeyName(i));
        }
    }
    return resultList;
}

/**
 * Wipes out the file associations in the suffixes registry and in the 
 * Windows progids for the true user for the
 * model's pet. 
 */ 
void PetModel::unsetFileAssociations(){
    std::vector<std::wstring> suffixes = getFileExtensionsForPet(petname);
    for (size_t i = 0; i < suffixes.size(); ++i) {
        unsetFileAssociation(suffixes[i]);
    }
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
}

void PetModel::undoRegisteredConfiguration(){
    RegKey polarisKey = RegKey::HKCU.open(Constants::registryBase());
    std::wstring defaultBrowserPet = polarisKey.getValue(Constants::defaultBrowserPet());
    if (defaultBrowserPet == petname) {
        polarisKey.setValue(Constants::defaultBrowserPet(), L"");
		RegKey clients = RegKey::HKCU.open(L"software\\clients");
        clients.deleteSubKey(L"StartMenuInternet");
    }
    unsetFileAssociations();
}

void PetModel::copyFrom(PetModel* other) {
    petname = other->petname;
    fileExtensions = other->fileExtensions;
    serversToAuthenticate=other->serversToAuthenticate;
    executables=other->executables;
    editableFolders=other->editableFolders;
    readonlyFolders=other->readonlyFolders;
    allowTempEdit = other -> allowTempEdit;
    allowAppFolderEdit = other -> allowAppFolderEdit;
    readOnlyWithLinks = other->readOnlyWithLinks;
    isDefaultBrowserPet = other -> isDefaultBrowserPet;
	grantFullNetworkCredentials = other -> grantFullNetworkCredentials;
	allowOpAfterWindowClosed = other -> allowOpAfterWindowClosed;
	commandLineArgs = other -> commandLineArgs;
}
    

bool PetModel::commit(){
    RegKey petKey = RegKey::HKCU.create(Constants::registryPets() + L"\\" + petname);
    petKey.setValue(Constants::accountName(), accountName);
    RegKey accountKey = RegKey::HKCU.open(Constants::registryAccounts() + L"\\" + accountName);
    std::wstring password = accountKey.getValue(Constants::password());

    std::wstring userProfilePath = _wgetenv(L"USERPROFILE");
    petKey.setValue(Constants::fileDialogPath(), userProfilePath + L"\\Desktop");

    petKey.deleteSubKey(Constants::editables());
    petKey.create(Constants::editables());

    petKey.deleteSubKey(Constants::executables());
    RegKey executablesKey = petKey.create(Constants::executables());
    wchar_t valueName[2] = { L'a' };
    for (size_t i = 0; i < executables.size(); ++i) {
        //put the executable into the executables registry key
        std::wstring nextPath = executables[i];
        if (nextPath.length() > 0) {
            executablesKey.setValue(valueName, nextPath);
            valueName[0]++;
            //make sure the executable can be read and executed by the pet accounts
            if (!accountCanLaunchFile(password, nextPath)) {
                //XXX ToDo set the acls for the nextpath parent folder transitively to allow group access
                //XXX TODO getting the path to the polaris installed execution directory sucks
                std::wstring parentPath = nextPath.substr(0, nextPath.rfind(L"\\"));
                FileACL acl(parentPath);
                acl.grant(FileACL::POLARIS_USERS(), FileACL::EXECUTE());
                acl.grant(FileACL::POLARIS_USERS(), FileACL::READ());
            }
            // names for all shortcuts for all executables except the shortcut for the default exec (which
            // is the first exec in the list) get the exec name appended to the petname
            std::wstring shortcutName = petname;
            if (i > 0) {
                size_t slashI = nextPath.rfind(L"\\");
                std::wstring execName = nextPath.substr(slashI + 1, nextPath.rfind(L".") - slashI -1);
                shortcutName = petname + L"-" + execName;
            }
            createShortcut(shortcutName, petname, nextPath);
        }
    }

    //clear any existing file associations before adding new ones
    unsetFileAssociations();
    for (size_t i = 0; i < fileExtensions.size(); ++i) {
        //TODO this only allows the first executable in the list to have file extensions assigned to it
        setFileAssociation(fileExtensions[i], executables[0], petname);
    }

    petKey.deleteSubKey(Constants::serversToAuthenticate());
    RegKey authenticatingServersKey = petKey.create(Constants::serversToAuthenticate());
    for (size_t i = 0; i < serversToAuthenticate.size(); ++i) {
        if (serversToAuthenticate[i].length() > 0) {
            RegKey nextKey = authenticatingServersKey.create(strongPassword());
            nextKey.setValue(L"",serversToAuthenticate[i]);
        }
    }

    petKey.deleteSubKey(Constants::editableFolders());
    RegKey editableFoldersKey = petKey.create(Constants::editableFolders());
    for (size_t i = 0; i < editableFolders.size(); ++i) {
        if (editableFolders[i].length() > 0) {
            RegKey nextKey = editableFoldersKey.create(strongPassword());
            nextKey.setValue(L"",editableFolders[i]);
            FileACL acl(editableFolders[i]);
            acl.grant(accountName, FileACL::MODIFY());
        }
    }
    petKey.deleteSubKey(Constants::readonlyFolders());
    RegKey readonlyFoldersKey = petKey.create(Constants::readonlyFolders());
    for (size_t i = 0; i < readonlyFolders.size(); ++i) {
        if (readonlyFolders[i].length() > 0) {
            RegKey nextKey = readonlyFoldersKey.create(strongPassword());
            nextKey.setValue(L"",readonlyFolders[i]);
            FileACL acl(readonlyFolders[i]);
            acl.grant(accountName, FileACL::READ());
        }
    }

    petKey.setValue(Constants::commandLineArgs(), commandLineArgs);

    if (allowAppFolderEdit) {
        petKey.setValue(Constants::allowAppFolderEdit(),L"true");
        // TODO: allowing app folder edit currently enables folder editing
        // for all the executables. This will in most cases be excess authority
        // but only in the rare case where there are multiple executables in the first
        // place. This algorithm trades off ease of human use (don't make them set
        // editability of app folder authority for all the executables separately)
        // and ensure successful operation, while reducing security. I think it is
        // the right choice, but YMMV
        for (size_t i = 0; i < executables.size(); ++i) {
            FileACL acl(executables[i].substr(0, executables[i].rfind(L"\\")));
            acl.grant(accountName, FileACL::MODIFY());
        }
    } else {        
        petKey.setValue(Constants::allowAppFolderEdit(),L"false");
    }

    if (allowTempEdit) {
        petKey.setValue(Constants::allowTempFolderEdit(), L"true");
        FileACL acl(L"c:\\temp");
        acl.grant(accountName, FileACL::MODIFY());
        std::wstring windir = _wgetenv(L"windir");
        FileACL winTempAcl(windir + L"\\temp");
        winTempAcl.grant(accountName, FileACL::MODIFY());
    } else {
        petKey.setValue(Constants::allowTempFolderEdit(), L"false");
    }

    if (readOnlyWithLinks) {
        petKey.setValue(Constants::isReadOnlyWithLinks(), L"true");
    } else {
        petKey.setValue(Constants::isReadOnlyWithLinks(), L"false");
    }

	if (grantFullNetworkCredentials) {
		petKey.setValue(Constants::grantFullNetworkCredentials(), L"true");
	} else {
		petKey.setValue(Constants::grantFullNetworkCredentials(), L"false");
	}

	if (allowOpAfterWindowClosed) {
		petKey.setValue(Constants::allowOpAfterWindowClosed(), L"true");
	} else {
		petKey.setValue(Constants::allowOpAfterWindowClosed(), L"false");
	}

    BrowserSettingVisitor visitor;
	visitor.visit(RegKey::HKCU);
    
    RegKey polarisKey = RegKey::HKCU.open(Constants::registryBase());
    std::wstring currentDefaultBrowserPet = polarisKey.getValue(Constants::defaultBrowserPet());
    if (isDefaultBrowserPet) {
		//if (currentDefaultBrowserPet != petname) {
		polarisKey.setValue(Constants::defaultBrowserPet(), petname);
		//}
		// Another heuristic. IE gives itself god power when set as default browser, the only way I've found
		// to disable godhood is to delete the CLSID key from htmlfile. Alternative: polaris could create a new
		// progid to replace htmlfile. This would have the disadvantage that it couldn't revert during depolarize,
		// would not know whether to revert to htmlfile or to firefoxhtml or some other progid
		RegKey godkey = RegKey::HKLM.open(L"Software\\Classes\\htmlfile");
		godkey.deleteSubKey(L"CLSID");
		// internet start menu must be configured to launch our boy as well
		RegKey startMenuInternetKey = RegKey::HKCU.create(L"software\\clients\\StartMenuInternet");
		startMenuInternetKey.setValue(L"", L"powercmd.exe");
		RegKey polaStartMenuInternetKey =
            RegKey::HKLM.create(L"software\\clients\\StartMenuInternet\\powercmd.exe");;
		polaStartMenuInternetKey.setValue(L"", L"Polarized Browser");
		RegKey iconKey = polaStartMenuInternetKey.create(L"DefaultIcon");
		iconKey.setValue(L"", L"c:\\windows\\system32\\shell32.dll,13");
		RegKey shellKey = polaStartMenuInternetKey.create(L"shell");
		RegKey openKey = shellKey.create(L"open");
		RegKey commandKey = openKey.create(L"command");
		commandKey.setValue(L"",
            L"\"" + Constants::polarisExecutablesFolderPath() + L"\\powercmd.exe\" openurl");
    } else {
        //must remember to unmake the default browser setting if this used to be the default
        // but now the user has untoggled the setting
        if (currentDefaultBrowserPet == petname) {
            polarisKey.setValue(Constants::defaultBrowserPet(), L"");
        }
    }

	// TODO: when we fully implement mail pets, will the mail pet need to not have
	// the registry setting  hkcu\software\clients\mail[default] set to polaris?
    std::wstring currentDefaultMailPet = polarisKey.getValue(Constants::defaultMailPet());
    if (isDefaultMailPet) {
        polarisKey.setValue(Constants::defaultMailPet(), petname);
    } else {
        if (currentDefaultMailPet == petname) {
            polarisKey.setValue(Constants::defaultMailPet(), L"");
        }
    }

	//configure usage of the polaris mapi system for sending mail
	RegKey userRoot = getUserRegistryRoot(accountName, password);
	userRoot.create(L"Software").create(L"Clients").create(L"Mail").setValue(L"",L"Polaris");

    return true;
}

/**
 * 
 **/
void PetModel::load(std::wstring newPetname) {
    clear();
    petname = newPetname;
    RegKey petKey = RegKey::HKCU.open(Constants::registryPets() + L"\\" + petname );
    RegKey exeKey = petKey.create(Constants::executables());
    for (int i = 0; true; ++i) {
        std::wstring exeValueName = exeKey.getValueName(i);
        if (exeValueName == exeKey.NO_VALUE_FOUND()) {break;}
        std::wstring val = exeKey.getValue(exeValueName);
        if (val.length() > 0) {executables.push_back(val);}
    }
    RegKey authenticatingServersKey = petKey.create(Constants::serversToAuthenticate());
    for (int i = 0; true; ++i) {
        RegKey nextServerKey = authenticatingServersKey.getSubKey(i);
        if (nextServerKey.isGood()) {
            serversToAuthenticate.push_back(nextServerKey.getValue(L""));
        } else {break;}
    }
    RegKey editableFoldersKey = petKey.create(Constants::editableFolders());
    for (int i = 0; true; ++i) {
        RegKey nextFolderKey = editableFoldersKey.getSubKey(i);
        if (nextFolderKey.isGood()) {
            editableFolders.push_back(nextFolderKey.getValue(L""));
        } else {break;}
    }
    RegKey readonlyFoldersKey = petKey.create(Constants::readonlyFolders());
    for (int i = 0; true; ++i) {
        RegKey nextFolderKey = readonlyFoldersKey.getSubKey(i);
        if (nextFolderKey.isGood()) {
            readonlyFolders.push_back(nextFolderKey.getValue(L""));
        } else {break;}
    }

    fileExtensions = getFileExtensionsForPet(petname);

    commandLineArgs = petKey.getValue(Constants::commandLineArgs());
    allowTempEdit = petKey.getValue(Constants::allowTempFolderEdit()) == L"true";
    allowAppFolderEdit = petKey.getValue(Constants::allowAppFolderEdit()) == L"true";
    readOnlyWithLinks = petKey.getValue(Constants::isReadOnlyWithLinks()) == L"true";
	grantFullNetworkCredentials = petKey.getValue(Constants::grantFullNetworkCredentials()) == L"true";
    allowOpAfterWindowClosed = petKey.getValue(Constants::allowOpAfterWindowClosed()) == L"true";
    isDefaultBrowserPet = RegKey::HKCU.open(Constants::registryBase()).getValue(
        Constants::defaultBrowserPet()) == petname;
    isDefaultMailPet = RegKey::HKCU.open(Constants::registryBase()).getValue(
        Constants::defaultMailPet()) == petname;
}

// stub function accountCanOpen with name different from the polaris.dll
// function accountCanOpenFile so that the 2-arg petmodel function accountcanopenfile
// can call it without the compiler getting confused and thinking that, when
// petmodel::accountcanopenfile calls accountcanopenfile, it is a call to the method
// rather than the function
bool accountCanLaunch(std::wstring accountName, std::wstring password, std::wstring filePath) {
    return accountCanOpenFile(accountName, password, filePath, FILE_EXECUTE | FILE_READ_DATA);
}
bool PetModel::accountCanLaunchFile(std::wstring password, std::wstring filePath) {
    return accountCanLaunch(accountName, password, filePath);
    //bool canOpen;
    //HANDLE htok;
    //if (!LogonUser(accountName.c_str(), NULL, password.c_str(),LOGON32_LOGON_INTERACTIVE, 0, &htok)) {
    //    DWORD err = GetLastError();
    //    printf("accountCanOpenFile could not logon user");
    //    return false;
    //}
    //ImpersonateLoggedOnUser(htok);
    //HANDLE hfile = CreateFile(filePath.c_str(), FILE_EXECUTE | FILE_READ_DATA, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
    //if (hfile == INVALID_HANDLE_VALUE) {
    //    canOpen = false;
    //} else {
    //    canOpen = true;
    //    CloseHandle(hfile);
    //}
    //RevertToSelf();
    //CloseHandle(htok);
    //return canOpen;
}

void PetModel::clear() {
    petname = L"";
    fileExtensions.clear();
    serversToAuthenticate.clear();
    executables.clear();
    std::wstring accountName = L"";
    editableFolders.clear();
    readonlyFolders.clear();
    allowTempEdit = false;
    allowAppFolderEdit = false;
    readOnlyWithLinks = false;
    isDefaultBrowserPet = false;
    isDefaultMailPet = false;
	grantFullNetworkCredentials = false;
    commandLineArgs = L"";
	allowOpAfterWindowClosed = false;
}

