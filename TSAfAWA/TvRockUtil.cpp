#include "framework.h"
#include "EventLog.h"
#include "TvRockUtil.h"
#include "utils.h"

bool TvRockUtil::GetTvRockRegistoryValues()
{
    memset(&m_tvrockRegistorySetting, 0, sizeof(m_tvrockRegistorySetting));
    HKEY hKey;
    LSTATUS lResult = RegOpenKeyEx(HKEY_CURRENT_USER, REGSUBKEY, 0, KEY_QUERY_VALUE, &hKey);
    if (lResult != ERROR_SUCCESS) {
        return false;
    }
    DWORD type;
    // システム設定
    // -> TvRock作業フォルダ
    BYTE* data = (BYTE*)m_tvrockRegistorySetting.DOCUMENT;
    DWORD cbData = (DWORD)(MAX_PATH - sizeof(WCHAR) * TVROCKSCHLEN);
    DWORD dwResult = RegQueryValueEx(hKey, L"DOCUMENT", NULL, &type, data, &cbData);
    if (dwResult != ERROR_SUCCESS || type!=REG_SZ) {
        RegCloseKey(hKey);
        return false;
    }
    // システム設定
    // -> コンピュータ設定グループ
    // -> スリープ復帰時間プルダウン
    data = (BYTE*)&m_tvrockRegistorySetting.WAKEUP;
    cbData = sizeof(DWORD);
    dwResult = RegQueryValueEx(hKey, L"WAKEUP", NULL, &type, data, &cbData);
    if (dwResult != ERROR_SUCCESS || type != REG_DWORD) {
        RegCloseKey(hKey);
        return false;
    }

    // TvRock設定
    // -> 録画基本設定タブ
    // -> 予約登録デフォルト値グループ
    // -> 欲が開始時間デフォルト (秒)
    data = (BYTE*)&m_tvrockRegistorySetting.DEFREC;
    cbData = sizeof(DWORD);
    dwResult = RegQueryValueEx(hKey, L"DEFREC", NULL, &type, data, &cbData);
    if (dwResult != ERROR_SUCCESS || type != REG_DWORD) {
        RegCloseKey(hKey);
        return false;
    }

    RegCloseKey(hKey);
    return true;
}

bool TvRockUtil::ParseTvrockSchFile(time_t nowt)
{
    if (m_schedules.size()) {
        m_schedules.clear();
    }

    HANDLE hFile = CreateFile(m_scheduleFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        m_plog->error(MRID_CANNOT_OPEN_TVROCK_SCH_FILE, L"%sを開けません(0x%x)", m_scheduleFileName, GetLastError());
        return false;
    }
    DWORD dwHigh = 0;
    DWORD fsize = GetFileSize(hFile, &dwHigh);
    if (INVALID_FILE_SIZE == fsize) {
        m_plog->error(L"GetFileSize failure(0x%x)", GetLastError());
        CloseHandle(hFile);
        return false;
    }
    if (dwHigh != 0 || fsize >= 100 * 1024 * 1024) {
        m_plog->error(L"%sにより100メガショックを受けました。撤退します。", m_scheduleFileName);
        CloseHandle(hFile);
        return false;
    }
    m_previousFileStat.size = fsize;
    if (fsize == 0) {
        CloseHandle(hFile);
#if _DEBUG
        m_plog->warn(L"%sのサイズがゼロ", m_scheduleFileName);
#endif
        return false;
    }
    GetFileTime(hFile, NULL, NULL, &m_previousFileStat.lastWriteTime);
    char* buf = (char*)VirtualAlloc(NULL, (SIZE_T)(fsize + 1UL), MEM_COMMIT, PAGE_READWRITE);
    if (!buf) {
        m_plog->error(L"VirtualAlloc failure(0x%x)", GetLastError());
        CloseHandle(hFile);
        return false;
    }
    DWORD dwRead = 0;
    BOOL bret = ReadFile(hFile, buf, fsize, &dwRead, NULL);
    if (bret == FALSE) {
        m_plog->error(L"ReadFile failure(0x%x)", GetLastError());
        CloseHandle(hFile);
        return false;
    }
    CloseHandle(hFile);

    int tokenindex = 0;
    char* token1 = NULL;
    char* token2 = NULL;
    char* token3 = NULL;
    //size_t token1len = 0;
    size_t token2len = 0;
    //size_t token3len = 0;
    char* spos = buf;

    schitem_t schitem{};
    schitem_t nearestschitem{};  //直近の未来の録画予定スケジュール

    if (bret && fsize == dwRead) {
        for (DWORD i = 0; i < fsize + 1; i++) {
            if (*(buf + i) == ' ') {
                if (tokenindex == 0) {
                    buf[i] = 0;
                    token1 = spos;
                    //token1len = i;
                    spos = &buf[i + 1];
                    if (isdigits(token1) == FALSE) {
                        tokenindex = -1;
                    }
                    else {
                        tokenindex++;
                    }
                }
                else if (tokenindex == 1) {
                    buf[i] = 0;
                    token2 = spos;
                    token2len = &buf[i] - spos;
                    spos = &buf[i + 1];
                    tokenindex++;
                }
            }
            else if (*(buf + i) == '\n' || i == fsize || *(buf + i) == '\r') {
                buf[i] = 0;
                if (tokenindex == 2) {
                    token3 = spos;
                    if (strncmp(token2, "START", token2len) == 0) {
                        schitem.index = atoi(token1);
                        schitem.start = strtoull(token3, NULL, 10);
                    }
                    else if (strncmp(token2, "VALIDATE", token2len) == 0) {
                        schitem.validate = atoi(token3);
                    }
                    else if (strncmp(token2, "TITLE", token2len) == 0) {
                        if (schitem.validate == 1) {
                            schitem.title = token3;
                            schitem.titlelen = ((buf + i) - spos);

                            if (schitem.start > nowt) { //未来
                                if (m_opt.m_bIsPollingMode) {
                                    scheduleItem_t item(&schitem);
                                    m_schedules.push_back(item);
                                }
                                if (nearestschitem.start > schitem.start || nearestschitem.start == 0) {
                                    nearestschitem = schitem;
                                }
                            }
                        }
                        memset(&schitem, 0, sizeof(schitem));
                    }
                }
                spos = &buf[i + 1];
                tokenindex = 0;
            }
            else {
                if (tokenindex == 0) {
                    if (!isdigit(*(buf + i))) {
                        if (fsize - 1 != i) {
                            /// tvrock.schの最終行はSPC1文字のようだ
                            m_plog->warn(MRID_INVALID_TVROCK_SCH_FILE, L"TvRock.schが破損している可能性があります。");
                            return false;
                        }
                    }
                }
                else if (tokenindex == 1) {
                    if (!isupper(*(buf + i))) {
                        m_plog->warn(MRID_INVALID_TVROCK_SCH_FILE, L"TvRock.schが破損している可能性があります。");
                        return false;
                    }
                }
            }
        }
    }
    if (m_opt.m_bIsPollingMode == false) {
        if (nearestschitem.title != NULL) {
            sjis2wcs(nearestschitem.title, m_wcszTitle, TARGET_TITLE_BUF_LEN);
            nearestschitem.title = NULL;
        }
        m_nearest_tv_program_schedule_time_JST = nearestschitem.start;
    }
    else {
        std::sort(
            m_schedules.begin(),
            m_schedules.end(),
            [](scheduleItem_t const& a, scheduleItem_t const& b) { return a.start < b.start; });
    }

    VirtualFree(buf, 0, MEM_RELEASE);
    return nearestschitem.start != 0;
}

bool TvRockUtil::GetLastestFromFile(time_t nowt)
{
    if (GetTvRockRegistoryValues() == false) {
        m_plog->error(MRID_CANNOT_ACCESS_TVROCK_REGISTORY, L"TvRockのレジストリ設定にアクセスできません");
        return false;
    }
    BuildTvRockSchFileName();
    if (this->IsPollingMode()) {
        DWORD dwErr;
        if (false == IsFileModified(m_scheduleFileName, m_previousFileStat.size, &m_previousFileStat.lastWriteTime, dwErr)) {
            if (dwErr != 0) {
                return false;
            }
            // tvrock.schは変更されていない
            return true;
        }
    }
    if (ParseTvrockSchFile(nowt) == false) {
        return false;
    }
    return true;
}

/// <summary>
/// xをスリープから復帰する時刻までの秒数、yを遅延秒数とすると、遅延時間を計算する式はだいたいy=0.008x-27.5くらい
/// </summary>
bool TvRockUtil::Build()
{
    m_adjustedSec = 0;
    time_t nowt = time(NULL);
    bool bret;
    if (m_opt.m_bIsPollingMode == false) {
        bret = GetLastestFromFile(nowt);
    }
    else {
        bret = GetLastestFromList(nowt);
    }
    if (bret == false) {
        return false;
    }
    m_llAdjustedWakeupTimeJST = m_nearest_tv_program_schedule_time_JST
        - m_tvrockRegistorySetting.DEFREC
        - m_tvrockRegistorySetting.WAKEUP;

    const time_t diffsec = m_llAdjustedWakeupTimeJST - nowt;
    if (diffsec > (time_t)(m_opt.m_dblAdjustTresholdHour * 60 * 60)) { //録画予定が3.5時間以上未来(今直ちにスリープした場合にACPI wake alarmデバイスによるwakeupとなる程度に未来)
        double dblAdjustedSec = (0.00807 * (double)diffsec - 27.5);
        if (dblAdjustedSec > 0.0) {
            m_adjustedSec = (time_t)dblAdjustedSec;
            m_llAdjustedWakeupTimeJST -= m_adjustedSec;
            m_ullAdjustedWakeupRelativeTime100ns = (diffsec- m_adjustedSec) * -10000000LL;
            return true;
        }
    }
    return false;
}
