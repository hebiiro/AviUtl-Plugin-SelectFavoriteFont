#include "pch.h"

//---------------------------------------------------------------------
//		フィルタ構造体のポインタを渡す関数
//---------------------------------------------------------------------
EXTERN_C FILTER_DLL __declspec(dllexport) * __stdcall GetFilterTable(void)
{
	static TCHAR g_filterName[] = TEXT("お気に入りフォント選択");
	static TCHAR g_filterInformation[] = TEXT("お気に入りフォント選択 version 1.1.0 by 蛇色");

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

const UINT ID_FONT_COMBO_BOX = 0x5654;

HINSTANCE g_instance = 0; // この DLL のハンドル
HFONT g_font = 0; // コントロール用のフォントハンドル
TCHAR g_popularFileName[MAX_PATH] = {};
TCHAR g_favoriteFileName[MAX_PATH] = {};
BOOL g_defaultProcessingOnly = FALSE;

HHOOK g_hook = 0;
HWND g_exeditObjectDialog = 0;
HWND g_container = 0;
HWND g_popular = 0;
HWND g_favorite = 0;

//---------------------------------------------------------------------

int GetWidth(LPCRECT rc);
int GetHeight(LPCRECT rc);
void SetClientRect(HWND hwnd, int x, int y, int w, int h);

void readFile(HWND comboBox, LPCTSTR fileName);
void writeFile(HWND comboBox, LPCTSTR fileName);
int findString(HWND comboBox, LPCTSTR fontName);
void addFontName(HWND comboBox, LPCTSTR fontName);
int selectFontName(HWND comboBox, LPCTSTR fontName);
tstring getCurrentText(HWND comboBox);

HWND createMyFontComboBox(HWND parent, LPCTSTR windowName);
void createContainer(HWND exeditObjectDialog);
void destroyContainer();
void showContainer();
void hideContainer();

LRESULT CALLBACK container_wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK hook_exeditObjectDialog_wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK cwprProc(int code, WPARAM wParam, LPARAM lParam);
void hook();
void unhook();

//---------------------------------------------------------------------

// 矩形の幅を返す
inline int GetWidth(LPCRECT rc)
{
	return rc->right - rc->left;
}

// 矩形の高さを返す
inline int GetHeight(LPCRECT rc)
{
	return rc->bottom - rc->top;
}

// クライアント領域が指定された位置に来るようにウィンドウの位置を調整する
void SetClientRect(HWND hwnd, int x, int y, int w, int h)
{
	RECT rcWindow; ::GetWindowRect(hwnd, &rcWindow);
	RECT rcClient; ::GetClientRect(hwnd, &rcClient);
//	::MapWindowPoints(hwnd, NULL, (LPPOINT)&rcClient, 2); // 幅と高さしか使わないので座標系を合わせる必要はない
	int rcWindowWidth = GetWidth(&rcWindow);
	int rcWindowHeight = GetHeight(&rcWindow);
	int rcClientWidth = GetWidth(&rcClient);
	int rcClientHeight = GetHeight(&rcClient);
	POINT diff =
	{
		rcWindowWidth - rcClientWidth,
		rcWindowHeight - rcClientHeight,
	};

	x -= diff.x / 2;
	y -= diff.y / 2;
	w += diff.x;
	h += diff.y;

	::SetWindowPos(hwnd, NULL,
		x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
}

//---------------------------------------------------------------------

// フォントリストをファイルから読み込む
void readFile(HWND comboBox, LPCTSTR fileName)
{
	_RPTFTN(_T("readFile(0x%08X, %s)\n"), comboBox, fileName);

	ComboBox_ResetContent(comboBox);

	// 入力ストリームを開く。
	std::ifstream file(fileName);

	tstring line;
	while (std::getline(file, line)) // 1行ずつ読み込む
	{
		// コンボボックスに読み込んだテキストを追加する。
		ComboBox_AddString(comboBox, line.c_str());
	}

	// 入力ストリームを閉じる。
	file.close();
}

// フォントリストをファイルに書き込む
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

// コンボボックスのリストから fontName を見つけてそのインデックスを返す
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

		// テキストが完全一致するかチェックする。
		if (::lstrcmp(text.c_str(), fontName) == 0)
			return i;
	}

	return CB_ERR;
}

// コンボボックスにテキストを追加する。ただし、すでに存在する場合は追加しない
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

// 指定されたテキストを持つアイテムを選択肢し、そのインデックスを返す
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

// コンボボックスで現在選択されているアイテムのテキストを返す
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

//---------------------------------------------------------------------

// フォントコンボボックスを作成する
HWND createMyFontComboBox(HWND parent, LPCTSTR windowName)
{
	HWND hwnd = ::CreateWindowEx(
		WS_EX_NOACTIVATE,
		_T("ComboBox"),
		windowName,
		WS_CHILD | WS_VISIBLE |
		WS_HSCROLL | WS_VSCROLL |
//		CBS_AUTOHSCROLL | CBS_DISABLENOSCROLL |
		CBS_DROPDOWNLIST | CBS_HASSTRINGS,
		0, 0, 100, 100,
		parent,
		0,
		g_instance,
		0);
	_RPTF_HEX(hwnd);

	// ここでフォントもセットする。
	SetWindowFont(hwnd, g_font, TRUE);

	return hwnd;
}

// コンテナウィンドウとコントロール群を作成し、初期設定する
void createContainer(HWND exeditObjectDialog)
{
	_RPTFTN(_T("createContainer(0x%08X)\n"), exeditObjectDialog);

	// オブジェクト編集ダイアログのウィンドウハンドルをグローバル変数に保存しておく。
	g_exeditObjectDialog = exeditObjectDialog;
	_RPTF_HEX(g_exeditObjectDialog);

	// コンテナウィンドウのウィンドウクラスを登録する。
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(wc);
	wc.lpszClassName = _T("SelectFavoriteFont");
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = container_wndProc;
	wc.hInstance = g_instance;
	wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW  + 1);
	::RegisterClassEx(&wc);

	// コンテナウィンドウを作成する。
	g_container = ::CreateWindowEx(
		WS_EX_NOACTIVATE,
		_T("SelectFavoriteFont"),
		_T("SelectFavoriteFont"),
		WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
		WS_POPUP,
//		WS_POPUP | WS_BORDER, // コンテナウィンドウに細い枠を付けたいならこっちを使う
//		WS_POPUP | WS_DLGFRAME, // コンテナウィンドウに立体枠を付けたいならこっちを使う
		0, 0, 100, 100,
		g_exeditObjectDialog,
		0,
		g_instance,
		0);
	_RPTF_HEX(g_container);

	// popular コンボボックスを作成し、初期設定する。
	g_popular = createMyFontComboBox(g_container, _T("popular"));
	_RPTF_HEX(g_popular);

	// favorite コンボボックスを作成し、初期設定する。
	g_favorite = createMyFontComboBox(g_container, _T("favorite"));
	_RPTF_HEX(g_favorite);
}

// コンテナウィンドウを削除する
void destroyContainer()
{
	_RPTFT0(_T("destroyContainer()\n"));

	::DestroyWindow(g_container);
}

// コンテナウィンドウを表示する
void showContainer()
{
	_RPTFT0(_T("showContainer()\n"));

	::ShowWindow(g_container, SW_SHOWNA);
}

// コンテナウィンドウを非表示にする
void hideContainer()
{
	_RPTFT0(_T("hideContainer()\n"));

	::ShowWindow(g_container, SW_HIDE);
}

// コンテナウィンドウとコントロール群の表示位置を調整する
void moveContainer()
{
	_RPTFT0(_T("moveContainer()\n"));

	// 基準となる AviUtl (exedit) 側のコントロールを取得する。
	HWND base = ::GetDlgItem(g_exeditObjectDialog, ID_FONT_COMBO_BOX);
	if (!base) return;
	if (!::IsWindowVisible(base)) return;
	RECT rcBase; ::GetWindowRect(base, &rcBase);
	int rcBaseWidth = GetWidth(&rcBase);
	int rcBaseHeight = GetHeight(&rcBase);

	{
		int x = rcBase.left - rcBaseWidth;
		int y = rcBase.top - rcBaseHeight;
		int w = rcBaseWidth;
		int h = rcBaseHeight * 2;

		SetClientRect(g_container, x, y, w, h);
	}

	HDWP dwp = ::BeginDeferWindowPos(2); // 引数は動かすウィンドウの数
	_RPTF_HEX(dwp);
	if (!dwp) return;

	{
		// 「よく使う」フォントコンボボックスを動かす。

		int x = 0;
		int y = 0;
		int w = rcBaseWidth;
		int h = rcBaseHeight;
		int w2 = w * 2;
		int h2 = h * 24;

		::SendMessage(g_popular, CB_SETDROPPEDWIDTH, w2, 0);
		dwp = ::DeferWindowPos(dwp, g_popular, NULL,
			x, y, w, h2, SWP_NOZORDER | SWP_NOACTIVATE);
		_RPTF_HEX(dwp);
		if (!dwp) return;
	}

	{
		// 「お気に入り」フォントコンボボックスを動かす。

		int x = 0;
		int y = rcBaseHeight;
		int w = rcBaseWidth;
		int h = rcBaseHeight;
		int w2 = w * 2;
		int h2 = h * 24;

		::SendMessage(g_favorite, CB_SETDROPPEDWIDTH, w2, 0);
		dwp = ::DeferWindowPos(dwp, g_favorite, NULL,
			x, y, w, h2, SWP_NOZORDER | SWP_NOACTIVATE);
		_RPTF_HEX(dwp);
		if (!dwp) return;
	}

	BOOL result = ::EndDeferWindowPos(dwp);
	_RPTF_NUM(result);
}

//---------------------------------------------------------------------

LRESULT CALLBACK container_wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		{
			_RPTFTN(_T("container_wndProc(WM_COMMAND, 0x%08X, 0x%08X)\n"), wParam, lParam);

			// WM_COMMAND メッセージの引数を分解する。
			UINT code = HIWORD(wParam);
			UINT id = LOWORD(wParam);
			HWND sender = (HWND)lParam;

			if (code == CBN_SELCHANGE) // コンテナ内のコンボボックスの選択が切り替えられた
			{
				_RPTFTN(_T("container_wndProc(CBN_SELCHANGE, 0x%08X, 0x%08X)\n"), id, sender);

				// フォントコンボボックスを取得する。
				HWND fontComboBox = ::GetDlgItem(g_exeditObjectDialog, ID_FONT_COMBO_BOX);
				if (!fontComboBox) break;

				// 選択されているフォント名を取得する。
				tstring text = getCurrentText(sender);

				// フォントコンボボックスの選択を切り替える。
				ComboBox_SelectString(fontComboBox, 0, text.c_str());

				// デフォルト処理のみ実行する。
				g_defaultProcessingOnly = TRUE;
				::SendMessage(
					g_exeditObjectDialog,
					WM_COMMAND,
					MAKEWPARAM(ID_FONT_COMBO_BOX, CBN_SELCHANGE),
					(LPARAM)fontComboBox);
				g_defaultProcessingOnly = FALSE;
			}

			break;
		}
	case WM_DESTROY:
		{
			_RPTFTN(_T("WM_DESTROY, 0x%08X, 0x%08X\n"), wParam, lParam);

			break;
		}
	}

	return ::DefWindowProc(hwnd, message, wParam, lParam);
}

LRESULT CALLBACK hook_exeditObjectDialog_wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		{
//			_RPTFTN(_T("hook_exeditObjectDialog_wndProc(WM_COMMAND, 0x%08X, 0x%08X)\n"), wParam, lParam);

			if (g_defaultProcessingOnly)
				break; // フラグが立っている場合は通常処理だけ。独自処理は行わない

			// WM_COMMAND メッセージの引数を分解する。
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
#if 1
	case WM_WINDOWPOSCHANGED:
		{
			_RPTFTN(_T("WM_WINDOWPOSCHANGED, 0x%08X, 0x%08X\n"), wParam, lParam);

			// コンテナウィンドウをターゲットダイアログに追従させる。
			moveContainer();

			break;
		}
#endif
	}

	return 0;
}

// CallWndProcRet フック関数
LRESULT CALLBACK cwprProc(int code, WPARAM wParam, LPARAM lParam)
{
	if (code >= 0)
	{
		CWPRETSTRUCT* cwpr = (CWPRETSTRUCT*)lParam;

		switch(cwpr->message)
		{
		case WM_CREATE:
			{
//				_RPTFT0(_T("WM_CREATE\n"));

				// このウィンドウのクラス名を取得する。
				TCHAR className[MAX_PATH] = {};
				::GetClassName(cwpr->hwnd, className, MAX_PATH);
//				_RPTF_STR(className);

				if (::StrStr(className, _T("ExtendedFilterClass")))
				{
					_RPTFT0(_T("拡張編集のオブジェクト編集ダイアログをフックします\n"));

					createContainer(cwpr->hwnd);
				}

				break;
			}
		case WM_SHOWWINDOW:
			{
//				_RPTFTN(_T("WM_SHOWWINDOW, 0x%08X, 0x%08X\n"), cwpr->wParam, cwpr->lParam);

				// フォントコンボボックスのメッセージかチェックする。
				HWND control = cwpr->hwnd;
				UINT id = ::GetDlgCtrlID(control);
//				_RPTF_NUM(id);

				if (id == ID_FONT_COMBO_BOX)
				{
					_RPTFT0(_T("フォントコンボボックスの WM_SHOWWINDOW\n"));

					if (cwpr->wParam)
					{
						_RPTFT0(_T("フォントコンボボックスが表示された\n"));

						readFile(g_popular, g_popularFileName);
						readFile(g_favorite, g_favoriteFileName);

						moveContainer();
						showContainer();
					}
					else
					{
						_RPTFT0(_T("フォントコンボボックスが非表示になった\n"));

						hideContainer();

						writeFile(g_popular, g_popularFileName);
					}
				}

				break;
			}
		}

		if (cwpr->hwnd == g_exeditObjectDialog)
		{
			hook_exeditObjectDialog_wndProc(cwpr->hwnd, cwpr->message, cwpr->wParam, cwpr->lParam);
		}
	}

	return ::CallNextHookEx(g_hook, code, wParam, lParam);
}

// CallWndProcRet フックを作成する
void hook()
{
	_RPTFT0(_T("hook()\n"));

	g_hook = ::SetWindowsHookEx(WH_CALLWNDPROCRET, cwprProc, 0, ::GetCurrentThreadId());
}

// CallWndProcRet フックを削除する
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
	// ロケールを設定する。これをやらないと日本語テキストが文字化けする。
	_tsetlocale(LC_ALL, _T(""));

	_RPTFT0(_T("func_init()\n"));

	// 色んな所で使う可能性があるので、
	// この DLL のハンドルをグローバル変数に保存しておく。
	g_instance = fp->dll_hinst;
	_RPTF_HEX(g_instance);

	// フォントリストファイルのパスを取得しておく。
	::GetModuleFileName(g_instance, g_popularFileName, MAX_PATH);
	::PathRenameExtension(g_popularFileName, _T(".popular.txt"));
	::GetModuleFileName(g_instance, g_favoriteFileName, MAX_PATH);
	::PathRenameExtension(g_favoriteFileName, _T(".favorite.txt"));

	// コントロール用のフォントを取得しておく。
	SYS_INFO si = {};
	fp->exfunc->get_sys_info(NULL, &si);
	g_font = si.hfont;
	_RPTF_HEX(g_font);

	// CallWndProcRet フックを開始する。
	hook();

	return TRUE;
}

//---------------------------------------------------------------------
//		終了
//---------------------------------------------------------------------
BOOL func_exit(FILTER *fp)
{
	_RPTFT0(_T("func_term()\n"));

	// CallWndProcRet フックを終了する。
	unhook();

	return TRUE;
}
