#include "StdAfx.h"
#include "EventMessage.h"
#include "../polaris/Runnable.h"

class EventMessage : public Runnable
{
	const char* m_message;

public:
    EventMessage(const char* message) : m_message(message) {}
    virtual ~EventMessage() {}

    virtual void run() {
        printf("fired: %s\n", m_message);
    }
};

Runnable* makeEventMessage(const char* message) {
    return new EventMessage(message);
}
