#include "stdafx.h"
#include "RestrictedShatter.h"
#include "resource.h"

#include "EventLoop.h"

ATOM RestrictedShatter::m_class = 0;
RestrictedShatter::ActiveMap RestrictedShatter::m_from_to;
TCHAR RestrictedShatter::m_szTitle[MAX_LOADSTRING];
TCHAR RestrictedShatter::m_szWindowClass[MAX_LOADSTRING];

HWND RestrictedShatter::make()
{
    HINSTANCE application = EventLoop::getApplication();
    if (0 == m_class) {

	    LoadString(application, IDC_RESTRICTEDSHATTERWINDOW, m_szWindowClass, MAX_LOADSTRING);
	    LoadString(application, IDS_RESTRICTEDSHATTERTITLE, m_szTitle, MAX_LOADSTRING);

	    WNDCLASSEX wcex;
	    wcex.cbSize = sizeof(WNDCLASSEX); 
	    wcex.style			= CS_HREDRAW | CS_VREDRAW;
	    wcex.lpfnWndProc	= (WNDPROC)WndProc;
	    wcex.cbClsExtra		= 0;
	    wcex.cbWndExtra		= 0;
	    wcex.hInstance		= application;
	    wcex.hIcon			= LoadIcon(application, (LPCTSTR)IDI_POWERWINDOW);
	    wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	    wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	    wcex.lpszMenuName	= NULL;
	    wcex.lpszClassName	= m_szWindowClass;
	    wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	    m_class = RegisterClassEx(&wcex);
    }

    return CreateWindow(m_szWindowClass, m_szTitle, WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, HWND_MESSAGE, NULL, application, NULL);
}

void RestrictedShatter::shatter(HWND proxy, HWND target)
{
    m_from_to.insert(ActiveMap::value_type(proxy, target));
}

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Forward a restricted set of messages to another window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK RestrictedShatter::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
	case WM_CHANGECBCHAIN: 
        {
            ActiveMap::iterator end = m_from_to.upper_bound(hWnd);
            ActiveMap::iterator i = m_from_to.lower_bound(hWnd);
            for (; i != end; ++i) {
                if (i->second == (HWND)wParam) {
		            // If the next window is closing, repair the chain. 
                    i->second = (HWND)lParam;
                } else {
                    // Otherwise, pass the message to the next link.
                    SendMessage(i->second, message, wParam, lParam);
                }
            }
        }
        break;
    case WM_DRAWCLIPBOARD:
        {
            ActiveMap::iterator end = m_from_to.upper_bound(hWnd);
            ActiveMap::iterator i = m_from_to.lower_bound(hWnd);
            for (; i != end; ++i) {
                SendMessage(i->second, message, wParam, lParam);
            }
        }
        break;
	case WM_DESTROY:
        {
            ActiveMap::iterator end = m_from_to.upper_bound(hWnd);
            ActiveMap::iterator i = m_from_to.lower_bound(hWnd);
            for (; i != end; ++i) {
                ChangeClipboardChain(hWnd, i->second);
            }
            m_from_to.erase(hWnd);
        }
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

