#include "stdafx.h"
#include "KillHandler.h"

#include "ActiveWorld.h"
#include "MessageHandler.h"
#include "MessageResponder.h"
#include "../polaris/Message.h"

class KillHandler : public MessageHandler
{
    ActiveWorld* m_world;

public:
    KillHandler(ActiveWorld* world) : m_world(world) {}
    virtual ~KillHandler() {}

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
                m_world->kill(pet);
            }
        }
        LocalFree(argv);

        out->run(0, NULL);
    }
};

MessageHandler* makeKillHandler(ActiveWorld* world) {
    return new KillHandler(world);
}