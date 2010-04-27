// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once

struct EventLoop;
struct Scheduler;

/**
 * Constructs a task queue.
 * @param event_loop    The event loop.
 */
Scheduler* makeTaskQueue(EventLoop* event_loop);