#pragma once

struct Runnable;

class EventLoop
{
	HANDLE* m_events;
	Runnable** m_handlers;
	int m_size;

public:
	EventLoop(void);
	virtual ~EventLoop(void);
	void add(HANDLE event, Runnable* handler);
	void remove(HANDLE event, Runnable* handler);
	void start();
};

