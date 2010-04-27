// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

// pethooks.cpp : Defines the entry point for the DLL application.

#include "stdafx.h"

#include <vector>

#include "hook.h"
#include "inject.h"
#include "pethooks.h"

#include "../polaris/GetClipboardDataParameters.h"
#include "../polaris/ChangeClipboardChainParameters.h"
#include "../polaris/Message.h"
#include "../polaris/Request.h"
#include "../polaris/RPCClient.h"

RPCClient* client = makeClient();

BOOL WINAPI GetFileNameA(const char* method_name, LPOPENFILENAMEA lpofn) {

    // We only know how to send stuff we understand and should only send stuff the client understands.
    DWORD sentStructSize = lpofn->lStructSize;
    if (sizeof(OPENFILENAMEA) < sentStructSize) {
        sentStructSize = sizeof(OPENFILENAMEA);
    }
    Request* request = client->initiate(method_name, sizeof(OPENFILENAMEA));
    request->send(&sentStructSize, sizeof sentStructSize);
    request->send(&lpofn->hwndOwner, sizeof lpofn->hwndOwner);
    request->send(&lpofn->hInstance, sizeof lpofn->hInstance);
    size_t lpstrFilter_length;
    if (lpofn->lpstrFilter) {
        const char* first = lpofn->lpstrFilter;
        const char* last = first;
        do {
            last += strlen(last) + 1;
            last += strlen(last) + 1;
        } while(*last != '\0');
        ++last;
        lpstrFilter_length = last - first;
    } else {
        lpstrFilter_length = 0;
    }
    request->promise(lpofn->lpstrFilter, lpstrFilter_length * sizeof(char));
    request->promise(lpofn->lpstrCustomFilter, lpofn->nMaxCustFilter * sizeof(char));
    request->send(&lpofn->nMaxCustFilter, sizeof lpofn->nMaxCustFilter);
    request->send(&lpofn->nFilterIndex, sizeof lpofn->nFilterIndex);
    request->promise(lpofn->lpstrFile, lpofn->nMaxFile * sizeof(char));
    request->send(&lpofn->nMaxFile, sizeof lpofn->nMaxFile);
    request->promise(lpofn->lpstrFileTitle, lpofn->nMaxFileTitle * sizeof(char));
    request->send(&lpofn->nMaxFileTitle, sizeof lpofn->nMaxFileTitle);
    request->promise(lpofn->lpstrInitialDir);
    request->promise(lpofn->lpstrTitle);
    request->send(&lpofn->Flags, sizeof lpofn->Flags);
    request->send(&lpofn->nFileOffset, sizeof lpofn->nFileOffset);
    request->send(&lpofn->nFileExtension, sizeof lpofn->nFileExtension);
    request->promise(lpofn->lpstrDefExt);
    request->send(&lpofn->lCustData, sizeof lpofn->lCustData);
    request->send(&lpofn->lpfnHook, sizeof lpofn->lpfnHook);
    request->send(&lpofn->lpTemplateName, sizeof lpofn->lpTemplateName);
    if (sentStructSize == sizeof(OPENFILENAMEA)) {
        request->send(&lpofn->pvReserved, sizeof lpofn->pvReserved);
        request->send(&lpofn->dwReserved, sizeof lpofn->dwReserved);
        request->send(&lpofn->FlagsEx, sizeof lpofn->FlagsEx);
    } else {
        void* pvReserved = 0;
        DWORD dwReserved = 0;
        DWORD FlagsEx = 0;
        request->send(&pvReserved, sizeof pvReserved);
        request->send(&dwReserved, sizeof dwReserved);
        request->send(&FlagsEx, sizeof FlagsEx);
    }

    request->send(lpofn->lpstrFilter, lpstrFilter_length * sizeof(char));
    request->send(lpofn->lpstrCustomFilter, lpofn->nMaxCustFilter * sizeof(char));
    request->send(lpofn->lpstrFile, lpofn->nMaxFile * sizeof(char));
    request->send(lpofn->lpstrFileTitle, lpofn->nMaxFileTitle * sizeof(char));
    request->send(lpofn->lpstrInitialDir);
    request->send(lpofn->lpstrTitle);
    request->send(lpofn->lpstrDefExt);

    Message response = request->finish(true, 60 * 60 * 24);
    LPOPENFILENAMEA selected;
    response.cast(&selected, sizeof(OPENFILENAMEA));
    if (selected) {
        if (lpofn->lpstrCustomFilter) {
            response.fix(&selected->lpstrCustomFilter, selected->nMaxCustFilter * sizeof(char));
            if (selected->lpstrCustomFilter) {
                memcpy(lpofn->lpstrCustomFilter,
                       selected->lpstrCustomFilter,
                       lpofn->nMaxCustFilter * sizeof(char));
            }
        }
        lpofn->nFilterIndex = selected->nFilterIndex;
        if (lpofn->lpstrFile) {
            response.fix(&selected->lpstrFile, selected->nMaxFile * sizeof(char));
            if (selected->lpstrFile) {
                memcpy(lpofn->lpstrFile, selected->lpstrFile, lpofn->nMaxFile * sizeof(char));
            }
        }
        if (lpofn->lpstrFileTitle) {
            response.fix(&selected->lpstrFileTitle, selected->nMaxFileTitle * sizeof(char));
            if (selected->lpstrFileTitle) {
                memcpy(lpofn->lpstrFileTitle, selected->lpstrFileTitle, lpofn->nMaxFileTitle * sizeof(char));
            }
        }
        lpofn->Flags = selected->Flags;
        lpofn->nFileOffset = selected->nFileOffset;
        lpofn->nFileExtension = selected->nFileExtension;
    }
    return NULL != selected;
}

BOOL WINAPI NewGetOpenFileNameA(LPOPENFILENAMEA lpofn) {
    return GetFileNameA("GetOpenFileNameA", lpofn);
}

BOOL WINAPI NewGetSaveFileNameA(LPOPENFILENAMEA lpofn) {
    return GetFileNameA("GetSaveFileNameA", lpofn);
}

BOOL WINAPI GetFileNameW(const char* method_name, LPOPENFILENAMEW lpofn) {

    // We only know how to send stuff we understand and should only send stuff the client understands.
    DWORD sentStructSize = lpofn->lStructSize;
    if (sizeof(OPENFILENAMEW) < sentStructSize) {
        sentStructSize = sizeof(OPENFILENAMEW);
    }
    Request* request = client->initiate(method_name, sizeof(OPENFILENAMEW));
    request->send(&sentStructSize, sizeof sentStructSize);
    request->send(&lpofn->hwndOwner, sizeof lpofn->hwndOwner);
    request->send(&lpofn->hInstance, sizeof lpofn->hInstance);
    size_t lpstrFilter_length;
    if (lpofn->lpstrFilter) {
        const wchar_t* first = lpofn->lpstrFilter;
        const wchar_t* last = first;
        do {
            last += wcslen(last) + 1;
            last += wcslen(last) + 1;
        } while(*last != L'\0');
        ++last;
        lpstrFilter_length = last - first;
    } else {
        lpstrFilter_length = 0;
    }
    request->promise(lpofn->lpstrFilter, lpstrFilter_length * sizeof(wchar_t));
    request->promise(lpofn->lpstrCustomFilter, lpofn->nMaxCustFilter * sizeof(wchar_t));
    request->send(&lpofn->nMaxCustFilter, sizeof lpofn->nMaxCustFilter);
    request->send(&lpofn->nFilterIndex, sizeof lpofn->nFilterIndex);
    request->promise(lpofn->lpstrFile, lpofn->nMaxFile * sizeof(wchar_t));
    request->send(&lpofn->nMaxFile, sizeof lpofn->nMaxFile);
    request->promise(lpofn->lpstrFileTitle, lpofn->nMaxFileTitle * sizeof(wchar_t));
    request->send(&lpofn->nMaxFileTitle, sizeof lpofn->nMaxFileTitle);
    request->promise(lpofn->lpstrInitialDir);
    request->promise(lpofn->lpstrTitle);
    request->send(&lpofn->Flags, sizeof lpofn->Flags);
    request->send(&lpofn->nFileOffset, sizeof lpofn->nFileOffset);
    request->send(&lpofn->nFileExtension, sizeof lpofn->nFileExtension);
    request->promise(lpofn->lpstrDefExt);
    request->send(&lpofn->lCustData, sizeof lpofn->lCustData);
    request->send(&lpofn->lpfnHook, sizeof lpofn->lpfnHook);
    request->send(&lpofn->lpTemplateName, sizeof lpofn->lpTemplateName);
    if (sentStructSize == sizeof(OPENFILENAMEW)) {
        request->send(&lpofn->pvReserved, sizeof lpofn->pvReserved);
        request->send(&lpofn->dwReserved, sizeof lpofn->dwReserved);
        request->send(&lpofn->FlagsEx, sizeof lpofn->FlagsEx);
    } else {
        void* pvReserved = 0;
        DWORD dwReserved = 0;
        DWORD FlagsEx = 0;
        request->send(&pvReserved, sizeof pvReserved);
        request->send(&dwReserved, sizeof dwReserved);
        request->send(&FlagsEx, sizeof FlagsEx);
    }

    request->send(lpofn->lpstrFilter, lpstrFilter_length * sizeof(wchar_t));
    request->send(lpofn->lpstrCustomFilter, lpofn->nMaxCustFilter * sizeof(wchar_t));
    request->send(lpofn->lpstrFile, lpofn->nMaxFile * sizeof(wchar_t));
    request->send(lpofn->lpstrFileTitle, lpofn->nMaxFileTitle * sizeof(wchar_t));
    request->send(lpofn->lpstrInitialDir);
    request->send(lpofn->lpstrTitle);
    request->send(lpofn->lpstrDefExt);

    Message response = request->finish(true, 60 * 60 * 24);
    LPOPENFILENAMEW selected;
    response.cast(&selected, sizeof(OPENFILENAMEW));
    if (selected) {
        if (lpofn->lpstrCustomFilter) {
            response.fix(&selected->lpstrCustomFilter, selected->nMaxCustFilter * sizeof(wchar_t));
            if (selected->lpstrCustomFilter) {
                memcpy(lpofn->lpstrCustomFilter,
                       selected->lpstrCustomFilter,
                       lpofn->nMaxCustFilter * sizeof(wchar_t));
            }
        }
        lpofn->nFilterIndex = selected->nFilterIndex;
        if (lpofn->lpstrFile) {
            response.fix(&selected->lpstrFile, selected->nMaxFile * sizeof(wchar_t));
            if (selected->lpstrFile) {
                memcpy(lpofn->lpstrFile, selected->lpstrFile, lpofn->nMaxFile * sizeof(wchar_t));
            }
        }
        if (lpofn->lpstrFileTitle) {
            response.fix(&selected->lpstrFileTitle, selected->nMaxFileTitle * sizeof(wchar_t));
            if (selected->lpstrFileTitle) {
                memcpy(lpofn->lpstrFileTitle, selected->lpstrFileTitle, lpofn->nMaxFileTitle * sizeof(wchar_t));
            }
        }
        lpofn->Flags = selected->Flags;
        lpofn->nFileOffset = selected->nFileOffset;
        lpofn->nFileExtension = selected->nFileExtension;
    }
    return NULL != selected;
}

BOOL WINAPI NewGetOpenFileNameW(LPOPENFILENAMEW lpofn) {
    return GetFileNameW("GetOpenFileNameW", lpofn);
}
 
BOOL WINAPI NewGetSaveFileNameW(LPOPENFILENAMEW lpofn) {
    return GetFileNameW("GetSaveFileNameW", lpofn);
}

std::vector<HGLOBAL> open_handles;

HANDLE WINAPI NewGetClipboardData(UINT format) {
    HANDLE result = ORIGINAL(GetClipboardData)(format);
    if (NULL == result) {
        DWORD original_error = GetLastError();

        HWND owner = GetOpenClipboardWindow();
        ORIGINAL(CloseClipboard)();
        Request* request = client->initiate("GetClipboardData", sizeof(GetClipboardDataParameters));
        request->send(&owner, sizeof owner);
        request->send(&format, sizeof format);
        Message response = request->finish(false, 3);
        OpenClipboard(owner);
        size_t data_size = response.length();
        if (data_size != 0) {
            HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE, data_size);
            if (handle) {
                void* ptr = GlobalLock(handle);
                if (ptr) {
                    void* data;
                    response.cast(&data, data_size);
                    memcpy(ptr, data, data_size);
                    GlobalUnlock(handle);
                    result = handle;
                    if (result) {
                        SetLastError(0);
                        open_handles.push_back(handle);
                        return result;
                    }
                }
                GlobalFree(handle);
            }
        }

        SetLastError(original_error);
    }
    return result;
}

BOOL WINAPI NewCloseClipboard() {
    BOOL r = ORIGINAL(CloseClipboard)();
    for (int i = open_handles.size(); i-- != 0;) {
        GlobalFree(open_handles[i]);
    }
    open_handles.clear();
    return r;
}

HWND WINAPI NewSetClipboardViewer(HWND hWndNewViewer) {
    Request* request = client->initiate("SetClipboardViewer", sizeof hWndNewViewer);
    request->send(&hWndNewViewer, sizeof hWndNewViewer);
    Message response = request->finish(true, 3);
    HWND* r;
    response.cast(&r, sizeof(HWND));
    return r ? *r : NULL;
}

BOOL WINAPI NewChangeClipboardChain(HWND hWndRemove, HWND hWndNewNext) {
    Request* request = client->initiate("ChangeClipboardChain", sizeof ChangeClipboardChainParameters);
    request->send(&hWndRemove, sizeof hWndRemove);
    request->send(&hWndNewNext, sizeof hWndNewNext);
    Message response = request->finish(true, 3);
    BOOL* r;
    response.cast(&r, sizeof(BOOL));
    return r ? *r : FALSE;
}

// This is an injected function; see InjectAndCall for calling conventions.
// Input: nothing.  Output: nothing.
EXPORT size_t __cdecl ApplyHooks(BYTE*, size_t) {
	HookAddHook("comdlg32", "GetOpenFileNameA", NewGetOpenFileNameA);
	HookAddHook("comdlg32", "GetSaveFileNameA", NewGetSaveFileNameA);
	HookAddHook("comdlg32", "GetOpenFileNameW", NewGetOpenFileNameW);
	HookAddHook("comdlg32", "GetSaveFileNameW", NewGetSaveFileNameW);
	HookAddHook("user32", "GetClipboardData", NewGetClipboardData);
	HookAddHook("user32", "CloseClipboard", NewCloseClipboard);
	//HookAddHook("user32", "SetClipboardViewer", NewSetClipboardViewer);
	//HookAddHook("user32", "ChangeClipboardChain", NewChangeClipboardChain);
	HookAddGetProcAddressHook();
	HookAllLoadedAndFutureModules();
	return 0;
}