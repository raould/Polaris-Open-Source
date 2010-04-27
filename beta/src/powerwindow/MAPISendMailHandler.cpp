// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "StdAfx.h"
#include "MAPISendMailHandler.h"

#include "MessageHandler.h"
#include "MessageResponder.h"

#include "../polaris/Message.h"

#include <mapi.h>
#include <mapix.h>

#include "../polaris/MAPISendMailParameters.h"

HINSTANCE hlibMAPI = LoadLibrary(L"MAPI32.DLL");
LPMAPISENDMAIL  m_MAPISendMail = (LPMAPISENDMAIL)GetProcAddress(hlibMAPI, "MAPISendMail");

class MAPISendMailHandler : public MessageHandler
{
public:
    MAPISendMailHandler() {}
    virtual ~MAPISendMailHandler() {}

    // MessageHandler interface.

    virtual void run(Message& request, MessageResponder* out) {

        // Deserialize the parameters.
        MAPISendMailParameters* argv;
        request.cast(&argv, sizeof(MAPISendMailParameters));
        if (argv) {
            argv->flFlags |= MAPI_DIALOG; // Enforce user confirmation of the send request.
            argv->ulReserved = 0;
            request.fix(&argv->lpMessage, sizeof(MapiMessage));
            if (argv->lpMessage) {
                argv->lpMessage->ulReserved = 0;
                request.fix(&argv->lpMessage->lpszSubject);
                request.fix(&argv->lpMessage->lpszNoteText);
                request.fix(&argv->lpMessage->lpszMessageType);
                request.fix(&argv->lpMessage->lpszDateReceived);
                request.fix(&argv->lpMessage->lpszConversationID);
                request.fix(&argv->lpMessage->lpOriginator, sizeof(MapiRecipDesc));
                if (argv->lpMessage->lpOriginator) {
                    argv->lpMessage->lpOriginator->ulReserved = 0;
                    request.fix(&argv->lpMessage->lpOriginator->lpszName);
                    request.fix(&argv->lpMessage->lpOriginator->lpszAddress);
                    request.fix(&argv->lpMessage->lpOriginator->lpEntryID,
                                argv->lpMessage->lpOriginator->ulEIDSize);
                }
                request.fix(&argv->lpMessage->lpRecips,
                            argv->lpMessage->nRecipCount * sizeof(MapiRecipDesc));
                if (argv->lpMessage->lpRecips) {
                    for (ULONG i = 0; i != argv->lpMessage->nRecipCount; ++i) {
                        MapiRecipDesc* recip = argv->lpMessage->lpRecips + i;
                        recip->ulReserved = 0;
                        request.fix(&recip->lpszName);
                        request.fix(&recip->lpszAddress);
                        request.fix(&recip->lpEntryID, recip->ulEIDSize);
                    }
                }
                request.fix(&argv->lpMessage->lpFiles,
                            argv->lpMessage->nFileCount * sizeof(MapiFileDesc));
                if (argv->lpMessage->lpFiles) {
                    for (ULONG i = 0; i != argv->lpMessage->nFileCount; ++i) {
                        MapiFileDesc* file = argv->lpMessage->lpFiles + i;
                        file->ulReserved = 0;
                        request.fix(&file->lpszPathName);
                        request.fix(&file->lpszFileName);
                        request.fix(&file->lpFileType, sizeof(MapiFileTagExt));
                        if (file->lpFileType) {
                            MapiFileTagExt* file_type = (MapiFileTagExt*)file->lpFileType;
                            file_type->ulReserved = 0;
                            request.fix(&file_type->lpTag, file_type->cbTag);
                            request.fix(&file_type->lpEncoding, file_type->cbEncoding);
                        }
                    }
                }
            }
            ULONG error = m_MAPISendMail(argv->lhSession, argv->ulUIParam,
                                         argv->lpMessage, argv->flFlags, argv->ulReserved);
            out->run(sizeof error, &error);
        } else {
            out->run(0, NULL);
        }
    }
};

MessageHandler* makeMAPISendMailHandler() {
    return new MAPISendMailHandler();
}