// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once

#include "Serializer.h"
#include "Message.h"

class Request : public Serializer {
public:
	Request(size_t promised) : Serializer(promised) { }
	virtual ~Request() { }

    /**
     * Complete the request.
     * @param pump      Should the Windows message queue be pumped while waiting for the response?
     * @param timeout   The maximum time in seconds to wait for a response.
     * @return The response.
     */
    virtual Message finish(bool pump, double timeout) = 0;
};