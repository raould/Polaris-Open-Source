// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

// petmapi.cpp : Defines the entry point for the DLL application.
// 
// To operate successfully, this DLL must be listed in the HKLM and HKCU registries
//     under HKLM/Software/Clients/Mail/Polaris/defaultValue="Polaris Mapi" value DLLPath=pathToDLL
// Also under HKCU/Software/Clients/Mail/defaultvalue="Polaris"

#include "stdafx.h"
#include <mapidefs.h>
#include <mapi.h>
#include "../polaris/MAPISendMailParameters.h"
#include "../polaris/Message.h"
#include "../polaris/Request.h"
#include "../polaris/RPCClient.h"

RPCClient* client = makeClient();

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    return TRUE;
}

ULONG FAR PASCAL
MAPILogon(ULONG_PTR ulUIParam,
          LPSTR lpszProfileName,
          LPSTR lpszPassword,
          FLAGS flFlags,
          ULONG ulReserved,
          LPLHANDLE lplhSession) {
	*lplhSession = lhSessionNull;
    return SUCCESS_SUCCESS;
}

ULONG FAR PASCAL
MAPILogoff(LHANDLE lhSession,
           ULONG_PTR ulUIParam,
           FLAGS flFlags,
           ULONG ulReserved) {
    return SUCCESS_SUCCESS;
}

ULONG FAR PASCAL
MAPISendMail(LHANDLE lhSession,
	         ULONG_PTR ulUIParam,
	         lpMapiMessage lpMessage,
	         FLAGS flFlags,
             ULONG ulReserved) {
    Request* request = client->initiate("MAPISendMail", sizeof(MAPISendMailParameters));
    request->send(&lhSession, sizeof lhSession);
    request->send(&ulUIParam, sizeof ulUIParam);
    request->promise(lpMessage, sizeof MapiMessage);
    request->send(&flFlags, sizeof flFlags);
    request->send(&ulReserved, sizeof ulReserved);

    if (lpMessage) {
        request->send(&lpMessage->ulReserved, sizeof lpMessage->ulReserved);
        request->promise(lpMessage->lpszSubject);
        request->promise(lpMessage->lpszNoteText);
        request->promise(lpMessage->lpszMessageType);
        request->promise(lpMessage->lpszDateReceived);
        request->promise(lpMessage->lpszConversationID);
        request->send(&lpMessage->flFlags, sizeof lpMessage->flFlags);
        request->promise(lpMessage->lpOriginator, sizeof(MapiRecipDesc));
        request->send(&lpMessage->nRecipCount, sizeof lpMessage->nRecipCount);
        request->promise(lpMessage->lpRecips, lpMessage->nRecipCount * sizeof(MapiRecipDesc));
        request->send(&lpMessage->nFileCount, sizeof lpMessage->nFileCount);
        request->promise(lpMessage->lpFiles, lpMessage->nFileCount * sizeof(MapiFileDesc));

        request->send(lpMessage->lpszSubject);
        request->send(lpMessage->lpszNoteText);
        request->send(lpMessage->lpszMessageType);
        request->send(lpMessage->lpszDateReceived);
        request->send(lpMessage->lpszConversationID);
        if (lpMessage->lpOriginator) {
            MapiRecipDesc* recipient = lpMessage->lpOriginator;
            request->send(&recipient->ulReserved, sizeof recipient->ulReserved);
            request->send(&recipient->ulRecipClass, sizeof recipient->ulRecipClass);
            request->promise(recipient->lpszName);
            request->promise(recipient->lpszAddress);
            request->send(&recipient->ulEIDSize, sizeof recipient->ulEIDSize);
            request->promise(recipient->lpEntryID, recipient->ulEIDSize);
        }
        for (ULONG i = 0; i != lpMessage->nRecipCount; ++i) {
            MapiRecipDesc* recipient = lpMessage->lpRecips + i;
            request->send(&recipient->ulReserved, sizeof recipient->ulReserved);
            request->send(&recipient->ulRecipClass, sizeof recipient->ulRecipClass);
            request->promise(recipient->lpszName);
            request->promise(recipient->lpszAddress);
            request->send(&recipient->ulEIDSize, sizeof recipient->ulEIDSize);
            request->promise(recipient->lpEntryID, recipient->ulEIDSize);
        }
        for (ULONG i = 0; i != lpMessage->nFileCount; ++i) {
            MapiFileDesc* file = lpMessage->lpFiles + i;
            request->send(&file->ulReserved, sizeof file->ulReserved);
            request->send(&file->flFlags, sizeof file->flFlags);
            request->send(&file->nPosition, sizeof file->nPosition);
            request->promise(file->lpszPathName);
            request->promise(file->lpszFileName);
            request->promise(file->lpFileType, sizeof(MapiFileTagExt));
        }

        if (lpMessage->lpOriginator) {
            MapiRecipDesc* recipient = lpMessage->lpOriginator;
            request->send(recipient->lpszName);
            request->send(recipient->lpszAddress);
            request->send(recipient->lpEntryID, recipient->ulEIDSize);
        }
        for (ULONG i = 0; i != lpMessage->nRecipCount; ++i) {
            MapiRecipDesc* recipient = lpMessage->lpRecips + i;
            request->send(recipient->lpszName);
            request->send(recipient->lpszAddress);
            request->send(recipient->lpEntryID, recipient->ulEIDSize);
        }
        for (ULONG i = 0; i != lpMessage->nFileCount; ++i) {
            MapiFileDesc* file = lpMessage->lpFiles + i;
            request->send(file->lpszPathName);
            request->send(file->lpszFileName);
            if (file->lpFileType) {
                MapiFileTagExt* file_type = (MapiFileTagExt*)file->lpFileType;
                request->send(&file_type->ulReserved, sizeof file_type->ulReserved);
                request->send(&file_type->cbTag, sizeof file_type->cbTag);
                request->promise(file_type->lpTag, file_type->cbTag);
                request->send(&file_type->cbEncoding, sizeof file_type->cbEncoding);
                request->promise(file_type->lpEncoding, file_type->cbEncoding);
            }
        }

        for (ULONG i = 0; i != lpMessage->nFileCount; ++i) {
            MapiFileDesc* file = lpMessage->lpFiles + i;
            if (file->lpFileType) {
                MapiFileTagExt* file_type = (MapiFileTagExt*)file->lpFileType;
                request->send(file_type->lpTag, file_type->cbTag);
                request->send(file_type->lpEncoding, file_type->cbEncoding);
            }
        }
    }

    Message response = request->finish(true, 60 * 60 * 24);
    ULONG* error;
    response.cast(&error, sizeof(ULONG));
	ULONG e = error ? *error : MAPI_E_FAILURE;
    return e;
}

ULONG FAR PASCAL
MAPISendDocuments(ULONG_PTR ulUIParam,
                  LPSTR lpszDelimChar,
                  LPSTR lpszFilePaths,
                  LPSTR lpszFileNames,
                  ULONG ulReserved) {
    return MAPI_E_FAILURE;
}

ULONG FAR PASCAL
MAPIFindNext(LHANDLE lhSession,
             ULONG_PTR ulUIParam,
             LPSTR lpszMessageType,
             LPSTR lpszSeedMessageID,
             FLAGS flFlags,
             ULONG ulReserved,
             LPSTR lpszMessageID) {
    return MAPI_E_FAILURE;
}

ULONG FAR PASCAL
MAPIReadMail(LHANDLE lhSession,
             ULONG_PTR ulUIParam,
             LPSTR lpszMessageID,
             FLAGS flFlags,
             ULONG ulReserved,
             lpMapiMessage FAR *lppMessage) {
    return MAPI_E_FAILURE;
}

ULONG FAR PASCAL
MAPISaveMail(LHANDLE lhSession,
             ULONG_PTR ulUIParam,
             lpMapiMessage lpMessage,
             FLAGS flFlags,
             ULONG ulReserved,
             LPSTR lpszMessageID) {
    return MAPI_E_FAILURE;
}

ULONG FAR PASCAL
MAPIDeleteMail(LHANDLE lhSession,
               ULONG_PTR ulUIParam,
               LPSTR lpszMessageID,
               FLAGS flFlags,
               ULONG ulReserved) {
    return MAPI_E_FAILURE;
}

ULONG FAR PASCAL
MAPIFreeBuffer(LPVOID pv) {
    return MAPI_E_FAILURE;
}

ULONG FAR PASCAL
MAPIAddress(LHANDLE lhSession,
            ULONG_PTR ulUIParam,
            LPSTR lpszCaption,
            ULONG nEditFields,
            LPSTR lpszLabels,
            ULONG nRecips,
            lpMapiRecipDesc lpRecips,
            FLAGS flFlags,
            ULONG ulReserved,
            LPULONG lpnNewRecips,
            lpMapiRecipDesc FAR *lppNewRecips) {
    return MAPI_E_FAILURE;
}

ULONG FAR PASCAL
MAPIDetails(LHANDLE lhSession,
            ULONG_PTR ulUIParam,
            lpMapiRecipDesc lpRecip,
            FLAGS flFlags,
            ULONG ulReserved) {
    return MAPI_E_FAILURE;
}

ULONG FAR PASCAL
MAPIResolveName(LHANDLE lhSession,
                ULONG_PTR ulUIParam,
                LPSTR lpszName,
                FLAGS flFlags,
                ULONG ulReserved,
                lpMapiRecipDesc FAR *lppRecip) {
    return MAPI_E_FAILURE;
}
