#include "StdAfx.h"
#include ".\fixandclean.h"
#include "..\polaris\Constants.h"
#include "..\polaris\RegKey.h"
#include "..\polaris\StrongPassword.h"
#include <shlobj.h>
#include "..\polaris\Logger.h"
#include "..\polaris\makeAccount.h"

FixAndClean::FixAndClean(void)
{
}

FixAndClean::~FixAndClean(void)
{
}


void FixAndClean::fix(){

    Logger logger(L"FixAndClean");
    // make sure basic polaris keys are present
    if (!RegKey::HKCU.open(Constants::registryPets()).isGood()) {
        logger.log(L"Pets key in registry does not exist; fixing");
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
    }
    // configure the icebox. An icebox does need and editables subkey, but it does not need
    // an executables subkey
    std::wstring iceboxPath = Constants::registryPets() + L"\\" + Constants::icebox();
    if (!RegKey::HKCU.open(iceboxPath).isGood()) {
        RegKey icekey = RegKey::HKCU.create(iceboxPath);
        std::wstring iceAcct = makeAccount();
        icekey.setValue(Constants::accountName(), iceAcct);
        icekey.create(Constants::editables());
    }

    //make sure the mapi dll keys are set so you can send mail from pets
    if (!RegKey::HKLM.open(L"Software\\Clients\\Mail\\Polaris").isGood()) {
        logger.log(L"mapi dll keys do not exist in registry");
        //setup the mapi dll so you can send mail from pets
        RegKey hklmSoft = RegKey::HKLM.create(L"Software");
        RegKey clientKey = hklmSoft.create(L"Clients");
        RegKey mailKey = clientKey.create(L"Mail");
        RegKey polarisKey = mailKey.create(Constants::polarisBaseName());
        polarisKey.setValue(L"", L"Polaris MAPI");
        polarisKey.setValue(L"DLLPATH", Constants::polarisExecutablesFolderPath() + L"\\petmapi.dll");
    }


    // make sure basic polaris folder hierarchy is present and cleanse old requests and responses
    std::wstring requestFolder = Constants::requestsPath(Constants::polarisDataPath(_wgetenv(L"USERPROFILE")));
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
        logger.log(L"requests folder for main user does not exist");
        //create
        SHCreateDirectoryEx(NULL, requestFolder.c_str(), NULL);
    }

    // put out the pola.dat file (which stores password)
    std::wstring polaDatPath = Constants::passwordFilePath(Constants::polarisDataPath(_wgetenv(L"USERPROFILE")));
    FILE * polaDatFile = _wfopen(polaDatPath.c_str(), L"a+");
    fclose(polaDatFile);
    EncryptFile(polaDatPath.c_str());


    // destroy old synch files carefully

    // perform the intention list, including deletion of stubborn folders




}
