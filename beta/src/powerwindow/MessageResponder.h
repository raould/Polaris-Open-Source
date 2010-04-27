// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once

struct MessageResponder
{
	/**
	 * Respond to the message.
     *
     * The implementation will delete itself.
     *
	 * @param size      The number of bytes to be written.
	 * @param buffer    The buffer to write, owned by the caller.
	 */
	virtual void run(size_t size, const void* buffer) = 0;
};