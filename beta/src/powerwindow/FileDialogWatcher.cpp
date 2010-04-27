// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "stdafx.h"
#include "FileDialogWatcher.h"

#include "EventLoop.h"
#include "ActivePet.h"
#include "ActiveWorld.h"
#include "../polaris/auto_handle.h"
#include "../polaris/auto_privilege.h"
#include "../pethooks/filedialog.h"
#include "../polaris/Logger.h"
#include "../polaris/Runnable.h"
#include "../polaris/utils.h"
#include "../polaris/InjectRequest.h"
#include "../polaris/CommonFileDialog.h"
#include <vector>
#include <shlwapi.h>
#include "../polaris/GetSetFileDialogPath.h"

using namespace std;

static vector<HWND> FindFileDialogs();
static vector<HWND> FindTopLevel(wstring classname);
static bool IsFileDialog(HWND window);
static FILEDIALOGPARAMS FileDialogParams(HWND window, ActiveAccount& account, FILEDIALOGINFO* info);
static wstring GetSavePath(FILEDIALOGPARAMS params, FILEDIALOGRESULTS results);
static bool ConfirmOverwrite(wstring pathname, HWND owner);

class FileDialogHandler : public ActiveAccount::Task {
	HWND m_dialog;

public:
    FileDialogHandler(HWND dialog) : m_dialog(dialog) {}
	virtual ~FileDialogHandler() {}

	virtual void run(ActiveAccount& account) {
		// Hide the pet's file dialog as quickly as possible.
		// Sometimes this takes more than one attempt.
		while (IsWindowVisible(m_dialog)) ShowWindow(m_dialog, SW_HIDE);

        DWORD pid = 0;
		GetWindowThreadProcessId(m_dialog, &pid);
        auto_privilege se_debug(SE_DEBUG_NAME);
        auto_handle process(::OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid));

		// Scan the contents of the file dialog.  (This must be performed from
		// within the dialog's owning process because the Windows accessibility
		// features we need don't always work on remote procesess.)
		Request* request = new InjectRequest(
			process.get(), "FileDialogInfo", sizeof(m_dialog), 20000);
		request->send(&m_dialog, sizeof(m_dialog));
		Message reply = request->finish(true, 1);

		if (reply.length()) {
			// Deserialize the reply.
			FILEDIALOGINFO* info;
			reply.cast(&info, sizeof(*info));
			size_t size = sizeof(*info) + sizeof(WCHAR*)*(info->nfiletypes);
			reply.cast(&info, size);
			reply.fix(&(info->title));
			reply.fix(&(info->path));
			reply.fix(&(info->filetypelabel));
			for (int i = 0; i < info->nfiletypes; i++) {
				reply.fix(&(info->filetypes[i]));
			}

			// Bring up an empowered dialog to substitute for the pet dialog.
			FILEDIALOGPARAMS params = FileDialogParams(m_dialog, account, info);
			//params.initialdir = m_pet->GetFileDialogPath();
			FILEDIALOGRESULTS results;
			while (1) {
				results = CommonFileDialog::Run(params);

				// If it's a save dialog, confirm before overwriting.
				if (!params.open && !results.cancelled) {
					results.pathname = GetSavePath(params, results);
					if (!ConfirmOverwrite(results.pathname, params.owner)) {
						params.filterindex = results.filterindex;
						params.pathname = results.pathname;
						continue;
					}
				}
				break;
			}

			// Carry out the user's intent.
			wstring petpathname;
			if (!results.cancelled) {
				// Set up a synchronized pet-local copy of the file.
				petpathname = account.edit(results.pathname, params.open);

				// Retain the current path for future file dialogs.
				const WCHAR* path = results.pathname.c_str();
				const WCHAR* filename = PathFindFileName(path);
				if (filename && filename > path) {
					//m_pet->SetFileDialogPath(results.pathname.substr(0, filename - path));
				}
			}

			// We stuff this information back into the original dialog
			// regardless of whether the user pressed "Cancel" because
			// some applications still use the information (e.g. Word
			// remembers the selected file type even if you cancel).
			FILEDIALOGSUBMIT submit;
			submit.window = m_dialog;
			submit.path = petpathname.c_str();
			submit.filetype = results.filterindex;
			submit.cancel = results.cancelled;

			Request* request = new InjectRequest(
				process.get(), "FileDialogSubmit", sizeof(submit), 0);
			request->send(&(submit.window), sizeof(submit.window));
			request->promise(submit.path);
			request->send(&(submit.filetype), sizeof(submit.filetype));
			request->send(&(submit.cancel), sizeof(submit.cancel));
			request->send(submit.path);
			request->finish(true, 100);
		}

		ShowWindow(m_dialog, SW_SHOW);
	}
};

class FileDialogWatcher : public Runnable {
    EventLoop* m_event_loop;
    ActiveWorld* m_world;
	HANDLE m_timer;
	Logger m_logger;
	set<HWND, less<HWND> > m_dialogs;

public:
	FileDialogWatcher(EventLoop* event_loop,
                      ActiveWorld* world) : m_event_loop(event_loop),
                                            m_world(world),
                                            m_logger(L"FileDialogWatcher") {
		m_timer = CreateWaitableTimer(NULL, TRUE, NULL);
		if (m_timer == NULL) {
            m_logger.log(L"CreateWaitableTimer()", GetLastError());
		}
        m_event_loop->watch(m_timer, this);

		run();
	}

	virtual ~FileDialogWatcher() {
        m_event_loop->ignore(m_timer, this);
        CloseHandle(m_timer);
    }

	// Runnable interface.
	virtual void run() {
		// Reset the timer.
		LARGE_INTEGER delay;
		// The "due time" is measured in 100-ns intervals; a negative due time
		// means a delay relative to the current time.
		delay.QuadPart = 0.1 / (-100e-9);
		if (!SetWaitableTimer(m_timer, &delay, 0, NULL, NULL, 0)) {
            m_logger.log(L"SetWaitableTimer()", GetLastError());
		}

		// Look for file dialogs.
		vector<HWND> dialogs = FindFileDialogs();
		for (vector<HWND>::iterator it = dialogs.begin(); it != dialogs.end(); it++) {
			if (m_dialogs.find(*it) == m_dialogs.end()) {
				// We haven't seen this dialog before.
				m_dialogs.insert(*it);

				// If it belongs to a pet, replace it with an empowered dialog.
				ActivePet* pet = m_world->findByWindow(*it);
                if (pet) {
                    pet->send(new FileDialogHandler(*it));
                } else {
					// To take care of dialogs that linger offscreen when they were
					// last opened on a multi-monitor setup and the second monitor
					// is no longer connected, move the dialog to a visible place.
					HWND owner = GetParent(*it);
					HWND parent = GetAncestor(*it, GA_PARENT);
					HWND desktop = GetDesktopWindow();
					// GetParent returns owner or parent, so make sure that owner
					// really is the owner and there is no parent window.
					if (owner && (parent == NULL || parent == desktop)) {
                        Sleep(100);

						RECT drect, orect, deskrect;
						GetWindowRect(*it, &drect);
						GetWindowRect(desktop, &deskrect);
						GetWindowRect(owner, &orect);
						int w = drect.right - drect.left;
						int h = drect.bottom - drect.top;

						// First, try to center the dialog on its owner.
						int x = (orect.left + orect.right)/2 - w/2;
						int y = (orect.top + orect.bottom)/2 - h/2;

						// Then try to avoid obscuring the title bar of the owner.
						if (x < orect.left + 5) x = orect.left + 5;
						if (y < orect.top + 40) y = orect.top + 40;

						// Then keep the dialog from going offscreen.
						if (x < deskrect.left) x = deskrect.left;
						if (y < deskrect.top) y = deskrect.top;
						if (x > deskrect.right - w) x = deskrect.right - w;
						if (y > deskrect.bottom - h) y = deskrect.bottom - h;

					    SetWindowPos(*it, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
					}
				}
			}
		}

		// Check for dialogs that have gone away in the last timer interval.
		for (set<HWND>::iterator it = m_dialogs.begin(); it != m_dialogs.end();) {
            if (!IsWindow(*it)) { m_dialogs.erase(it++); }
            else { it++; }
		}
	}
};

Runnable* MakeFileDialogWatcher(EventLoop* event_loop, ActiveWorld* world) {
	return new FileDialogWatcher(event_loop, world);
}

// Get a list of all existing common file dialogs and Office file dialogs.
static vector<HWND> FindFileDialogs() {
	wchar_t* classnames[6] = {
		L"#32770", // standard Windows dialogs
		L"bosa_sdm_Microsoft Word 10.0", // Word 2000 dialogs
		L"bosa_sdm_Microsoft Office Word 11.0", // Word 2003 dialogs
		L"bosa_sdm_XL9", // Excel 2000 or 2003 dialogs
		L"bosa_sdm_Mso96", // PowerPoint, Access, FrontPage, Outlook
		NULL
	};

	// Check all dialog windows and collect the ones that are file dialogs.
	vector<HWND> dialogs;
	for (int i = 0; classnames[i]; i++) {
		vector<HWND> found = FindTopLevel(classnames[i]);
		for (vector<HWND>::iterator it = found.begin(); it != found.end(); it++) {
			if (IsFileDialog(*it)) dialogs.push_back(*it);
		}
	}
	return dialogs;
}

// Get a vector of all the top-level windows of a given class.
static vector<HWND> FindTopLevel(wstring classname) {
	vector<HWND> windows;
	HWND window = 0;
	while (true) {
		window = FindWindowEx(NULL, window, classname.c_str(), NULL);
		if (!window) break;
		windows.push_back(window);
	}
	return windows;
}

// Return true if the window looks like a common file dialog or Office file dialog.
static bool IsFileDialog(HWND window) {
    if (window) {
        // Common file dialog.
		if (FindWindowEx(window, NULL, L"SHELLDLL_DefView", NULL) &&
			FindWindowEx(window, NULL, L"ComboBox", NULL)) return true;
        // Microsoft Office file dialog.
		if (FindWindowEx(window, NULL, L"Snake List", NULL) &&
			FindWindowEx(window, NULL, L"RichEdit20W", NULL)) return true;
    }
	return false;
}

// Choose parameters for our empowered file dialog (except the initial directory)
// based on information obtained by scanning the original file dialog.
static FILEDIALOGPARAMS FileDialogParams(HWND window, ActiveAccount& account, FILEDIALOGINFO* info) {
	FILEDIALOGPARAMS params;

    // Decide whether this an Open dialog or a Save dialog.
	params.open = !StartsWith(info->filetypelabel, L"Save");

	// Set the empowered dialog's owner window.
	params.owner = GetParent(window); // actually returns the parent OR the owner
	if (params.owner == GetAncestor(window, GA_PARENT)) params.owner = NULL;

    // Set the empowered dialog window's title to associate it with the pet.
	params.title = Trim(info->title ? info->title : L"");
	wstring prefix = wstring(L"\xab") + account.getPetname() + L"\xbb";
	wstring suffix = wstring(L" for ") + prefix;
	if (params.title.length() == 0) {
		params.title = wstring(params.open ? L"Open" : L"Save");
	}
	if (StartsWith(params.title, prefix)) {
		params.title = Trim(params.title.substr(prefix.length()));
	}
	if (!EndsWith(params.title, suffix)) {
		params.title += suffix;
	}
	
    // Set the empowered dialog's file name and file type list.
	params.pathname = info->path ? info->path : L"";
	params.filterindex = info->filetype;
	for (int i = 0; i < info->nfiletypes; i++) {
		wstring filetype(info->filetypes[i]);
		int left = filetype.find_last_of(L"(");
		int right = filetype.find_last_of(L")");
		if (left > 0 && right == filetype.length() - 1) {
			wstring globlist = filetype.substr(left + 1, right - (left + 1));
			vector<wstring> globs = Split(globlist, L";");
			for (vector<wstring>::iterator it = globs.begin(); it != globs.end(); it++) {
				*it = Trim(*it);
			}
			FILTER filter = {filetype, globs};
			params.filters.push_back(filter);
		}
	}

    params.flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    if (params.open) params.flags |= OFN_FILEMUSTEXIST;
    return params;
}

// Get the path of the file to save from the results of a file dialog,
// adding a file extension if necessary.
static wstring GetSavePath(FILEDIALOGPARAMS params, FILEDIALOGRESULTS results) {
	vector<wstring>& globs = 0 == results.filterindex
        ? params.customfilter.globs
        : params.filters[results.filterindex - 1].globs;

	// If the filename matches one of the globs, it doesn't need fixing.
	for (vector<wstring>::iterator it = globs.begin(); it != globs.end(); it++) {
		if (PathMatchSpec(results.pathname.c_str(), it->c_str())) return results.pathname;
	}

	// Otherwise, append the first literal extension among the globs.
	for (vector<wstring>::iterator it = globs.begin(); it != globs.end(); it++) {
		size_t lastdot = it->find_last_of(L".");
		if (lastdot != wstring.npos) {
			wstring extension = it->substr(lastdot);
			if (extension.find_first_of(L"*?") == wstring.npos) {
				return results.pathname + extension;
			}
		}
	}

	// Oh, well.
	return results.pathname;
}

// Ask the user whether it's okay to overwrite a file with a saved file.
static bool ConfirmOverwrite(wstring pathname, HWND owner) {
	WIN32_FIND_DATA found;
	if (FindFirstFile(pathname.c_str(), &found) == INVALID_HANDLE_VALUE) return true;
	wstring message = pathname + L" already exists.\nDo you want to replace it?";
	return MessageBoxW(owner, message.c_str(), L"Confirm Overwrite",
					   MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDYES;
}