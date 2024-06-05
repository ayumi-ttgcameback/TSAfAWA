#include "framework.h"
#include "EventLog.h"

bool EventLog::Install()
{
    DWORD dwCategoryNum = 0;
    HKEY hk;
    DWORD dwTypesSupported;
    TCHAR szBuf[MAX_PATH];
    size_t cchSize = MAX_PATH;
    WCHAR dllName[MAX_PATH + 1];
    DWORD dllNameLen = MAX_PATH;
    if (FALSE == QueryFullProcessImageName(GetCurrentProcess(), 0, dllName, &dllNameLen)) {
        m_dwErr = GetLastError();
        return false;
    }
    dllName[dllNameLen] = 0;

    // イベントログ用サブキー作成
    if (S_OK != StringCchPrintf(szBuf, cchSize,
        L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\%s\\%s", m_wszLogName, m_wszSourceName)) {
        m_dwErr = ERROR_OUTOFMEMORY;
        return false;
    }
    LSTATUS ls = RegCreateKeyEx(HKEY_LOCAL_MACHINE, szBuf, 0, NULL, REG_OPTION_NON_VOLATILE,
        KEY_WRITE, NULL, &hk, NULL);
    if (ls != ERROR_SUCCESS) {
        m_dwErr = (DWORD)ls;
        return false;
    }

    // メッセージファイルへのパス
    if (RegSetValueEx(hk, L"EventMessageFile", 0, REG_EXPAND_SZ,
        (LPBYTE)dllName, (DWORD)(dllNameLen + 1) * (DWORD)sizeof(TCHAR))){
        m_dwErr = GetLastError();
        RegCloseKey(hk);
        SetLastError(m_dwErr);
        return false;
    }

    // TypesSupported 
    dwTypesSupported = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;
    if (RegSetValueEx(hk, L"TypesSupported", 0, REG_DWORD, (LPBYTE)&dwTypesSupported, sizeof(DWORD))) {
        m_dwErr = GetLastError();
        RegCloseKey(hk);
        SetLastError(m_dwErr);
        return false;
    }

    // カテゴリメッセージファイルへのパス
    if (RegSetValueEx(hk,  L"CategoryMessageFile",  0,REG_EXPAND_SZ,
        (LPBYTE)dllName, (DWORD)(dllNameLen + 1) * (DWORD)sizeof(TCHAR))) {
        m_dwErr = GetLastError();
        RegCloseKey(hk);
        SetLastError(m_dwErr);
        return false;
    }

    // カテゴリ数
    if (RegSetValueEx(hk, L"CategoryCount", 0, REG_DWORD, (LPBYTE)&dwCategoryNum, sizeof(DWORD))) {
        m_dwErr = GetLastError();
        RegCloseKey(hk);
        SetLastError(m_dwErr);
        return false;
    }

    RegCloseKey(hk);
    return true;
}

bool EventLog::Uninstall()
{
    TCHAR szBuf[MAX_PATH + 1];

    if (S_OK != StringCchPrintf(szBuf, MAX_PATH,
        L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\%s\\%s",
        m_wszLogName, m_wszSourceName)) {
        m_dwErr = ERROR_OUTOFMEMORY;
        return false;
    }

    if (RegDeleteKeyEx(HKEY_LOCAL_MACHINE, szBuf, KEY_WOW64_64KEY, 0)){
        m_dwErr = GetLastError();
        return false;
    }

    return true;
}
bool EventLog::Report(const DWORD dwEventId, const WORD wEventType, const wchar_t* format, va_list arglist) 
{
    m_dwErr = 0;
    if (m_hEventSource == NULL) {
        m_hEventSource = RegisterEventSource(NULL, m_wszSourceName);
        if (m_hEventSource == NULL) {
            m_dwErr = GetLastError();
            return false;
        }
    }
    if (m_hEventSource != NULL) {
        wchar_t buf[4096]{};
        HRESULT hr = StringCchVPrintf(buf,
            sizeof(buf) / sizeof(wchar_t) - sizeof(wchar_t) * 5,
            format,
            arglist);
        if (hr == STRSAFE_E_INSUFFICIENT_BUFFER) {
            wcscat_s(buf, sizeof(buf) / sizeof(wchar_t) - 1, L"...");
        }
        else if (hr != S_OK) {
            m_dwErr = ERROR_OUTOFMEMORY;
            return false;
        }
        LPWSTR  lpszStrings[1]{};
        lpszStrings[0] = buf;
        if (ReportEvent(m_hEventSource,
            wEventType,
            0, //イベント カテゴリ
            dwEventId,
            NULL, //userSID
            1, // lpStrings パラメーターが指す配列内の挿入文字列の数
            0, // バイナリ データのバイト数
            (LPCWSTR*)lpszStrings, // null で終わる文字列の配列、各文字列は 31,839 文字に制限
            NULL) == FALSE) { //バイナリ データを含むバッファーへのポインタ
            m_dwErr = GetLastError();
            return false;
        }
    }
    return true;
}
