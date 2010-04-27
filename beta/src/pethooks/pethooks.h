// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "inject.h"

typedef BOOL (WINAPI *GetOpenFileNameA_t)(LPOPENFILENAMEA);
BOOL WINAPI NewGetOpenFileNameA(LPOPENFILENAMEA lpofn);

typedef BOOL (WINAPI *GetSaveFileNameA_t)(LPOPENFILENAMEA);
BOOL WINAPI NewGetSaveFileNameA(LPOPENFILENAMEA lpofn);

typedef BOOL (WINAPI *GetOpenFileNameW_t)(LPOPENFILENAMEW);
BOOL WINAPI NewGetOpenFileNameW(LPOPENFILENAMEW lpofn);

typedef BOOL (WINAPI *GetSaveFileNameW_t)(LPOPENFILENAMEW);
BOOL WINAPI NewGetSaveFileNameW(LPOPENFILENAMEW lpofn);

typedef HANDLE (WINAPI *GetClipboardData_t)(UINT);
HANDLE WINAPI NewGetClipboardData(UINT format);

typedef BOOL (WINAPI *CloseClipboard_t)(VOID);
BOOL WINAPI NewCloseClipboard(VOID);

typedef HWND (WINAPI *SetClipboardViewer_t)(HWND);
HWND WINAPI NewSetClipboardViewer(HWND hWndNewViewer);

typedef BOOL (WINAPI *ChangeClipboardChain_t)(HWND,HWND);
BOOL WINAPI NewChangeClipboardChain(HWND hWndRemove, HWND hWndNewNext);

EXPORT size_t __cdecl ApplyHooks(BYTE*, size_t);