#pragma once
#include "framework.h"
#include "EventLog.h"
#include "utils.h"

struct TvRockUtilOpt_t {
	/// <summary>
	/// 定期的にtvrock.schを監視しスケジュールを抽出しておくモードの場合true
	/// (2秒以内に仕事が終わらない機体向け)
	/// </summary>
	bool m_bIsPollingMode{ false };
	/// <summary>
	/// 遅延補正最低時間(単位:時)
	/// この時間よりもスリープ時間が長い場合に補正済時間でレジューム時間をセットします
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
	/// HKCU\Software\TvRock の値
	/// </summary>
	struct TvRockReg_t {
		DWORD DEFREC{}; //TvRock設定 -> 録画基本設定タブ  -> 予約登録デフォルト値グループ -> 録画開始時間デフォルト (秒)
		DWORD WAKEUP{};	//システム設定 -> コンピュータ設定グループ -> スリープ復帰時間プルダウン
		wchar_t DOCUMENT[MAX_PATH + 1]{};	//TvRock作業フォルダ HKCU\Software\TvRock\DOCUMENT の値
	}m_tvrockRegistorySetting{};
	static const SSIZE_T TARGET_TITLE_BUF_LEN{ 1024 };	/// タイトル名最大長。数値に特に意味はない
	const wchar_t* REGSUBKEY{ L"Software\\TvRock" };
	const wchar_t* TVROCKSCH = L"tvrock.sch";
	const SSIZE_T TVROCKSCHLEN{ 20 };
	wchar_t m_scheduleFileName[MAX_PATH + 1]{};	/// tvrock.sch fullpath名

	EventLog* m_plog;
	time_t m_adjustedSec{};				///ACPI Wake Alarm対策のために前倒して復帰すべき秒数(ACPI Wake Alarmによる遅れを予測した秒数)
	wchar_t m_wcszTitle[TARGET_TITLE_BUF_LEN + 1]{};	/// 復帰する対象となった録画予約の番組名
	time_t m_nearest_tv_program_schedule_time_JST{};	/// TvRock.schより抽出した直近の未来かつACPI Wake Alarmの対象となった録画予約の番組が開始するunix epoch時刻
	ULONGLONG m_ullAdjustedWakeupRelativeTime100ns{};	/// SetWaitableTimerへ与える値(レジュームすべき補正後の現時刻からの相対100ナノ秒。負数)
	time_t m_llAdjustedWakeupTimeJST{};	///補正後のレジューム時刻JST(ACPI Wake Alarmによって遅れることを前提にしたunix epoch時刻)

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
	bool GetLastestFromList(time_t nowt)	// ポーリングモード時にリストから直近のスケジュールを選択する
	{
		if (m_opt.m_bIsPollingMode == false) {
			return false;
		}
		bool bret = false;
		m_wcszTitle[0] = 0;
		m_nearest_tv_program_schedule_time_JST = 0;
		for (int i = 0; i < m_schedules.size(); i++) {// m_schedulesはソート済みであること
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

