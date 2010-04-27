// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "stdafx.h"
#include "LaunchHandler.h"

#include "ActivePet.h"
#include "ActiveWorld.h"
#include "MessageHandler.h"
#include "MessageResponder.h"
#include "../polaris/Message.h"

class Launch : public ActiveAccount::Task
{
    std::wstring m_exe;
    std::wstring m_file;
    MessageResponder* m_out;

public:
    Launch(std::wstring exe,
           std::wstring file,
           MessageResponder* out) : m_exe(exe),
                                    m_file(file),
                                    m_out(out) {}
    virtual ~Launch() {}

    virtual void run(ActiveAccount& account) {
        std::wstring args = L"";
        if (L"" != m_file) {
            args = L"\"" + account.edit(m_file, true) + L"\"";
        }
        DWORD pid = account.launch(m_exe, args);
        m_out->run(sizeof pid, &pid);
    }
};

ActiveAccount::Task* makeLaunch(std::wstring exe,
                                std::wstring file,
                                MessageResponder* out) {
    return new Launch(exe, file, out);
}

class LaunchHandler : public MessageHandler
{
	ActiveWorld* m_world;

public:
    LaunchHandler(ActiveWorld* world) : m_world(world) {}
    virtual ~LaunchHandler() {}

    /**
     * The args list may contain any of the 3 following lists of values:
     *  - the petname
     *  - the executable path
     *  - the document path.
     */
    virtual void run(Message& request, MessageResponder* out) {

        // Deserialize the command line arguments.
        wchar_t* cmd_line;
        request.cast(&cmd_line, 0);
        int argc = 0;
        wchar_t** argv = CommandLineToArgvW(cmd_line, &argc);

        if (argc >= 2) {
            ActivePet* pet = m_world->load(argv[0]);
            if (pet) {
                pet->send(makeLaunch(argv[1], argc > 2 ? argv[2] : L"", out));
            } else {
                out->run(0, NULL);
            }
        } else {
                out->run(0, NULL);
        }
        LocalFree(argv);
    }
};

MessageHandler* makeLaunchHandler(ActiveWorld* world) {
    return new LaunchHandler(world);
}
