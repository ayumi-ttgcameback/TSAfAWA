#pragma once
#include "EventLog.h"
#include "TvRockUtil.h"
class PowerEventHandler
{
private:
	struct events_t {
		HANDLE hEnd{};
		HANDLE hToSleep{};
		HANDLE hWakeUp{};
	};
	events_t m_events{};

	static unsigned __stdcall ThreadMain(void* pArguments);
	HANDLE ToSleep(TvRockUtil* ptru, HANDLE hWaitableTimer);
protected:
	EventLog* m_pLog;
	HANDLE m_hThread{};
	bool BuildEvents();
	bool CloseEvents();
	bool Run();

	virtual void ReportStartPending() {

	}
	virtual void ReportStopPending() {

	}
	virtual void ReportRunning() {

	}
	virtual void ReportStop(DWORD dwErr = 0) {

	}

public:
	PowerEventHandler(EventLog*plog) {
		m_pLog = plog;
	}
	BOOL Dispatch(WPARAM w);
	void StartThread();
	void StopThread();
	bool StopRequest()
	{
		if (m_events.hEnd == NULL) {
			return false;
		}
		SetEvent(m_events.hEnd);
		return true;
	}
	void error(const wchar_t* format, ...) {
		va_list arglist;
		va_start(arglist, format);
		if (m_pLog) {
			m_pLog->verror(format, arglist);
		}
		va_end(arglist);
	}
	void info(const wchar_t* format, ...) {
		va_list arglist;
		va_start(arglist, format);
		if (m_pLog) {
			m_pLog->vinfo(format, arglist);
		}
		va_end(arglist);
	}
	void warn(const wchar_t* format, ...) {
		va_list arglist;
		va_start(arglist, format);
		if (m_pLog) {
			m_pLog->vwarn(format, arglist);
		}
		va_end(arglist);
	}
	void error(const DWORD id, const wchar_t* format, ...) {
		va_list arglist;
		va_start(arglist, format);
		if (m_pLog) {
			m_pLog->verror(id, format, arglist);
		}
		va_end(arglist);
	}
	void info(const DWORD id, const wchar_t* format, ...) {
		va_list arglist;
		va_start(arglist, format);
		if (m_pLog) {
			m_pLog->vinfo(id, format, arglist);
		}
		va_end(arglist);
	}
	void warn(const DWORD id, const wchar_t* format, ...) {
		va_list arglist;
		va_start(arglist, format);
		if (m_pLog) {
			m_pLog->vwarn(id, format, arglist);
		}
		va_end(arglist);
	}
};

