// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "stdafx.h"
#include "InjectRequest.h"
#include "../pethooks/inject.h"

InjectRequest::InjectRequest(HANDLE process, char* funcname,
							 size_t promised, size_t maxreply) : Request(promised) {
	HANDLE p = GetCurrentProcess();
	DuplicateHandle(p, process, p, &m_process, 0, FALSE, DUPLICATE_SAME_ACCESS);
	m_funcname = strcpy(new char[strlen(funcname) + 1], funcname);
	m_maxreply = maxreply;
}

InjectRequest::~InjectRequest(void) {
	CloseHandle(m_process);
	delete[] m_funcname;
}

Message InjectRequest::finish(bool pump, double timeout) {
    assert(m_written == m_promised);

	// Execute the request by injecting a thread into a remote process.
	size_t bufsize = m_maxreply > m_written ? m_maxreply : m_written;
	BYTE* buffer = new BYTE[bufsize];
	CopyMemory(buffer, m_buffer, m_written);
	size_t outsize = 0;
	InjectAndCall(m_process, m_funcname, (DWORD) (timeout*1000),
		          buffer, bufsize, &outsize);
	Message reply(buffer, outsize);
	delete[] buffer;
	return reply;
}