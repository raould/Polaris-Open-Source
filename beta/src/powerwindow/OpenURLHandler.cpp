// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "stdafx.h"
#include "OpenURLHandler.h"

#include "ActiveWorld.h"
#include "MessageHandler.h"
#include "MessageResponder.h"
#include "ShellHandler.h"
#include "../polaris/Constants.h"
#include "../polaris/Logger.h"
#include "../polaris/Message.h"
#include "../polaris/RegKey.h"

class LaunchURL : public ActiveAccount::Task {

    std::wstring m_exe;
    std::wstring m_args;
    MessageResponder* m_out;

public:
    LaunchURL(std::wstring exe,
            std::wstring args,
            MessageResponder* out) : m_exe(exe),
                                     m_args(args),
                                     m_out(out) {}
    virtual ~LaunchURL() {}

    virtual void run(ActiveAccount& account) {
        DWORD pid = account.launch(m_exe, m_args);
        m_out->run(sizeof pid, &pid);
    }
};

class OpenURLHandler : public MessageHandler
{
	ActiveWorld* m_world;

public:
    OpenURLHandler(ActiveWorld* world) : m_world(world) {}
    virtual ~OpenURLHandler() {}

    /**
     * The args list is just a URL.
     */
    virtual void run(Message& request, MessageResponder* out) {

        // Deserialize the command line arguments.
        wchar_t* cmd_line;
        request.cast(&cmd_line, 0);
        int argc = 0;
        wchar_t** argv = CommandLineToArgvW(cmd_line, &argc);

        // Determine the default browser pet.
        RegKey polaris = RegKey::HKCU.open(Constants::registryBase());
        std::wstring petname = polaris.getValue(Constants::defaultBrowserPet());
        if (petname != L"") {
            RegKey petkey = RegKey::HKCU.open(Constants::registryPets() + L"\\" + petname);
            RegKey executables = petkey.open(Constants::executables());
            std::wstring executable = executables.getValue(L"a");
            if (executable != L"") {
                ActivePet* pet = m_world->load(petname);
                if (pet) {
                    pet->send(new LaunchURL(executable, argc == 1 ? cmd_line : L"", out));
                } else {
                    out->run(0, NULL);
                }
            } else {
                out->run(0, NULL);
                Logger::basicLog(L"The browser pet has no default executable");
            }
        } else {
            // So use the icebox.
            if (argc == 1) {
                ActivePet* pet = m_world->load(Constants::icebox());
                if (pet) {
                    pet->send(makeShellTask(cmd_line, out));
                } else {
                    out->run(0, NULL);
                }
            } else {
                out->run(0, NULL);  // No browser pet is configured.
                Logger::basicLog(L"Start Browser request with no browser pet configured.");
            }
        }
        LocalFree(argv);
    }
};

MessageHandler* makeOpenURLHandler(ActiveWorld* world) {
    return new OpenURLHandler(world);
}
