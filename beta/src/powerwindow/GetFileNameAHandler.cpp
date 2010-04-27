// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "stdafx.h"
#include "GetOpenFileNameAHandler.h"
#include "GetSaveFileNameAHandler.h"

#include "MessageHandler.h"
#include "MessageResponder.h"
#include "ActivePet.h"

#include "../polaris/Message.h"
#include <shlwapi.h>

class GetFileNameAHandler : public MessageHandler {
    bool m_open;
    ActiveAccount* m_account;

public:
    GetFileNameAHandler(bool open, ActiveAccount* account) : m_open(open), m_account(account) {}
    virtual ~GetFileNameAHandler() {}

    // MessageHandler interface.

    virtual void run(Message& request, MessageResponder* out) {
        LPOPENFILENAMEA lpofn;
        request.cast(&lpofn, sizeof(OPENFILENAMEA));
        if (!lpofn) {
            out->run(0, NULL);
            return;
        }
        if (lpofn->lStructSize > sizeof(OPENFILENAMEA)) {
            lpofn->lStructSize = sizeof(OPENFILENAMEA);
        }
        lpofn->hInstance = NULL;
        request.fix(&lpofn->lpstrFilter);
        request.fix(&lpofn->lpstrCustomFilter, lpofn->nMaxCustFilter * sizeof(char));
        request.fix(&lpofn->lpstrFile, lpofn->nMaxFile * sizeof(char));
        request.fix(&lpofn->lpstrFileTitle, lpofn->nMaxFileTitle * sizeof(char));
        request.fix(&lpofn->lpstrInitialDir);
        request.fix(&lpofn->lpstrTitle);
        request.fix(&lpofn->lpstrDefExt);
        lpofn->lpTemplateName = NULL;

		// Adjust the title to associate the file dialog with the pet.
        const char* original_title = lpofn->lpstrTitle;
        char petname_a[256];
        wcstombs(petname_a, m_account->getPetname().c_str(), sizeof petname_a);
        std::string m_title = std::string(lpofn->lpstrTitle ? lpofn->lpstrTitle : "") +
			" for \xab" + petname_a + "\xbb";
		lpofn->lpstrTitle = m_title.c_str();

        // Adjust suggested file name
        if (lpofn->lpstrFile) {
            wchar_t pet_file[MAX_PATH];
            mbstowcs(pet_file, lpofn->lpstrFile, sizeof pet_file);
            std::wstring user_file = m_account->invert(pet_file);
            wcstombs(lpofn->lpstrFile, user_file.c_str(), lpofn->nMaxFile);
        }

        // Override any InitialDir setting.
        const char* initial_dir = lpofn->lpstrInitialDir;
        char initial_user_dir[MAX_PATH];
        if (initial_dir) {
            wchar_t pet_dir[MAX_PATH];
            mbstowcs(pet_dir, initial_dir, sizeof pet_dir);
            std::wstring user_dir = m_account->invert(pet_dir);
            wcstombs(initial_user_dir, user_dir.c_str(), sizeof initial_user_dir);
            lpofn->lpstrInitialDir = initial_user_dir;
        }

        // Turn off the "no validation" flag, which controls whether the
        // dialog will automatically append the file extension for the
        // selected type.  The only case where I've seen this flag used
        // is Acrobat Reader, which (as recommended in the documentation
        // for OFN_NOVALIDATE) uses a hook procedure to append the file
        // extension.  Since we can't support hook procedures, we have to
        // let the dialog take care of validation.
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

		BOOL result = (m_open ? GetOpenFileNameA : GetSaveFileNameA)(lpofn);
        if (!result) {
            DWORD error = ::CommDlgExtendedError();
            out->run(sizeof error, &error);
            return;
        }

        // Should the file be copied into the pet account?
        bool copy = m_open || lpofn->Flags & OFN_FILEMUSTEXIST;

        // Fix up the response.
        if (lpofn->Flags & OFN_ALLOWMULTISELECT) {

            // Setup the mirror directory.
            std::string original_dir_a = lpofn->lpstrFile;
			if (lpofn->nFileOffset < original_dir_a.length()) {
				original_dir_a = original_dir_a.substr(0, lpofn->nFileOffset - 1);
			}
            const char* file_list = lpofn->lpstrFile + lpofn->nFileOffset;
            const char* file = file_list;
            while (*file) {
                std::string original_file_a = original_dir_a + "\\" + file;
                wchar_t original_file_w[MAX_PATH + 1];
                mbstowcs(original_file_w, original_file_a.c_str(), sizeof original_file_w);
                m_account->edit(original_file_w, copy);
                file += strlen(file) + 1;
            }
            ++file;

            // Adjust the return arguments.
            wchar_t original_dir_w[MAX_PATH + 1];
            mbstowcs(original_dir_w, original_dir_a.c_str(), sizeof original_dir_w / sizeof(wchar_t));
            std::wstring editable_dir_w = m_account->mirror(original_dir_w);
            char editable_dir_a[MAX_PATH + 1];
            size_t editable_dir_a_len = wcstombs(editable_dir_a, editable_dir_w.c_str(),
                                                 sizeof editable_dir_a / sizeof(char));
            if (editable_dir_a_len + 1 + (file - file_list) > lpofn->nMaxFile) {
                DWORD error = FNERR_BUFFERTOOSMALL;
                out->run(sizeof error, &error);
                return;
            }
            WORD delta = editable_dir_a_len - original_dir_a.length();
            memmove((void*)(file_list - 1 + delta), file_list - 1,
                    (file - (file_list - 1)) * sizeof(char));
			memcpy(lpofn->lpstrFile, editable_dir_a, strlen(editable_dir_a) * sizeof(char));
            lpofn->nFileOffset += delta;
            if (lpofn->nFileExtension) {
                lpofn->nFileExtension += delta;
            }
        } else {
            std::string original_file_a = lpofn->lpstrFile;
            wchar_t original_file_w[MAX_PATH + 1];
            mbstowcs(original_file_w, original_file_a.c_str(), sizeof original_file_w);
            std::wstring editable_file_w = m_account->edit(original_file_w, copy);
            char editable_file_a[MAX_PATH + 1];
            size_t editable_file_a_len = wcstombs(editable_file_a, editable_file_w.c_str(),
                                                  sizeof editable_file_a);
            if (editable_file_a_len + 1 > lpofn->nMaxFile) {
                DWORD error = FNERR_BUFFERTOOSMALL;
                out->run(sizeof error, &error);
                return;
            }
            strcpy(lpofn->lpstrFile, editable_file_a);
            WORD delta = editable_file_a_len - original_file_a.length();
            lpofn->nFileOffset += delta;
            if (lpofn->nFileExtension) {
                lpofn->nFileExtension += delta;
            }
        }

        // Undo overrides.
		lpofn->lpstrTitle = original_title;
        lpofn->lpstrInitialDir = initial_dir;

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

MessageHandler* makeGetOpenFileNameAHandler(ActiveAccount* account) {
    return new GetFileNameAHandler(true, account);
}

MessageHandler* makeGetSaveFileNameAHandler(ActiveAccount* account) {
    return new GetFileNameAHandler(false, account);
}
