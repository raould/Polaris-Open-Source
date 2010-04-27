#include "stdafx.h"
#include "OpenBrowserHandler.h"

#include "ActivePet.h"
#include "ActiveWorld.h"
#include "MessageHandler.h"
#include "MessageResponder.h"
#include "../polaris/Message.h"
#include "../polaris/Logger.h"
#include "../polaris/Constants.h"
#include "../polaris/RegKey.h"


/**
 * The open browser handler follows the following algorithm: 
 * Look in the polaris registry entry to see which pet/app is 
 * specified as the default browser. Launch that pet's 
 * primary application, giving it the requested url
 * on the command line
 */
class OpenBrowserHandler : public MessageHandler
{
    Logger* logger;
    ActiveWorld* m_world; 

public:
    OpenBrowserHandler(ActiveWorld* world) : m_world(world) {
     logger = new Logger(L"OpenBrowserHandler");
    }
    virtual ~OpenBrowserHandler() {
        delete logger;
    }

    // MessageHandler interface.

    virtual void run(Message& request, MessageResponder* out) {

        // Deserialize the command line arguments.
        char* cmd_line;
        request.cast(&cmd_line, 0);
        size_t const maxChars = 10000;
        wchar_t converter[maxChars];
        mbstowcs(converter, cmd_line,maxChars-1);
        std::wstring url = converter;
        //strip off quote marks
        if (url.length() > 0) {
            if (url[0] != L'\"') {
                logger->log(L"no quote at beginning of url, bad format");
            } else {
                url = url.substr(1);
                url = url.substr(0, url.length() - 1);
            }
        }
        RegKey polarisKey = RegKey::HKCU.open(Constants::registryBase());
        std::wstring defaultBrowsePet = polarisKey.getValue(Constants::defaultBrowserPet());
        RegKey browsePetKey = RegKey::HKCU.open(Constants::registryPets() + L"\\" + defaultBrowsePet);
        if (!browsePetKey.isGood()) {
            logger->log(L"request made for launch of default browse pet, but no browse pet present");
            logger->log(defaultBrowsePet + L"< was the specific requested petname");
        } else {
            //limitation: the default browse app is the first app in the pet executables list. this
            // is probably just fine.
            std::wstring appPath = browsePetKey.open(Constants::executables()).getValue(L"a");
            ActivePet* pet = m_world->get(defaultBrowsePet);
            if (pet) {
                pet->launch(appPath, url);
            }
        }        

        out->run(0, NULL);
    }
};

MessageHandler* makeOpenBrowserHandler(ActiveWorld* world) {
    return new OpenBrowserHandler(world);
}