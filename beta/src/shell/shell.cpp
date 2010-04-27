// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

// open.cpp : Defines the entry point for the application.
//

#include "stdafx.h"

/**
 * The parameters are:
 *  1. The file path.
 *  2. The operation.
 *  3. The default directory.
 *  4. The parameters.
 */
int APIENTRY _tWinMain(HINSTANCE hInstance,
                       HINSTANCE hPrevInstance,
                       LPTSTR    lpCmdLine,
                       int       nCmdShow)
{
    // Parse the command line.
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(lpCmdLine, &argc);

    SHELLEXECUTEINFO execInfo = {};
    execInfo.cbSize = sizeof execInfo;
    execInfo.fMask = SEE_MASK_FLAG_DDEWAIT | SEE_MASK_NOCLOSEPROCESS;
    execInfo.lpVerb = argc > 1 ? argv[1] : NULL;
    execInfo.lpFile = argc > 0 ? argv[0] : NULL;
    execInfo.lpParameters = argc > 3 ? argv[3] : NULL;
    execInfo.lpDirectory = argc > 2 ? argv[2] : NULL;
    execInfo.nShow = SW_SHOWNORMAL;
    ShellExecuteEx(&execInfo);
    LocalFree(argv);
    return (int)execInfo.hInstApp;
}
