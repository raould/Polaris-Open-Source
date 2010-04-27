// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

// powerwindow.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "powerwindow.h"

#include "ActivePet.h"
#include "ActiveWorld.h"
#include "EventLoop.h"
#include "FixAndClean.h"
#include "../polaris/auto_privilege.h"
#include "../polaris/Constants.h"
#include "../polaris/Runnable.h"
#include "../polaris/Logger.h"
#include "../polaris/RegKey.h"
#include <Wincrypt.h>
#include <shellapi.h>

#define MAX_LOADSTRING 100

const UINT TRAYICONID = 1;
const UINT TRAYMSG = WM_APP;
const UINT KILLPET = WM_APP + 1;

class MainLoop;

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
NOTIFYICONDATA icon_data = {};                  // The system tray icon info.

Logger m_logger(L"WinMain");                    // The logger for this module.
bool m_waiting_to_die = false;                  // Are we waiting for the WM_QUIT message?
MainLoop* m_main_loop = NULL;
ActiveWorld* m_world = NULL;
HANDLE* m_events = new HANDLE[0];               // The list of events to wait on.
Runnable** m_event_handlers = new Runnable*[0]; // The list of corresponding event handlers.
size_t m_event_count = 0;                       // The event list size.

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);


class MainLoop : public EventLoop {

public:
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

class KillAndNotify : public ActiveAccount::Task
{
    HANDLE m_notify;

public:
    KillAndNotify(HANDLE notify) : m_notify(notify) {}
    virtual ~KillAndNotify() {}

    virtual void run(ActiveAccount& account) {
        account.kill();
        SetEvent(m_notify);
    }
};

int APIENTRY _tWinMain(HINSTANCE hInstance,
                       HINSTANCE hPrevInstance,
                       LPTSTR    lpCmdLine,
                       int       nCmdShow) {

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_POWERWINDOW, szWindowClass, MAX_LOADSTRING);
    if (NULL != FindWindow(szWindowClass, NULL)) {
        return 0;
    }
    MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow)) {
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_POWERWINDOW);

	// Main message loop:
	MSG msg;
    while (true) {

        // Wait for the next event or message.
        DWORD result = ::MsgWaitForMultipleObjects(m_event_count, m_events, FALSE,
                                                   INFINITE, QS_ALLINPUT);
        if (WAIT_OBJECT_0 + m_event_count == result) {
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                if (WM_QUIT == msg.message && m_waiting_to_die) {
                    goto done;
                }
		        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
			        TranslateMessage(&msg);
			        DispatchMessage(&msg);
		        }
            }
        } else if (WAIT_OBJECT_0 <= result && WAIT_OBJECT_0 + m_event_count > result) {
            m_event_handlers[result - WAIT_OBJECT_0]->run();

            // Ensure fairness by doing an explicit check on the remaining events.
            for (size_t i = result - WAIT_OBJECT_0 + 1; i < m_event_count; ++i) {
                if (WAIT_OBJECT_0 == ::WaitForSingleObject(m_events[i], 0)) {
                    m_event_handlers[i]->run();
                }
            }
        } else if (WAIT_FAILED == result) {
            // Should not happen.
            m_logger.log(L"MsgWaitForMultipleObjects()", GetLastError());
        } else {
            // Should not happen.
            m_logger.log(L"MsgWaitForMultipleObjects() invalid return", result);
        }
    }

done:
    // Kill all the running pets.
    m_logger.log(L"Killing all active pets...");
    std::list<ActivePet*>& pets = m_world->list();
    size_t pet_count = pets.size();
    HANDLE* pet_done = new HANDLE[pet_count];
    HANDLE* notify = pet_done;
    for (std::list<ActivePet*>::iterator i = pets.begin(); i != pets.end(); ++i) {
        *notify = CreateEvent(NULL, TRUE, FALSE, NULL);
        (*i)->send(new KillAndNotify(*notify));
        ++notify;
    }
    WaitForMultipleObjects(pet_count, pet_done, TRUE, INFINITE);
    delete m_world;
    m_logger.log(L"Done.");
	return (int)msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_POWERWINDOW);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= (LPCTSTR)IDC_POWERWINDOW;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_POWERWINDOW);

	return RegisterClassEx(&wcex);
}

UINT_PTR CALLBACK CloseFileDialog(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam) {
    ::PostMessage(::GetParent(hdlg), WM_COMMAND, IDCANCEL, 0);
    return 0;
}

//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd) {
      return FALSE;
   }

   FixAndClean::fix();
   m_main_loop = new MainLoop();

   // Figure out the password for network authentication
   // If no authentication needed, a "" password is created
   // TODO: The main password setting mechanism has been moved to the activepet useServers run method,
   // so if the password is changed, the change is picked up when a pet is launched. Passing the
   // empty password through 500 method calls now makes little sense. Remove the passing of this stuff
   // through the stack.
   wchar_t password[1024] = {};
   //RegKey polarisKey = RegKey::HKCU.open(Constants::registryBase());
   //bool needsPassword = polarisKey.getValue(Constants::registryNeedsPassword()) == L"true";
   //bool passwordStored = polarisKey.getValue(Constants::registryPasswordIsStored()) == L"true";
   //if (needsPassword && !passwordStored) {
   //    puts("Enter your password:\n");
   //    _getws(password);
   //    for (int i = 0; i < 30; ++i) {printf("\n");}
   //} else if (needsPassword && passwordStored) {
   //    FILE* passwordFile = _wfopen(Constants::passwordFilePath(Constants::polarisDataPath(
   //                                             _wgetenv(L"USERPROFILE"))).c_str(), L"rb");
   //     DATA_BLOB clearPass;
   //     DATA_BLOB cryptPass;
   //     byte cryptBytes[1000];
   //     size_t bytesSize = fread(cryptBytes,1, 900, passwordFile); 
   //     cryptPass.pbData = cryptBytes;
   //     cryptPass.cbData = bytesSize;
   //     if (CryptUnprotectData(&cryptPass, NULL, NULL, NULL, NULL,
   //                            CRYPTPROTECT_UI_FORBIDDEN, &clearPass)) {
   //         wcscpy(password, (wchar_t*) clearPass.pbData);
   //         LocalFree(clearPass.pbData);
   //     } else {
   //         m_logger.log(L"cryptUnprotectData failed", GetLastError());
   //     }
   //    //fgetws(password, sizeof(password) -1, passwordFile);
   //    fclose(passwordFile);
   //}
   m_world = makeActiveWorld(m_main_loop, password);

   // Setup the systray icon.
   HICON tray_icon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_POWERWINDOW), IMAGE_ICON,
                      ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON),
                      LR_DEFAULTCOLOR);
   icon_data.cbSize = sizeof(NOTIFYICONDATA);
   icon_data.hWnd = hWnd;
   icon_data.uID = TRAYICONID;
   icon_data.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
   icon_data.uCallbackMessage = TRAYMSG;
   icon_data.hIcon = tray_icon;
   lstrcpyn(icon_data.szTip, L"Polaris is active", sizeof icon_data.szTip / sizeof(wchar_t));
   ::Shell_NotifyIcon(NIM_ADD, &icon_data);
   ::DestroyIcon(tray_icon);

   // Don't show the window.
   // ShowWindow(hWnd, nCmdShow);
   // UpdateWindow(hWnd);

   return TRUE;
}

void ShowSysTrayMenu(HWND hWnd)
{
	POINT pt;
	GetCursorPos(&pt);
	HMENU hMenu = CreatePopupMenu();
	if(hMenu) {
        AppendMenu(hMenu, MF_STRING, IDM_EXIT, _T("Exit"));

        HMENU hPets = CreatePopupMenu();
        if (hPets) {
            std::list<ActivePet*>& pets = m_world->list();
            if (pets.empty()) {
                AppendMenu(hPets, MF_GRAYED | MF_STRING, KILLPET, _T("No active pets"));
            } else {
                UINT index = 0;
                for (std::list<ActivePet*>::iterator i = pets.begin(); i != pets.end(); ++i) {
                    std::wstring petname = (*i)->getName().c_str();
                    AppendMenu(hPets, MF_STRING, KILLPET + index, petname.c_str());
                    ++index;
                }
            }
            AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hPets, _T("Kill"));
        }

		// note:	must set window to the foreground or the
		//			menu won't disappear when it should
		SetForegroundWindow(hWnd);

		TrackPopupMenu(hMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, NULL );
        DestroyMenu(hPets);
		DestroyMenu(hMenu);
	}
}

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message) 
	{
    case TRAYMSG:
        switch (lParam) {
		    case WM_RBUTTONDOWN:
		    case WM_CONTEXTMENU:
			    ShowSysTrayMenu(hWnd);
                break;
        }
        break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
            {
                std::list<ActivePet*>& pets = m_world->list();
                if (wmId >= KILLPET && wmId < KILLPET + pets.size()) {
                    std::list<ActivePet*>::iterator i = pets.begin();
                    for (int n = wmId; n != KILLPET; --n) {
                        ++i;
                    }
                    (*i)->send(new ActiveAccount::Kill());
                    pets.erase(i);
                } else {
			        return DefWindowProc(hWnd, message, wParam, lParam);
                }
            }
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
        m_waiting_to_die = true;
        ::Shell_NotifyIcon(NIM_DELETE, &icon_data);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		return TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
		{
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}
