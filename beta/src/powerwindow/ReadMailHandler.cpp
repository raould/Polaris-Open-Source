#include "stdafx.h"
#include "ReadMailHandler.h"

#include "MessageHandler.h"
#include "MessageResponder.h"
#include "../polaris/Message.h"

class ReadMailHandler : public MessageHandler
{
public:
    virtual void run(Message& request, MessageResponder* out) {
        
        TCHAR client[16383 + 1];
        LONG client_size = sizeof client;
        LONG client_error = ::RegQueryValue(HKEY_CURRENT_USER, L"Software\\Clients\\Mail", client, &client_size);
        if (ERROR_SUCCESS != client_error) {
            out->run(sizeof client_error, &client_error);
            return;
        }
        TCHAR client_key[16383 + 1];
        LONG client_key_size = sizeof client_key;
        wcscpy(client_key, L"Software\\Clients\\Mail\\");
        wcscat(client_key, client);
        wcscat(client_key, L"\\shell\\open\\command");

        TCHAR command[MAX_PATH + 1];
        LONG command_size = sizeof command;
        LONG command_error = ::RegQueryValue(HKEY_LOCAL_MACHINE, client_key, command, &command_size);
        if (ERROR_SUCCESS != command_error) {
            out->run(sizeof command_error, &command_error);
            return;
        }
        STARTUPINFO si = { sizeof(STARTUPINFO) };
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_SHOW;
        PROCESS_INFORMATION pi = {};

        ::CreateProcess(NULL, command, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

        out->run(0, NULL);
    }
};
 
MessageHandler* makeReadMailHandler() {
    return new ReadMailHandler();
}
