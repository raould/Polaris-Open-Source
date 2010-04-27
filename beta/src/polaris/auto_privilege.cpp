// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "stdafx.h"
#include "auto_privilege.h"

#include "Logger.h"

Logger privilege__Logger(L"auto_privilege");

HANDLE auto_privilege__open_current_process_token()
{
    HANDLE h = INVALID_HANDLE_VALUE;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &h)) {
        privilege__Logger.log(L"OpenProcessToken()", GetLastError());
	}
    return h;
}

auto_handle auto_privilege::CURRENT_PROCESS_TOKEN(auto_privilege__open_current_process_token());

bool auto_privilege__enable(HANDLE token, LPCTSTR pszPrivilege, BOOL enable)
{
	TOKEN_PRIVILEGES tp;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;
	if (!LookupPrivilegeValue(NULL, pszPrivilege, &tp.Privileges[0].Luid)) {
        privilege__Logger.log(L"LookupPrivilegeValue()", GetLastError());
        return false;
	}
	TOKEN_PRIVILEGES otp;
	DWORD cbold = sizeof otp;
	if (!AdjustTokenPrivileges(token, FALSE, &tp, cbold, &otp, &cbold)) {
        privilege__Logger.log(L"AdjustTokenPrivileges()", GetLastError());
        return false;
	}
	int error = GetLastError();
	if (ERROR_SUCCESS != error) {
        privilege__Logger.log(pszPrivilege, error);
		return false;
	}
	return true;
}

auto_privilege::auto_privilege(LPCTSTR name, HANDLE token) : m_name(name), m_token(token)
{
    auto_privilege__enable(m_token, m_name, TRUE);
}

auto_privilege::~auto_privilege()
{
    auto_privilege__enable(m_token, m_name, FALSE);
}
