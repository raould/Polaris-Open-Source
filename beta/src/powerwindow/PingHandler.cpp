// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "stdafx.h"
#include "PingHandler.h"

#include "ActiveWorld.h"
#include "MessageHandler.h"
#include "MessageResponder.h"
#include "../polaris/Message.h"

class PingHandler : public MessageHandler
{
    ActiveWorld* m_world;

public:
    PingHandler(ActiveWorld* world) : m_world(world) {}
    virtual ~PingHandler() {}

    // MessageHandler interface.

    virtual void run(Message& request, MessageResponder* out) {

        // Deserialize the command line arguments.
        wchar_t* cmd_line;
        request.cast(&cmd_line, 0);
        int argc = 0;
        wchar_t** argv = CommandLineToArgvW(cmd_line, &argc);

        if (argc == 1) {
            const ActivePet* pet = m_world->findByName(argv[0]);
            if (pet) {
                out->run(sizeof L"true", L"true");
            } else {
                out->run(sizeof L"false", L"false");
            }
        } else {
            out->run(0, NULL);
        }
        LocalFree(argv);
    }
};

MessageHandler* makePingHandler(ActiveWorld* world) {
    return new PingHandler(world);
}