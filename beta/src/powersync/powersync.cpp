// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

// powersync.cpp : Defines the entry point for the application.
//

#include "stdafx.h"

#include "SyncLeaderHandler.h"
#include "SyncFollowerHandler.h"
#include "../polaris/Runnable.h"
#include <shellapi.h>

int APIENTRY _tWinMain(HINSTANCE hInstance,
                       HINSTANCE hPrevInstance,
                       LPTSTR    lpCmdLine,
                       int       nCmdShow)
{
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(lpCmdLine, &argc);
    if (argc != 2) {
        return -1;
    }
    std::wstring leader = argv[0];
    std::wstring follower = argv[1];
    LocalFree(argv);

    // Setup the leader.
	Runnable* leader_sync = makeSyncLeaderHandler(leader, follower);
    std::wstring leader_dir = leader.substr(0, leader.find_last_of('\\') + 1);
    HANDLE leader_event = FindFirstChangeNotification(leader_dir.c_str(), FALSE,
                                    FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE);
    if (INVALID_HANDLE_VALUE == leader_event) {
        MessageBox(NULL, leader_dir.c_str(), _T("Leader directory missing"), MB_OK);
        return -1;
    }
	
	// Eventually, the leader and the follower syncers will use custom logic.
	Runnable* follower_sync = makeSyncFollowerHandler(leader, follower);
    std::wstring follower_dir = follower.substr(0, follower.find_last_of('\\') + 1);
    HANDLE follower_event = FindFirstChangeNotification(follower_dir.c_str(), FALSE,
                                    FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE);
    if (INVALID_HANDLE_VALUE == follower_event) {
        MessageBox(NULL, follower_dir.c_str(), _T("Follower directory missing"), MB_OK);
        return -1;
    }


    // Go into the event loop.
    HANDLE event[] = { leader_event, follower_event };
    Runnable* handler[] = { leader_sync, follower_sync };
    int event_size = sizeof event / sizeof(HANDLE);
	MSG msg;
    while (true) {
        DWORD result = ::MsgWaitForMultipleObjects(event_size, event, FALSE, INFINITE,
                                                   QS_ALLINPUT);
        if (WAIT_OBJECT_0 + event_size == result) {
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                if (WM_QUIT == msg.message) {
                    goto done;
                }
			    TranslateMessage(&msg);
			    DispatchMessage(&msg);
            }
        } else if (WAIT_OBJECT_0 <= result && WAIT_OBJECT_0 + event_size > result) {
	        FindNextChangeNotification(event[result - WAIT_OBJECT_0]);
            handler[result - WAIT_OBJECT_0]->run();
        }
    }

    // Cleanup.
done:
    for (int i = event_size; i-- != 0;) {
	    FindCloseChangeNotification(event[i]);
        delete handler[i];
    }
    DeleteFile(leader.c_str());

	return 0;
}
