#include "pch.h"

//---------------------------------------------------------------------
//		フィルタ構造体のポインタを渡す関数
//---------------------------------------------------------------------
EXTERN_C FILTER_DLL __declspec(dllexport) * __stdcall GetFilterTable(void)
{
	static TCHAR g_filterName[] = TEXT("お気に入りフォント選択");
	static TCHAR g_filterInformation[] = TEXT("お気に入りフォント選択 version 1.0.0 by 蛇色");

	static FILTER_DLL g_filter =
	{
		FILTER_FLAG_NO_CONFIG |
		FILTER_FLAG_ALWAYS_ACTIVE |
		FILTER_FLAG_DISP_FILTER |
		FILTER_FLAG_EX_INFORMATION |
		0,
		0, 0,
		g_filterName,
		NULL, NULL, NULL,
		NULL, NULL,
		NULL, NULL, NULL,
		NULL,
		func_init,
		func_exit,
		NULL,
		NULL,
		NULL, NULL,
		NULL,
		NULL,
		g_filterInformation,
		NULL, NULL,
		NULL, NULL, NULL, NULL,
		NULL,
	};

	return &g_filter;
}

//---------------------------------------------------------------------

const UINT WM_HEBIIRO_SUBCLASS_FONT_COMBOBOX = ::RegisterWindowMessage(_T("WM_HEBIIRO_SUBCLASS_FONT_COMBOBOX"));
const UINT ID_FONT_COMBO_BOX = 0x5654;

HINSTANCE g_instance = 0; // この DLL のハンドル
TCHAR g_popularFileName[MAX_PATH] = {};
TCHAR g_favoriteFileName[MAX_PATH] = {};

HWND g_exeditObjectDialog = 0;
HWND g_fontComboBox = 0;
HHOOK g_hook = 0;
BOOL g_noNotify = FALSE;

HWND g_popular = 0;
HWND g_favorite = 0;

WNDPROC orig_popular_wndProc = 0;
WNDPROC orig_favorite_wndProc = 0;
WNDPROC orig_fontComboBox_wndProc = 0;
WNDPROC orig_exeditObjectDialog_wndProc = 0;

void readFile(HWND comboBox, LPCTSTR fileName);
void writeFile(HWND comboBox, LPCTSTR fileName);
int findString(HWND comboBox, LPCTSTR fontName);
void addFontName(HWND comboBox, LPCTSTR fontName);
int selectFontName(HWND comboBox, LPCTSTR fontName);
tstring getCurrentText(HWND comboBox);
void showControls(BOOL visible);
HWND createMyFontComboBox();
void subclassFontComboBox(HWND fontComboBox);

LRESULT CALLBACK cbtProc(int code, WPARAM wParam, LPARAM lParam);
void hook();
void unhook();

LRESULT CALLBACK hook_popular_wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK hook_favorite_wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK hook_fontComboBox_wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK hook_exeditObjectDialog_wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

//---------------------------------------------------------------------

void readFile(HWND comboBox, LPCTSTR fileName)
{
	_RPTFTN(_T("readFile(0x%08X, %s)\n"), comboBox, fileName);

	// 入力ストリームを開く。
	std::ifstream file(fileName);

	tstring line;
	while (std::getline(file, line)) // 1行ずつ読み込む。
	{
		// コンボボックスに読み込んだテキストを追加する。
		ComboBox_AddString(comboBox, line.c_str());
	}

	// 入力ストリームを閉じる。
	file.close();
}

void writeFile(HWND comboBox, LPCTSTR fileName)
{
	_RPTFTN(_T("writeFile(0x%08X, %s)\n"), comboBox, fileName);

	// 出力ストリームを開く。
	std::ofstream file(fileName);

	// コンボボックス内のフォント名を1個ずつ書き込んでいく。
	int count = ComboBox_GetCount(comboBox);
	for (int i = 0; i < count; i++)
	{
		// テキストの長さを取得する。
		int textLength = ComboBox_GetLBTextLen(comboBox, i);
		_RPTF_NUM(textLength);
		if (textLength <= 0) continue;

		// テキストを取得する。
		tstring text(textLength, _T('\0'));
		ComboBox_GetLBText(comboBox, i, &text[0]);
		_RPTF_STR(text.c_str());
		if (text.empty()) continue;

		// 出力ストリームにテキストを書き込む。
		file << text << _T("\n");
	}

	// 出力ストリームを閉じる。
	file.close();
}

// コンボボックスのリストから fontName を見つけてそのインデックスを返す。
// (CB_FINDSTRING は部分一致でもヒットするので使用できない)
int findString(HWND comboBox, LPCTSTR fontName)
{
//	_RPTFTN(_T("findString(0x%08X, %s)\n"), comboBox, fontName);

	int count = ComboBox_GetCount(comboBox);
	for (int i = 0; i < count; i++)
	{
		// テキストの長さを取得する。
		int textLength = ComboBox_GetLBTextLen(comboBox, i);
		_RPTF_NUM(textLength);
		if (textLength <= 0) continue;

		// テキストを取得する。
		tstring text(textLength, _T('\0'));
		ComboBox_GetLBText(comboBox, i, &text[0]);
		_RPTF_STR(text.c_str());
		if (text.empty()) continue;

		if (::lstrcmp(text.c_str(), fontName) == 0)
			return i;
	}

	return CB_ERR;
}

void addFontName(HWND comboBox, LPCTSTR fontName)
{
	// 指定されたフォント名がすでに存在するか調べる。
	int index = findString(comboBox, fontName);
	if (index >= 0)
	{
		// 一旦このフォント名を削除する。
		ComboBox_DeleteString(comboBox, index);
	}

	// リストの先頭にフォント名を追加する。
	ComboBox_InsertString(comboBox, 0, fontName);
	ComboBox_SetCurSel(comboBox, 0);
}

int selectFontName(HWND comboBox, LPCTSTR fontName)
{
	// 指定されたフォント名が存在するか調べる。
	int index = findString(comboBox, fontName);
	if (index >= 0)
	{
		// 指定されたフォント名が存在する場合は選択する。
		ComboBox_SetCurSel(comboBox, index);
	}

	// インデックスを返す。
	return index;
}

tstring getCurrentText(HWND comboBox)
{
	// 選択インデックスを取得する。
	int sel = ComboBox_GetCurSel(comboBox);
	_RPTF_NUM(sel);
	if (sel < 0) return _T("");

	// テキストの長さを取得する。
	int textLength = ComboBox_GetLBTextLen(comboBox, sel);
	_RPTF_NUM(textLength);
	if (textLength <= 0) return _T("");

	// テキストを取得する。
	tstring text(textLength, _T('\0'));
	ComboBox_GetLBText(comboBox, sel, &text[0]);
	_RPTF_STR(text.c_str());

	// テキストを返す。
	return text;
}

// 自作コントロールの位置を調整し、表示する。
void showControls(BOOL visible)
{
	if (visible)
	{
		RECT rc; ::GetWindowRect(g_fontComboBox, &rc);

		int w = (rc.right - rc.left);
		int h = (rc.bottom - rc.top);
		int w2 = w * 2;
		int h2 = h * 24;

		{
			int x = rc.left - w;
			int y = rc.top;

			::SendMessage(g_popular, CB_SETDROPPEDWIDTH, w2, 0);
			::SetWindowPos(g_popular, 0, x, y, w, h2,
				SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
		}

		{
			int x = rc.left - w;
			int y = rc.top + h;

			::SendMessage(g_favorite, CB_SETDROPPEDWIDTH, w2, 0);
			::SetWindowPos(g_favorite, 0, x, y, w, h2,
				SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
		}
	}
	else
	{
		::ShowWindow(g_popular, SW_HIDE);
		::ShowWindow(g_favorite, SW_HIDE);
	}
}

// コンボボックスを作成する。
HWND createMyFontComboBox()
{
	HWND hwnd = ::CreateWindowEx(
		WS_EX_NOACTIVATE,
		_T("ComboBox"),
		_T("SelectFavoriteFont"),
		WS_POPUP | WS_HSCROLL | WS_VSCROLL |
//		CBS_AUTOHSCROLL | CBS_DISABLENOSCROLL |
		CBS_DROPDOWNLIST | CBS_HASSTRINGS,
		0, 0, 100, 100,
		g_exeditObjectDialog,
		0,
		g_instance,
		0);
	_RPTF_HEX(hwnd);

	// フォントを元のコンボボックスのものに合わせる。
	HFONT font = GetWindowFont(g_fontComboBox);
	_RPTF_HEX(font);
	SetWindowFont(hwnd, font, TRUE);

	return hwnd;
}

// exedit のフォントコンボボックスをサブクラス化する。
void subclassFontComboBox(HWND fontComboBox)
{
	_RPTFT0(_T("subclassFontComboBox()\n"));

	// フォントコンボボックスをサブクラス化する。
	g_fontComboBox = fontComboBox;
	_RPTF_HEX(g_fontComboBox);
	orig_fontComboBox_wndProc = SubclassWindow(g_fontComboBox, hook_fontComboBox_wndProc);
	_RPTF_HEX(orig_fontComboBox_wndProc);

	// popular コンボボックスを作成し、初期設定する。
	g_popular = createMyFontComboBox();
	_RPTF_HEX(g_popular);
	orig_popular_wndProc = SubclassWindow(g_popular, hook_popular_wndProc);
	readFile(g_popular, g_popularFileName);

	// favorite コンボボックスを作成し、初期設定する。
	g_favorite = createMyFontComboBox();
	_RPTF_HEX(g_favorite);
	orig_favorite_wndProc = SubclassWindow(g_favorite, hook_favorite_wndProc);
	readFile(g_favorite, g_favoriteFileName);

	// 作成したコントロールを表示する。
	showControls(::IsWindowVisible(g_fontComboBox));
}

//---------------------------------------------------------------------

LRESULT CALLBACK hook_popular_wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		{
			_RPTFTN(_T("hook_popular_wndProc(WM_COMMAND, 0x%08X, 0x%08X)\n"), wParam, lParam);

			UINT code = HIWORD(wParam);
			UINT id = LOWORD(wParam);
			HWND sender = (HWND)lParam;

			if (code == LBN_SELCHANGE)
			{
				_RPTFTN(_T("hook_popular_wndProc(LBN_SELCHANGE, 0x%08X, 0x%08X)\n"), id, sender);

				tstring text = getCurrentText(hwnd);
				ComboBox_SelectString(g_fontComboBox, 0, text.c_str());
				g_noNotify = TRUE;
				::SendMessage(g_exeditObjectDialog, WM_COMMAND,
					MAKEWPARAM(ID_FONT_COMBO_BOX, CBN_SELCHANGE), (LPARAM)g_fontComboBox);
				g_noNotify = FALSE;
			}

			break;
		}
	case WM_DESTROY:
		{
			_RPTFTN(_T("hook_popular_wndProc(WM_DESTROY, 0x%08X, 0x%08X)\n"), wParam, lParam);

			writeFile(hwnd, g_popularFileName);

			break;
		}
	}

	return ::CallWindowProc(orig_fontComboBox_wndProc, hwnd, message, wParam, lParam);
}

LRESULT CALLBACK hook_favorite_wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		{
			_RPTFTN(_T("hook_favorite_wndProc(WM_COMMAND, 0x%08X, 0x%08X)\n"), wParam, lParam);

			UINT code = HIWORD(wParam);
			UINT id = LOWORD(wParam);
			HWND sender = (HWND)lParam;

			if (code == LBN_SELCHANGE)
			{
				_RPTFTN(_T("hook_favorite_wndProc(LBN_SELCHANGE, 0x%08X, 0x%08X)\n"), id, sender);

				tstring text = getCurrentText(hwnd);
				ComboBox_SelectString(g_fontComboBox, 0, text.c_str());
				g_noNotify = TRUE;
				::SendMessage(g_exeditObjectDialog, WM_COMMAND,
					MAKEWPARAM(ID_FONT_COMBO_BOX, CBN_SELCHANGE), (LPARAM)g_fontComboBox);
				g_noNotify = FALSE;
			}

			break;
		}
	}

	return ::CallWindowProc(orig_fontComboBox_wndProc, hwnd, message, wParam, lParam);
}

LRESULT CALLBACK hook_fontComboBox_wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_SHOWWINDOW:
		{
			_RPTFTN(_T("hook_fontComboBox_wndProc(WM_SHOWWINDOW, %d)\n"), wParam);

			showControls((BOOL)wParam);

			break;
		}
	}

	return ::CallWindowProc(orig_fontComboBox_wndProc, hwnd, message, wParam, lParam);
}

LRESULT CALLBACK hook_exeditObjectDialog_wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_PARENTNOTIFY:
		{
//			_RPTFTN(_T("hook_exeditObjectDialog_wndProc(WM_PARENTNOTIFY, 0x%08X, 0x%08X)\n"), wParam, lParam);

			if (wParam == WM_CREATE)
				::PostMessage(hwnd, WM_HEBIIRO_SUBCLASS_FONT_COMBOBOX, wParam, lParam);

			break;
		}
	case WM_COMMAND:
		{
//			_RPTFTN(_T("hook_exeditObjectDialog_wndProc(WM_COMMAND, 0x%08X, 0x%08X)\n"), wParam, lParam);

			if (g_noNotify) // フラグが立っている場合は通知を処理しない。
				break;

			UINT code = HIWORD(wParam);
			UINT id = LOWORD(wParam);
			HWND sender = (HWND)lParam;

			if (code == CBN_SELCHANGE && id == ID_FONT_COMBO_BOX)
			{
				_RPTFTN(_T("hook_exeditObjectDialog_wndProc(CBN_SELCHANGE, 0x%08X, 0x%08X)\n"), id, sender);

				// テキストオブジェクトのフォントコンボボックスでの選択処理が行われた場合ここに来る。

				tstring text = getCurrentText(sender);

				// まず favorite の中にあるか調べる。
				if (selectFontName(g_favorite, text.c_str()) < 0)
				{
					// favorite の中になかったので popular に追加する。
					addFontName(g_popular, text.c_str());
				}
			}

			break;
		}
	}

	if (message == WM_HEBIIRO_SUBCLASS_FONT_COMBOBOX)
	{
		HWND hwnd = (HWND)lParam;
		UINT id = ::GetDlgCtrlID(hwnd);
//		_RPTFTN(_T("hook_exeditObjectDialog_wndProc(WM_HEBIIRO_SUBCLASS_FONT_COMBOBOX, 0x%08X, 0x%08X)\n"), hwnd, id);
		if (id == ID_FONT_COMBO_BOX)
		{
			subclassFontComboBox(hwnd);
		}
	}

	return ::CallWindowProc(orig_exeditObjectDialog_wndProc, hwnd, message, wParam, lParam);
}

//---------------------------------------------------------------------

LRESULT CALLBACK cbtProc(int code, WPARAM wParam, LPARAM lParam)
{
	if (code >= 0)
	{
		if (code == HCBT_CREATEWND)
		{
			CBT_CREATEWND* cw = (CBT_CREATEWND*)lParam;

			if ((DWORD)cw->lpcs->lpszClass > 0xFFFF &&
				::lstrcmpi(cw->lpcs->lpszClass, _T("ExtendedFilterClass")) == 0)
			{
				// exedit のオブジェクト編集ダイアログをサブクラス化する。
				g_exeditObjectDialog = (HWND)wParam;
				_RPTF_HEX(g_exeditObjectDialog);
				orig_exeditObjectDialog_wndProc = SubclassWindow(
					g_exeditObjectDialog, hook_exeditObjectDialog_wndProc);
				_RPTF_HEX(orig_exeditObjectDialog_wndProc);

				// フックはこれで終わり。
				unhook();
			}
		}
	}

	return ::CallNextHookEx(g_hook, code, wParam, lParam);
}

// ファイル選択ダイアログのフックを開始する
void hook()
{
	_RPTFT0(_T("hook()\n"));

	g_hook = ::SetWindowsHookEx(WH_CBT, cbtProc, 0, ::GetCurrentThreadId());
}

// ファイル選択ダイアログのフックを終了する
void unhook()
{
	_RPTFT0(_T("unhook()\n"));

	::UnhookWindowsHookEx(g_hook), g_hook = 0;
}

//---------------------------------------------------------------------
//		初期化
//---------------------------------------------------------------------

BOOL func_init(FILTER *fp)
{
	_tsetlocale(LC_ALL, _T(""));

	_RPTFT0(_T("func_init()\n"));

	g_instance = fp->dll_hinst;
	::GetModuleFileName(g_instance, g_popularFileName, MAX_PATH);
	::PathRenameExtension(g_popularFileName, _T(".popular.txt"));
	::GetModuleFileName(g_instance, g_favoriteFileName, MAX_PATH);
	::PathRenameExtension(g_favoriteFileName, _T(".favorite.txt"));

	hook();

	return TRUE;
}

//---------------------------------------------------------------------
//		終了
//---------------------------------------------------------------------
BOOL func_exit(FILTER *fp)
{
	_RPTFT0(_T("func_term()\n"));

	unhook();

	return TRUE;
}
