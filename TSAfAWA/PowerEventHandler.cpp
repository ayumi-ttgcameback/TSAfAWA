#include "framework.h"
#include "TSAfAWA.h"
#include "EventLog.h"
#include "PowerEventHandler.h"


bool PowerEventHandler::BuildEvents()
{
	DWORD dwErr = 0;
	m_events.hEnd = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (m_events.hEnd == NULL) {
		return false;
	}
	m_events.hToSleep = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (m_events.hToSleep == NULL) {
		dwErr=GetLastError();
		CloseHandle(m_events.hEnd);
		m_events.hEnd = NULL;
		SetLastError(dwErr);
		return false;
	}
	m_events.hWakeUp = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (m_events.hWakeUp == NULL) {
		dwErr = GetLastError();
		CloseHandle(m_events.hEnd);
		m_events.hEnd = NULL;
		CloseHandle(m_events.hToSleep);
		m_events.hToSleep = NULL;
		SetLastError(dwErr);
		return false;
	}
	ResetEvent(m_events.hEnd);
	ResetEvent(m_events.hToSleep);
	ResetEvent(m_events.hWakeUp);
	return true;
}
bool PowerEventHandler::CloseEvents()
{
	if (m_events.hEnd) {
		CloseHandle(m_events.hEnd);
		m_events.hEnd = NULL;
	}
	if (m_events.hToSleep) {
		CloseHandle(m_events.hToSleep);
		m_events.hToSleep = NULL;
	}
	if (m_events.hWakeUp) {
		CloseHandle(m_events.hWakeUp);
		m_events.hWakeUp = NULL;
	}
	return true;
}
void PowerEventHandler::StartThread()
{
	ReportStartPending();
	if (m_hThread != NULL) {
		ReportStop((DWORD)-1);
		error(L"バグ");
	}
	unsigned threadID;
	if (BuildEvents() == false) {
		ReportStop(GetLastError());
		return;
	}

	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &ThreadMain, this, 0, &threadID);
	if (m_hThread == NULL) {
		DWORD dwErr = GetLastError();
		CloseEvents();
		ReportStop(dwErr);
		return;
	}
}
void PowerEventHandler::StopThread()
{
	ReportStopPending();
	if (m_events.hEnd == NULL) {
		ReportStop((DWORD)-1);
		return;
	}
	SetEvent(m_events.hEnd);

	if (m_hThread != NULL) {
		while (WAIT_TIMEOUT == WaitForSingleObject(m_hThread, 3000)) {
			ReportStopPending();
		}
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}
	CloseEvents();
}
#define EXPAND_TM(x) x.tm_year+1900, x.tm_mon + 1, x.tm_mday, x.tm_hour, x.tm_min, x.tm_sec
HANDLE PowerEventHandler::ToSleep(TvRockUtil* ptru, HANDLE hWaitableTimer)
{
	LARGE_INTEGER dueTime{};

	if (hWaitableTimer != NULL) {
		CancelWaitableTimer(hWaitableTimer);
		CloseHandle(hWaitableTimer);
		hWaitableTimer = NULL;
	}

	if (ptru->Build() == false) {
		return NULL;
	}

	dueTime.QuadPart = ptru->GetAdjustedWakeupRelativeTime100ns();

	if (dueTime.QuadPart != 0) {
		hWaitableTimer = CreateWaitableTimer(NULL, FALSE, NULL);
		if (hWaitableTimer != NULL) {
			BOOL bRet = SetWaitableTimer(hWaitableTimer, &dueTime, 0, NULL, NULL, TRUE);
			DWORD dwErr = GetLastError();
			if (dwErr == ERROR_NOT_SUPPORTED || bRet == FALSE) {
				error(L"待機可能タイマーを設定できません(CODE=0x%x)", dwErr);
			}
			else if (dwErr == ERROR_NOT_SUPPORTED) {
				error(L"システムが復元をサポートしていません");
				CancelWaitableTimer(hWaitableTimer);
				CloseHandle(hWaitableTimer);
				hWaitableTimer = NULL;
			}
			else {
				struct tm tmtvProgramBeginTime;
				struct tm tmResume;
				struct tm tmOriginalWakeup;
				time_t adjustedWakeupRelativeTime = ptru->GetAdjustedWakeupTimeJST();
				time_t tvProgramBeginTime = ptru->GetTvProgramBeginTime();
				time_t originalWakeup = ptru->GetOriginalWakeupTimeJST();
				localtime_s(&tmtvProgramBeginTime, &tvProgramBeginTime);
				localtime_s(&tmResume, &adjustedWakeupRelativeTime);
				localtime_s(&tmOriginalWakeup, &originalWakeup);
				info(MRID_ADJUSTED_SCHEDULE, TEXT(
					"ACPI Wake Alarm対策用待機可能タイマーを設定しました。\n"
					"補正済: %04d/%02d/%02d %02d:%02d:%02d(%d秒補正)\n"
					"補正前: %04d/%02d/%02d %02d:%02d:%02d(録画%u分前復帰, 録画時間デフォルト%u秒)\n"
					"番組開始時刻: %04d/%02d/%02d %02d:%02d:%02d\n"
					"TITLE : %s\n"
					"このタイマーは理由の如何によらずレジューム時に削除されます。"),
					EXPAND_TM(tmResume),
					ptru->GetAdjustedSec(),
					EXPAND_TM(tmOriginalWakeup),
					(DWORD)(ptru->GetTvRockWakeupBeforeRecordSec() / 60), ptru->GetTvRockDefaultRecordSec(),
					EXPAND_TM(tmtvProgramBeginTime),
					ptru->GetTitle());
			}
		}
		else {
			error(L"CreateWaitableTimer fail(0x%x)", GetLastError());
		}
	}
	return hWaitableTimer;
}
bool PowerEventHandler::Run()
{
	ReportStartPending();

	HANDLE hWaitableTimer = NULL;
	HANDLE handles[3]{ m_events.hEnd, m_events.hToSleep, m_events.hWakeUp };
	DWORD dwRet;
	bool bret = false;
	TvRockUtil tru(m_pLog, &g_opt.m_TvRockUtilOpt);
	ReportRunning();
	info(MRID_PROC_START, L"作業を開始しました");
	DWORD dwWait = tru.IsPollingMode() == false ? INFINITE : g_opt.m_dwPollingIntervalMinutes * 60 * 1000;
	tru.Poll();
	while (WAIT_FAILED != (dwRet = WaitForMultipleObjects(3, handles, FALSE, dwWait))) {
		if (dwRet == WAIT_TIMEOUT) {
			tru.Poll();
		}
		if (dwRet == WAIT_OBJECT_0) {
			ReportStopPending();
			bret = true;
			break;
		}
		if (dwRet == WAIT_OBJECT_0 + 1) {
			ResetEvent(m_events.hToSleep);
			hWaitableTimer = ToSleep(&tru, hWaitableTimer);
		}
		if (dwRet == WAIT_OBJECT_0 + 2) {
			ResetEvent(m_events.hWakeUp);
			if (hWaitableTimer != NULL) {
				CancelWaitableTimer(hWaitableTimer);
				CloseHandle(hWaitableTimer);
				hWaitableTimer = NULL;
			}
		}
	}
	DWORD dwErr = 0;
	if (dwRet == WAIT_FAILED) {
		dwErr = GetLastError();
		error(L"カーネルオブジェクト待機異常 0x%x", dwErr);
	}
	else {
		ReportStopPending();
	}

	if (hWaitableTimer != NULL) {
		CancelWaitableTimer(hWaitableTimer);
		CloseHandle(hWaitableTimer);
		hWaitableTimer = NULL;
	}
	info(MRID_PROC_STOP, L"作業を終了しました");
	if (dwErr) {
		SetLastError(dwErr);
	}
	return bret;
}
unsigned __stdcall PowerEventHandler::ThreadMain(void* pArguments)
{
	PowerEventHandler* myself = (PowerEventHandler*)pArguments;
	myself->Run();
	return 0;
}

BOOL PowerEventHandler::Dispatch(WPARAM w)
{
	if (w == PBT_APMSUSPEND || w == PBT_APMSTANDBY) {
		if (m_events.hToSleep) {
			if (FALSE == SetEvent(m_events.hToSleep)) {
				error(L"SetEvent fail(0x%x)", GetLastError());
			}
		}
		return TRUE;
	}
	else if(w== PBT_APMRESUMEAUTOMATIC){
		if (m_events.hWakeUp) {
			if (FALSE == SetEvent(m_events.hWakeUp)) {
				error(L"SetEvent fail(0x%x)", GetLastError());
			}
		}
		return TRUE;
	}
	return FALSE;
}
