// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once

#include <string> 

struct Constants {

    static std::wstring polarisBaseName() {return L"Polaris";}
    static std::wstring hp() {return L"Hewlett-Packard";}
    static std::wstring polarisRelativePath() { return L"\\" + hp() + L"\\" + polarisBaseName(); }
    static std::wstring registryBase() { return L"Software" + polarisRelativePath(); }
    static std::wstring registryPets() { return registryBase() + L"\\Pets"; }
    static std::wstring editables() { return L"editables"; }
    static std::wstring registryAccounts() { return registryBase() + L"\\Accounts"; }
    static std::wstring userProfilePath(std::wstring accountName);
    static std::wstring userAppDataPath(std::wstring userProfilePath) {
        return userProfilePath + L"\\Application Data";
    }
    static std::wstring polarisDataPath(std::wstring userProfilePath) {
        return userAppDataPath(userProfilePath) + polarisRelativePath();
    }
    static std::wstring requestsPath(std::wstring polarisDataPath) {return polarisDataPath + L"\\requests";}
    static std::wstring editablesPath(std::wstring polarisDataPath) {return polarisDataPath + L"\\" + Constants::editables();} 
    static std::wstring nextAccountToUse() {return  L"nextAccountToUse";}
    static std::wstring nextAccountToCreate() {return L"nextAccountToCreate";}
    static std::wstring executables() {return L"executables";}
    static std::wstring accountName() {return L"accountName";}
    static std::wstring password() {return L"password";}
    static std::wstring petAccountNameBase() {return L"petAccountNameBase";}
    static std::wstring registrySuffixes() {return registryBase() + L"\\Suffixes";}
    static std::wstring registryBootActions() {return registryBase() + L"\\BootActions";};
    static std::wstring registrySuffixAppName() {return L"appPath";}
    static std::wstring registrySuffixPetname() {return L"petname";}
    static std::wstring registryNeedsPassword() {return L"needsPassword";}
    static std::wstring registryPasswordIsStored() {return L"passwordIsStored";}
    static std::wstring passwordFilePath(std::wstring polarisDataPath) {return polarisDataPath + L"\\pola.dat";}
    static std::wstring icebox() {return L"icebox";}
    static std::wstring boxedFileExtension() {return L"boxed";}
    static std::wstring actionDeleteDir() {return L"deleteDir";}
    static std::wstring actionRevokeAccount() {return L"revokeAccount";}
    static std::wstring serversToAuthenticate() {return L"serversToAuthenticate";}
    static std::wstring editableFolders() {return L"editableFolders";}
    static std::wstring readonlyFolders() {return L"readonlyFolders";}
    static std::wstring isReadOnlyWithLinks() {return L"isReadOnlyWithLinks";}
    static std::wstring allowAppFolderEdit() {return L"allowAppFolderEdit";}
    static std::wstring allowTempFolderEdit() {return L"allowTempFolderEdit";}
    static std::wstring allowOpAfterWindowClosed() {return L"allowOpAfterWindowClosed";}
    static std::wstring fileDialogPath() {return L"FileDialogPath";}
    static std::wstring defaultBrowserPet() {return L"defaultBrowserPet";}
    static std::wstring defaultMailPet() {return L"defaultMailPet";}
    static std::wstring commandLineArgs() {return L"commandLineArgs";}
	static std::wstring grantFullNetworkCredentials() {return L"grantNetCredentials";}
    
    static int polarizeFromScratchRequested() {return IDOK;}
    // the repolarize flag computation algorithm guarantees not to be identical to either IDOK or IDCANCEL
    static int repolarizeRequested() {return IDOK + IDCANCEL + 1;}
    static int goBackChangePetnameRequested() {return IDCANCEL;}
	static int horridDeleteError() {return 10;}


    /**
     * returns the executable folder path without a final backslash
     */
    static std::wstring polarisExecutablesFolderPath();
};
