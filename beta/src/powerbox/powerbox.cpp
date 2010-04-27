// powerbox.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "EventLoop.h"
#include "EventMessage.h"
#include "MessageDispatcher.h"
#include "LaunchHandler.h"
#include "ActiveWorld.h"
#include "FileEventLoop.h"
#include "../polaris/Constants.h"
#include "../polaris/Logger.h"
#include "../polaris/auto_handle.h"
#include "../polaris/auto_privilege.h"
#include "WindowWatcher.h"
#include "SendMailHandler.h"
#include "FixAndClean.h"
#include "../polaris/RegKey.h"

int _tmain(int argc, _TCHAR* argv[])
{
    
    FixAndClean::fix();

    // Figure out the password for network authentication
    // If no authentication needed, a "" password is created
    // TODO: if the password is "", do not try to set authentication privileges during launch of the pet
    wchar_t password[1024];
    ZeroMemory(password, sizeof(password));
    RegKey polarisKey = RegKey::HKCU.open(Constants::registryBase());
    bool needsPassword = polarisKey.getValue(Constants::registryNeedsPassword()) == L"true";
    bool passwordStored = polarisKey.getValue(Constants::registryPasswordIsStored()) == L"true";
    if (needsPassword && !passwordStored) {
        puts("Enter your password:\n");
        _getws(password);
        for (int i = 0; i < 30; ++i) {printf("\n");}
    } else if (needsPassword && passwordStored) {
        FILE* passwordFile = _wfopen(Constants::passwordFilePath(Constants::polarisDataPath(_wgetenv(L"USERPROFILE"))).c_str(),
            L"r");
        fgetws(password, sizeof(password) -1, passwordFile);
        fclose(passwordFile);
    }
    
    // Enable the needed privileges.
    auto_privilege se_debug(SE_DEBUG_NAME);
    auto_privilege se_increase_quota(SE_INCREASE_QUOTA_NAME);
    auto_privilege se_assign_primary_token(SE_ASSIGNPRIMARYTOKEN_NAME);
    auto_privilege se_restore(SE_RESTORE_NAME);
    auto_privilege se_backup(SE_BACKUP_NAME);



	EventLoop* loop = new EventLoop();
    FileEventLoop* files = new FileEventLoop(loop);
	ActiveWorld* activeWorld = new ActiveWorld(password, files);

    // make this getEnviron for the application data location
    //const std::string appDataPath = getenv("APPDATA");
	//std::string dir = appDataPath + Constants::requestPath();
    std::wstring dir = Constants::requestsPath(Constants::polarisDataPath(_wgetenv(L"USERPROFILE")));
	MessageDispatcher* dispatcher = new MessageDispatcher(dir);
    files->watch(dir, dispatcher);
	dispatcher->add("launch", makeLaunchHandler(activeWorld));
    dispatcher->add("sendmail", makeSendMailHandler());

	addWindowWatcher(loop, activeWorld);

	loop->start();

	printf("made it");
	return 0;
}



