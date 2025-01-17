#pragma once
#include "TvRockUtil.h"
#include "EventLog.h"
#define APPNAME L"TSAfAWA"
struct globalOptions_t
{
	TvRockUtilOpt_t m_TvRockUtilOpt{};
	/// <summary>
	/// m_bIsPollingModeがtrueの場合にtvrock.schの更新を監視する間隔(単位:分)
	/// </summary>
	DWORD m_dwPollingIntervalMinutes{ 5 };

};

extern globalOptions_t g_opt;
extern EventLog g_log;