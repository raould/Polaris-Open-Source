#include "stdafx.h"
#include "OpenHandler.h"

#include "ActivePet.h"
#include "ActiveWorld.h"
#include "MessageHandler.h"
#include "MessageResponder.h"
#include "../polaris/Message.h"

class OpenHandler : public MessageHandler
{
    ActiveWorld* m_world;

public:
    OpenHandler(ActiveWorld* world) : m_world(world) {}
    virtual ~OpenHandler() {}

    // MessageHandler interface.

    virtual void run(Message& request, MessageResponder* out) {

        // Deserialize the command line arguments.
        wchar_t* cmd_line;
        request.cast(&cmd_line, 0);
        int argc = 0;
        wchar_t** argv = CommandLineToArgvW(cmd_line, &argc);

        if (argc >= 2) {
            const ActivePet* pet = m_world->get(argv[0]);
            if (pet) {
                pet->shell(pet->edit(argv[1], false));
            }
        }
        LocalFree(argv);

        out->run(0, NULL);
    }
};

MessageHandler* makeOpenHandler(ActiveWorld* world) {
    return new OpenHandler(world);
}