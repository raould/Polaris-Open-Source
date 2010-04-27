#include "stdafx.h"
#include "GetOpenFileNameWHandler.h"
#include "GetSaveFileNameWHandler.h"

#include "MessageHandler.h"
#include "MessageResponder.h"
#include "ActivePet.h"

#include "../polaris/Runnable.h"
#include "../polaris/Message.h"
#include <shlwapi.h>

class GetFileNameWTask : public Runnable {
	bool m_open; // True for open; false for save.
    ActivePet* m_pet;
    Message m_request;
    MessageResponder* m_out;
	const WCHAR* m_originalInitialDir;
	std::wstring m_initialDir;
	const WCHAR* m_originalTitle;
	std::wstring m_title;
    LPOPENFILENAMEW m_lpofn;

public:
    GetFileNameWTask(bool open,
                     ActivePet* pet,
                     Message& request,
                     MessageResponder* out) : m_open(open),
                                              m_pet(pet),
                                              m_request(request),
                                              m_out(out) {
        m_request.cast(&m_lpofn, sizeof(OPENFILENAMEW));
        if (m_lpofn) {
            if (m_lpofn->lStructSize > sizeof(OPENFILENAMEW)) {
                m_lpofn->lStructSize = sizeof(OPENFILENAMEW);
            }
            m_lpofn->hInstance = NULL;
            m_request.fix(&m_lpofn->lpstrFilter);
            m_request.fix(&m_lpofn->lpstrCustomFilter, m_lpofn->nMaxCustFilter * sizeof(wchar_t));
            m_request.fix(&m_lpofn->lpstrFile, m_lpofn->nMaxFile * sizeof(wchar_t));
            m_request.fix(&m_lpofn->lpstrFileTitle, m_lpofn->nMaxFileTitle * sizeof(wchar_t));
            m_request.fix(&m_lpofn->lpstrInitialDir);
            m_request.fix(&m_lpofn->lpstrTitle);
            m_request.fix(&m_lpofn->lpstrDefExt);
            //m_request.fix(&m_lpofn->lpTemplateName);

			// Open the file dialog in the last used location.
			m_originalInitialDir = m_lpofn->lpstrInitialDir;
			m_initialDir = pet->GetFileDialogPath();
			m_lpofn->lpstrInitialDir = m_initialDir.c_str();

			if (!m_open) {
				// Override the default save path given in lpstrFile as well.
				std::wstring path = m_initialDir;
				if (!EndsWith(path, L"\\")) path += L"\\";
				path += PathFindFileNameW(m_lpofn->lpstrFile);
				wcsncpy(m_lpofn->lpstrFile, path.c_str(), m_lpofn->nMaxFile - 1);
				m_lpofn->lpstrFile[m_lpofn->nMaxFile - 1] = 0;
			}
				
			// Adjust the title to associate the file dialog with the pet.
			m_originalTitle = m_lpofn->lpstrTitle;
			m_title = std::wstring(m_lpofn->lpstrTitle ? m_lpofn->lpstrTitle: L"") +
				L" for \xab" + m_pet->getName() + L"\xbb";
			m_lpofn->lpstrTitle = m_title.c_str();

            // Turn off the "no validation" flag, which controls whether the
            // dialog will automatically append the file extension for the
            // selected type.  The only case where I've seen this flag used
            // is Acrobat Reader, which (as recommended in the documentation
            // for OFN_NOVALIDATE) uses a hook procedure to append the file
            // extension.  Since we can't support hook procedures, we have to
            // let the dialog take care of validation.
            m_lpofn->Flags &= ~OFN_NOVALIDATE;
            m_lpofn->Flags &= ~OFN_ENABLEHOOK;
            m_lpofn->Flags &= ~OFN_ENABLETEMPLATE;
            m_lpofn->Flags &= ~OFN_ENABLETEMPLATEHANDLE;
            m_lpofn->Flags &= ~OFN_ENABLEINCLUDENOTIFY;

            m_lpofn->nFileOffset = 0;
            m_lpofn->nFileExtension = 0;
            m_lpofn->lCustData = NULL;
            m_lpofn->lpfnHook = NULL;
            m_lpofn->lpTemplateName = NULL;
            m_lpofn->pvReserved = NULL;
            m_lpofn->dwReserved = 0;
        }
    }
    virtual ~GetFileNameWTask() {}

    // Runnable interface.

    virtual void run() {
		BOOL result = (m_open ? GetOpenFileNameW : GetSaveFileNameW)(m_lpofn);
        if (!result) {
            DWORD error = ::CommDlgExtendedError();
            m_out->run(sizeof error, &error);
            return;
        }

		// Remember the file dialog's current location.
		std::wstring path(m_lpofn->lpstrFile, m_lpofn->nFileOffset);
		m_pet->SetFileDialogPath(path);

        // Fix up the response.
        if (m_lpofn->Flags & OFN_ALLOWMULTISELECT) {

            // Setup the mirror directory.
            std::wstring original_dir = m_lpofn->lpstrFile;
			if (m_lpofn->nFileOffset < original_dir.length()) {
				original_dir = original_dir.substr(0, m_lpofn->nFileOffset - 1);
			}
            const wchar_t* file_list = m_lpofn->lpstrFile + m_lpofn->nFileOffset;
            const wchar_t* file = file_list;
            while (*file) {
                m_pet->edit(original_dir + L"\\" + file, !m_open);
                file += wcslen(file) + 1;
            }
            ++file;

            // Adjust the return arguments.
            std::wstring editable_dir = m_pet->mirror(original_dir + L"\\");
            if (editable_dir.length() + 1 + (file - file_list) > m_lpofn->nMaxFile) {
                DWORD error = FNERR_BUFFERTOOSMALL;
                m_out->run(sizeof error, &error);
                return;
            }
            WORD delta = editable_dir.length() - original_dir.length();
            memmove((void*)(file_list - 1 + delta), file_list - 1, (file - (file_list - 1)) * sizeof(wchar_t));
			memcpy(m_lpofn->lpstrFile, editable_dir.c_str(), editable_dir.length() * sizeof(wchar_t));
            m_lpofn->nFileOffset += delta;
            if (m_lpofn->nFileExtension) {
                m_lpofn->nFileExtension += delta;
            }
        } else {
            std::wstring original_file = m_lpofn->lpstrFile;
            std::wstring editable_file = m_pet->edit(original_file, !m_open);
            if (editable_file.length() + 1 > m_lpofn->nMaxFile) {
                DWORD error = FNERR_BUFFERTOOSMALL;
                m_out->run(sizeof error, &error);
                return;
            }
            wcscpy(m_lpofn->lpstrFile, editable_file.c_str());
            WORD delta = editable_file.length() - original_file.length();
            m_lpofn->nFileOffset += delta;
            if (m_lpofn->nFileExtension) {
                m_lpofn->nFileExtension += delta;
            }
        }

        // Serialize the response.
        m_request.serialize(&m_lpofn->lpstrFilter);
        m_request.serialize(&m_lpofn->lpstrCustomFilter);
        m_request.serialize(&m_lpofn->lpstrFile);
        m_request.serialize(&m_lpofn->lpstrFileTitle);
		m_lpofn->lpstrInitialDir = m_originalInitialDir;
        m_request.serialize(&m_lpofn->lpstrInitialDir);
		m_lpofn->lpstrTitle = m_originalTitle;
        m_request.serialize(&m_lpofn->lpstrTitle);
        m_request.serialize(&m_lpofn->lpstrDefExt);
        m_request.serialize(&m_lpofn->lpTemplateName);

        m_out->run(m_request.length(), m_lpofn);
    }
};

class GetFileNameWHandler : public MessageHandler {
    bool m_open;
    ActivePet* m_pet;

public:
    GetFileNameWHandler(bool open, ActivePet* pet) : m_open(open), m_pet(pet) {}
    virtual ~GetFileNameWHandler() {}

    // MessageHandler interface.

    virtual void run(Message& request, MessageResponder* out) {
        spawn(new GetFileNameWTask(m_open, m_pet, request, out));
    }
};

MessageHandler* makeGetOpenFileNameWHandler(ActivePet* pet) {
    return new GetFileNameWHandler(true, pet);
}

MessageHandler* makeGetSaveFileNameWHandler(ActivePet* pet) {
    return new GetFileNameWHandler(false, pet);
}
