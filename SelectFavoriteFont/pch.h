#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

#include <tchar.h>
#include <crtdbg.h>
#include <strsafe.h>
#include <locale.h>

#include <string>
#include <iostream>
#include <fstream>

#include "aviutl_plugin_sdk/filter.h"

//---------------------------------------------------------------------

#ifdef UNICODE
	typedef std::wstring tstring;
#else	// ifdef UNICODE else
	typedef std::string tstring;
#endif	// ifdef UNICODE

//---------------------------------------------------------------------

#ifndef _DEBUG
	#define _RPTT0(msg)
	#define _RPTTN(msg, ...)
	#define _RPTFT0(msg)
	#define _RPTFTN(msg, ...)
#else	// ifndef _DEBUG else
	#ifdef UNICODE
		#define _RPTT0(msg)      _RPT_BASE_W(_CRT_WARN, NULL, 0, NULL, L"%ls", msg)
		#define _RPTTN(msg, ...) _RPT_BASE_W(_CRT_WARN, NULL, 0, NULL, msg, __VA_ARGS__)
		#define _RPTFT0(msg)      _RPT_BASE_W(_CRT_WARN, _CRT_WIDE(__FILE__), __LINE__, NULL, L"%ls", msg)
		#define _RPTFTN(msg, ...) _RPT_BASE_W(_CRT_WARN, _CRT_WIDE(__FILE__), __LINE__, NULL, msg, __VA_ARGS__)
	#else	// ifdef UNICODE else
		#define _RPTT0(msg)      _RPT_BASE(_CRT_WARN, NULL, 0, NULL, "%s", msg)
		#define _RPTTN(msg, ...) _RPT_BASE(_CRT_WARN, NULL, 0, NULL, msg, __VA_ARGS__)
		#define _RPTFT0(msg)      _RPT_BASE(_CRT_WARN, __FILE__, __LINE__, NULL, "%s", msg)
		#define _RPTFTN(msg, ...) _RPT_BASE(_CRT_WARN, __FILE__, __LINE__, NULL, msg, __VA_ARGS__)
	#endif	// ifdef UNICODE
#endif	// ifndef _DEBUG

#define _RPTF_NUM(x)		_RPTFTN(_T(#x) _T(" = %d\n"), x)
#define _RPTF_HEX(x)		_RPTFTN(_T(#x) _T(" = 0x%08X\n"), x)
#define _RPTF_STR(x)		_RPTFTN(_T(#x) _T(" = %s\n"), x)
#define _RPTF_WSTR(x)		_RPTFTN(_T(#x) _T(" = %ws\n"), x)

//---------------------------------------------------------------------
