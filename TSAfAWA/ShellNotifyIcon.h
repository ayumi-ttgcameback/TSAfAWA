#pragma once
#include "framework.h"
#include "resource.h"
class ShellNotifyIcon
{
    UINT m_uTaskbarCreatedMessage{};
    NOTIFYICONDATA m_nid{ 0 };
    HMENU m_hMenu{};
    HWND m_hWnd;
    const wchar_t* APPHINT{ L"TvRock Schedule Adjuster for ACPI Wake Alarm" };

    void OnWM_CREATE(HWND hWnd, HICON hIcon, HMENU hMenu, const wchar_t* appname)
    {
        m_hWnd = hWnd;
        m_hMenu = hMenu;
        m_nid.cbSize = sizeof(NOTIFYICONDATA);
        m_nid.uFlags = (NIF_ICON | NIF_MESSAGE | NIF_TIP);
        m_nid.hWnd = hWnd;
        m_nid.hIcon = hIcon;
        m_nid.uID = 1;
        m_nid.uCallbackMessage = WM_APP;
        lstrcpy(m_nid.szTip, appname);
        Shell_NotifyIcon(NIM_ADD, &m_nid);

        m_uTaskbarCreatedMessage = RegisterWindowMessage(_T("TaskbarCreated")); //ï÷óòÇ…Ç»Ç¡ÇΩ
    }
    void OnWM_DESTROY() {
        Shell_NotifyIcon(NIM_DELETE, &m_nid);
        DestroyMenu(m_hMenu);
    }
    void ReResisterMe(UINT message)
    {
        if (message == m_uTaskbarCreatedMessage) {
            Shell_NotifyIcon(NIM_ADD, &m_nid);
        }
    }
    /// <summary>
    /// 
    /// </summary>
    /// <param name="wParam">ICONéØï éqÅB</param>
    /// <param name="lParam"></param>
    void OnWM_APP(WPARAM wParam, LPARAM lParam)
    {
        switch (lParam) {
        case WM_RBUTTONUP:
            SetForegroundWindow(m_hWnd);
            ShowContextMenu();
            break;
        default:
            break;
        }
        PostMessage(m_hWnd, NULL, 0, 0);
    }
    void OnWM_COMMAND(WPARAM wParam)
    {
        int wmId = LOWORD(wParam);
        bool bret = false;
        switch (wmId) {
        case IDM_SHELLNOTIFYICON_QUIT:
            PostQuitMessage(0);
            break;
        default:
            break;
        }
    }
    void ShowContextMenu() const
    {
        POINT pt;
        GetCursorPos(&pt);
        if (m_hMenu) {
            HMENU hSubMenu = GetSubMenu(m_hMenu, 0);
            if (hSubMenu) {
                UINT uFlags = TPM_RIGHTBUTTON;
                if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0) {
                    uFlags |= TPM_RIGHTALIGN;
                }
                else {
                    uFlags |= TPM_LEFTALIGN;
                }
                TrackPopupMenuEx(hSubMenu, uFlags, pt.x, pt.y, m_hWnd, NULL);
            }
        }
    }
public:
    void Dispatch(HINSTANCE hInst, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case WM_CREATE:
            OnWM_CREATE(hWnd, 
                LoadIcon(hInst, MAKEINTRESOURCE(IDI_SMALL)), 
                LoadMenu(hInst, MAKEINTRESOURCE(IDR_MENU_SHELLNOTIFYICON)),
                APPHINT);
            break;
        case WM_APP:
            OnWM_APP(wParam, lParam);
            break;
        case WM_COMMAND:
            OnWM_COMMAND(wParam);
            break;
        case WM_DESTROY:
            OnWM_DESTROY();
            break;
        default:
            ReResisterMe(message);
            break;
        }
    }
};
