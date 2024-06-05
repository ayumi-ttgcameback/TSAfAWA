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

    // �C�x���g���O�p�T�u�L�[�쐬
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

    // ���b�Z�[�W�t�@�C���ւ̃p�X
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

    // �J�e�S�����b�Z�[�W�t�@�C���ւ̃p�X
    if (RegSetValueEx(hk,  L"CategoryMessageFile",  0,REG_EXPAND_SZ,
        (LPBYTE)dllName, (DWORD)(dllNameLen + 1) * (DWORD)sizeof(TCHAR))) {
        m_dwErr = GetLastError();
        RegCloseKey(hk);
        SetLastError(m_dwErr);
        return false;
    }

    // �J�e�S����
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
            0, //�C�x���g �J�e�S��
            dwEventId,
            NULL, //userSID
            1, // lpStrings �p�����[�^�[���w���z����̑}��������̐�
            0, // �o�C�i�� �f�[�^�̃o�C�g��
            (LPCWSTR*)lpszStrings, // null �ŏI��镶����̔z��A�e������� 31,839 �����ɐ���
            NULL) == FALSE) { //�o�C�i�� �f�[�^���܂ރo�b�t�@�[�ւ̃|�C���^
            m_dwErr = GetLastError();
            return false;
        }
    }
    return true;
}
