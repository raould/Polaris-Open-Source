// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once

struct EventLoop;
struct Runnable;

struct EventThread {
    EventLoop* loop;
    Runnable* spin;
};

EventThread makeEventThread(Runnable* done);
