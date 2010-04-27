#pragma once

class RestrictedShatter
{
    static const size_t MAX_LOADSTRING = 100;
    static TCHAR m_szTitle[];
    static TCHAR m_szWindowClass[];
    static ATOM m_class;

    typedef std::multimap<HWND,HWND> ActiveMap;
    static ActiveMap m_from_to;

    /**
     * The window procedure.
     */
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

public:

    /**
     * Makes a proxy window.
     */
    static HWND make();

    /**
     * Begin shattering a target window.
     * @param proxy     The proxy window.
     * @param target    The target window.
     */
    static void shatter(HWND proxy, HWND target);
};