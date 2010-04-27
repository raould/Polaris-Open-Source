#include "StdAfx.h"
#include "SendMailHandler.h"
#include "MessageHandler.h"
#include "MessageResponder.h"
#include <mapi.h>
#include <mapix.h>
#include "../polaris/conversion.h"
#include "../polaris/Runnable.h"

HINSTANCE hlibMAPI = LoadLibrary(L"MAPI32.DLL");
LPMAPISENDMAIL  m_MAPISendMail = (LPMAPISENDMAIL)GetProcAddress(hlibMAPI, "MAPISendMail");

class SendMailTask : public Runnable
{
    MapiMessage* m_msg;
    MessageResponder* m_out;

public:
    SendMailTask(MapiMessage* msg, MessageResponder* out) : m_msg(msg), m_out(out) {}
    virtual ~SendMailTask() {
        delete[] m_msg->lpszSubject;
        delete[] m_msg->lpszNoteText;
        delete m_msg;
    }

    virtual void run() {
        ULONG error = m_MAPISendMail(NULL, NULL, m_msg, MAPI_DIALOG | MAPI_LOGON_UI, 0);
        m_out->run(error, std::vector<std::string>());
    }
};

class SendMailHandler : public MessageHandler
{

public:
    SendMailHandler()  {}
    virtual ~SendMailHandler() {}

    virtual void run(int PID, const std::vector<std::string>& args, MessageResponder* out) {
	    // make sure the exe path and petname are present
	    if (args.size() < 2) {
            out->run(-11, std::vector<std::string>());
        } else {
            MapiMessage* msg = new MapiMessage();
            ZeroMemory(msg, sizeof(MapiMessage));
            Deserialize(args, "ss", &msg->lpszSubject, &msg->lpszNoteText);
            spawn(new SendMailTask(msg, out));
        }
    }
};

MessageHandler* makeSendMailHandler() {
    return new SendMailHandler();
}