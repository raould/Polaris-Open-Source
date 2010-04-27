#pragma once

class EventLoop;
struct Runnable;
class DirectoryEventHandler;

class FileEventLoop
{
    EventLoop* m_event_loop;
    typedef std::map<std::wstring, DirectoryEventHandler*> HandlerMap;
    HandlerMap m_event_handlers;

public:
    FileEventLoop(EventLoop* event_loop);
    ~FileEventLoop();

    void watch(std::wstring directory, Runnable* watcher);
    void ignore(std::wstring directory, Runnable* watcher);
};
