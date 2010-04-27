// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once
#include "Request.h"

class InjectRequest : public Request {
	// Works just like a Request object, but serializes the request to a
	// memory buffer instead of a file on disk, then makes the request by
	// injecting a thread into another process.
	HANDLE m_process;
	char* m_funcname;
	size_t m_capacity;
	size_t m_maxreply;

public:
	InjectRequest(HANDLE process, char* funcname, size_t promised, size_t maxreply);
	~InjectRequest(void);
	
    /**
     * Complete the request.
     * @param pump      Should the Windows message queue be pumped while waiting for the response?
     * @param timeout   The maximum time in seconds to wait for a response.
     * @return The response.
     */
    virtual Message finish(bool pump, double timeout);
};
