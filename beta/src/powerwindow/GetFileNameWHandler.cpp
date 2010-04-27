// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "stdafx.h"
#include "GetOpenFileNameWHandler.h"
#include "GetSaveFileNameWHandler.h"

#include "MessageHandler.h"
#include "MessageResponder.h"
#include "ActivePet.h"

#include "../polaris/Message.h"
#include <shlwapi.h>

class GetFileNameWHandler : public MessageHandler {
    bool m_open;
    ActiveAccount* m_account;

public:
    GetFileNameWHandler(bool open, ActiveAccount* account) : m_open(open), m_account(account) {}
    virtual ~GetFileNameWHandler() {}

    // MessageHandler interface.

    virtual void run(Message& request, MessageResponder* out) {
        LPOPENFILENAMEW lpofn;
        request.cast(&lpofn, sizeof(OPENFILENAMEW));
        if (!lpofn) {
            out->run(0, NULL);
            return;
        }
        if (lpofn->lStructSize > sizeof(OPENFILENAMEW)) {
            lpofn->lStructSize = sizeof(OPENFILENAMEW);
        }
        lpofn->hInstance = NULL;
        request.fix(&lpofn->lpstrFilter);
        request.fix(&lpofn->lpstrCustomFilter, lpofn->nMaxCustFilter * sizeof(wchar_t));
        request.fix(&lpofn->lpstrFile, lpofn->nMaxFile * sizeof(wchar_t));
        request.fix(&lpofn->lpstrFileTitle, lpofn->nMaxFileTitle * sizeof(wchar_t));
        request.fix(&lpofn->lpstrInitialDir);
        request.fix(&lpofn->lpstrTitle);
        request.fix(&lpofn->lpstrDefExt);
        lpofn->lpTemplateName = NULL;

		// Adjust the title to associate the file dialog with the pet.
        const wchar_t* original_title = lpofn->lpstrTitle;
        std::wstring m_title = std::wstring(lpofn->lpstrTitle ? lpofn->lpstrTitle : L"") +
			L" for \xab" + m_account->getPetname() + L"\xbb";
		lpofn->lpstrTitle = m_title.c_str();

        // Adjust suggested file name
        if (lpofn->lpstrFile) {
            wcsncpy(lpofn->lpstrFile, m_account->invert(lpofn->lpstrFile).c_str(), lpofn->nMaxFile);
        }

        // Override any InitialDir setting.
        const wchar_t* initial_dir = lpofn->lpstrInitialDir;
        std::wstring initial_user_dir;
        if (initial_dir) {
            initial_user_dir = m_account->invert(initial_dir);
            lpofn->lpstrInitialDir = initial_user_dir.c_str();
        }

        // Turn off the "no validation" flag, which controls whether the
        // dialog will automatically append the file extension for the
        // selected type.  The only case where I've seen this flag used
        // is Acrobat Reader, which (as recommended in the documentation
        // for OFN_NOVALIDATE) uses a hook procedure to append the file
        // extension.  Since we can't support hook procedures, we have to
        // let the dialog take care of validation.
        DWORD original_flags = lpofn->Flags;
        lpofn->Flags &= ~OFN_NOVALIDATE;
        lpofn->Flags &= ~OFN_ENABLEHOOK;
        lpofn->Flags &= ~OFN_ENABLETEMPLATE;
        lpofn->Flags &= ~OFN_ENABLETEMPLATEHANDLE;
        lpofn->Flags &= ~OFN_ENABLEINCLUDENOTIFY;

        lpofn->nFileOffset = 0;
        lpofn->nFileExtension = 0;
        lpofn->lCustData = NULL;
        lpofn->lpfnHook = NULL;
        lpofn->pvReserved = NULL;
        lpofn->dwReserved = 0;

        DWORD pre_flags = lpofn->Flags;
		BOOL result = (m_open ? GetOpenFileNameW : GetSaveFileNameW)(lpofn);
        if (!result) {
            DWORD error = ::CommDlgExtendedError();
            out->run(sizeof error, &error);
            return;
        }
        DWORD post_flags = lpofn->Flags;
        DWORD changed_flags = pre_flags ^ post_flags;

        // Should the file be copied into the pet account?
        bool copy = m_open || lpofn->Flags & OFN_FILEMUSTEXIST;

        // Fix up the response.
        if (lpofn->Flags & OFN_ALLOWMULTISELECT) {

            // Setup the mirror directory.
            std::wstring original_dir = lpofn->lpstrFile;
			if (lpofn->nFileOffset < original_dir.length()) {
				original_dir = original_dir.substr(0, lpofn->nFileOffset - 1);
			}
            const wchar_t* file_list = lpofn->lpstrFile + lpofn->nFileOffset;
            const wchar_t* file = file_list;
            while (*file) {
                m_account->edit(original_dir + L"\\" + file, copy);
                file += wcslen(file) + 1;
            }
            ++file;

            // Adjust the return arguments.
            std::wstring editable_dir = m_account->mirror(original_dir);
            if (editable_dir.length() + 1 + (file - file_list) > lpofn->nMaxFile) {
                DWORD error = FNERR_BUFFERTOOSMALL;
                out->run(sizeof error, &error);
                return;
            }
            WORD delta = editable_dir.length() - original_dir.length();
            memmove((void*)(file_list - 1 + delta), file_list - 1,
                    (file - (file_list - 1)) * sizeof(wchar_t));
			memcpy(lpofn->lpstrFile, editable_dir.c_str(), editable_dir.length() * sizeof(wchar_t));
            lpofn->nFileOffset += delta;
            if (lpofn->nFileExtension) {
                lpofn->nFileExtension += delta;
            }
        } else {
            std::wstring original_file = lpofn->lpstrFile;
            std::wstring editable_file = m_account->edit(original_file, copy);
            if (editable_file.length() + 1 > lpofn->nMaxFile) {
                DWORD error = FNERR_BUFFERTOOSMALL;
                out->run(sizeof error, &error);
                return;
            }
            wcscpy(lpofn->lpstrFile, editable_file.c_str());
            WORD delta = editable_file.length() - original_file.length();
            lpofn->nFileOffset += delta;
            if (lpofn->nFileExtension) {
                lpofn->nFileExtension += delta;
            }
        }

        // Undo overrides.
		lpofn->lpstrTitle = original_title;
        lpofn->lpstrInitialDir = initial_dir;
        lpofn->Flags = original_flags ^ changed_flags;

        // Serialize the response.
        request.serialize(&lpofn->lpstrFilter);
        request.serialize(&lpofn->lpstrCustomFilter);
        request.serialize(&lpofn->lpstrFile);
        request.serialize(&lpofn->lpstrFileTitle);
        request.serialize(&lpofn->lpstrInitialDir);
        request.serialize(&lpofn->lpstrTitle);
        request.serialize(&lpofn->lpstrDefExt);

        out->run(request.length(), lpofn);
    }
};

MessageHandler* makeGetOpenFileNameWHandler(ActiveAccount* account) {
    return new GetFileNameWHandler(true, account);
}

MessageHandler* makeGetSaveFileNameWHandler(ActiveAccount* account) {
    return new GetFileNameWHandler(false, account);
}
