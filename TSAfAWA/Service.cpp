#include "framework.h"
#include "EventLog.h"
#include "Service.h"
#include "utils.h"

extern EventLog g_log;

void WINAPI Service::ServiceMain(DWORD dwArgc, LPWSTR* lpszArgv)
{
    Service myself(&g_log);

    myself.m_hServiceStatus = RegisterServiceCtrlHandlerEx((myself.SZSERVICENAME), ServiceCtrl, &myself);

    if (!myself.m_hServiceStatus) {
        myself.error(L"RegisterServiceCtrlHandlerEx failure 0x%x",GetLastError());
        return;
    }

    myself.Run();
    return;
}

DWORD WINAPI Service::ServiceCtrl(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
    Service* pService = (Service*)lpContext;
    switch (dwControl)
    {
    case SERVICE_CONTROL_STOP:
        pService->ReportStopPending();
        pService->StopRequest();
        return NO_ERROR;

    case SERVICE_CONTROL_INTERROGATE:
        break;

    case SERVICE_CONTROL_TIMECHANGE:
    case SERVICE_CONTROL_POWEREVENT:
        pService->Dispatch(dwEventType);
        return NO_ERROR;

    default:
        break;
    }
    return NO_ERROR;
}

void Service::Run()
{
    ReportStartPending();
    if (BuildEvents() == false) {
        ReportStopFailure();
        return;
    }
    DWORD dwErr = 0;
    if (PowerEventHandler::Run() == false) {
        dwErr = GetLastError();
    }
    CloseEvents();

    ReportStop(dwErr);
}

BOOL Service::ReportServiceStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
    SERVICE_STATUS sSeriveStatus{};
    BOOL fResult = TRUE;

    if (dwCurrentState == SERVICE_START_PENDING) {
        sSeriveStatus.dwControlsAccepted = 0;
    }
    else {
        sSeriveStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
        if (dwCurrentState== SERVICE_RUNNING) {
            sSeriveStatus.dwControlsAccepted |= SERVICE_ACCEPT_POWEREVENT | SERVICE_ACCEPT_TIMECHANGE;
        }
    }

    sSeriveStatus.dwCurrentState = dwCurrentState;
    sSeriveStatus.dwWin32ExitCode = dwWin32ExitCode;
    sSeriveStatus.dwWaitHint = dwWaitHint;
    sSeriveStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    sSeriveStatus.dwServiceSpecificExitCode = 0;

    if ((dwCurrentState == SERVICE_RUNNING) ||
        (dwCurrentState == SERVICE_STOPPED) ||
        (dwCurrentState == SERVICE_PAUSED)) {
        sSeriveStatus.dwCheckPoint = 0;
        m_dwCheckPoint = 1;
    }
    else {
        sSeriveStatus.dwCheckPoint = m_dwCheckPoint++;
    }
    fResult = SetServiceStatus(m_hServiceStatus, &sSeriveStatus);
    if (fResult == FALSE) {
        error(L"SetServiceStatus dwCurrentState=%u, 0x%x threadId=%u", dwCurrentState, GetLastError(), GetCurrentThreadId());
    }
    return fResult;
}

bool Service::Install()
{
    bool bret = false;
    SC_HANDLE   schService{};
    SC_HANDLE   schSCManager{};
    SERVICE_DESCRIPTION sdesc{};

    TCHAR szPath[MAX_PATH + 1]{};

    if (GetModuleFileName(NULL, szPath, MAX_PATH) == 0) {
        PutConsole(L"GetModuleFileName failed - 0x%x\n", GetLastError());
        return false;
    }

    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE);
    if (schSCManager) {
        schService = CreateService(
            schSCManager,               // SCManager database
            (SZSERVICENAME),        // name of service
            (SZSERVICEDISPLAYNAME), // name to display
            SERVICE_QUERY_STATUS | SERVICE_CHANGE_CONFIG,         // desired access
            SERVICE_WIN32_OWN_PROCESS,  // service type
            SERVICE_DEMAND_START,       // start type
            SERVICE_ERROR_NORMAL,       // error control type
            szPath,                     // service's binary
            NULL,                       // no load ordering group
            NULL,                       // no tag identifier
            (SZDEPENDENCIES),       // dependencies
            NULL,                       // LocalSystem account
            NULL);                      // no password

        if (schService) {
            sdesc.lpDescription = (wchar_t*)SERVICEDESC;
            ChangeServiceConfig2(schService, SERVICE_CONFIG_DESCRIPTION, &sdesc);

            PutConsole(L"installed\n");
            bret = true;
            CloseServiceHandle(schService);
        }
        else {
            PutConsole(L"CreateService failed - 0x%x\n", GetLastError());
        }

        CloseServiceHandle(schSCManager);
    }
    else{
        PutConsole(L"OpenSCManager failed - 0x%x\n", GetLastError());
    }
    return bret;
}

bool Service::Uninstall()
{
    bool bret = false;
    SC_HANDLE schService;
    SC_HANDLE schSCManager;
    SERVICE_STATUS ss{};

    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (schSCManager) {
        schService = OpenService(schSCManager, SZSERVICENAME, DELETE | SERVICE_STOP | SERVICE_QUERY_STATUS);
        if (schService) {
            if (ControlService(schService, SERVICE_CONTROL_STOP, &ss)) {
                PutConsole(L"Stopping %s.\n", SZSERVICEDISPLAYNAME);
                Sleep(1000);

                (void)QueryServiceStatus(schService, &ss);

                if (ss.dwCurrentState != SERVICE_STOPPED) {
                    PutConsole(L"%s failed to stop\n", SZSERVICEDISPLAYNAME);
                }
            }

            if (DeleteService(schService)) {
                PutConsole(L"Service uninstalled\n");
                bret = true;
            }
            else {
                PutConsole(L"DeleteService failed - 0x%x\n", GetLastError());
            }

            CloseServiceHandle(schService);
        }
        else {
            PutConsole(L"OpenService failed - 0x%x\n", GetLastError());
        }
        CloseServiceHandle(schSCManager);
    }
    else {
        PutConsole(L"OpenSCManager failed - 0x%x\n", GetLastError());
    }
    return bret;
}