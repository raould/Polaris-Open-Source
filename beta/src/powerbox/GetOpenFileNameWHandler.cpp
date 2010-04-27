#include "stdafx.h"

#include "GetOpenFileNameWHandler.h"
#include "MessageHandler.h"
#include "MessageResponder.h"
#include "ActivePet.h"
#include "../polaris/Runnable.h"
#include "../polaris/conversion.h"

class GetOpenFileNameWTask : public Runnable
{
    ActivePet* m_pet;
    OPENFILENAMEW* m_ofn;
    MessageResponder* m_out;

public:
    GetOpenFileNameWTask(ActivePet* pet,
                         OPENFILENAMEW* ofn,
                         MessageResponder* out) : m_pet(pet),
                                                  m_ofn(ofn),
                                                  m_out(out) {}
    virtual ~GetOpenFileNameWTask() {
        delete[] m_ofn->lpstrFilter;
        delete[] m_ofn->lpstrCustomFilter;
        delete[] m_ofn->lpstrFile;
        delete[] m_ofn->lpstrFileTitle;
        delete[] m_ofn->lpstrInitialDir;
        delete[] m_ofn->lpstrTitle;
        delete[] m_ofn->lpstrDefExt;
        delete m_ofn;
    }

    virtual void run() {
        DWORD error = GetOpenFileNameW(m_ofn);
        if (error) {
            if (m_ofn->Flags & OFN_ALLOWMULTISELECT) {

                // Setup the mirror directory.
                std::wstring original_dir = m_ofn->lpstrFile;
                std::wstring editable_dir = m_pet->mirror(original_dir);
                wchar_t* buffer = new wchar_t[m_ofn->nMaxFile];
                wchar_t* first = buffer;
                wchar_t* last = first + m_ofn->nMaxFile - 1;
                *last = '\0';
                wcsncpy(first, editable_dir.c_str(), last - first);
                first += editable_dir.length() + 1;
                for (const wchar_t* v = m_ofn->lpstrFile + original_dir.length() + 1; *v;) {
                    wcsncpy(first, v, last - first);
                    std::wstring file = v;
                    first += file.length() + 1;
                    if (first >= last) {
                        error = FNERR_BUFFERTOOSMALL;
                        ZeroMemory(buffer, m_ofn->nMaxFile * sizeof(wchar_t));
                        editable_dir = L"";
                    }
                    m_pet->edit(original_dir + L"\\" + file);
                }
                delete[] m_ofn->lpstrFile;
                m_ofn->lpstrFile = buffer;
                m_ofn->nFileOffset = (WORD) (editable_dir.length() + 1);
            } else {
                std::wstring editable_file = m_pet->edit(m_ofn->lpstrFile);
                if (editable_file.length() >= m_ofn->nMaxFile) {
                    error = FNERR_BUFFERTOOSMALL;
                    ZeroMemory(m_ofn->lpstrFile, m_ofn->nMaxFile * sizeof(wchar_t));
                } else {
                    wcsncpy(m_ofn->lpstrFile, editable_file.c_str(), m_ofn->nMaxFile);
                    m_ofn->nFileOffset = (WORD) (editable_file.find_last_of('\\') + 1);
                    m_ofn->nFileExtension = (WORD) (editable_file.find_first_of('.', m_ofn->nFileOffset) + 1);
                }
            }
        } else {
            error = CommDlgExtendedError();
        }

        std::vector<std::string> args;
        Serialize(&args,
                  m_ofn->Flags & OFN_ALLOWMULTISELECT
                    ? "ippVTiiSiSiSSiiiSipSpii"
                    : "ippVTiiViSiSSiiiSipSpii",
                  m_ofn->lStructSize,
                  m_ofn->hwndOwner,
                  m_ofn->hInstance,
                  m_ofn->lpstrFilter,
                  m_ofn->lpstrCustomFilter,
                  m_ofn->nMaxCustFilter,
                  m_ofn->nFilterIndex,
                  m_ofn->lpstrFile,
                  m_ofn->nMaxFile,
                  m_ofn->lpstrFileTitle,
                  m_ofn->nMaxFileTitle,
                  m_ofn->lpstrInitialDir,
                  m_ofn->lpstrTitle,
                  m_ofn->Flags,
                  m_ofn->nFileOffset,
                  m_ofn->nFileExtension,
                  m_ofn->lpstrDefExt,
                  m_ofn->lCustData,
                  m_ofn->lpfnHook,
                  m_ofn->lpTemplateName,
                  m_ofn->pvReserved,
                  m_ofn->dwReserved,
                  m_ofn->FlagsEx);
        m_out->run(error, args);
    }
};

class GetOpenFileNameWHandler : public MessageHandler
{
    ActivePet* m_pet;

public:
    GetOpenFileNameWHandler(ActivePet* pet) : m_pet(pet) {}
    virtual ~GetOpenFileNameWHandler() {}

    // MessageHandler interface.

    virtual void run(int PID, const std::vector<std::string>& args, MessageResponder* out) {
	    // make sure the exe path and petname are present
	    if (args.size() < 23) {
            out->run(ERROR_INVALID_PARAMETER, std::vector<std::string>());
            return;
        }

        // Deserialize the arguments.
        OPENFILENAMEW* lpofn = new OPENFILENAMEW();
        ZeroMemory(lpofn, sizeof(OPENFILENAMEW));
        Deserialize(args,
                    "ippVTiiSiSiSSiiiSipSpii",                                 
                    &lpofn->lStructSize,
                    &lpofn->hwndOwner,
                    &lpofn->hInstance,
                    &lpofn->lpstrFilter,
                    &lpofn->lpstrCustomFilter,
                    &lpofn->nMaxCustFilter,
                    &lpofn->nFilterIndex,
                    &lpofn->lpstrFile,
                    &lpofn->nMaxFile,
                    &lpofn->lpstrFileTitle,
                    &lpofn->nMaxFileTitle,
                    &lpofn->lpstrInitialDir,
                    &lpofn->lpstrTitle,
                    &lpofn->Flags,
                    &lpofn->nFileOffset,
                    &lpofn->nFileExtension,
                    &lpofn->lpstrDefExt,
                    &lpofn->lCustData,
                    &lpofn->lpfnHook,
                    &lpofn->lpTemplateName,
                    &lpofn->pvReserved,
                    &lpofn->dwReserved,
                    &lpofn->FlagsEx);

        // Fix up the data structure.
        lpofn->lStructSize = sizeof(OPENFILENAMEW);
        lpofn->hInstance = NULL;
        if (NULL != lpofn->lpstrCustomFilter) {
            // Allocate a buffer of size nMaxCustFilter.
            wchar_t* buffer = new wchar_t[lpofn->nMaxCustFilter];
            wcsncpy(buffer, lpofn->lpstrCustomFilter, lpofn->nMaxCustFilter);
            size_t first_len = wcslen(lpofn->lpstrCustomFilter) + 1;
            wcsncpy(buffer + first_len, lpofn->lpstrCustomFilter + first_len,
                    lpofn->nMaxCustFilter - first_len);
            delete[] lpofn->lpstrCustomFilter;
            lpofn->lpstrCustomFilter = buffer;
        } else {
            lpofn->nMaxCustFilter = 0;
        }
        if (NULL != lpofn->lpstrFile) {
            wchar_t* buffer = new wchar_t[lpofn->nMaxFile];
            wcsncpy(buffer, lpofn->lpstrFile, lpofn->nMaxFile);
            delete[] lpofn->lpstrFile;
            lpofn->lpstrFile = buffer;
        } else {
            lpofn->nMaxFile = 0;
        }
        if (NULL != lpofn->lpstrFileTitle) {
            delete[] lpofn->lpstrFileTitle;
            lpofn->lpstrFileTitle = new wchar_t[lpofn->nMaxFileTitle];
            ZeroMemory(lpofn->lpstrFileTitle, lpofn->nMaxFileTitle * sizeof(wchar_t));
        } else {
            lpofn->nMaxFileTitle = 0;
        }
        lpofn->Flags &= ~OFN_ENABLEHOOK;
        lpofn->Flags &= ~OFN_ENABLETEMPLATE;
        lpofn->Flags &= ~OFN_ENABLETEMPLATEHANDLE;
        lpofn->Flags &= ~OFN_ENABLEINCLUDENOTIFY;
        
        // Turn off the "no validation" flag, which controls whether the
        // dialog will automatically append the file extension for the
        // selected type.  The only case where I've seen this flag used
        // is Acrobat Reader, which (as recommended in the documentation
        // for OFN_NOVALIDATE) uses a hook procedure to append the file
        // extension.  Since we can't support hook procedures, we have to
        // let the dialog take care of validation.
        lpofn->Flags &= ~OFN_NOVALIDATE;

        lpofn->nFileOffset = 0;
        lpofn->nFileExtension = 0;
        lpofn->lCustData = NULL;
        lpofn->lpfnHook = NULL;
        delete[] lpofn->lpTemplateName;
        lpofn->lpTemplateName = NULL;
        lpofn->pvReserved = NULL;
        lpofn->dwReserved = 0;

        spawn(new GetOpenFileNameWTask(m_pet, lpofn, out));
    }
};

MessageHandler* makeGetOpenFileNameWHandler(ActivePet* pet) {
    return new GetOpenFileNameWHandler(pet);
}
