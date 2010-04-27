// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once
#include "commdlg.h"

using namespace std;

typedef struct {
	wstring name; // name of filter (e.g. "HTML Documents")
	vector<wstring> globs; // list of patterns (e.g. "*.html", "*.htm")
} FILTER;

typedef struct {
	bool open; // true for an Open dialog; false for a Save dialog
	HWND owner; // owner window
	wstring title; // title of dialog window
	wstring pathname; // initially selected pathname
	wstring initialdir; // path to initial directory
	wstring defaultext; // extension to be appended if none typed
	int filterindex; // index of initially selected filter in filters
	vector<FILTER> filters; // list of selection filters
	FILTER customfilter; // user-customized filter pattern
	int flags; // see MSDN documentation on OPENFILENAME.Flags
} FILEDIALOGPARAMS;

typedef struct {
	bool cancelled; // user selected "Cancel"
	wstring pathname; // full path to selected file
	wstring filename; // filename of selected file
	int fileoffset; // offset of filename in path
	int extoffset; // offset of file extension in path
	int filterindex; // index of selected filter in filters
	FILTER customfilter; // user-customized filter pattern
	int flags; // see MSDN documentation on OPENFILENAME.Flags
} FILEDIALOGRESULTS;

// Provide a clean interface to the Windows common file dialog.  The
// corresponding Win32 calls are GetOpenFileName and GetSaveFileName;
// these take a pointer to an OPENFILENAME structure that they modify
// in place.  The CommonFileDialog class encapsulates them using
// safer, non-mutating structures for the parameters and results, and
// also takes care of hooking issues.
class CommonFileDialog {
	OPENFILENAMEW m_ofn;
	WCHAR* m_title;
	WCHAR* m_pathname;
	WCHAR* m_filename;
	WCHAR* m_initialdir;
	WCHAR* m_defaultext;
	WCHAR* m_filters;
	WCHAR* m_customfilter;

	// Instances of CommonFileDialog are private.
	CommonFileDialog();
	~CommonFileDialog();
	void Import(FILEDIALOGPARAMS params);
	FILEDIALOGRESULTS Export(BOOL success);
	static UINT_PTR CALLBACK ExtensionFixHook(HWND child, UINT message, WPARAM wparam, LPARAM lparam);

public:
	static FILEDIALOGRESULTS Run(FILEDIALOGPARAMS params);
};
