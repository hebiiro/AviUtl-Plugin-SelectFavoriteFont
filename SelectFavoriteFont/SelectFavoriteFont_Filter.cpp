#include "pch.h"
#include "SelectFavoriteFont.h"

//--------------------------------------------------------------------
//		初期化
//--------------------------------------------------------------------

BOOL func_init(AviUtl::FilterPlugin* fp)
{
	// ロケールを設定する。これをやらないと日本語テキストが文字化けする。
	_tsetlocale(LC_CTYPE, _T(""));

	MY_TRACE(_T("func_init()\n"));

	INITCOMMONCONTROLSEX icex = {};
	icex.dwICC = ICC_TREEVIEW_CLASSES;
	::InitCommonControlsEx(&icex);

	// 色んな所で使う可能性があるので、
	// この DLL のハンドルをグローバル変数に保存しておく。
	g_instance = fp->dll_hinst;
	MY_TRACE_HEX(g_instance);

	// 設定ファイルのファイルパスを取得しておく。
	::GetModuleFileName(g_instance, g_settingsFileName, MAX_PATH);
	::PathRenameExtension(g_settingsFileName, _T(".xml"));
	MY_TRACE_TSTR(g_settingsFileName);

	// コントロール用のフォントを取得しておく。
	AviUtl::SysInfo si = {};
	fp->exfunc->get_sys_info(NULL, &si);
	g_font = si.hfont;
	MY_TRACE_HEX(g_font);

	// CallWndProcRet フックを開始する。
	hook();

	return TRUE;
}

//--------------------------------------------------------------------
//		終了
//--------------------------------------------------------------------
BOOL func_exit(AviUtl::FilterPlugin* fp)
{
	MY_TRACE(_T("func_term()\n"));

	// CallWndProcRet フックを終了する。
	unhook();

	return TRUE;
}

//--------------------------------------------------------------------
//		WndProc
//--------------------------------------------------------------------
BOOL func_WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, AviUtl::EditHandle* editp, AviUtl::FilterPlugin* fp)
{
	switch (message)
	{
	case AviUtl::detail::FilterPluginWindowMessage::Init:
		{
			MY_TRACE(_T("func_WndProc(Init, 0x%08X, 0x%08X)\n"), wParam, lParam);

			// コントロールを作成してレイアウトを更新する。
			createControls(hwnd);
			recalcLayout();

			// ファイルから設定を読み込む。
			loadSettings();

			break;
		}
	case AviUtl::detail::FilterPluginWindowMessage::Exit:
		{
			MY_TRACE(_T("func_WndProc(Exit, 0x%08X, 0x%08X)\n"), wParam, lParam);

			// ファイルに設定を保存する。
			saveSettings();

			break;
		}
	case WM_SIZE:
		{
			MY_TRACE(_T("func_WndProc(WM_SIZE, 0x%08X, 0x%08X)\n"), wParam, lParam);

			// レイアウトを更新する。
			recalcLayout();

			break;
		}
	case WM_CONTEXTMENU:
		{
			MY_TRACE(_T("func_WndProc(WM_CONTEXTMENU, 0x%08X, 0x%08X)\n"), wParam, lParam);

			HWND control = (HWND)wParam;

			if (control == g_recent)
			{
				// コンボボックスのコンテキストメニューを表示する。
				showComboBoxContextMenu(hwnd, wParam, lParam);
			}
			else
			{
				// ツリービューのコンテキストメニューを表示する。
				showTreeViewContextMenu(hwnd, wParam, lParam);
			}

			break;
		}
	case WM_COMMAND:
		{
			MY_TRACE(_T("func_WndProc(WM_COMMAND, 0x%08X, 0x%08X)\n"), wParam, lParam);

			// WM_COMMAND メッセージの引数を分解する。
			UINT code = HIWORD(wParam);
			UINT id = LOWORD(wParam);
			HWND sender = (HWND)lParam;

			if (code == CBN_SELCHANGE) // コンテナ内のコンボボックスの選択が切り替えられた。
			{
				MY_TRACE(_T("func_WndProc(CBN_SELCHANGE, 0x%08X, 0x%08X)\n"), id, sender);

				// フォントコンボボックスを取得する。
				HWND fontComboBox = ExEdit_TextObject_GetFontComboBox();
				if (!fontComboBox) break;

				// 選択されているフォント名を取得する。
				tstring text = ComboBox_GetCurrentText(sender);

				// フォントコンボボックスの選択を切り替える。
				ExEdit_TextObject_SetFont(fontComboBox, text.c_str());
			}

			break;
		}
	case WM_NOTIFY:
		{
			//MY_TRACE(_T("func_WndProc(WM_NOTIFY, 0x%08X, 0x%08X)\n"), wParam, lParam);

			switch (wParam)
			{
			case ID_FAVORITE:
				{
					NMHDR* nm = (NMHDR*)lParam;

					static BOOL g_ignoreNotify = FALSE;

					switch (nm->code)
					{
					case NM_RCLICK:
						{
							MY_TRACE(_T("func_WndProc(NM_RCLICK)\n"));

							HTREEITEM itemHandle = TreeView_GetDropHilight(g_favorite);
							MY_TRACE_HEX(itemHandle);
							if (itemHandle) // NULL の場合もあるのでチェックしないと逆に選択が解除される場合がある。
							{
								// 右クリックで選択したときは TVN_SELCHANGED を
								// 処理したくないのでフラグを立てておく。
								g_ignoreNotify = TRUE;
								TreeView_SelectItem(g_favorite, itemHandle);
								g_ignoreNotify = FALSE;
							}

							break;
						}
					case TVN_SELCHANGED:
						{
							MY_TRACE(_T("func_WndProc(TVN_SELCHANGED)\n"));

							if (g_ignoreNotify) // フラグが立っている場合は何もしない。
							{
								MY_TRACE(_T("TVN_SELCHANGED を無視します)\n"));
								break;
							}

							NMTREEVIEW* nm = (NMTREEVIEW*)lParam;

							// コンテナ内のツリービューの選択が切り替えられた。

							// フォントコンボボックスを取得する。
							HWND fontComboBox = ExEdit_TextObject_GetFontComboBox();
							if (!fontComboBox) break;

							TreeItemContainer::iterator it = g_favoriteItems.find(nm->itemNew.hItem);
							if (it == g_favoriteItems.end()) break;
							TreeItemPtr& item = it->second;
							MY_TRACE_WSTR((BSTR)item->m_name);
							if (item->m_name.length() == 0) break;

							// フォントコンボボックスの選択を切り替える。
							ExEdit_TextObject_SetFont(fontComboBox, item->m_name);

							break;
						}
					}

					break;
				}
			}

			break;
		}
	}

	return FALSE;
}

//--------------------------------------------------------------------
//		フィルタ構造体のポインタを渡す関数
//--------------------------------------------------------------------
EXTERN_C __declspec(dllexport) AviUtl::FilterPluginDLL* CALLBACK GetFilterTable()
{
	LPCSTR name = "お気に入りフォント選択";
	LPCSTR information = "お気に入りフォント選択 6.0.2 by 蛇色";

	static AviUtl::FilterPluginDLL filter =
	{
		.flag =
			AviUtl::detail::FilterPluginFlag::AlwaysActive |
			AviUtl::detail::FilterPluginFlag::WindowThickFrame |
			AviUtl::detail::FilterPluginFlag::WindowSize |
			AviUtl::detail::FilterPluginFlag::DispFilter |
			AviUtl::detail::FilterPluginFlag::ExInformation,
		.x = 400,
		.y = 400,
		.name = name,
		.func_init = func_init,
		.func_exit = func_exit,
		.func_WndProc = func_WndProc,
		.information = information,
	};

	return &filter;
}

//--------------------------------------------------------------------
