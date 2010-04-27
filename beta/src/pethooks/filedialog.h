// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once
#include "inject.h"

// Data structure for information scanned from an existing file dialog.
typedef struct {
	const WCHAR* title;
	const WCHAR* path;
	const WCHAR* filetypelabel;
	int filetype;
	int nfiletypes;
	const WCHAR* filetypes[0];
} FILEDIALOGINFO;

// Data structure for information submitted in a file dialog.
typedef struct {
	HWND window;
	const WCHAR* path;
	int filetype;
	int cancel;
} FILEDIALOGSUBMIT;

// These functions are injected into a remote process to manipulate
// file dialogs that are running in the remote process.
EXPORT size_t __cdecl FileDialogInfo(BYTE* buffer, size_t bufsize);
EXPORT size_t __cdecl FileDialogSubmit(BYTE* buffer, size_t bufsize);