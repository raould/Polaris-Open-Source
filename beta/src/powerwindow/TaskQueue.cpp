// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "StdAfx.h"
#include "TaskQueue.h"

#include "EventLoop.h"
#include "../polaris/Runnable.h"

struct TaskQueue
{
    HANDLE pending;
    std::queue<Runnable*> tasks;

    TaskQueue() : pending(CreateEvent(NULL, TRUE, FALSE, NULL)) {}
    ~TaskQueue() { CloseHandle(pending); }
};

class TaskRunner : public Runnable
{
    HANDLE m_mutex;
    TaskQueue& m_queue;

public:
    TaskRunner(HANDLE mutex,
               TaskQueue& queue) : m_mutex(mutex),
                                   m_queue(queue) {}
    virtual ~TaskRunner() {}

    virtual void run() {
        if (WAIT_OBJECT_0 == WaitForSingleObject(m_mutex, INFINITE)) {
            Runnable* task = m_queue.tasks.front();
            m_queue.tasks.pop();
            if (m_queue.tasks.empty()) {
                ResetEvent(m_queue.pending);
            }
            ReleaseMutex(m_mutex);

            task->run();
            delete task;
        }
    }
};

class Stop : public Runnable {
    HANDLE m_mutex;
    TaskQueue* m_queue;
    TaskRunner* m_runner;
    EventLoop* m_event_loop;

public:
    Stop(HANDLE mutex,
         TaskQueue* queue,
         TaskRunner* runner,
         EventLoop* event_loop) : m_mutex(mutex),
                                  m_queue(queue),
                                  m_runner(runner),
                                  m_event_loop(event_loop) {}
    virtual ~Stop() {}

    virtual void run() {
        m_event_loop->ignore(m_queue->pending, m_runner);
        CloseHandle(m_mutex);
        delete m_runner;
        delete m_queue;
    }
};

class Asynchronous : public Scheduler
{
    HANDLE m_mutex;
    TaskQueue& m_queue;
    Runnable* m_stop;

public:
    Asynchronous(HANDLE mutex,
                 TaskQueue& queue,
                 Runnable* stop) : m_mutex(mutex),
                                   m_queue(queue),
                                   m_stop(stop) {}
    virtual ~Asynchronous() {}

    virtual void close() { send(m_stop); }

    virtual void send(Runnable* task) {
        DWORD error = WaitForSingleObject(m_mutex, INFINITE);
        if (WAIT_OBJECT_0 == error) {
            m_queue.tasks.push(task);
            SetEvent(m_queue.pending);
            ReleaseMutex(m_mutex);
        }
    }
};

Scheduler* makeTaskQueue(EventLoop* event_loop) {
    HANDLE mutex = CreateMutex(NULL, FALSE, NULL);
    TaskQueue* queue = new TaskQueue();
    TaskRunner* runner = new TaskRunner(mutex, *queue);
    event_loop->watch(queue->pending, runner);
    return new Asynchronous(mutex, *queue, new Stop(mutex, queue, runner, event_loop));
}
