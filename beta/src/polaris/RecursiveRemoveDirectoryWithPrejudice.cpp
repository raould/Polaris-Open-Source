// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "stdafx.h" 
#include "RecursiveRemoveDirectoryWithPrejudice.h"
#include "RegKey.h"
#include "Constants.h"
#include "StrongPassword.h"
#include "auto_handle.h"
#include "Logger.h"
#include <shellapi.h>

void unsetReadOnly(std::wstring absolute_filename) {
    // must remove readOnly attribute to delete
    DWORD attributes = GetFileAttributes(absolute_filename.c_str());
    // shut off file attribute read only, by first ensuring the flag
    // is turned on, then xoring the flag
    DWORD newAttrs = attributes | FILE_ATTRIBUTE_READONLY;
    newAttrs = attributes ^ FILE_ATTRIBUTE_READONLY;
    SetFileAttributes(absolute_filename.c_str(), newAttrs);
}

bool dirExists(std::wstring dirpath) {
    HANDLE dir = CreateFile(dirpath.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    bool answer =  dir != INVALID_HANDLE_VALUE;
    CloseHandle(dir);
    return answer;
}

int RecursiveRemoveDirectory(std::wstring dirpath) {
	// the shellapi doesn't work well enough in that, even with all the flags set to "don't
	// pop dialogs at the user no matter what happens", it still pops dialogs at the user
	// when deleting folders that Windows considers too important to let be not-deleted
	// without a warning that it is not being deleted
    // first try the winapi shellapi recursive folder delete to see if that works
    //SHFILEOPSTRUCT killer;
    //killer.hwnd = NULL;
    //killer.wFunc = FO_DELETE;
    //wchar_t doubleNulledString[1000] = {};
    //wcscpy(doubleNulledString, dirpath.c_str());
    //killer.pFrom = doubleNulledString;
    //killer.pTo = NULL;
    //killer.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT ;
    //killer.lpszProgressTitle = NULL;
    //SHFileOperation(&killer);
    //if (!dirExists(dirpath)) {return 0;}

    // here we go, the "simple" deletion didn't work
    //unsetReadOnly(dirpath);
	// simply unsetting the readonly attribute is not enough, apparently there 
	// is another attribute that will cause "access denied" if you try to delete,
	// unset all attributes so that delete can be reliable
	SetFileAttributes(dirpath.c_str(), FILE_ATTRIBUTE_NORMAL);
    // Delete directory contents.
    {
        if (dirpath[dirpath.length()-1] != '\\') {dirpath = dirpath + L"\\";}
        wchar_t DirSpec[MAX_PATH];
        wcsncpy(DirSpec, dirpath.c_str(), dirpath.length() + 1);
        wcsncat(DirSpec, L"*", wcslen(L"*") + 1);

        WIN32_FIND_DATA FindFileData = {};
        auto_close<HANDLE, &::FindClose> hFind(FindFirstFile(DirSpec, &FindFileData));
        if (hFind.get() != INVALID_HANDLE_VALUE) {
            // the first get always gets ".", so we are properly skipping that first get
            while (FindNextFile(hFind.get(), &FindFileData) != 0) {
                std::wstring absolute_filename = dirpath + FindFileData.cFileName;
                unsetReadOnly(absolute_filename);
                if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    if (std::wstring(L"..") == FindFileData.cFileName) {
                        // Skip it.
                    } else {
                        RecursiveRemoveDirectory(absolute_filename);
                    }
                } else {
                    if (!DeleteFile(absolute_filename.c_str())) {
                        Logger::basicLog(L"Delete File Failed " + absolute_filename);
                        printf("DeleteFile() failed: %d\n", GetLastError());
                        //break;
                    }
                }
            }
        }
    }

    int blah1 = RemoveDirectory(dirpath.c_str());
	int blah = GetLastError();
    // All kinds of problems may have arisen, the only way to know for sure
    // whether the directory is gone (remember, it may have already been deleted
    // earlier, so the remove directory may fail because it was removed earlier)
    // is to actually check if  directory exists
    HANDLE dir = CreateFile(dirpath.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (!dirExists(dirpath)) {
        return 0;
    } else {
        return -1;
    }
}

/**
 * returns zero if successful, -1 if fails, Constants::HorridDeleteError(ie, 10)
 * if the attempt is to destroy something that doesn't make sense
 **/
int RecursiveRemoveDirectoryWithPrejudice(std::wstring dirpath) {
	// Having once had a bug that caused the dirpath for extreme deletion to be "c:\documents and settings",
	// now make darn sure that we aren't deleting the universe, if we are,
	// log the disaster
	std::wstring badpaths[] = {L"C:",
		L"C:\\Documents and Settings", 
		L"C:\\Program Files", 
		L"C:\\WINDOWS"};
	for (int i = 0; i < 4; ++i) {
		if (dirpath == badpaths[i] || dirpath == (badpaths[i] + L"\\")) {
			Logger::basicLog(L"!!!recursive remove dir bad dir: " + dirpath);
			return (Constants::horridDeleteError());
		}
	}
	if (dirpath.find(L"pola") == std::wstring::npos) {
		Logger::basicLog(L"recursive delete dir trying to kill something inappropriate: " + dirpath);
		return Constants::horridDeleteError();
	}
    int status = RecursiveRemoveDirectory(dirpath);
    // TODO: exercise prejudice. Put a note in the registry to try again to kill the darn thing
    // the next time the powerbox starts up.
    if (status != 0) {
        RegKey cleanupKey = RegKey::HKCU.open(Constants::registryBootActions());
        RegKey deleteDirKey = cleanupKey.create(strongPassword());
        deleteDirKey.setValue(Constants::actionDeleteDir(), dirpath);
    }
    return status;
}