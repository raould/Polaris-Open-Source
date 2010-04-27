#include "stdafx.h"
#include "Request.h"

Request::Request(size_t promised) {
	assert(sizeof m_promised == sizeof(void*));
	m_promised = promised;
	m_written = 0;
}

Request::~Request() {
}

size_t Request::count() const {
    return m_written;
}

void Request::promise(const void* ptr, size_t size) {
    if (ptr) {
        send(&m_promised, sizeof m_promised);
        m_promised += size;
    } else {
        send(&ptr, sizeof ptr);
    }
}

void Request::promise(const char* ptr) {
    promise(ptr, ptr ? (strlen(ptr) + 1) * sizeof(char) : 0);
}

void Request::promise(const wchar_t* ptr) {
    promise(ptr, ptr ? (wcslen(ptr) + 1) * sizeof(wchar_t) : 0);
}

void Request::send(const char* buffer) {
    send(buffer, buffer ? (strlen(buffer) + 1) * sizeof(char) : 0);
}

void Request::send(const wchar_t* buffer) {
    send(buffer, buffer ? (wcslen(buffer) + 1) * sizeof(wchar_t) : 0);
}