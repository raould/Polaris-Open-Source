#include "StdAfx.h"
#include "FileEventLoop.h"

#include <algorithm>
#include <vector>

#include "../polaris/Runnable.h"
#include "EventLoop.h"

FileEventLoop::FileEventLoop(EventLoop* event_loop) : m_event_loop(event_loop)
{
}

class DirectoryEventHandler : public Runnable {

    EventLoop* m_event_loop;
    HANDLE m_event;
    std::vector<Runnable*> m_watchers;

public:
    DirectoryEventHandler(EventLoop* event_loop, std::wstring directory) : m_event_loop(event_loop) {
	    m_event = FindFirstChangeNotification(directory.c_str(), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME);
		if (INVALID_HANDLE_VALUE == m_event) {
			printf("FindFirstChangeNotification() failed: %d\n", GetLastError());
		}
        event_loop->add(m_event, this);
    }
	virtual ~DirectoryEventHandler() {
	    FindCloseChangeNotification(m_event);
        std::for_each(m_watchers.begin(), m_watchers.end(), delete_element<Runnable*>());
    }

    // Runnable interface.

    virtual void run() {
	    FindNextChangeNotification(m_event);
        std::for_each(m_watchers.begin(), m_watchers.end(), run_element<Runnable*>());
    }

    // DirectoryEventHandler interface.

    bool isIgnored() { return m_watchers.empty(); }

    void add(Runnable* watcher) {
        assert(std::find(m_watchers.begin(), m_watchers.end(), watcher) == m_watchers.end());
        m_watchers.push_back(watcher);
    }

    void remove(Runnable* watcher) {
        std::vector<Runnable*>::iterator i = std::find(m_watchers.begin(), m_watchers.end(), watcher);
        if (i != m_watchers.end()) {
            m_watchers.erase(i);
            delete watcher;
        }
        assert(std::find(m_watchers.begin(), m_watchers.end(), watcher) == m_watchers.end());
    }

    void stop() {
        m_event_loop->remove(m_event, this);
    }
};

FileEventLoop::~FileEventLoop()
{
}

void FileEventLoop::watch(std::wstring directory, Runnable* watcher) {
    HandlerMap::iterator i = m_event_handlers.find(directory);
    DirectoryEventHandler* handler;
	if (i != m_event_handlers.end()) {
		handler = (*i).second;
	} else {
        handler = new DirectoryEventHandler(m_event_loop, directory);
		m_event_handlers[directory] = handler;
	}
    handler->add(watcher);
}

void FileEventLoop::ignore(std::wstring directory, Runnable* watcher) {
    HandlerMap::iterator i = m_event_handlers.find(directory);
    if (i != m_event_handlers.end()) {
        DirectoryEventHandler* handler = (*i).second;
        handler->remove(watcher);
        if (handler->isIgnored()) {
            m_event_handlers.erase(i);
            handler->stop();
        }
    }
}
