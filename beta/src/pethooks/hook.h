// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

// hook.cpp and hook.h are all you need to do API interception in four easy steps:
//
// 1.  Define a replacement function and declare it with WINAPI.  If you need to
//     call the original "Foo" function, say ORIGINAL(Foo)(args...) and make sure
//     you have a type declaration for "Foo_t", also with WINAPI.
//
// 2.  In your program, add the hook to the internal table by calling HookAddHook(...).
//     Do this for each function you want to intercept.
//
// 3.  If you also want to intercept retrieval of function addresses by GetProcAddress,
//     call HookInterceptGetProcAddress().
//
// 4.  Apply the hooks by calling HookModule (to intercept calls originating from just
//     a single module), HookAllLoadedModules (to intercept calls from all currently
//     loaded modules), or HookAllLoadedAndFutureModules.
//
// Note that to compile hook.cpp you will need to link in psapi.lib.

#define MAX_MODULES 1000

#define ORIGINAL(func) ((func##_t) HookGetOriginal(#func))
void* HookGetOriginal(const char* funcname);

void HookAddHook(const char* dllname, const char* funcname, void* newfunc);
void HookAddGetProcAddressHook();
void HookModule(HMODULE module);
void HookAllLoadedModules();
void HookAllLoadedAndFutureModules();