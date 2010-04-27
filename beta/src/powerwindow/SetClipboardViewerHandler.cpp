#include "stdafx.h"
#include "SetClipboardViewerHandler.h"

#include "ActivePet.h"
#include "MessageHandler.h"
#include "MessageResponder.h"
#include "../polaris/Message.h"

class SetClipboardViewerHandler : public MessageHandler
{
    ActivePet* m_pet;

public:
    SetClipboardViewerHandler(ActivePet* pet) : m_pet(pet) {}
    virtual ~SetClipboardViewerHandler() {}

    // MessageHandler interface.

	virtual void run(Message& request, MessageResponder* out)
    {
        HWND* argv;
        request.cast(&argv, sizeof(HWND));
        if (argv) {
            HWND proxy = m_pet->watchClipboard(*argv);
            out->run(sizeof proxy, &proxy);
        } else {
            out->run(0, NULL);
        }
    }
};

MessageHandler* makeSetClipboardViewerHandler(ActivePet* pet)
{
    return new SetClipboardViewerHandler(pet);
}