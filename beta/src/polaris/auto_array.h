// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once

template<class T>
class auto_array
{
    T* m_v;

    // Forbidden operations.
	auto_array& operator=(const auto_array& x);
	auto_array(const auto_array& x);

public:
    auto_array() : m_v(0) {}
    auto_array(size_t n) : m_v(new T[n]) {}
    ~auto_array() { delete[] m_v; }

    T** operator&() { return &m_v; }
    T* get() { return m_v; }
};