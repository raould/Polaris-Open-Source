#include "StdAfx.h"
#include "GetClipboardDataHandler.h"
#include "MessageHandler.h"
#include "MessageResponder.h"
#include "../polaris/conversion.h"

class GetClipboardDataHandler : public MessageHandler
{
public:
	GetClipboardDataHandler() {}
    virtual ~GetClipboardDataHandler() {}

    virtual void run(int PID, const std::vector<std::string>& args, MessageResponder* out) {
		std::vector<std::string> results;
	    if (args.size() < 1) {
            out->run(ERROR_INVALID_PARAMETER, results);
            return;
        }

        // Deserialize the arguments.
		int format = 0;
        HWND owner = NULL;
		Deserialize(args, "ip", &format, &owner);

        // Get the clipboard data.
        int error = 0;
        if (OpenClipboard(owner)) {
		    HANDLE handle = GetClipboardData(format);
            if (handle) {
                SIZE_T size = GlobalSize(handle);
                if (size) {
                    void* ptr = GlobalLock(handle);
                    if (ptr) {
					    Serialize(&results, "b", size, ptr);
                        GlobalUnlock(handle);
                    } else {
                        error = GetLastError();
                    }
                } else {
                    error = GetLastError();
                }
            } else {
                error = GetLastError();
            }
		    CloseClipboard();
        } else {
            error = GetLastError();
        }

        if (0 == results.size()) {
			Serialize(&results, "b", 0, "");
        }

        out->run(error, results);
	}
};

MessageHandler* makeGetClipboardDataHandler() {
	return new GetClipboardDataHandler();
}
