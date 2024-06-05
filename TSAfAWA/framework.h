// header.h : 標準のシステム インクルード ファイルのインクルード ファイル、
// またはプロジェクト専用のインクルード ファイル
//

#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Windows ヘッダーからほとんど使用されていない部分を除外する
// Windows ヘッダー ファイル
#include <windows.h>
#include <shellapi.h>
// C ランタイム ヘッダー ファイル
#include <stdlib.h>
#include <time.h>
#include <tchar.h>
#include <process.h>
#include <strsafe.h>
#include "MessageResourceFile.h"
#include <vector>
#include <string>
#include <algorithm>