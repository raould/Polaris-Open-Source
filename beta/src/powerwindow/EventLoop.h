// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once

struct Runnable;

struct EventLoop
{
    virtual void watch(HANDLE event, Runnable* watcher) = 0;
    virtual void ignore(HANDLE event, Runnable* watcher) = 0;
};
