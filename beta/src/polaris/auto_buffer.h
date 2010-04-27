// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once

template<class T>
class auto_buffer
{
    T m_buffer;

    void reset(T buffer) {
        if (buffer != m_buffer) {
            HeapFree(GetProcessHeap(), 0, m_buffer);
            m_buffer = buffer;
        }
    }

public:

    /**
     * Construct a NULL pointer.
     */
    auto_buffer() : m_buffer(NULL) {}

    /**
     * Construct an auto_buffer.
     * @param size  The number of bytes to allocate.
     */
    auto_buffer(SIZE_T size) : m_buffer((T)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size)) {}

    /**
     * Claim the buffer from another auto_buffer.
     */
    auto_buffer(auto_buffer<T>& x) : m_buffer(x.release()) {}

    /**
     * Free the underlying buffer.
     */
    ~auto_buffer() { reset(NULL); }

    /**
     * Claim the buffer from another auto_buffer.
     */
    auto_buffer<T>& operator=(auto_buffer<T>& x) {
        reset(x.release());
        return *this;
    }

    /**
     * Pass an auto_buffer as if it were a raw buffer.
     */
    T get() { return m_buffer; }

    /**
     * Smart pointer syntax sugar.
     */
    T operator->() { return m_buffer; }

    /**
     * Prevent the buffer from being freed.
     * @return The raw buffer.
     */
    T release() {
        T tmp = m_buffer;
        m_buffer = NULL;
        return tmp; 
    }
};