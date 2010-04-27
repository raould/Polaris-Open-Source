// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once

/**
 * Facilitates use of a Win32 handle.
 */
template<class H, BOOL (WINAPI *close)(H)>
class auto_close
{
    H m_handle;

    // Forbidden operations.
    typedef auto_close<H, close> TYPE;
	TYPE& operator=(const TYPE& x);
	auto_close(const TYPE& x);

public:
    /**
     * Construct an auto_close from a Win32 function return.
     * @param handle    The raw handle.
     */
    auto_close(H handle) : m_handle(handle) {}

    /**
     * Close the underlying raw handle.
     */
    ~auto_close() { (*close)(m_handle); }

    /**
     * Receive a raw handle from a Win32 call that returns via a pointer to a handle.
     */
    H* operator&() { return &m_handle; }

    /**
     * Get the raw handle.
     */
    H get() { return m_handle; }
};

typedef auto_close<HANDLE, &::CloseHandle> auto_handle;
