#pragma once
#include "framework.h"
#include "EventLog.h"
#include "utils.h"

struct TvRockUtilOpt_t {
	/// <summary>
	/// ����I��tvrock.sch���Ď����X�P�W���[���𒊏o���Ă������[�h�̏ꍇtrue
	/// (2�b�ȓ��Ɏd�����I���Ȃ��@�̌���)
	/// </summary>
	bool m_bIsPollingMode{ false };
	/// <summary>
	/// �x���␳�Œ᎞��(�P��:��)
	/// ���̎��Ԃ����X���[�v���Ԃ������ꍇ�ɕ␳�ώ��ԂŃ��W���[�����Ԃ��Z�b�g���܂�
	/// </summary>
	double m_dblAdjustTresholdHour = { 3.5 };
};

class TvRockUtil 
{
private:
	struct schitem_t {
		int index;
		time_t start;
		int validate;
		char* title;
		size_t titlelen;
	};
	struct scheduleItem_t
	{
		scheduleItem_t(schitem_t*p) {
			start = p->start;
			title = p->title;
		}
		time_t start;
		std::string title;
	};
	std::vector< scheduleItem_t> m_schedules;
	TvRockUtilOpt_t m_opt{};

	struct filestat_t
	{
		DWORD size{};
		FILETIME lastWriteTime{};
	};
	filestat_t m_previousFileStat{};

	/// <summary>
	/// HKCU\Software\TvRock �̒l
	/// </summary>
	struct TvRockReg_t {
		DWORD DEFREC{}; //TvRock�ݒ� -> �^���{�ݒ�^�u  -> �\��o�^�f�t�H���g�l�O���[�v -> �^��J�n���ԃf�t�H���g (�b)
		DWORD WAKEUP{};	//�V�X�e���ݒ� -> �R���s���[�^�ݒ�O���[�v -> �X���[�v���A���ԃv���_�E��
		wchar_t DOCUMENT[MAX_PATH + 1]{};	//TvRock��ƃt�H���_ HKCU\Software\TvRock\DOCUMENT �̒l
	}m_tvrockRegistorySetting{};
	static const SSIZE_T TARGET_TITLE_BUF_LEN{ 1024 };	/// �^�C�g�����ő咷�B���l�ɓ��ɈӖ��͂Ȃ�
	const wchar_t* REGSUBKEY{ L"Software\\TvRock" };
	const wchar_t* TVROCKSCH = L"tvrock.sch";
	const SSIZE_T TVROCKSCHLEN{ 20 };
	wchar_t m_scheduleFileName[MAX_PATH + 1]{};	/// tvrock.sch fullpath��

	EventLog* m_plog;
	time_t m_adjustedSec{};				///ACPI Wake Alarm�΍�̂��߂ɑO�|���ĕ��A���ׂ��b��(ACPI Wake Alarm�ɂ��x���\�������b��)
	wchar_t m_wcszTitle[TARGET_TITLE_BUF_LEN + 1]{};	/// ���A����ΏۂƂȂ����^��\��̔ԑg��
	time_t m_nearest_tv_program_schedule_time_JST{};	/// TvRock.sch��蒊�o�������߂̖�������ACPI Wake Alarm�̑ΏۂƂȂ����^��\��̔ԑg���J�n����unix epoch����
	ULONGLONG m_ullAdjustedWakeupRelativeTime100ns{};	/// SetWaitableTimer�֗^����l(���W���[�����ׂ��␳��̌���������̑���100�i�m�b�B����)
	time_t m_llAdjustedWakeupTimeJST{};	///�␳��̃��W���[������JST(ACPI Wake Alarm�ɂ���Ēx��邱�Ƃ�O��ɂ���unix epoch����)

	bool GetTvRockRegistoryValues();
	void BuildTvRockSchFileName() {
		wcscpy_s(m_scheduleFileName, MAX_PATH - 1, m_tvrockRegistorySetting.DOCUMENT);
		if (m_scheduleFileName[0]) {
			size_t doclen = wcslen(m_tvrockRegistorySetting.DOCUMENT);
			if (doclen > 0) {
				if (m_tvrockRegistorySetting.DOCUMENT[doclen - 1] != L'\\') {
					wcscat_s(m_scheduleFileName, sizeof(m_scheduleFileName) / sizeof(WCHAR) - 1, L"\\");
				}
			}
		}
		wcscat_s(m_scheduleFileName, sizeof(m_scheduleFileName) / sizeof(WCHAR) - 1, TVROCKSCH);
	}
	bool ParseTvrockSchFile(time_t nowt);
	bool GetLastestFromFile(time_t nowt);	// 
	bool GetLastestFromList(time_t nowt)	// �|�[�����O���[�h���Ƀ��X�g���璼�߂̃X�P�W���[����I������
	{
		if (m_opt.m_bIsPollingMode == false) {
			return false;
		}
		bool bret = false;
		m_wcszTitle[0] = 0;
		m_nearest_tv_program_schedule_time_JST = 0;
		for (int i = 0; i < m_schedules.size(); i++) {// m_schedules�̓\�[�g�ς݂ł��邱��
			if (m_schedules[i].start > nowt) {
				m_nearest_tv_program_schedule_time_JST = m_schedules[i].start;
				sjis2wcs(m_schedules[i].title.c_str(), m_wcszTitle, TARGET_TITLE_BUF_LEN);
				bret = true;
				break;
			}
		}
		return bret;
	}
public:
	TvRockUtil(EventLog* plog, TvRockUtilOpt_t* popt) {
		m_plog = plog;
		m_opt = *popt;
	}

	DWORD GetTvRockWakeupBeforeRecordSec() const {
		return m_tvrockRegistorySetting.WAKEUP;
	}
	DWORD GetTvRockDefaultRecordSec() const {
		return m_tvrockRegistorySetting.DEFREC;
	}
	time_t GetAdjustedSec() const {
		return m_adjustedSec;
	}
	const wchar_t* GetTitle() const {
		return m_wcszTitle;
	}
	time_t GetOriginalWakeupTimeJST() const {
		return m_nearest_tv_program_schedule_time_JST
			- m_tvrockRegistorySetting.DEFREC
			- m_tvrockRegistorySetting.WAKEUP;
	}
	time_t GetTvProgramBeginTime() const
	{
		return m_nearest_tv_program_schedule_time_JST;
	}
	ULONGLONG GetAdjustedWakeupRelativeTime100ns() const {
		return m_ullAdjustedWakeupRelativeTime100ns;
	}
	time_t GetAdjustedWakeupTimeJST() const {
		return m_llAdjustedWakeupTimeJST;
	}
	
	bool Build();
	
	bool IsPollingMode() const {
		return m_opt.m_bIsPollingMode;
	}
	bool Poll()
	{
		if (m_opt.m_bIsPollingMode == false) {
			return false;
		}
		return GetLastestFromFile(time(NULL));
	}
};

