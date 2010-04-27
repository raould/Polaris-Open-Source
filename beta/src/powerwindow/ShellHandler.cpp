// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "stdafx.h"
#include "ShellHandler.h"

#include "ActivePet.h"
#include "ActiveWorld.h"
#include "MessageHandler.h"
#include "MessageResponder.h"
#include "../polaris/Constants.h"
#include "../polaris/Message.h"

class ShellTask : public ActiveAccount::Task
{
    std::wstring m_args;
    MessageResponder* m_out;

public:
    ShellTask(std::wstring args,
              MessageResponder* out) : m_args(args),
                                       m_out(out) {}
    virtual ~ShellTask() {}

    virtual void run(ActiveAccount& account) {
        std::wstring exe = Constants::polarisExecutablesFolderPath() + L"\\shell.exe";
        DWORD pid = account.launch(exe, m_args);
        m_out->run(sizeof pid, &pid);
    }
};

ActiveAccount::Task* makeShellTask(std::wstring args, MessageResponder* out) {
    return new ShellTask(args, out);
}

/**
 * The parameters are:
 *  1. The file path.
 *  2. The operation.
 *  3. The default directory.
 *  4. The parameters.
 */
class ShellHandler : public MessageHandler
{
    ActiveWorld* m_world;

public:
    ShellHandler(ActiveWorld* world) : m_world(world) {}
    virtual ~ShellHandler() {}

    // MessageHandler interface.

    virtual void run(Message& request, MessageResponder* out) {

        // Deserialize the command line arguments.
        wchar_t* cmd_line;
        request.cast(&cmd_line, sizeof(wchar_t));
        if (cmd_line) {
            // Extract the petname.
            wchar_t* begin_petname = cmd_line;
            while (*begin_petname && *begin_petname == L' ') {
                ++begin_petname;
            }
            wchar_t* end_petname = begin_petname;
            while (*end_petname && *end_petname != L' ') {
                ++end_petname;
            }
            wchar_t* args = *end_petname ? end_petname + 1 : L"";
            *end_petname = L'\0';

            ActivePet* pet = m_world->load(begin_petname);
            if (pet) {
                pet->send(makeShellTask(args, out));
            } else {
                out->run(0, NULL);
            }
        } else {
            out->run(0, NULL);
        }
    }
};

MessageHandler* makeShellHandler(ActiveWorld* world) {
    return new ShellHandler(world);
}