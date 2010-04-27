#pragma once

#include "../polaris/Runnable.h"

struct MessageHandler;

class MessageDispatcher : public Runnable
{
	std::wstring m_folderpath;
	typedef std::map<std::string, MessageHandler*> HandlerMap;
	HandlerMap m_handlers;

public:
	MessageDispatcher(std::wstring folderpath);
	virtual ~MessageDispatcher();

	virtual void run();
	
	void add(std::string verb, MessageHandler* handler);
	void remove(std::string verb);
};
