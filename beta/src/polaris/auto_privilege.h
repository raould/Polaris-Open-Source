// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once

#include "auto_handle.h"

/**
 * Automated enabling/disabling of privileges.
 */
class auto_privilege
{
    static auto_handle CURRENT_PROCESS_TOKEN;

    LPCTSTR m_name;
    HANDLE m_token;

    // Forbidden operations.
	auto_privilege& operator=(const auto_privilege& x);
	auto_privilege(const auto_privilege& x);

public:

    /**
     * Enable the named privilege.
     * @param name  The privilege name.
     * @param token The process token.
     */
    auto_privilege(LPCTSTR name, HANDLE token = CURRENT_PROCESS_TOKEN.get());

    /**
     * Disable the privilege.
     */
    ~auto_privilege();
};