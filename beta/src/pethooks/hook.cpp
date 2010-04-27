// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "stdafx.h"

#include <stdio.h>
#include <search.h>
#include <psapi.h>
#include "hook.h"

typedef FARPROC (WINAPI *GetProcAddress_t)(HMODULE, LPCSTR);
typedef HMODULE (WINAPI *LoadLibraryExA_t)(LPCSTR, HANDLE, DWORD);
typedef HMODULE (WINAPI *LoadLibraryExW_t)(LPCWSTR, HANDLE, DWORD);

typedef struct {
	char dllname[MAX_PATH];		// name of the DLL containing the function to replace
	char funcname[MAX_PATH];	// name of the function being replaced
	void* oldfunc;				// original function pointer will be stored here
	void* newfunc;				// pointer to the replacement function
} HOOKENTRY;

static int nhooks = 0, capacity = 0;
static HOOKENTRY* hooktable = NULL;

// Add an entry to the hook table, expanding the table as necessary.
void HookAddHook(const char* dllname, const char* funcname, void* func) {
	if (!hooktable) {
		capacity = 64;
		hooktable = (HOOKENTRY*) HeapAlloc(
			GetProcessHeap(), 0, capacity*sizeof(HOOKENTRY));
	}
	if (nhooks == capacity) {
		capacity *= 2;
		hooktable = (HOOKENTRY*) HeapReAlloc(
			GetProcessHeap(), 0, hooktable, capacity*sizeof(HOOKENTRY));
	}
	strcpy(hooktable[nhooks].dllname, dllname);
	strcpy(hooktable[nhooks].funcname, funcname);
	hooktable[nhooks].oldfunc = GetProcAddress(LoadLibraryA(dllname), funcname);
	hooktable[nhooks].newfunc = func;
	nhooks++;
}

// Find an original function pointer saved in the hook table.
void* HookGetOriginal(const char* funcname) {
	for (int i = 0; i < nhooks; i++) {
		if (strcmp(hooktable[i].funcname, funcname) == 0) {
			return hooktable[i].oldfunc;
		}
	}
	return NULL;
}

// Store a pointer at a given destination address.
static BOOL WritePointer(void** destination, void* value) {
	// If the destination is writable, just write there.
	if (!IsBadWritePtr(destination, sizeof(void*))) {
		*destination = value;
		return TRUE;
	}

	// Otherwise, try to make the destination writable, then do the write.
	DWORD oldaccess, newaccess = PAGE_EXECUTE_READWRITE;
	if (VirtualProtect(destination, sizeof(void*), newaccess, &oldaccess)) {
		*destination = value;
		VirtualProtect(destination, sizeof(void*), oldaccess, &newaccess);
		return TRUE;
	}
	return FALSE;
}

// Declare a pointer of a given type that points at a given relative virtual address.
#define GETPOINTER(type, var, base, offset) \
	type* var = (type*) ((DWORD) (base) + (DWORD) (offset))

// Edit a single module's import table so that wherever it would have called the
// imported function pointer 'oldfunc', it now calls 'newfunc'.  (When a module calls
// a function in another module, it uses an indirect JMP instruction that looks up
// the target address in the calling module's import table.  Modifying an entry in
// the import table changes the target address.  For a good description of the data
// structures involved, see "Peering Inside the PE" by Matt Pietrek on MSDN.)
static BOOL HookFunction(HMODULE module, void* oldfunc, void* newfunc) {
	// Make sure we can read the module header.
	if (IsBadReadPtr(module, sizeof(PIMAGE_NT_HEADERS))) return FALSE;

	// Navigate to the module's import table.
	GETPOINTER(IMAGE_DOS_HEADER, dosheader, module, 0);
	if (dosheader->e_magic != IMAGE_DOS_SIGNATURE) return FALSE;

	GETPOINTER(IMAGE_NT_HEADERS, ntheader, module, dosheader->e_lfanew);
	if (ntheader->Signature != IMAGE_NT_SIGNATURE) return FALSE;

	IMAGE_DATA_DIRECTORY* directory = ntheader->OptionalHeader.DataDirectory;
	DWORD offset = directory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
	GETPOINTER(IMAGE_IMPORT_DESCRIPTOR, importdesc, module, offset);
	if (importdesc == (IMAGE_IMPORT_DESCRIPTOR*) ntheader) return FALSE;

	// Scan the import table and replace all occurrences of the given function pointer.
	// The table contains a list of import descriptors, one for each imported module,
	// and each descriptor points to a list of thunks, one for each imported function.
	BOOL success = FALSE;
	for (; importdesc->Name; importdesc++) {
		GETPOINTER(char, name, module, importdesc->Name);
		GETPOINTER(IMAGE_THUNK_DATA, thunk, module, importdesc->FirstThunk);
		for (; thunk->u1.Function; thunk++) {
			void** funcptr = (void**) &thunk->u1.Function;
			if (*funcptr == oldfunc) {
				success |= WritePointer(funcptr, newfunc);
			}
		}
	}

	return success;
}

// This routine ensures that when a function is called by name,
// the retrieved pointer is replaced with the new function pointer.
FARPROC WINAPI HookGetProcAddress(HMODULE module, LPCSTR funcname) {
	FARPROC addr = ORIGINAL(GetProcAddress)(module, funcname);
	for (int i = 0; i < nhooks; i++) {
        if (addr == hooktable[i].oldfunc) {
            return (FARPROC) hooktable[i].newfunc;
        }
	}
	return addr;
}

// Add a hook to intercept API calls by name.
void HookAddGetProcAddressHook() {
	HookAddHook("kernel32", "GetProcAddress", HookGetProcAddress);
}

// Install all the hooks into a given module.
void HookModule(HMODULE module) {
	for (int i = 0; i < nhooks; i++) {
        HookFunction(module, hooktable[i].oldfunc, hooktable[i].newfunc);
	}
}

static int __cdecl ComparePointers(const void* a, const void* b) {
	return *((HMODULE*) a) - *((HMODULE*) b);
}

// Apply hooks to all the modules in this process that we haven't hooked yet.
void HookAllLoadedModules() {
	static HMODULE hooked[MAX_MODULES]; // sorted list of all hooked modules
	static int nhooked = 0;

	// Get a list of all the modules in the current process.
	HANDLE process = GetCurrentProcess();
	HMODULE modules[MAX_MODULES];
	DWORD length = 0;
	if (!EnumProcessModules(process, modules, sizeof(modules), &length)) return;
	int nmodules = (int) (length / sizeof(HMODULE));

	// Include the main executable of the process.
	modules[nmodules++] = GetModuleHandle(NULL);

	// Find out what module we're running in right now so we can avoid hooking it.
	HMODULE thismodule = NULL;
	GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
		(char*) HookAllLoadedModules, &thismodule);

	// Check each module against our list of previously hooked modules.
	for (int i = 0; i < nmodules; i++) {
		if (modules[i] != thismodule &&
			!bsearch(&(modules[i]), hooked, nhooked, sizeof(HMODULE), ComparePointers)) {
			// We've found a new one.  Apply hooks and add it to our list.
			HookModule(modules[i]);
			hooked[nhooked++] = modules[i];
			qsort(hooked, nhooked, sizeof(HMODULE), ComparePointers);
		}
	}
}

// These routines ensure that when new modules are loaded, their
// import tables are also examined and intercepted as necessary.
HMODULE WINAPI HookLoadLibraryExW(LPCWSTR filename, HANDLE handle, DWORD flags) {
	bool newload = (GetModuleHandleW(filename) == NULL);
	HMODULE result = ORIGINAL(LoadLibraryExW)(filename, handle, flags);
	if (newload && result) HookAllLoadedModules();
	return result;
}

HMODULE WINAPI HookLoadLibraryExA(LPCSTR filename, HANDLE handle, DWORD flags) {
	bool newload = (GetModuleHandleA(filename) == NULL);
	HMODULE result = ORIGINAL(LoadLibraryExA)(filename, handle, flags);
	if (newload && result) HookAllLoadedModules();
	return result;
}

HMODULE WINAPI HookLoadLibraryW(LPCWSTR filename) {
	return HookLoadLibraryExW(filename, NULL, 0);
}

HMODULE WINAPI HookLoadLibraryA(LPCSTR filename) {
	return HookLoadLibraryExA(filename, NULL, 0);
}

// Apply hooks to all modules, also adding hooks to catch future loaded modules.
void HookAllLoadedAndFutureModules() {
	HookAddHook("kernel32", "LoadLibraryA", HookLoadLibraryA);
	HookAddHook("kernel32", "LoadLibraryW", HookLoadLibraryW);
	HookAddHook("kernel32", "LoadLibraryExA", HookLoadLibraryExA);
	HookAddHook("kernel32", "LoadLibraryExW", HookLoadLibraryExW);
	HookAllLoadedModules();
}