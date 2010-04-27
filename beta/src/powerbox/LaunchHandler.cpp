#include "StdAfx.h"
#include "LaunchHandler.h"
#include "MessageHandler.h"
#include "MessageResponder.h"
#include "ActiveWorld.h"
#include "ActivePet.h"
#include "../polaris/conversion.h"
#include "../polaris/RegKey.h"
#include "../polaris/Constants.h"

class LaunchHandler : public MessageHandler
{
	ActiveWorld* m_activeWorld;

public:
    LaunchHandler(ActiveWorld* activeWorld) : m_activeWorld(activeWorld) {}
    virtual ~LaunchHandler() {}

    /**
     * The args list may contain any of the 3 following lists of values:
     *     - an executable path and a petname
     *     - an executable path, a petname, and a document path
     *     - the special value "/default" followed by a document path
     * In the event of a /default request, the executable path and the petname are
     * determined by looking up the defaults based on the suffix of the document
     *
     * TODO: if a default launch is requested, and there is no default entry, 
     *     do something -- pop a warning message, launch in icebox, etc.
     */
    virtual void run(int PID, const std::vector<std::string>& args, MessageResponder* out) {
	    // make sure the exe path and petname are present
	    if (args.size() < 2) {
            out->run(-10, std::vector<std::string>());
            return;
        }

        // Deserialize the arguments.

        std::wstring exePath = L"";
        std::wstring petname = L"";
        std::wstring docPath = L"";
        wchar_t* arg1 = NULL;
        wchar_t* arg2 = NULL;
        wchar_t* wdoc = NULL;
        if (args.size() < 3) {
            Deserialize(args, "SS", &arg1, &arg2);
            if (wcscmp(arg1, L"/default") != 0) {
                exePath = arg1;
                petname = arg2;
            } else {
                docPath = arg2;
                //compute the suffix for which we will find default app and pet
                size_t lastDotIndex = docPath.rfind(L'.');
                std::wstring suffix = docPath.substr(lastDotIndex + 1, docPath.length() - lastDotIndex);
                RegKey suffixKey = RegKey::HKCU.open(Constants::registrySuffixes() + L"\\" + suffix);
                exePath = suffixKey.getValue(Constants::registrySuffixAppName());
                petname = suffixKey.getValue(Constants::registrySuffixPetname());
            }
        } else {
            Deserialize(args, "SSS", &arg1, &arg2, &wdoc);
            exePath = arg1;
            petname = arg2;
            docPath = wdoc;
        }        

        //petname and exepath can be empty if registry not setup with file suffix default launch data,
        // suggesting that there are multiple polarized users, and someone else configured this file extension
        if (exePath == L"" || petname == L"") {                    
            STARTUPINFO si;
            PROCESS_INFORMATION pi;
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            ZeroMemory(&pi, sizeof(pi));
            std::wstring messager = Constants::polarisExecutablesFolderPath() + L"\\Messager.exe";
            CreateProcess(messager.c_str(), 
                L" \"No pet for this file extension. Perhaps another polarized user has set up a file association for this extension. Right-click the document, or polarize this file extension.\" ",
                NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            out -> run(-26, std::vector<std::string>());
        } else {
            // Now we can actually do something.
	        ActivePet* pet = m_activeWorld->getActivePet(petname);
		    int error = pet->launch(exePath, docPath);
            delete[] arg1;
            delete[] arg2;
            delete[] wdoc;
            out->run(error, std::vector<std::string>());
        }
    }
};

MessageHandler* makeLaunchHandler(ActiveWorld* world) {
    return new LaunchHandler(world);
}
