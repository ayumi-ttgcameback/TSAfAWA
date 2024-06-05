#pragma once

BOOL isdigits(const char* p);
BOOL iswdigits(const wchar_t* p);
BOOL iswfloatDigits(const wchar_t* p);
bool sjis2wcs(const char* str, wchar_t* wstr, int wstrbuflen);
void PutConsole(const wchar_t* format, ...);
bool IsFileModified(_In_ const wchar_t* fname, _In_ const DWORD dwFileSize, _In_ const FILETIME* pftLastWrite, _Out_ DWORD& dwErr);
