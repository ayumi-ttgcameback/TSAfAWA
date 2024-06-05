#pragma once
#include "TvRockUtil.h"
#include "EventLog.h"
#define APPNAME L"TSAfAWA"
struct globalOptions_t
{
	TvRockUtilOpt_t m_TvRockUtilOpt{};
	/// <summary>
	/// m_bIsPollingMode‚ªtrue‚Ìê‡‚Étvrock.sch‚ÌXV‚ğŠÄ‹‚·‚éŠÔŠu(’PˆÊ:•ª)
	/// </summary>
	DWORD m_dwPollingIntervalMinutes{ 5 };

};

extern globalOptions_t g_opt;
extern EventLog g_log;