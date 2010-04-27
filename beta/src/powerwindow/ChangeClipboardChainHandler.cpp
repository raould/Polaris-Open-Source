#include "stdafx.h"
#include "ChangeClipboardChainHandler.h"

#include "ActivePet.h"
#include "MessageHandler.h"
#include "MessageResponder.h"
#include "../polaris/ChangeClipboardChainParameters.h"
#include "../polaris/Message.h"

class ChangeClipboardChainHandler : public MessageHandler
{
    ActivePet* m_pet;

public:
    ChangeClipboardChainHandler(ActivePet* pet) : m_pet(pet) {}
    virtual ~ChangeClipboardChainHandler() {}

    // MessageHandler interface.

    virtual void run(Message& request, MessageResponder* out)
    {
        ChangeClipboardChainParameters* argv;
        request.cast(&argv, sizeof(ChangeClipboardChainParameters));
        if (argv) {
            BOOL r = m_pet->ignoreClipboard(argv->hWndRemove, argv->hWndNewNext);
            out->run(sizeof r, &r);
        } else {
            out->run(0, NULL);
        }
    }
};

MessageHandler* makeChangeClipboardChainHandler(ActivePet* pet)
{
    return new ChangeClipboardChainHandler(pet);
}