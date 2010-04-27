// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "stdafx.h"
#include "inject.h"
#include <malloc.h>

typedef HMODULE (WINAPI *GetModuleHandleA_t)(LPCSTR);
typedef FARPROC (WINAPI *GetProcAddress_t)(HMODULE, LPCSTR);
typedef HMODULE (WINAPI *LoadLibraryA_t)(LPCSTR);
typedef BOOL (WINAPI *FreeLibrary_t)(HMODULE);
typedef BOOL (WINAPI *TerminateThread_t)(HANDLE, DWORD);
typedef HANDLE (WINAPI *GetCurrentThread_t)();

typedef struct {
	GetModuleHandleA_t GetModuleHandleA;
	GetProcAddress_t GetProcAddress;
	LoadLibraryA_t LoadLibraryA;
	FreeLibrary_t FreeLibrary;
	TerminateThread_t TerminateThread;
	GetCurrentThread_t GetCurrentThread;
	char injectdllpath[MAX_PATH];
	char injectfuncname[MAX_PATH];
	size_t bufsize;
	size_t outsize;
	DWORD error;
} INJECTDATA;

char injectdllpath[MAX_PATH]; // filled in by DllMain

// This function runs in a foreign process.  It loads this DLL into the process,
// finds the payload function, and calls it.  When this function wakes up in the
// foreign process, it has no data segment.  Any constants or function pointers
// it needs must be obtained from the INJECTDATA structure that is passed in.
static DWORD __stdcall InjectLoader(void* ptr) {
	INJECTDATA* data = (INJECTDATA*) ptr;
	HMODULE injectdll = data->LoadLibraryA(data->injectdllpath);
	INJECTEDFUNCTION function =
		(INJECTEDFUNCTION) data->GetProcAddress(injectdll, data->injectfuncname);

	BYTE* buffer = (BYTE*) (data + 1); // end of the INJECTDATA structure
	if (function) data->outsize = function(data->bufsize ? buffer : NULL, data->bufsize);
	
	if (!injectdll) data->error = ERROR_FILE_NOT_FOUND;
	else if (!function) data->error = ERROR_INVALID_FUNCTION;
	else data->error = ERROR_SUCCESS;
	if (injectdll) data->FreeLibrary(injectdll);

	// For some reason, if we just return here, occasionally the thread gets
	// stuck and never returns.  Explicitly terminating the thread at this
	// point seems to make injected calls finish much more reliably.
	return data->TerminateThread(data->GetCurrentThread(), 0);
}

// This function must be declared immediately after InjectLoader so we can use
// its address to determine the size of InjectLoader's code.
static void InjectLoaderEnd() { }

// Use InjectAndCall to inject and call a function in another process.
// The timeout is specified in milliseconds or it can be the constant INFINITE.
// To pass data to and from the function, provide a pointer to a data buffer in
// the 'buffer' parameter and give the size of the buffer in 'bufsize'.  The data
// in the buffer is initially provided as input to the function; the function
// can return output data by overwriting the buffer contents.  The caller is
// responsible for providing a buffer large enough to hold whatever output the
// injected function wants to return.  If the 'outsize' pointer is not NULL,
// '*outsize' will receive the amount of output data that the injected function
// claims to have written into the buffer.
//
// The function being injected must match the type INJECTEDFUNCTION, which has
// signature size_t func(BYTE* buffer, size_t bufsize).  The 'buffer' parameter
// will point to the buffer containing the input, or it will be NULL if nothing
// was passed in.  The 'bufsize' parameter gives the capacity of the buffer; if
// the function wants to return data, it should overwrite the buffer with its
// output data and return a size_t to indicate the size of the returned data.
DWORD InjectAndCall(HANDLE process, const char* funcname, DWORD timeout,
					BYTE* buffer, size_t bufsize, size_t* outsize) {
	DWORD error = ERROR_SUCCESS;
#define FAIL { error = GetLastError(); goto cleanup; }

	void* remotecode = NULL;
	void* remotedata = NULL;
	HANDLE thread = NULL;
	if (buffer == NULL) bufsize = 0;
	if (bufsize == 0) buffer = NULL;
	if (outsize) *outsize = 0;

	// Prepare all the data that the InjectLoader will need.
	INJECTDATA data;
	HMODULE kernel = LoadLibraryA("kernel32.dll");
	data.GetModuleHandleA = (GetModuleHandleA_t) GetProcAddress(kernel, "GetModuleHandleA");
	data.GetProcAddress = (GetProcAddress_t) GetProcAddress(kernel, "GetProcAddress");
	data.LoadLibraryA = (LoadLibraryA_t) GetProcAddress(kernel, "LoadLibraryA");
	data.FreeLibrary = (FreeLibrary_t) GetProcAddress(kernel, "FreeLibrary");
	data.TerminateThread = (TerminateThread_t) GetProcAddress(kernel, "TerminateThread");
	data.GetCurrentThread = (GetCurrentThread_t) GetProcAddress(kernel, "GetCurrentThread");
	strcpy(data.injectdllpath, injectdllpath);
	strcpy(data.injectfuncname, funcname);
	data.bufsize = bufsize;
	data.outsize = 0;
	data.error = 0;

	// Transfer the loader code, data, and input buffer if any.
	size_t codesize = (char*) InjectLoaderEnd - (char*) InjectLoader;
	remotecode = VirtualAllocEx(process, NULL, codesize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	remotedata = VirtualAllocEx(process, NULL, sizeof(data) + bufsize, MEM_COMMIT, PAGE_READWRITE);
	void* remotebuffer = (INJECTDATA*) remotedata + 1;
	if (!remotecode || !remotedata) FAIL;
	if (!WriteProcessMemory(process, remotecode, InjectLoader, codesize, NULL)) FAIL;
	if (!WriteProcessMemory(process, remotedata, &data, sizeof(data), NULL)) FAIL;
	if (buffer && !WriteProcessMemory(process, remotebuffer, buffer, bufsize, NULL)) FAIL;

	// Launch the loader in the remote process.
	thread = CreateRemoteThread(process, NULL, 0, (LPTHREAD_START_ROUTINE) remotecode, remotedata, 0, NULL);
	if (!thread) FAIL;
	switch (WaitForSingleObject(thread, timeout)) {
		case WAIT_FAILED:
			FAIL;
		case WAIT_TIMEOUT:
			TerminateThread(thread, FALSE);
			FAIL;
	}

	// Retrieve the result.
	if (!ReadProcessMemory(process, remotedata, &data, sizeof(data), NULL)) FAIL;
	if (buffer && !ReadProcessMemory(process, remotebuffer, buffer, bufsize, NULL)) FAIL;
	if (outsize) *outsize = data.outsize;
	error = data.error;

cleanup:
	if (thread) CloseHandle(thread);
	if (remotecode) VirtualFreeEx(process, remotecode, 0, MEM_RELEASE);
	if (remotedata) VirtualFreeEx(process, remotedata, 0, MEM_RELEASE);
	return error;
}

// When this DLL is loaded, save the full path to this DLL in a global variable.
BOOL WINAPI DllMain(HMODULE module, DWORD reason, void* reserved) {
	if (reason == DLL_PROCESS_ATTACH) {
		GetModuleFileNameA(module, injectdllpath, MAX_PATH - 2);
	}
    return TRUE;
}
