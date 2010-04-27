// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once

#include <functional>

struct Runnable {
    virtual ~Runnable() {}
    virtual void run() = 0;
};

struct run : public std::unary_function<Runnable*, void> {
    void operator()(Runnable* p) { p->run(); }
};

template<class T>
struct delete_element : public std::unary_function<T, void> {
    void operator()(T p) { delete p; }
};

template<class T>
struct delete_value : public std::unary_function<T, void> {
    void operator()(T p) { delete p.second; }
};

/**
 * Runs a task in a separate thread.
 * @param task  The task to run.
 * @return The thread id.
 */
DWORD spawn(Runnable* task);

/**
 * Something that runs Runnables.
 */
struct Scheduler {
    virtual ~Scheduler() {}

    virtual void close() = 0;
    virtual void send(Runnable* task) = 0;
};
