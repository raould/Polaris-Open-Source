#include "StdAfx.h"
#include "WindowWatcher.h"
#include "EventLoop.h"
#include "ActivePet.h"
#include "ActiveWorld.h"
#include "../polaris/Runnable.h"
#include "../polaris/auto_handle.h"

BOOL CALLBACK EnumWindowsProc(HWND window, LPARAM param) {
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
                DWORD process_id;
                GetWindowThreadProcessId(window, &process_id);
                auto_handle process(OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, process_id));
                if (process.get() != NULL) {
                    std::set<ActivePet*>* remaining = (std::set<ActivePet*>*)param;
                    std::set<ActivePet*>::iterator last = remaining->end();
                    for (std::set<ActivePet*>::iterator i = remaining->begin(); i != last; ++i) {
                        if ((*i)->ownsProcess(process.get())) {
                            (*i)->resetAge();
                            remaining->erase(i);
                            break;
                        }
                    }
                } else {
                    printf("OpenProcess() failed: %d\n", GetLastError());
                }
			}
		}
	}
	return true;
}

class WindowWatcher : public Runnable
{
    ActiveWorld* m_activeWorld;
	auto_handle m_timer;

public:
    WindowWatcher(EventLoop* event_loop,
                  ActiveWorld* activeWorld) : m_activeWorld(activeWorld),
                                              m_timer(CreateWaitableTimer(NULL, TRUE, NULL)) {        
		if (m_timer.get() == NULL) {
			printf("CreateWaitableTimer() failed: %d\n", GetLastError());
		}
		event_loop->add(m_timer.get(), this);

		run();
	}
	virtual ~WindowWatcher() {}

	// Runnable interface.

	virtual void run() {

		// Manually reset the timer.
		LARGE_INTEGER delay;
		delay.QuadPart = -10000000;
		if (!SetWaitableTimer(m_timer.get(), &delay, 0, NULL, NULL, 0)) {
			printf("SetWaitableTimer() failed: %d\n", GetLastError());
		}

		// Check the active windows.
        std::set<ActivePet*> candidatesForTermination = m_activeWorld->getActivePets();
		EnumDesktopWindows(GetThreadDesktop(GetCurrentThreadId()),
						   EnumWindowsProc,
						   (LPARAM)&candidatesForTermination);
        for (std::set<ActivePet*>::iterator i = candidatesForTermination.begin(); i!= candidatesForTermination.end(); ++i) {
            if ((*i)->age()) {
                m_activeWorld->terminateActivePet(*i);
            }
        }
	}
};

void addWindowWatcher(EventLoop* event_loop, ActiveWorld* activeWorld) {
	new WindowWatcher(event_loop, activeWorld);
}
