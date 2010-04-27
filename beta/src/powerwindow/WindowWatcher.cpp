// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "StdAfx.h"
#include "WindowWatcher.h"

#include "ActivePet.h"
#include "ActiveWorld.h"
#include "EventLoop.h"
#include "../polaris/auto_handle.h"
#include "../polaris/auto_privilege.h"
#include "../polaris/Logger.h"
#include "../polaris/Runnable.h"
#include "../polaris/Constants.h"
#include "../polaris/RegKey.h"

const int MAX_INITIAL_TICKS_WHILE_INVISIBLE = 60;
const int MAX_TICKS_WHILE_INVISIBLE = 60;

BOOL CALLBACK EnumWindowsProc(HWND window, LPARAM param) {
    std::map<ActivePet*,int>* candidates = (std::map<ActivePet*,int>*)param;

    // Determine the corresponding pet.
    std::map<ActivePet*,int>::iterator i = candidates->begin();
    std::map<ActivePet*,int>::iterator last = candidates->end();
    DWORD process_id = 0;
    ::GetWindowThreadProcessId(window, &process_id);
    auto_privilege se_debug(SE_DEBUG_NAME);
    auto_handle process(::OpenProcess(PROCESS_ALL_ACCESS, FALSE, process_id));
    if (process.get() != NULL) {
        for (; i != last; ++i) {
            if ((*i).first->ownsProcess(process.get())) {
                break;
            }
        }
    }
    if (i != last) {
        ActivePet* pet = (*i).first;

        // Update the window title.
        std::wstring prefix = L"\xab" + pet->getName() + L"\xbb ";
        wchar_t title_buffer[1024] = {};
        ::GetWindowText(window, title_buffer + prefix.length(),
                        (sizeof title_buffer / sizeof(wchar_t)) - prefix.length());
        if (prefix.compare(0, prefix.length(), title_buffer + prefix.length(), prefix.length()) != 0) {
            memcpy(title_buffer, prefix.c_str(), prefix.length() * sizeof(wchar_t));
            // SetWindowText(window, title_buffer);
            // ::SetWindowText will block if the target process is blocked.
            DWORD result;
            ::SendMessageTimeout(window, WM_SETTEXT, 0, (LPARAM)title_buffer,
                                 SMTO_ABORTIFHUNG, 100, &result);
        }

        // This code tries to replicate the logic that the taskbar uses for determining
	    // whether or not to display an application button.
	    // TODO: Verify that a pet cannot invoke ITaskbarList::DeleteTab().
	    if (IsWindowVisible(window) && GetParent(window) == NULL) {
		    WINDOWINFO info = { sizeof(WINDOWINFO), 0 };
		    GetWindowInfo(window, &info);
		    if (!(info.dwExStyle & WS_EX_TOOLWINDOW)) {
			    WINDOWPLACEMENT placement = { sizeof(WINDOWPLACEMENT), 0 };
			    GetWindowPlacement(window, &placement);
			    if (placement.showCmd != SW_HIDE) {
                    (*i).second = MAX_TICKS_WHILE_INVISIBLE;
			    }
		    }
	    }
    }
	return TRUE;
}

class WindowWatcher : public Runnable
{
    EventLoop* m_event_loop;
    ActiveWorld* m_world;

    Logger m_logger;
	HANDLE m_timer;
    std::map<ActivePet*,int> m_candidates;

    void reset() {
		LARGE_INTEGER delay;
		delay.QuadPart = -10000000;
        if (!::SetWaitableTimer(m_timer, &delay, 0, NULL, NULL, 0)) {
            m_logger.log(L"SetWaitableTimer()", GetLastError());
		}
    }

    class Scan : public Runnable
    {
        WindowWatcher& m;

    public:
        Scan(WindowWatcher& outer) : m(outer) {}
        virtual ~Scan() {}

        virtual void run() {
            ::EnumDesktopWindows(::GetThreadDesktop(::GetCurrentThreadId()),
						         EnumWindowsProc, (LPARAM)&m.m_candidates);
            m.reset();
        }
    };

public:
    WindowWatcher(EventLoop* event_loop,
                  ActiveWorld* world) : m_event_loop(event_loop),
                                        m_world(world),
                                        m_logger(L"WindowWatcher") {
        m_timer = ::CreateWaitableTimer(NULL, FALSE, NULL);
		if (NULL == m_timer) {
            m_logger.log(L"CreateWaitableTimer()", GetLastError());
		}
        reset();
        m_event_loop->watch(m_timer, this);
	}
	virtual ~WindowWatcher() {
        m_event_loop->ignore(m_timer, this);
        CloseHandle(m_timer);
    }

	// Runnable interface.

	virtual void run() {

        // Kill off any invisible pets.
        std::list<ActivePet*>& current = m_world->list();
        for (std::map<ActivePet*,int>::iterator i = m_candidates.begin(); i != m_candidates.end();) {
            if (--(*i).second < 0) {
                ActivePet* invisible = (*i).first;

                // Pet has exceeded it's TTL, delete it from the watch list.
				std::map<ActivePet*,int>::iterator tmp = i;
				++i;
				m_candidates.erase(tmp);

                // Also kill the pet if it's not already dead,
                // and the policy is to kill it when it's invisible.
                std::list<ActivePet*>::iterator j = std::find(current.begin(), current.end(), invisible);
                if (current.end() != j) {
                    // Pet is still active.
				    RegKey petKey = RegKey::HKCU.open(Constants::registryPets()).open(invisible->getName());
				    if (petKey.getValue(Constants::allowOpAfterWindowClosed()) != L"true") {
					    m_logger.log(L"Killing " + invisible->getName());
					    invisible->send(new ActiveAccount::Kill());
                        current.erase(j);
					    delete invisible;
                    }
                } else {
                    // Pet is no longer active.
					delete invisible;
                }
            } else {
                ++i;
            }
        }

        // Update the candidate pet list.
        for (std::list<ActivePet*>::iterator i = current.begin(); i != current.end(); ++i) {
            if (m_candidates.count(*i) == 0) {
                m_candidates[*i] = MAX_INITIAL_TICKS_WHILE_INVISIBLE;
            }
        }

        // Scan the candidates.
        spawn(new Scan(*this));
	}
};

Runnable* MakeWindowWatcher(EventLoop* event_loop, ActiveWorld* world) {
	return new WindowWatcher(event_loop, world);
}
