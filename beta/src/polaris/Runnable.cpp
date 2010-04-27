// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "stdafx.h"

#include "Runnable.h"
#include "auto_handle.h"

DWORD WINAPI ThreadProc(LPVOID msg) {
    Runnable* task = (Runnable*)msg;
    task->run();
    delete task;
    return 0;
}

DWORD spawn(Runnable* task) {
    DWORD threadID;
    auto_handle thread(CreateThread(NULL, 0, &ThreadProc, task, 0, &threadID));
    return threadID;
}
