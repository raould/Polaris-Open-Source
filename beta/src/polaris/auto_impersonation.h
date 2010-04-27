// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once

class auto_impersonation
{
    // Forbidden operations.
	auto_impersonation& operator=(const auto_impersonation& x);
	auto_impersonation(const auto_impersonation& x);

public:
    auto_impersonation(HANDLE session);
    ~auto_impersonation();
};