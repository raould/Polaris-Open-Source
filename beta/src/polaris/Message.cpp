// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "stdafx.h"
#include "Message.h"

Message::Message() {
	assert(sizeof(size_t) == sizeof(void*));
	m_owner = false;
	m_buffer = NULL;
	m_min = 0;
	m_max = 0;
	m_fixed = 0;
}

Message::Message(BYTE* buffer, size_t min, size_t max) {
	// Point to the data within the provided buffer, and take responsibility for
	// deleting the provided buffer with delete[].
	assert(sizeof(size_t) == sizeof(void*));
	m_owner = true;
	m_buffer = buffer;
	m_min = min;
	m_max = max;
	m_fixed = min;
}

Message::Message(const BYTE* buffer, size_t size) {
	// Allocate a new buffer to contain a copy of the provided data.
    assert(sizeof(size_t) == sizeof(void*));
	m_owner = true;
	m_buffer = new BYTE[size];
	CopyMemory(m_buffer, buffer, size);
	m_min = 0;
	m_max = size;
	m_fixed = 0;
}

Message::Message(Message& x) {
	// Acquire ownership of the buffer in another message.
	assert(sizeof(size_t) == sizeof(void*));
	m_owner = x.m_owner;
	m_buffer = x.m_buffer;
	m_min = x.m_min;
	m_max = x.m_max;
	m_fixed = x.m_fixed;
	x.m_owner = false;
}

Message::~Message() {
    release();
}

Message& Message::operator=(Message& x) {
	// Acquire ownership of the buffer in another message.
    if (this != &x) {
        release();
        m_owner = x.m_owner;
        m_buffer = x.m_buffer;
        m_min = x.m_min;
        m_max = x.m_max;
        m_fixed = x.m_fixed;
        x.m_owner = false;
    }
    return *this;
}

void Message::release() {
    if (m_owner) {
        delete[] m_buffer;
    }
}

size_t Message::length() const {
    return m_max - m_min;
}

void Message::cast(void* slot, size_t size) {
    if (m_min + size > m_max) {
        *(void**)slot = NULL;
    } else {
        *(void**)slot = m_buffer + m_min;
        m_fixed = m_min + size;
    }
}

void Message::cast(char** slot) {
    cast(slot, sizeof(char));
}

void Message::cast(wchar_t** slot) {
    cast(slot, sizeof(wchar_t));
}

void Message::fix(void* slot, size_t size) {
    assert(m_owner);

    size_t offset = *(size_t*)slot;
    if (0 == offset) {
        // Sender specified NULL pointer.
    } else if (offset > m_max - size || offset < m_fixed) {
        // Sender specified invalid pointer.
        *(void**)slot = NULL;
    } else {
        // Translate sender specified pointer to receiver's address space.
        *(void**)slot = m_buffer + offset;
        m_fixed = offset + size;
    }
}

void Message::fix(const char** slot) {
    fix(slot, sizeof(char));
}

void Message::fix(char** slot) {
    fix(slot, sizeof(char));
}

void Message::fix(const wchar_t** slot) {
    fix(slot, sizeof(wchar_t));
}

void Message::fix(wchar_t** slot) {
    fix(slot, sizeof(wchar_t));
}

void Message::serialize(void* slot) {
    assert(slot >= m_buffer + m_min && slot < m_buffer + m_max - sizeof(void*));

    BYTE* ptr = *(BYTE**)slot;
    if (ptr) {
        assert(ptr >= m_buffer + m_min && ptr < m_buffer + m_max);
        *(size_t*)slot = ptr - (m_buffer + m_min);
    }
}
