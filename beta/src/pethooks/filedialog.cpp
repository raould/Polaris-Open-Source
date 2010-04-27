// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "stdafx.h"
#include "filedialog.h"
#include <winable.h>
#include <stdio.h>
#include "../polaris/Message.h"
#include "../polaris/Serializer.h"
#include "../polaris/AccessibleElement.h"

// Walk the tree of UI elements in a file dialog to find the filename field
// and file type list.  This routine is tuned to quickly find the right controls
// in both common and Office file dialogs without walking the entire tree.
static bool FindElements(AccessibleElement element,
						 AccessibleElement** filename,
						 AccessibleElement** typelist) {
	for (AccessibleChildIterator children = element.GetChildren(); children.HasNext();) {
		AccessibleElement child = children.GetNext();
		WCHAR* name = child.GetName();
        switch (child.GetRole()) {
            case ROLE_SYSTEM_LIST:
				if (name && EndsWith(name, L"type:")) *typelist = new AccessibleElement(child);
                break;
            case ROLE_SYSTEM_TEXT:
                if (name && EndsWith(name, L"name:")) *filename = new AccessibleElement(child);
                break;
            case ROLE_SYSTEM_COMBOBOX:
                if (name && EndsWith(name, L"type:")) {
					for (AccessibleChildIterator items = child.GetChildren(); items.HasNext();) {
						AccessibleElement item = items.GetNext();
						if (item.GetRole() == ROLE_SYSTEM_LISTITEM) *typelist = new AccessibleElement(child);
                        break;
                    }
                }
                if (name) FindElements(child, filename, typelist);
                break;
            case ROLE_SYSTEM_CLIENT:
            case ROLE_SYSTEM_DIALOG:
            case ROLE_SYSTEM_WINDOW:
                if (name) FindElements(child, filename, typelist);
                break;
        }
        if (*filename && *typelist) return true;
    }
    return false;
}

// Get the 0-based index of the selected item in a list control, or -1 if not found.
static int GetSelection(AccessibleElement list) {
	int count = list.CountChildren();
	AccessibleChildIterator children = list.GetChildren();
	for (int i = 0; i < count; i++) {
		AccessibleElement item = children.GetNext();
		// In common file dialogs, the selected item has the STATE_SYSTEM_SELECTED flag.
		// In Office file dialogs, the selected item has the STATE_SYSTEM_HOTTRACKED flag.
        if (item.GetState() & (STATE_SYSTEM_SELECTED | STATE_SYSTEM_HOTTRACKED)) return i;
	}
	return -1;
}

// Select an item in a list control, given the 0-based index of the item.
static void SetSelection(AccessibleElement list, int index) {
	int count = list.CountChildren();
	AccessibleChildIterator children = list.GetChildren();
	for (int i = 0; i < count; i++) {
		AccessibleElement item = children.GetNext();
		if (i == index) item.Select();
	}
}

// Send a single keypress (press and release the specified virtual key).
static void SendKey(WORD virtkey) {
	INPUT inputs[2];
	inputs[0].type = INPUT_KEYBOARD;
	inputs[0].ki.wVk = virtkey;
	inputs[0].ki.wScan = 0;
	inputs[0].ki.dwFlags = 0;
	inputs[0].ki.time = 0;
	inputs[0].ki.dwExtraInfo = 0;
	inputs[1] = inputs[0];
	inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
	SendInput(2, inputs, sizeof(*inputs));
	WaitForInputIdle(GetCurrentProcess(), INFINITE);
}

// This is an injected function; see InjectAndCall for calling conventions.
// Input: the HWND of a file dialog.  Output: a FILEDIALOGINFO structure
// followed by an array of 'nfiletypes' WCHAR pointers.
EXPORT size_t __cdecl FileDialogInfo(BYTE* buffer, size_t bufsize) {
	Message request(buffer, bufsize);
	HWND* window = NULL;
	request.cast(&window, sizeof(*window));

	// Find the relevant elements of the file dialog.
	AccessibleElement dialog(*window);
	AccessibleElement* filename = NULL;
	AccessibleElement* typelist = NULL;
	if (!FindElements(dialog, &filename, &typelist)) return 0;

	// Allocate an extended FILETYPEINFO structure of the right size.
	int nfiletypes = typelist->CountChildren();
	size_t size = sizeof(FILEDIALOGINFO) + sizeof(WCHAR*) * nfiletypes;
	FILEDIALOGINFO* info = (FILEDIALOGINFO*) malloc(size);

	// Fill in the contents of the FILEDIALOGINFO structure.
	WCHAR title[1000];
	GetWindowTextW(*window, title, 1000);
	info->title = title;
	info->path = filename->GetValue();
	info->filetypelabel = typelist->GetName();
	info->filetype = GetSelection(*typelist) + 1;
	info->nfiletypes = nfiletypes;
	AccessibleChildIterator children = typelist->GetChildren();
	for (int i = 0; i < nfiletypes; i++) {
		AccessibleElement child = children.GetNext();
		info->filetypes[i] = child.GetName();
	}

	// Serialize the FILEDIALOGINFO structure to form the reply.
	Serializer reply(size);
	reply.promise(info->title);
	reply.promise(info->path);
	reply.promise(info->filetypelabel);
	reply.send(&(info->filetype), sizeof(info->filetype));
	reply.send(&(info->nfiletypes), sizeof(info->nfiletypes));
	for (int i = 0; i < nfiletypes; i++) reply.promise(info->filetypes[i]);
	reply.send(info->title);
	reply.send(info->path);
	reply.send(info->filetypelabel);
	for (int i = 0; i < nfiletypes; i++) reply.send(info->filetypes[i]);
	free(info);

	delete filename;
	delete typelist;
	return reply.dump(buffer, bufsize);
}

// This is an injected function; see InjectAndCall for calling conventions.
// Input: a FILEDIALOGSUBMIT structure.  Output: nothing.
EXPORT size_t __cdecl FileDialogSubmit(BYTE* buffer, size_t bufsize) {
	Message request(buffer, bufsize);
	FILEDIALOGSUBMIT* submit;
	request.cast(&submit, sizeof(*submit));
	request.fix(&(submit->path));

	// Find the relevant elements of the file dialog.
	AccessibleElement dialog(submit->window);
	AccessibleElement* filename = NULL;
	AccessibleElement* typelist = NULL;
	if (!FindElements(dialog, &filename, &typelist)) return 0;

	// Prevent other mouse or keyboard input from interfering with our
	// sequence of synthesized keypresses.
	BlockInput(true);

	// Bring up the window somewhere inconspicuous.
    //SetWindowPos(submit->window, NULL, 5000, 5000, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	ShowWindow(submit->window, SW_SHOW);
	SetForegroundWindow(submit->window);

	// Only Office file dialogs contain a "Snake List" control.
	if (FindWindowEx(submit->window, NULL, "Snake List", NULL)) {
		// In Office file dialogs, the filename field has control ID 54.
		HWND textfield = GetDlgItem(submit->window, 54);

        // For Office file dialogs, we have to set the file type first.  If we
        // set the file name first, Office will fix its extension to match the
        // currently selected file type, which may not be the desired file type.
		// To make the file type setting take effect, Return must be pressed on
		// the file type list.
        typelist->Focus();
		SendKey(VK_DOWN);
        SetSelection(*typelist, submit->filetype - 1);
		SendKey(VK_RETURN);

		// Now put the path into the filename field.  Its extension will be
		// automatically fixed up when the dialog is submitted.
		SetWindowTextW(textfield, submit->path);
		if (!submit->cancel) {
			// The text field's contents have to be changed by a keypress
			// in order to take effect.
	        filename->Focus();
			SendKey(VK_END);
			SendKey(VK_SPACE);
		}
    } else {
    	// In common file dialogs, the filename field has control ID 1148.
		HWND textfield = GetDlgItem(submit->window, 1148);

	    // For common file dialogs, we have to set the file name first, so that
        // setting the file type can fix up the file name extension as necessary.
		SetWindowTextW(textfield, submit->path);
    
        // To make the file type setting take effect, Return has to be pressed
        // on the file type list, and it must cause the selection to change.
        SetFocus(textfield);
		SendKey(VK_TAB);
		SendKey(VK_DOWN);
        SetSelection(*typelist, submit->filetype - 1);
        SendKey(VK_RETURN);
    }

	// Submit or cancel the dialog.
	SendKey(submit->cancel ? VK_ESCAPE : VK_RETURN);
	BlockInput(false);
	return 0;
}