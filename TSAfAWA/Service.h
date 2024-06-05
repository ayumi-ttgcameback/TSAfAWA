#pragma once
#include "TSAfAWA.h"
#include "PowerEventHandler.h"
#include "eventlog.h"
class Service : PowerEventHandler
{
	// internal variables
    DWORD m_dwCheckPoint{ 1 };
    SERVICE_STATUS_HANDLE m_hServiceStatus{}; ///ServiceMainの自動変数なので寿命に注意
	DWORD dwErr = 0;
public: 
    Service(EventLog* plog): PowerEventHandler(plog) 
    {
    }
    static constexpr const wchar_t* SZSERVICENAME = APPNAME;
    static constexpr const wchar_t* SZSERVICEDISPLAYNAME = L"TvRock復帰時刻補正サービス";
    static constexpr const wchar_t* SERVICEDESC = L"TvRockによって3.5時間以上スリープする際の復帰時刻を補正します。TvRockの設定が保存されているレジストリやスケジュールファイルにアクセスするため、TvRockを実行しているユーザアカウントで実行してください。";
    static constexpr const wchar_t* SZDEPENDENCIES = L"";// list of service dependencies - "dep1\0dep2\0\0"

    static void  WINAPI ServiceMain(DWORD dwArgc, LPWSTR* lpszArgv);
    static DWORD WINAPI ServiceCtrl(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);
	BOOL ReportServiceStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode = NO_ERROR, DWORD dwWaitHint=5000);

    void ReportStartPending() override {
        ReportServiceStatus(SERVICE_START_PENDING);
    }
    void ReportStopPending() override {
        ReportServiceStatus(SERVICE_STOP_PENDING);
    }
    void ReportRunning() override {
        ReportServiceStatus(SERVICE_RUNNING, NO_ERROR, 0);
    }
    void ReportStop(DWORD dwErr) override {
        ReportServiceStatus(SERVICE_STOPPED, dwErr, 0);
    }
    void ReportStopSuccess()  {
        ReportStop(NO_ERROR);
    }
    void ReportStopFailure()  {
        ReportStop(GetLastError());
    }
    void ReportStopFailure(DWORD dwErr) {
        ReportStop(dwErr);
    }
    static bool Install();
    static bool Uninstall();
    void Run();
    static BOOL StartDispatch()
    {
        SERVICE_TABLE_ENTRY dispatchTable[] 
        {
           {  (wchar_t*)SZSERVICENAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
           { NULL, NULL}
        };
        return StartServiceCtrlDispatcher(dispatchTable);
    }
};