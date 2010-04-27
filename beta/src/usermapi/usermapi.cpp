// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

// usermapi.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include <mapidefs.h>
#include <mapi.h>
#include <tchar.h>

HINSTANCE hlibMAPI;
LPMAPILOGON  m_MAPILogon;
LPMAPILOGOFF  m_MAPILogoff;
LPMAPISENDMAIL  m_MAPISendMail;
LPMAPISENDDOCUMENTS  m_MAPISendDocuments;
LPMAPIFINDNEXT  m_MAPIFindNext;
LPMAPIREADMAIL  m_MAPIReadMail;
LPMAPISAVEMAIL  m_MAPISaveMail;
LPMAPIDELETEMAIL  m_MAPIDeleteMail;
LPMAPIFREEBUFFER  m_MAPIFreeBuffer;
LPMAPIADDRESS  m_MAPIAddress;
LPMAPIDETAILS  m_MAPIDetails;
LPMAPIRESOLVENAME  m_MAPIResolveName;

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	if (DLL_PROCESS_ATTACH == ul_reason_for_call) {

		// Determine the user's preferred MAPI client.
		BYTE name[128 * sizeof(TCHAR)];
		{
			HKEY hkcu_mail;
			LONG error = RegOpenKey(HKEY_CURRENT_USER, _T("Software\\Clients\\Mail"), &hkcu_mail);
			if (ERROR_SUCCESS != error) {
				return FALSE;
			}

			DWORD type;
			DWORD size = sizeof name;
			error = RegQueryValueEx(hkcu_mail, NULL, NULL, &type, name, &size);
			if (ERROR_SUCCESS != error) {
				return FALSE;
			}
			RegCloseKey(hkcu_mail);
		}

		// Determine the preferred MAPI client's DLLPath.
		BYTE dll_path[(MAX_PATH + 1) * sizeof(TCHAR)];
		{
			HKEY hklm_mail;
			LONG error = RegOpenKey(HKEY_LOCAL_MACHINE, _T("Software\\Clients\\Mail"), &hklm_mail);
			if (ERROR_SUCCESS != error) {
				return FALSE;
			}

			HKEY hklm_mail_prog;
			error = RegOpenKey(hklm_mail, (LPCTSTR)name, &hklm_mail_prog);
			if (ERROR_SUCCESS != error) {
				return FALSE;
			}

			DWORD type;
			DWORD size = sizeof dll_path;
			error = RegQueryValueEx(hklm_mail_prog, _T("DLLPathEx"), NULL, &type, dll_path, &size);
			if (ERROR_SUCCESS != error) {
				size = sizeof dll_path;
				error = RegQueryValueEx(hklm_mail_prog, _T("DLLPath"), NULL, &type, dll_path, &size);
				if (ERROR_SUCCESS != error) {
					return FALSE;
				}
			}
			RegCloseKey(hklm_mail);
			RegCloseKey(hklm_mail_prog);
		}

		// Get pointers to all the MAPI methods.
		hlibMAPI = LoadLibrary((LPCTSTR)dll_path);
		m_MAPILogon = (LPMAPILOGON)GetProcAddress(hlibMAPI, "MAPILogon");
		m_MAPILogoff = (LPMAPILOGOFF)GetProcAddress(hlibMAPI, "MAPILogoff");
		m_MAPISendMail = (LPMAPISENDMAIL)GetProcAddress(hlibMAPI, "MAPISendMail");
		m_MAPISendDocuments = (LPMAPISENDDOCUMENTS)GetProcAddress(hlibMAPI, "MAPISendDocuments");
		m_MAPIFindNext = (LPMAPIFINDNEXT)GetProcAddress(hlibMAPI, "MAPIFindNext");
		m_MAPIReadMail = (LPMAPIREADMAIL)GetProcAddress(hlibMAPI, "MAPIReadMail");
		m_MAPISaveMail = (LPMAPISAVEMAIL)GetProcAddress(hlibMAPI, "MAPISaveMail");
		m_MAPIDeleteMail = (LPMAPIDELETEMAIL)GetProcAddress(hlibMAPI, "MAPIDeleteMail");
		m_MAPIFreeBuffer = (LPMAPIFREEBUFFER)GetProcAddress(hlibMAPI, "MAPIFreeBuffer");
		m_MAPIAddress = (LPMAPIADDRESS)GetProcAddress(hlibMAPI, "MAPIAddress");
		m_MAPIDetails = (LPMAPIDETAILS)GetProcAddress(hlibMAPI, "MAPIDetails");
		m_MAPIResolveName = (LPMAPIRESOLVENAME)GetProcAddress(hlibMAPI, "MAPIResolveName");
	}

    return TRUE;
}

ULONG FAR PASCAL
MAPILogon(ULONG_PTR ulUIParam,
          LPSTR lpszProfileName,
          LPSTR lpszPassword,
          FLAGS flFlags,
          ULONG ulReserved,
          LPLHANDLE lplhSession) {
    return m_MAPILogon(ulUIParam, lpszProfileName, lpszPassword, flFlags, ulReserved, lplhSession);
}

ULONG FAR PASCAL
MAPILogoff(LHANDLE lhSession,
           ULONG_PTR ulUIParam,
           FLAGS flFlags,
           ULONG ulReserved) {
    return m_MAPILogoff(lhSession, ulUIParam, flFlags, ulReserved);
}

ULONG FAR PASCAL
MAPISendMail(LHANDLE lhSession,
	         ULONG_PTR ulUIParam,
	         lpMapiMessage lpMessage,
	         FLAGS flFlags,
             ULONG ulReserved) {
	return m_MAPISendMail(lhSession, ulUIParam, lpMessage, flFlags, ulReserved);
}

ULONG FAR PASCAL
MAPISendDocuments(ULONG_PTR ulUIParam,
                  LPSTR lpszDelimChar,
                  LPSTR lpszFilePaths,
                  LPSTR lpszFileNames,
                  ULONG ulReserved) {
    return m_MAPISendDocuments(ulUIParam, lpszDelimChar, lpszFilePaths, lpszFileNames, ulReserved);
}

ULONG FAR PASCAL
MAPIFindNext(LHANDLE lhSession,
             ULONG_PTR ulUIParam,
             LPSTR lpszMessageType,
             LPSTR lpszSeedMessageID,
             FLAGS flFlags,
             ULONG ulReserved,
             LPSTR lpszMessageID) {
    return m_MAPIFindNext(lhSession, ulUIParam, lpszMessageType, lpszSeedMessageID, flFlags, ulReserved, lpszMessageID);
}

ULONG FAR PASCAL
MAPIReadMail(LHANDLE lhSession,
             ULONG_PTR ulUIParam,
             LPSTR lpszMessageID,
             FLAGS flFlags,
             ULONG ulReserved,
             lpMapiMessage FAR *lppMessage) {
    return m_MAPIReadMail(lhSession, ulUIParam, lpszMessageID, flFlags, ulReserved, lppMessage);
}

ULONG FAR PASCAL
MAPISaveMail(LHANDLE lhSession,
             ULONG_PTR ulUIParam,
             lpMapiMessage lpMessage,
             FLAGS flFlags,
             ULONG ulReserved,
             LPSTR lpszMessageID) {
    return m_MAPISaveMail(lhSession, ulUIParam, lpMessage, flFlags, ulReserved, lpszMessageID);
}

ULONG FAR PASCAL
MAPIDeleteMail(LHANDLE lhSession,
               ULONG_PTR ulUIParam,
               LPSTR lpszMessageID,
               FLAGS flFlags,
               ULONG ulReserved) {
    return m_MAPIDeleteMail(lhSession, ulUIParam, lpszMessageID, flFlags, ulReserved);
}

ULONG FAR PASCAL
MAPIFreeBuffer(LPVOID pv) {
    return m_MAPIFreeBuffer(pv);
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
    return m_MAPIAddress(lhSession, ulUIParam, lpszCaption, nEditFields, lpszLabels, nRecips, lpRecips, flFlags, ulReserved, lpnNewRecips, lppNewRecips);
}

ULONG FAR PASCAL
MAPIDetails(LHANDLE lhSession,
            ULONG_PTR ulUIParam,
            lpMapiRecipDesc lpRecip,
            FLAGS flFlags,
            ULONG ulReserved) {
    return m_MAPIDetails(lhSession, ulUIParam, lpRecip, flFlags, ulReserved);
}

ULONG FAR PASCAL
MAPIResolveName(LHANDLE lhSession,
                ULONG_PTR ulUIParam,
                LPSTR lpszName,
                FLAGS flFlags,
                ULONG ulReserved,
                lpMapiRecipDesc FAR *lppRecip) {
    return m_MAPIResolveName(lhSession, ulUIParam, lpszName, flFlags, ulReserved, lppRecip);
}
