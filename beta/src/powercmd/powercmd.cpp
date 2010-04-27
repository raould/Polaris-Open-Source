// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "stdafx.h"

#include "../polaris/Constants.h"
#include "../polaris/Message.h"
#include "../polaris/Request.h"
#include "../polaris/RPCClient.h"

int APIENTRY _tWinMain(HINSTANCE hInstance,
                       HINSTANCE hPrevInstance,
                       LPTSTR    lpCmdLine,
                       int       nCmdShow)
{
    // if the request folder does not exist, opensafe got launched from a non-polaris account, probably
    // as a result of a default launch request from some unpolarized user
    std::wstring requestFolder = Constants::requestsPath(Constants::polarisDataPath(_wgetenv(L"USERPROFILE")));
    HANDLE dir = CreateFile(requestFolder.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (dir == INVALID_HANDLE_VALUE) {
        MessageBox(NULL, 
            L"You may be running a polarized application from an unpolarized Account. Right-click to open the document.",
            L"Unpolarized account",
            MB_OK | MB_ICONERROR);
    } else {
        CloseHandle(dir);

        // Find the end of the verb.
        wchar_t* begin_verb = lpCmdLine;
        wchar_t* end_verb = lpCmdLine;
        while (*end_verb && *end_verb != L' ') {
            ++end_verb;
        }
        *end_verb = L'\0';
        size_t verb_len = wcstombs(NULL, begin_verb, 1000);
        if (-1 != verb_len) {
            char* verb = new char[verb_len + 1];
            wcstombs(verb, begin_verb, verb_len + 1);
            wchar_t* begin_args = end_verb + 1;

            // Write out the launch request.
	        RPCClient* client = makeClient();
            Request* request = client->initiate(verb, (wcslen(begin_args) + 1) * sizeof(wchar_t));
            request->send(begin_args);
            request->finish(false, 60);

            delete[] verb;
        }
    }
	return 0;
}
