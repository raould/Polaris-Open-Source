// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once

class Message;
struct MessageResponder;

struct MessageHandler
{
	virtual ~MessageHandler() {}

	/**
	 * Do the action.
	 * @param request   The request.
	 * @param out	    The responder.
	 */
	virtual void run(Message& request, MessageResponder* out) = 0;
};