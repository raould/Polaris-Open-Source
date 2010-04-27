// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "StdAfx.h"
#include "ActiveWorld.h"

#include "ActivePet.h"
#include "EchoHandler.h"
#include "EventLoop.h"
#include "FileDialogWatcher.h"
#include "LaunchHandler.h"
#include "MessageDispatcher.h"
#include "OpenStrictHandler.h"
#include "OpenURLHandler.h"
#include "PingHandler.h"
#include "ProcessWatcher.h"
#include "ShellHandler.h"
#include "WindowWatcher.h"
#include "../polaris/auto_handle.h"
#include "../polaris/auto_privilege.h"
#include "../polaris/Constants.h"
#include "../polaris/Runnable.h"

class ActiveWorldImpl : public ActiveWorld
{
    wchar_t* m_password;

    MessageDispatcher* m_dispatcher;
    Runnable* m_process_watcher;
    Runnable* m_window_watcher;
	Runnable* m_file_dialog_watcher;

	std::list<ActivePet*> m_pets;

public:
	// note, the password may change during operations, you cannot make the password
	// that is copied here simply be the same size as the earlier-computed password,
	// because the later password may be larger
	ActiveWorldImpl(EventLoop* event_loop,
                    const wchar_t* password) : m_password(wcscpy(new wchar_t[1000],
                                                          password))
    {
        m_dispatcher = makeMessageDispatcher(event_loop,
            Constants::requestsPath(Constants::polarisDataPath(_wgetenv(L"USERPROFILE"))));
        m_dispatcher->serve("echo", makeEchoHandler());
        m_dispatcher->serve("launch", makeLaunchHandler(this));
        m_dispatcher->serve("ping", makePingHandler(this));
        m_dispatcher->serve("shell", makeShellHandler(this));
        m_dispatcher->serve("openstrict", makeOpenStrictHandler(this));
        m_dispatcher->serve("openurl", makeOpenURLHandler(this));

        m_process_watcher = MakeProcessWatcher(event_loop, this);
        m_window_watcher = MakeWindowWatcher(event_loop, this);
	    m_file_dialog_watcher = MakeFileDialogWatcher(event_loop, this);
    }
    virtual ~ActiveWorldImpl() {
        delete[] m_password;
        delete m_dispatcher;
        delete m_process_watcher;
        delete m_window_watcher;
	    delete m_file_dialog_watcher;
        std::for_each(m_pets.begin(), m_pets.end(), delete_element<ActivePet*>());
    }

    virtual std::list<ActivePet*>& list() { return m_pets; }
    
    virtual ActivePet* findByName(std::wstring petname) {
        for (std::list<ActivePet*>::iterator i = m_pets.begin(); i != m_pets.end(); ++i) {
            if ((*i)->getName() == petname) {
                return *i;
            }
        }
        return NULL;
    }

    virtual ActivePet* findByProcess(HANDLE process) {
        for (std::list<ActivePet*>::iterator i = m_pets.begin(); i != m_pets.end(); ++i) {
		    if ((*i)->ownsProcess(process)) {
                return *i;
		    }
	    }
	    return NULL;
    }

    virtual ActivePet* findByUser(PSID user_sid) {
        for (std::list<ActivePet*>::iterator i = m_pets.begin(); i != m_pets.end(); ++i) {
            if (::EqualSid(user_sid, (*i)->getUserSID())) {
                return *i;
		    }
	    }
	    return NULL;
    }

    virtual ActivePet* findByWindow(HWND window) {
	    DWORD pid = 0;
	    GetWindowThreadProcessId(window, &pid);
        auto_privilege se_debug(SE_DEBUG_NAME);
        auto_handle process(::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid));
	    return findByProcess(process.get());
    }

    virtual ActivePet* load(std::wstring petname) {
        ActivePet* r = findByName(petname);
        if (!r) {
            r = openPet(petname, m_password);
            if (r) {
                m_pets.push_back(r);
            }
        }
	    return r;
    }
};

ActiveWorld* makeActiveWorld(EventLoop* event_loop, const wchar_t* password) {
    return new ActiveWorldImpl(event_loop, password);
}
