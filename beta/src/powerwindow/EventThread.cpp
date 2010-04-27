// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "StdAfx.h"
#include "EventThread.h"

#include "EventLoop.h"
#include "../polaris/Logger.h"
#include "../polaris/Runnable.h"

class EventList : public EventLoop
{
    Runnable* m_done;               // The task to run when done processing events.
    size_t m_event_count;           // The event list size.
    HANDLE* m_events;               // The list of events to wait on.
    Runnable** m_event_handlers;    // The list of corresponding event handlers.

    class Spin : public Runnable
    {
        EventList& m;
    
    public:
        Spin(EventList& outer) : m(outer) {}
        virtual ~Spin() {}

        virtual void run() { m.spin(); }
    };

    void spin() {
        Logger::basicLog(L"Processing events...");
        while (0 != m_event_count) {
            DWORD result = ::WaitForMultipleObjects(m_event_count, m_events, FALSE, INFINITE);
            if (WAIT_OBJECT_0 <= result && WAIT_OBJECT_0 + m_event_count > result) {
                m_event_handlers[result - WAIT_OBJECT_0]->run();

                // Ensure fairness by doing an explicit check on the remaining events.
                for (size_t i = result - WAIT_OBJECT_0 + 1; i < m_event_count; ++i) {
                    if (WAIT_OBJECT_0 == ::WaitForSingleObject(m_events[i], 0)) {
                        m_event_handlers[i]->run();
                    }
                }
            }
        }
        m_done->run();
        delete this;
        Logger::basicLog(L"Done processing.");
    }

    EventList(Runnable* done) : m_done(done),
                                m_event_count(0),
                                m_events(new HANDLE[0]),
                                m_event_handlers(new Runnable*[0]) {}
    ~EventList() {
        delete m_done;
        delete[] m_events;
        delete[] m_event_handlers;
    }
public:
    static EventThread make(Runnable* done) {
        EventList* list = new EventList(done);
        EventThread r = { list, new Spin(*list) };
        return r;
    }

    // EventLoop interface.

    virtual void watch(HANDLE event, Runnable* handler) {
	    assert(INVALID_HANDLE_VALUE != event);
	    assert(NULL != handler);

	    HANDLE* old_events = m_events;
	    m_events = new HANDLE[m_event_count + 1];
	    memcpy(m_events, old_events, m_event_count * sizeof(HANDLE));
	    m_events[m_event_count] = event;
	    delete[] old_events;

	    Runnable** old_handlers = m_event_handlers;
	    m_event_handlers = new Runnable*[m_event_count + 1];
	    memcpy(m_event_handlers, old_handlers, m_event_count * sizeof(Runnable*));
	    m_event_handlers[m_event_count] = handler;
	    delete[] old_handlers;

	    ++m_event_count;
    }

    virtual void ignore(HANDLE event, Runnable* handler) {
        size_t index = std::find(m_event_handlers, m_event_handlers + m_event_count, handler) -
                       m_event_handlers;
        if (index != m_event_count) {

	        HANDLE* old_events = m_events;
	        m_events = new HANDLE[m_event_count - 1];
	        memcpy(m_events, old_events, index * sizeof(HANDLE));
	        memcpy(m_events + index, old_events + index + 1,
                   (m_event_count - index - 1) * sizeof(HANDLE));
	        delete[] old_events;

	        Runnable** old_handlers = m_event_handlers;
	        m_event_handlers = new Runnable*[m_event_count - 1];
	        memcpy(m_event_handlers, old_handlers, index * sizeof(Runnable*));
	        memcpy(m_event_handlers + index, old_handlers + index + 1,
                   (m_event_count - index - 1) * sizeof(Runnable*));
	        delete[] old_handlers;

	        --m_event_count;
        }
    }
};

EventThread makeEventThread(Runnable* done) { return EventList::make(done); }