#pragma once
#include "TSAfAWA.h"
#include "PowerEventHandler.h"
#include "eventlog.h"
class Service : PowerEventHandler
{
	// internal variables
    DWORD m_dwCheckPoint{ 1 };
    SERVICE_STATUS_HANDLE m_hServiceStatus{}; ///ServiceMain�̎����ϐ��Ȃ̂Ŏ����ɒ���
	DWORD dwErr = 0;
public: 
    Service(EventLog* plog): PowerEventHandler(plog) 
    {
    }
    static constexpr const wchar_t* SZSERVICENAME = APPNAME;
    static constexpr const wchar_t* SZSERVICEDISPLAYNAME = L"TvRock���A�����␳�T�[�r�X";
    static constexpr const wchar_t* SERVICEDESC = L"TvRock�ɂ����3.5���Ԉȏ�X���[�v����ۂ̕��A������␳���܂��BTvRock�̐ݒ肪�ۑ�����Ă��郌�W�X�g����X�P�W���[���t�@�C���ɃA�N�Z�X���邽�߁ATvRock�����s���Ă��郆�[�U�A�J�E���g�Ŏ��s���Ă��������B";
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