// TSAfAWA.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "framework.h"
#include "EventLog.h"
#include "Service.h"
#include "DebugMain.h"
#include "utils.h"
#include "TSAfAWA.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    int argc;
    LPWSTR* argv;
    bool bGUIMode = false;
    bool bDebugMode = false;

    argv = CommandLineToArgvW(GetCommandLineW(), &(argc));
    for (int i = 1; i < argc; i++) {
        if (((*argv[i] == L'-') || (*argv[i] == L'/'))) {
            if (lstrcmpi(L"install", argv[i] + 1) == 0) {
                if (g_log.Install() == false) {
                    PutConsole(L"message file install faiure(0x%x)\n", GetLastError());
                }
                Service::Install();
                exit(0);
            }
            else if (lstrcmpi(L"remove", argv[i] + 1) == 0
                || lstrcmpi(L"uninstall", argv[i] + 1) == 0) {
                Service::Uninstall();
                if (g_log.Uninstall() == false) {
                    PutConsole(L"message file uninstall faiure(0x%x)\n", GetLastError());
                }
                exit(0);
            }
            else if (lstrcmpi(L"debug", argv[i] + 1) == 0) {
                bDebugMode = true;
                bGUIMode = true;
            }
            else if (lstrcmpi(L"gui", argv[i] + 1) == 0 || lstrcmpi(L"desktop", argv[i] + 1) == 0) {
                bGUIMode = true;
            }
            else if (lstrcmpi(L"poll", argv[i] + 1) == 0
                || lstrcmpi(L"polling", argv[i] + 1) == 0
                || lstrcmpi(L"pollingmode", argv[i] + 1) == 0) {

                g_opt.m_TvRockUtilOpt.m_bIsPollingMode = true;
            }
            else if (lstrcmpi(L"pollinterval", argv[i] + 1) == 0
                || lstrcmpi(L"pollintervalminute", argv[i] + 1) == 0) {
                i++;
                if (i >= argc) {
                    PutConsole(L"Polling interval need more argument.\n");
                    exit(1);
                }
                if (iswdigits(argv[i])==FALSE) {
                    PutConsole(L"invalid argument.\n");
                    exit(1);
                }
                int dwMinute = _wtoi(argv[i]);
                if (dwMinute > 0) {
                    g_opt.m_dwPollingIntervalMinutes = (DWORD)dwMinute;
                }
            }
            else if (lstrcmpi(L"adjusttreshold", argv[i] + 1) == 0) {
                i++;
                if (i >= argc) {
                    PutConsole(L"Adjust treshold need more argument.\n");
                    exit(1);
                } 
                if (iswfloatDigits(argv[i]) == FALSE) {
                    PutConsole(L"invalid argument.\n");
                    exit(1);
                }
                double dbl = _wtof(argv[i]);
                if (dbl < 1.0) {
                    PutConsole(L"Adjust treshold must be more than an hour.\n");
                    exit(1);
                }
                g_opt.m_TvRockUtilOpt.m_dblAdjustTresholdHour = dbl;
            }
        }
        else {
            PutConsole(L"invalid arguments.\n");
            exit(1);
        }
    }
    if (bGUIMode) {
        int showmode = bDebugMode == true ? nCmdShow : SW_HIDE;
        return DebugMain(hInstance, hPrevInstance, lpCmdLine, showmode);
    }
    else {
        if (!Service::StartDispatch()) {
            g_log.error(L"StartServiceCtrlDispatcher failed.0x%x", GetLastError());
        }
    }
    return 0;
}