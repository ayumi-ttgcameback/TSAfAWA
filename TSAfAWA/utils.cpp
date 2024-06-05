#include "framework.h"
#include "utils.h"

BOOL isdigits(const char* p)
{
    if (*p == '\0') {
        return FALSE;
    }
    for (; *p != '\0'; p++) {
        if (!isdigit(*p)) {
            return FALSE;
        }
    }
    return TRUE;
}
BOOL iswdigits(const wchar_t* p)
{
    if (*p == '\0') {
        return FALSE;
    }
    for (; *p != '\0'; p++) {
        if (!iswdigit(*p)) {
            return FALSE;
        }
    }
    return TRUE;
}
BOOL iswfloatDigits(const wchar_t* p)
{
    if (*p == '\0') {
        return FALSE;
    }
    for (; *p != '\0'; p++) {
        if (!iswdigit(*p) && (*p != L'.')) {
            return FALSE;
        }
    }
    return TRUE;
}
bool sjis2wcs(const char* str, wchar_t* wstr, int wstrbuflen)
{
    wstr[0] = 0;
    const int needlen = MultiByteToWideChar(932, 0, str, -1, NULL, 0);
    int len = wstrbuflen < needlen ? wstrbuflen : needlen;
    if (0 == MultiByteToWideChar(CP_ACP, 0, str, -1, wstr, len)) {
        return false;
    }
    return true;
}
void PutConsole(const wchar_t* format, ...)
{
    if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
        AllocConsole();
    }
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    va_list arglist;
    va_start(arglist, format);

    wchar_t buf[1024]{};
    buf[sizeof(buf) / sizeof(wchar_t) - 1] = 0;
    va_start(arglist, format);
    StringCchVPrintf(buf, sizeof(buf) / sizeof(wchar_t) - 1, format, arglist);
    va_end(arglist);

    if (INVALID_HANDLE_VALUE != hStdOut) {
        WriteConsole(hStdOut, buf, lstrlen(buf), NULL, NULL);
    }

    FreeConsole();
}
bool IsFileModified(_In_ const wchar_t* fname, _In_ const DWORD dwFileSize, _In_ const FILETIME* pftLastWrite, _Out_ DWORD& dwErr)
{
    WIN32_FIND_DATA ffd;
    HANDLE hFind;

    hFind = FindFirstFile(fname, &ffd);
    if (hFind == INVALID_HANDLE_VALUE) {
        dwErr = GetLastError();
        return false;
    }
    dwErr = 0;
    FindClose(hFind);
    bool bret = false;
    if(pftLastWrite->dwHighDateTime != ffd.ftLastWriteTime.dwHighDateTime 
        || pftLastWrite->dwLowDateTime != ffd.ftLastWriteTime.dwLowDateTime ){
        bret = true;
    }
    else if (dwFileSize != ffd.nFileSizeLow) {
        bret = true;
    }
    return bret;
}
