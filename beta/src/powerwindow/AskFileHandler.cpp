#include "stdafx.h"
#include "AskOpenHandler.h"
#include "AskSaveHandler.h"

#include "MessageHandler.h"
#include "MessageResponder.h"
#include "ActivePet.h"
#include "../polaris/Message.h"
#include "../polaris/RegKey.h"
#include "../polaris/Runnable.h"
#include "../polaris/Constants.h"

typedef BOOL (APIENTRY *GetFileName)(LPOPENFILENAME);

class AskFileTask : public Runnable
{
    GetFileName m_command;
    ActivePet* m_pet;
    Message m_request;
    MessageResponder* m_out;

public:
    AskFileTask(GetFileName command,
                ActivePet* pet,
                Message& request,
                MessageResponder* out) : m_command(command),
                                         m_pet(pet),
                                         m_request(request),
                                         m_out(out) {}
    virtual ~AskFileTask() {}

    // Runnable interface.

    virtual void run() {

        // Deserialize the command line arguments.
        wchar_t* cmd_line;
        m_request.cast(&cmd_line, 0);
        int argc = 0;
        wchar_t** argv = CommandLineToArgvW(cmd_line, &argc);

        // Fill out the file request.
        OPENFILENAME ofn = { sizeof(OPENFILENAME) };

        // Setup the filter.
        const size_t filter_option = wcslen(L"/FILTER=");
        wchar_t filter[2048] = {};
        size_t filter_length = 0;
        for (int i = 0; i != argc; ++i) {
            if (wcsncmp(L"/FILTER=", argv[i], filter_option) == 0) {
                wcsncpy(filter + filter_length, argv[i] + filter_option, sizeof filter - filter_length);
                wcstok(filter + filter_length, L";");
            }
        }
        ofn.lpstrFilter = filter;

        wchar_t file[MAX_PATH] = {};
        ofn.lpstrFile = file;
        ofn.nMaxFile = sizeof file;

        // Setup the title.
        const size_t title_option = wcslen(L"/TITLE=");
        for (int i = 0; i != argc; ++i) {
            if (wcsncmp(L"/TITLE=", argv[i], title_option) == 0) {
                ofn.lpstrTitle = argv[i] + title_option;
                break;
            }
        }

        if ((*m_command)(&ofn)) {
            m_out->run(wcslen(ofn.lpstrFile) + 1, ofn.lpstrFile);
        } else {
            m_out->run(0, NULL);
        }
        LocalFree(argv);
    }
};

class AskOpenHandler : public MessageHandler
{
    GetFileName m_command;
	ActivePet* m_pet;

public:
    AskOpenHandler(GetFileName command, ActivePet* pet) : m_command(command), m_pet(pet) {}
    virtual ~AskOpenHandler() {}

    virtual void run(Message& request, MessageResponder* out) {
        spawn(new AskFileTask(m_command, m_pet, request, out));
    }
};

MessageHandler* makeAskOpenHandler(ActivePet* pet) {
    return new AskOpenHandler(&::GetOpenFileName, pet);
}

MessageHandler* makeAskSaveHandler(ActivePet* pet) {
    return new AskOpenHandler(&::GetSaveFileName, pet);
}
