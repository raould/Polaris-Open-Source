// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "stdafx.h"
#include "Serializer.h"
#include <stdio.h>

Serializer::Serializer(size_t promised) {
	assert(sizeof(m_promised) == sizeof(void*));
	m_promised = promised;
	m_written = 0;
	m_capacity = 64;
	m_buffer = new BYTE[m_capacity];
}

Serializer::~Serializer(void) {
	delete[] m_buffer;
}

size_t Serializer::count() const {
    return m_written;
}

void Serializer::promise(const void* ptr, size_t size) {
    if (ptr) {
        send(&m_promised, sizeof m_promised);
        m_promised += size;
    } else {
        send(&ptr, sizeof ptr);
    }
}

void Serializer::promise(const char* ptr) {
    promise(ptr, ptr ? (strlen(ptr) + 1) * sizeof(char) : 0);
}

void Serializer::promise(const wchar_t* ptr) {
    promise(ptr, ptr ? (wcslen(ptr) + 1) * sizeof(wchar_t) : 0);
}

void Serializer::send(const void* data, size_t size) {
    if (data) {
        assert(m_written + size <= m_promised);
	    if (m_written + size > m_capacity) {
		    // Double the size of the memory buffer.
		    size_t new_capacity = m_capacity;
		    while (new_capacity < m_written + size) new_capacity *= 2;
		    BYTE* extended_buffer = new BYTE[new_capacity];
		    CopyMemory(extended_buffer, m_buffer, m_capacity);
		    delete[] m_buffer;
		    m_buffer = extended_buffer;
		    m_capacity = new_capacity;
	    }
	    // Append the given data to the buffer.
	    CopyMemory(m_buffer + m_written, data, size);
        m_written += size;
    }
}

void Serializer::send(const char* buffer) {
    send(buffer, buffer ? (strlen(buffer) + 1) * sizeof(char) : 0);
}

void Serializer::send(const wchar_t* buffer) {
    send(buffer, buffer ? (wcslen(buffer) + 1) * sizeof(wchar_t) : 0);
}

size_t Serializer::dump(BYTE* buffer, size_t size) {
	size_t copied = size < m_written ? size : m_written;
	CopyMemory(buffer, m_buffer, copied);
	return copied;
}