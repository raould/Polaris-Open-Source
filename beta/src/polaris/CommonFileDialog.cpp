// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "stdafx.h"

#include "CommonFileDialog.h"
#include "utils.h"

// ---------------------------------------------- instance methods

CommonFileDialog::CommonFileDialog() {
	// Buffers to hold string parameters and results.
	m_title = NULL;
	m_pathname = NULL;
	m_filename = NULL;
	m_initialdir = NULL;
	m_defaultext = NULL;
	m_filters = NULL;
	m_customfilter = NULL;
}

CommonFileDialog::~CommonFileDialog() {
	delete[] m_title;
	delete[] m_pathname;
	delete[] m_filename;
	delete[] m_initialdir;
	delete[] m_defaultext;
	delete[] m_filters;
	delete[] m_customfilter;
}

static WCHAR* MakeImmutableBuffer(wstring ws, const WCHAR** bufferp) {
	WCHAR* buffer = new WCHAR[ws.length() + 1];
	ZeroMemory(buffer, sizeof(WCHAR)*(ws.length() + 1));
	CopyMemory(buffer, ws.c_str(), sizeof(WCHAR)*(ws.length() + 1));
	if (bufferp) *bufferp = buffer;
	return buffer;	
}

static WCHAR* MakeMutableBuffer(wstring ws, WCHAR** bufferp, DWORD* lengthp) {
	DWORD length = (DWORD) (ws.length() * 2 + MAX_PATH);
	WCHAR* buffer = new WCHAR[length + 1];
	ZeroMemory(buffer, sizeof(WCHAR)*(length + 1));
	CopyMemory(buffer, ws.c_str(), sizeof(WCHAR)*(ws.length() + 1));
	if (bufferp) *bufferp = buffer;
	if (lengthp) *lengthp = length;
	return buffer;	
}

// Set up the m_ofn structure for a Get*FileName call according to the given parameters.
void CommonFileDialog::Import(FILEDIALOGPARAMS params) {
	ZeroMemory(&m_ofn, sizeof(m_ofn));
	m_ofn.lStructSize = sizeof(m_ofn);
	m_ofn.hwndOwner = params.owner;
	m_ofn.hInstance = NULL;
	wstring zero(L"\x00", 1);
	wstring filters = L"";
	for (vector<FILTER>::iterator it = params.filters.begin();
		 it != params.filters.end(); it++) {
        filters += wstring(it->name.c_str()) + zero + Join(it->globs, L";") + zero;
	}
	filters += zero;
	m_filters = MakeImmutableBuffer(filters, &(m_ofn.lpstrFilter));
	wstring customfilter = wstring(params.customfilter.name.c_str()) + zero +
                           Join(params.customfilter.globs, L";") + zero;
	m_customfilter = MakeMutableBuffer(customfilter, &(m_ofn.lpstrCustomFilter), &(m_ofn.nMaxCustFilter));
	m_ofn.nFilterIndex = params.filterindex; // nFilterIndex is 1-based
	m_pathname = MakeMutableBuffer(params.pathname, &(m_ofn.lpstrFile), &(m_ofn.nMaxFile));
	m_filename = MakeMutableBuffer(L"", &(m_ofn.lpstrFileTitle), &(m_ofn.nMaxFileTitle));
	m_initialdir = MakeImmutableBuffer(params.initialdir, &(m_ofn.lpstrInitialDir));
	m_title = MakeImmutableBuffer(params.title, &(m_ofn.lpstrTitle));

	m_ofn.Flags = params.flags;

	// Turn off flags because we can't support any callbacks or templates.
	m_ofn.Flags &= ~OFN_ENABLEHOOK;
	m_ofn.Flags &= ~OFN_ENABLETEMPLATE;
	m_ofn.Flags &= ~OFN_ENABLETEMPLATEHANDLE;
	m_ofn.Flags &= ~OFN_ENABLEINCLUDENOTIFY;

    // Turn off the "no validation" flag, which controls whether the dialog
	// will automatically append the file extension for the selected type.
	// The only case where I've seen this flag used is Acrobat Reader, which
	// (as recommended in the documentation for OFN_NOVALIDATE) uses a hook
	// procedure to append the file extension.  Since we can't support hook
	// procedures, we have to let the dialog take care of validation.
    m_ofn.Flags &= ~OFN_NOVALIDATE;

	m_defaultext = MakeImmutableBuffer(params.defaultext, &(m_ofn.lpstrDefExt));
}

// Extract results from the m_ofn structure after a Get*FileName call.
FILEDIALOGRESULTS CommonFileDialog::Export(BOOL success) {
	FILEDIALOGRESULTS results;
	results.cancelled = !success;
	results.pathname = m_ofn.lpstrFile;	// should match m_pathname
	results.filename = m_ofn.lpstrFileTitle; // should match m_filename
	results.fileoffset = m_ofn.nFileOffset;
	results.extoffset = m_ofn.nFileExtension;
	results.filterindex = m_ofn.nFilterIndex;
	WCHAR* wc = m_ofn.lpstrCustomFilter; // should match m_customfilter
	results.customfilter.name = wc;
	wc += wcslen(wc) + 1; // Skip to beginning of filter glob list.
	results.customfilter.globs = Split(wc, L";");
	results.flags = m_ofn.Flags;
	return results;
}

// Static method: run a file dialog.
FILEDIALOGRESULTS CommonFileDialog::Run(FILEDIALOGPARAMS params) {
	CommonFileDialog* dialog = new CommonFileDialog();
	dialog->Import(params);
	if (!params.open) {
		dialog->m_ofn.lpfnHook = ExtensionFixHook;
		dialog->m_ofn.Flags |= OFN_ENABLEHOOK;
	}
	BOOL success = params.open ? GetOpenFileNameW(&(dialog->m_ofn)) :
	                             GetSaveFileNameW(&(dialog->m_ofn));
	return dialog->Export(success);
}

// Static method: hook function for the file dialog.  When the user selects
// a file type, fix up the extension on the filename to match the file type.
UINT_PTR CALLBACK CommonFileDialog::ExtensionFixHook(
	HWND child, UINT message, WPARAM wparam, LPARAM lparam) {
	if (message == WM_NOTIFY && lparam) {
		OFNOTIFY* notify = (OFNOTIFY*) lparam;
		if (notify->hdr.code == CDN_TYPECHANGE) {
			const WCHAR* wc = notify->lpOFN->lpstrFilter;
			int index = notify->lpOFN->nFilterIndex - 1;

			// Skip filters until we get to the indexth filter.
			// There are two strings (name and glob) for each filter; also skip the
			// name of the indexth filter, bringing us to the indexth glob pattern.
			for (int i = 0; i < index*2 + 1; i++) { 
				wc += wcslen(wc) + 1; // Skip a string and its terminating null.
				if (!*wc) return 0; // End of the filter list (index out of range).
			}

			// Split up the list of globs by semicolons.
			vector<wstring> globs = Split(wstring(wc), L";");
			for (vector<wstring>::iterator it = globs.begin(); it != globs.end(); it++) {
				// Use the first pattern that looks like "*.extension".
				if (StartsWith(*it, L"*.")) {
					wstring extension = it->substr(2);
					if (extension.find_first_of(L"?*") == wstring.npos) {
						HWND dialog = GetParent(child);
						HWND textfield = GetDlgItem(dialog, 1152);
						WCHAR buffer[MAX_PATH];
						GetWindowText(textfield, buffer, MAX_PATH);
						wstring filename(buffer);
						size_t dot = filename.find_last_of(L".");
						if (dot != wstring.npos) filename = filename.substr(0, dot);
						filename += wstring(L".") + extension;
						SetWindowText(textfield, filename.c_str());
						break;
					}
				}
			}
		}
	}
	return 0;
}