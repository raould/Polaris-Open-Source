// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "stdafx.h"
#include "auto_impersonation.h"

auto_impersonation::auto_impersonation(HANDLE session)
{
    if (!ImpersonateLoggedOnUser(session)) {
        printf("ImpersonateLoggedOnUser() failed: %d\n", GetLastError());
    }
}

auto_impersonation::~auto_impersonation()
{
    if (!RevertToSelf()) {
        printf("RevertToSelf() failed: %d\n", GetLastError());
    }
}