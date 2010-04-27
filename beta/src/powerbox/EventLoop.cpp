#include "StdAfx.h"
#include "EventLoop.h"

#include <algorithm>

#include "../polaris/Runnable.h"

EventLoop::EventLoop() : m_events(new HANDLE[0]),
                         m_handlers(new Runnable*[0]),
                         m_size(0)
{
}

EventLoop::~EventLoop()
{
    std::for_each(m_handlers, m_handlers + m_size, delete_element<Runnable*>());
	delete[] m_events;
	delete[] m_handlers;
}

void EventLoop::add(HANDLE event, Runnable* handler) {
	assert(INVALID_HANDLE_VALUE != event);
	assert(NULL != handler);

	HANDLE* old_events = m_events;
	m_events = new HANDLE[m_size + 1];
	memcpy(m_events, old_events, m_size * sizeof(HANDLE));
	m_events[m_size] = event;
	delete[] old_events;

	Runnable** old_handlers = m_handlers;
	m_handlers = new Runnable*[m_size + 1];
	memcpy(m_handlers, old_handlers, m_size * sizeof(Runnable*));
	m_handlers[m_size] = handler;
	delete[] old_handlers;

	++m_size;
}

void EventLoop::remove(HANDLE event, Runnable* handler) {
    size_t index = std::find(m_handlers, m_handlers + m_size, handler) - m_handlers;
    if (index != m_size) {
        delete m_handlers[index];

	    HANDLE* old_events = m_events;
	    m_events = new HANDLE[m_size - 1];
	    memcpy(m_events, old_events, index * sizeof(HANDLE));
	    memcpy(m_events + index, old_events + index + 1, (m_size - index - 1) * sizeof(HANDLE));
	    delete[] old_events;

	    Runnable** old_handlers = m_handlers;
	    m_handlers = new Runnable*[m_size - 1];
	    memcpy(m_handlers, old_handlers, index * sizeof(Runnable*));
	    memcpy(m_handlers + index, old_handlers + index + 1, (m_size - index - 1) * sizeof(Runnable*));
	    delete[] old_handlers;

	    --m_size;
    }
}

void EventLoop::start() {
	while (true) {
		DWORD theEvent = WaitForMultipleObjects(m_size, m_events, FALSE, INFINITE);
		if (theEvent != WAIT_FAILED) {
			int index = theEvent - WAIT_OBJECT_0;
			if (index >= 0 && index < m_size) {
				m_handlers[index]->run();
			}
		}
	}
}