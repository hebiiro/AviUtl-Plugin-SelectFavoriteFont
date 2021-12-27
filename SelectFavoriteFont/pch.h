#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")
#include <uxtheme.h>
#pragma comment(lib, "uxtheme.lib")

#include <tchar.h>
#include <crtdbg.h>
#include <strsafe.h>
#include <locale.h>

#include <string>
#include <memory>
#include <map>

#include <comdef.h>
#import <msxml3.dll>

#include "aviutl_plugin_sdk/filter.h"
#include "../Common/MyTracer.h"
#include "../Common/MyMSXML.h"

//---------------------------------------------------------------------

#ifdef UNICODE
	typedef std::wstring tstring;
#else	// ifdef UNICODE else
	typedef std::string tstring;
#endif	// ifdef UNICODE

//---------------------------------------------------------------------
