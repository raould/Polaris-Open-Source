#include "stdafx.h"

#include "GetOpenFileNameAHandler.h"
#include "MessageHandler.h"
#include "MessageResponder.h"
#include "ActivePet.h"
#include "../polaris/Runnable.h"
#include "../polaris/conversion.h"

class GetOpenFileNameATask : public Runnable
{
    ActivePet* m_pet;
    OPENFILENAMEA* m_ofn;
    MessageResponder* m_out;

public:
    GetOpenFileNameATask(ActivePet* pet, 
                         OPENFILENAMEA* ofn,
                         MessageResponder* out) : m_pet(pet),
                                                  m_ofn(ofn),
                                                  m_out(out) {}
    virtual ~GetOpenFileNameATask() {
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
        DWORD error = GetOpenFileNameA(m_ofn);
        if (error) {
            if (m_ofn->Flags & OFN_ALLOWMULTISELECT) {

                // Setup the mirror directory.
                std::string original_dir = m_ofn->lpstrFile;
                std::string editable_dir = m_pet->mirror(original_dir);
                char* buffer = new char[m_ofn->nMaxFile];
                char* first = buffer;
                char* last = first + m_ofn->nMaxFile - 1;
                *last = '\0';
                strncpy(first, editable_dir.c_str(), last - first);
                first += editable_dir.length() + 1;
                for (const char* v = m_ofn->lpstrFile + m_ofn->nFileOffset; *v;) {
                    strncpy(first, v, last - first);
                    std::string file = v;
                    first += file.length() + 1;
                    if (first >= last) {
                        error = FNERR_BUFFERTOOSMALL;
                        ZeroMemory(buffer, m_ofn->nMaxFile);
                        editable_dir = "";
                    }
                    m_pet->edit(original_dir + "\\" + file);
                }
                delete[] m_ofn->lpstrFile;
                m_ofn->lpstrFile = buffer;
                m_ofn->nFileOffset = editable_dir.length() + 1;
            }
        } else {
            printf("GetOpenFileNameA() failed: %d\n", CommDlgExtendedError());
        }

        std::vector<std::string> args;
        Serialize(&args,
                  "ippvtiisisissiiisipspii",                                 
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

class GetOpenFileNameAHandler : public MessageHandler
{
    ActivePet* m_pet;

public:
    GetOpenFileNameAHandler(ActivePet* pet) : m_pet(pet) {}
    virtual ~GetOpenFileNameAHandler() {}

    // MessageHandler interface.

    virtual void run(int PID, const std::vector<std::string>& args, MessageResponder* out) {
	    // make sure the exe path and petname are present
	    if (args.size() < 23) {
            out->run(ERROR_INVALID_PARAMETER, std::vector<std::string>());
            return;
        }

        // Deserialize the arguments.
        OPENFILENAMEA* lpofn = new OPENFILENAMEA();
        ZeroMemory(lpofn, sizeof(OPENFILENAMEA));
        Deserialize(args, "ippvtiisisissiiisipspii",
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
        lpofn->lStructSize = sizeof(OPENFILENAMEA);
        lpofn->hInstance = NULL;
        if (NULL != lpofn->lpstrCustomFilter) {
            // Allocate a buffer of size nMaxCustFilter.
            char* buffer = new char[lpofn->nMaxCustFilter];
            strncpy(buffer, lpofn->lpstrCustomFilter, lpofn->nMaxCustFilter);
            size_t first_len = strlen(lpofn->lpstrCustomFilter) + 1;
            strncpy(buffer + first_len, lpofn->lpstrCustomFilter + first_len,
                    lpofn->nMaxCustFilter - first_len);
            delete[] lpofn->lpstrCustomFilter;
            lpofn->lpstrCustomFilter = buffer;
        } else {
            lpofn->nMaxCustFilter = 0;
        }
        if (NULL != lpofn->lpstrFile) {
            char* buffer = new char[lpofn->nMaxFile];
            strncpy(buffer, lpofn->lpstrFile, lpofn->nMaxFile);
            delete[] lpofn->lpstrFile;
            lpofn->lpstrFile = buffer;
        } else {
            lpofn->nMaxFile = 0;
        }
        if (NULL != lpofn->lpstrFileTitle) {
            delete[] lpofn->lpstrFileTitle;
            lpofn->lpstrFileTitle = new char[lpofn->nMaxFileTitle];
            ZeroMemory(lpofn->lpstrFileTitle, lpofn->nMaxFileTitle);
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

        spawn(new GetOpenFileNameATask(m_pet, lpofn, out));
    }
};

MessageHandler* makeGetOpenFileNameAHandler(ActivePet* pet) {
    return new GetOpenFileNameAHandler(pet);
}
