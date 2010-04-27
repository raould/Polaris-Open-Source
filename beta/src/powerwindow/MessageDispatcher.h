// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once

struct EventLoop;
struct MessageHandler;

struct MessageDispatcher
{
    virtual ~MessageDispatcher() {}

	virtual void serve(const char* verb, MessageHandler* handler) = 0;
};

MessageDispatcher* makeMessageDispatcher(EventLoop* event_loop, std::wstring dir);
