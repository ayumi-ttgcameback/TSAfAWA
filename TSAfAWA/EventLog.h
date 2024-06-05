#pragma once
#include "framework.h"

class EventLog
{
private:
	DWORD m_dwErr;
	HANDLE m_hEventSource = NULL;
	wchar_t m_wszLogName[MAX_PATH + 1]{ L"Application" };
	wchar_t m_wszSourceName[MAX_PATH + 1]{};

public:
	EventLog(const wchar_t* pwszSourceName) {
		m_wszSourceName[MAX_PATH] = 0;
		wcscpy_s(m_wszSourceName, sizeof(m_wszSourceName) / sizeof(wchar_t) - 1, pwszSourceName);
	}
	~EventLog() {
		if (m_hEventSource != NULL) {
			DeregisterEventSource(m_hEventSource);
			m_hEventSource = NULL;
		}
	}
	bool Report(const DWORD dwEventId, const WORD wEventType, const wchar_t* format, va_list arglist);
	bool Install();
	bool Uninstall();
	DWORD GetError() const {
		return m_dwErr;
	}

	void error(const wchar_t* format, ...) {
		va_list arglist;
		va_start(arglist, format);
		verror(1, format, arglist);
		va_end(arglist);
	}
	void info(const wchar_t* format, ...) {
		va_list arglist;
		va_start(arglist, format);
		vinfo(1, format, arglist);
		va_end(arglist);
	}
	void warn(const wchar_t* format, ...) {
		va_list arglist;
		va_start(arglist, format);
		vwarn(1, format, arglist);
		va_end(arglist);
	}
	void error(const DWORD id, const wchar_t* format, ...) {
		va_list arglist;
		va_start(arglist, format);
		verror(id, format, arglist);
		va_end(arglist);
	}
	void info(const DWORD id, const wchar_t* format, ...) {
		va_list arglist;
		va_start(arglist, format);
		vinfo(id, format, arglist);
		va_end(arglist);
	}
	void warn(const DWORD id, const wchar_t* format, ...) {
		va_list arglist;
		va_start(arglist, format);
		vwarn(id, format, arglist);
		va_end(arglist);
	}

	void verror(const wchar_t* format, va_list arglist) {
		verror(1, format, arglist);
	}
	void vinfo(const wchar_t* format, va_list arglist) {
		vinfo(1, format, arglist);
	}
	void vwarn(const wchar_t* format, va_list arglist) {
		vwarn(1, format, arglist);
	}
	void verror(const DWORD id, const wchar_t* format, va_list arglist) {
		Report(id, EVENTLOG_ERROR_TYPE, format, arglist);
	}
	void vinfo(const DWORD id, const wchar_t* format, va_list arglist) {
		Report(id, EVENTLOG_INFORMATION_TYPE, format, arglist);
	}
	void vwarn(const DWORD id, const wchar_t* format, va_list arglist) {
		Report(id, EVENTLOG_WARNING_TYPE, format, arglist);
	}

};

