// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

// inject.cpp and inject.h are all you need to do code injection in three easy steps:
//
// 1.  Define the function you want to inject.  Make sure the function is declared:
//     EXPORT void __cdecl FunctionName();
//
// 2.  Put the code in a file to be compiled within this DLL.
//
// 3.  Have your main program link in this DLL.  When you want to inject and run the
//     function in another process, open the process with PROCESS_ALL_ACCESS and call
//     InjectAndCall(process, "FunctionName", timeout).  The timeout is a number of
//     milliseconds for the function to complete, or it can be set to INFINITE.

#ifdef PETHOOKS_EXPORTS
#define EXPORT extern "C" __declspec(dllexport)
#else
#define EXPORT extern "C" __declspec(dllimport)
#endif

EXPORT DWORD InjectAndCall(HANDLE process, const char* funcname, DWORD timeout,
						   BYTE* buffer, size_t size, size_t* outsize);
EXPORT char injectdllpath[MAX_PATH];

typedef size_t (__cdecl *INJECTEDFUNCTION)(BYTE*, size_t);