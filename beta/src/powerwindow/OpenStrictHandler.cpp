// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "stdafx.h"
#include "OpenStrictHandler.h"

#include "ActivePet.h"
#include "ActiveWorld.h"
#include "LaunchHandler.h"
#include "MessageHandler.h"
#include "MessageResponder.h"
#include "../polaris/Message.h"
#include "../polaris/Logger.h"
#include "../polaris/Constants.h"
#include "../polaris/RegKey.h"

class SyncAndShell : public ActiveAccount::Task
{
    std::wstring m_file;
    MessageResponder* m_out;

public:
    SyncAndShell(std::wstring file,
                 MessageResponder* out) : m_file(file),
                                          m_out(out) {}
    virtual ~SyncAndShell() {}

    virtual void run(ActiveAccount& account) {
        // Sync.
        std::wstring args = L"\"" + account.edit(m_file, true) + L"\"";

        // And shell.
        std::wstring exe = Constants::polarisExecutablesFolderPath() + L"\\shell.exe";
        DWORD pid = account.launch(exe, args);
        m_out->run(sizeof pid, &pid);
    }
};
/**
 * The strict open handler follows the following algorithm: 
 * given a doc path, look at the file extension. If the file extension
 * is a known polaris extension (found in the suffixes section of the
 * polaris registry area), then launch the doc in the specified
 * pet with the specified app. If the suffix does not exist, 
 * launch the doc in the icebox, using whatever app is 
 * chosen by the shell.
 *
 * TODO: still doesn't confirm that the requestor has the authority over
 * the file that it is asking the powerbox to exercise. Must impersonate
 * the requestor account, try opening the file.
 */
class OpenStrictHandler : public MessageHandler
{
    ActiveWorld* m_world; 
    Logger m_logger;

public:
    OpenStrictHandler(ActiveWorld* world) : m_world(world), m_logger(L"OpenStrictHandler") {}
    virtual ~OpenStrictHandler() {}

    // MessageHandler interface.

    virtual void run(Message& request, MessageResponder* out) {

		m_logger.log(L"invoked to run");
        // Deserialize the command line arguments.
        wchar_t* cmd_line;
        request.cast(&cmd_line, 0);
        int argc = 0;
        wchar_t** argv = CommandLineToArgvW(cmd_line, &argc);

        // The only command line argument is the name of the file to edit.
        if (argc == 1) {
            std::wstring docPath = argv[0];
            size_t dotIndex = docPath.rfind(L".");
            if (dotIndex == std::wstring::npos) {
                m_logger.log(docPath + L" file to launch has no extension");
                out->run(0, NULL);
            } else {
                std::wstring suffix = docPath.substr(dotIndex + 1);
                RegKey suffixesKey = RegKey::HKCU.open(Constants::registrySuffixes());
                RegKey suffixKey = suffixesKey.open(suffix);
                if (suffixKey.isGood()) {
                    std::wstring petname = suffixKey.getValue(Constants::registrySuffixPetname());
                    std::wstring appPath = suffixKey.getValue(Constants::registrySuffixAppName());
                    ActivePet* pet = m_world->load(petname);
                    if (pet) {
                        pet->send(makeLaunch(appPath, docPath, out));
                    } else {
                        out->run(0, NULL);
                    }
                } else {
                    //use icebox, suffix not recognized
                    ActivePet* pet = m_world->load(Constants::icebox());
                    if (pet) {
                        pet->send(new SyncAndShell(docPath, out));
                    } else {
                        out->run(0, NULL);
                    }
                }
            }
        } else {
            out->run(0, NULL);
        }
    }
};

MessageHandler* makeOpenStrictHandler(ActiveWorld* world) {
    return new OpenStrictHandler(world);
}