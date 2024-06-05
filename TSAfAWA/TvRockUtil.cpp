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
    // �V�X�e���ݒ�
    // -> TvRock��ƃt�H���_
    BYTE* data = (BYTE*)m_tvrockRegistorySetting.DOCUMENT;
    DWORD cbData = (DWORD)(MAX_PATH - sizeof(WCHAR) * TVROCKSCHLEN);
    DWORD dwResult = RegQueryValueEx(hKey, L"DOCUMENT", NULL, &type, data, &cbData);
    if (dwResult != ERROR_SUCCESS || type!=REG_SZ) {
        RegCloseKey(hKey);
        return false;
    }
    // �V�X�e���ݒ�
    // -> �R���s���[�^�ݒ�O���[�v
    // -> �X���[�v���A���ԃv���_�E��
    data = (BYTE*)&m_tvrockRegistorySetting.WAKEUP;
    cbData = sizeof(DWORD);
    dwResult = RegQueryValueEx(hKey, L"WAKEUP", NULL, &type, data, &cbData);
    if (dwResult != ERROR_SUCCESS || type != REG_DWORD) {
        RegCloseKey(hKey);
        return false;
    }

    // TvRock�ݒ�
    // -> �^���{�ݒ�^�u
    // -> �\��o�^�f�t�H���g�l�O���[�v
    // -> �~���J�n���ԃf�t�H���g (�b)
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
        m_plog->error(MRID_CANNOT_OPEN_TVROCK_SCH_FILE, L"%s���J���܂���(0x%x)", m_scheduleFileName, GetLastError());
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
        m_plog->error(L"%s�ɂ��100���K�V���b�N���󂯂܂����B�P�ނ��܂��B", m_scheduleFileName);
        CloseHandle(hFile);
        return false;
    }
    m_previousFileStat.size = fsize;
    if (fsize == 0) {
        CloseHandle(hFile);
#if _DEBUG
        m_plog->warn(L"%s�̃T�C�Y���[��", m_scheduleFileName);
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
    schitem_t nearestschitem{};  //���߂̖����̘^��\��X�P�W���[��

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

                            if (schitem.start > nowt) { //����
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
                            /// tvrock.sch�̍ŏI�s��SPC1�����̂悤��
                            m_plog->warn(MRID_INVALID_TVROCK_SCH_FILE, L"TvRock.sch���j�����Ă���\��������܂��B");
                            return false;
                        }
                    }
                }
                else if (tokenindex == 1) {
                    if (!isupper(*(buf + i))) {
                        m_plog->warn(MRID_INVALID_TVROCK_SCH_FILE, L"TvRock.sch���j�����Ă���\��������܂��B");
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
        m_plog->error(MRID_CANNOT_ACCESS_TVROCK_REGISTORY, L"TvRock�̃��W�X�g���ݒ�ɃA�N�Z�X�ł��܂���");
        return false;
    }
    BuildTvRockSchFileName();
    if (this->IsPollingMode()) {
        DWORD dwErr;
        if (false == IsFileModified(m_scheduleFileName, m_previousFileStat.size, &m_previousFileStat.lastWriteTime, dwErr)) {
            if (dwErr != 0) {
                return false;
            }
            // tvrock.sch�͕ύX����Ă��Ȃ�
            return true;
        }
    }
    if (ParseTvrockSchFile(nowt) == false) {
        return false;
    }
    return true;
}

/// <summary>
/// x���X���[�v���畜�A���鎞���܂ł̕b���Ay��x���b���Ƃ���ƁA�x�����Ԃ��v�Z���鎮�͂�������y=0.008x-27.5���炢
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
    if (diffsec > (time_t)(m_opt.m_dblAdjustTresholdHour * 60 * 60)) { //�^��\�肪3.5���Ԉȏ㖢��(�������ɃX���[�v�����ꍇ��ACPI wake alarm�f�o�C�X�ɂ��wakeup�ƂȂ���x�ɖ���)
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
