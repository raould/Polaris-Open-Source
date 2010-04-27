// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once

#include "Request.h"

class FileRequest : public Request {
    std::wstring m_dir;
    std::wstring m_id;
    HANDLE m_out;

public:
	/** Constructs a FileRequest.
     * @param dir       The request directory.
     * @param id        The request identifier.
     * @param promised  The number of bytes promised.
	 */
	FileRequest::FileRequest(const std::wstring& dir,
						 	 const std::wstring& id,
  							 size_t promised);
	~FileRequest(void);

    /**
     * Complete the request.
     * @param pump      Should the Windows message queue be pumped while waiting for the response?
     * @param timeout   The maximum time in seconds to wait for a response.
     * @return The response.
     */
    virtual Message finish(bool pump, double timeout);
};