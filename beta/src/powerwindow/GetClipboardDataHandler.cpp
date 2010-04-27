// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "StdAfx.h"
#include "GetClipboardDataHandler.h"

#include "MessageHandler.h"
#include "MessageResponder.h"

#include "../polaris/GetClipboardDataParameters.h"
#include "../polaris/Logger.h"
#include "../polaris/Message.h"

class GetClipboardDataHandler : public MessageHandler
{
public:
	GetClipboardDataHandler() {}
    virtual ~GetClipboardDataHandler() {}

    virtual void run(Message& request, MessageResponder* out) {

        Logger logger(L"GetClipboardData");

        // Deserialize the parameters.
        GetClipboardDataParameters* argv;
        request.cast(&argv, sizeof(GetClipboardDataParameters));
        if (argv) {
            // if (OpenClipboard(argv->owner)) {
            if (OpenClipboard(NULL)) {
		        HANDLE handle = GetClipboardData(argv->format);
                if (handle) {
                    SIZE_T size = GlobalSize(handle);
                    if (size) {
                        void* ptr = GlobalLock(handle);
                        if (ptr) {
                            out->run(size, ptr);
                            GlobalUnlock(handle);
                        } else {
                            logger.log(L"GlobalLock()", GetLastError());
                            out->run(0, NULL);
                        }
                    } else {
                        logger.log(L"GlobalSize()", GetLastError());
                        out->run(0, NULL);
                    }
                } else {
                    logger.log(L"GetClipboardData()", GetLastError());
                    out->run(0, NULL);
                }
		        CloseClipboard();
            } else {
                logger.log(L"OpenClipboard()", GetLastError());
                out->run(0, NULL);
            }
        } else {
            logger.log(L"missing arguments", request.length());
            out->run(0, NULL);
        }
	}
};

MessageHandler* makeGetClipboardDataHandler() {
	return new GetClipboardDataHandler();
}
