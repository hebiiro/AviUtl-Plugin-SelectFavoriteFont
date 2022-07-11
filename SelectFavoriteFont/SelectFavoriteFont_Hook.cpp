#include "pch.h"
#include "SelectFavoriteFont.h"

//--------------------------------------------------------------------

LRESULT CALLBACK settingDialog_wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		{
//			MY_TRACE(_T("settingDialog_wndProc(WM_COMMAND, 0x%08X, 0x%08X)\n"), wParam, lParam);

			if (g_defaultProcessingOnly)
				break; // フラグが立っている場合は通常処理だけ。独自処理は行わない。

			// WM_COMMAND メッセージの引数を分解する。
			UINT code = HIWORD(wParam);
			UINT id = LOWORD(wParam);
			HWND sender = (HWND)lParam;

			if (code == CBN_SELCHANGE && id == ID_FONT_COMBO_BOX)
			{
				MY_TRACE(_T("settingDialog_wndProc(CBN_SELCHANGE, 0x%08X, 0x%08X)\n"), id, sender);

				// テキストオブジェクトのフォントコンボボックスでの選択処理が行われた場合ここに来る。

				tstring text = ComboBox_GetCurrentText(sender);

				// まず favorite の中にあるか調べる。
				if (ComboBox_SelectFontName(g_favorite, text.c_str()) < 0)
				{
					// favorite の中になかったので recent に追加する。
					ComboBox_AddFontName(g_recent, text.c_str());
				}
			}
			else if (code == CBN_DROPDOWN && id == ID_FONT_COMBO_BOX)
			{
				MY_TRACE(_T("settingDialog_wndProc(CBN_DROPDOWN, 0x%08X, 0x%08X)\n"), id, sender);

				COMBOBOXINFO cbi = { sizeof(cbi) };
				::GetComboBoxInfo(sender, &cbi);
				preview_dropDown(sender, cbi.hwndList);
			}

			break;
		}
	}

	return 0;
}

// CallWndProcRet フック関数。
LRESULT CALLBACK cwprProc(int code, WPARAM wParam, LPARAM lParam)
{
	if (code >= 0)
	{
		CWPRETSTRUCT* cwpr = (CWPRETSTRUCT*)lParam;
#if 0
		if (::GetKeyState(VK_MENU) < 0)
		{
			static int i = 0;
			MY_TRACE(_T("%d, 0x%08X, 0x%08X, 0x%08X, 0x%08X\n"), i++, cwpr->hwnd, cwpr->message, cwpr->wParam, cwpr->lParam);
		}
#endif
		switch(cwpr->message)
		{
		case WM_CREATE:
			{
//				MY_TRACE(_T("WM_CREATE, 0x%08X, 0x%08X\n"), cwpr->wParam, cwpr->lParam);

				// このウィンドウのクラス名を取得する。
				TCHAR className[MAX_PATH] = {};
				::GetClassName(cwpr->hwnd, className, MAX_PATH);
//				MY_TRACE_TSTR(className);

				if (::StrStr(className, _T("ExtendedFilterClass")))
				{
					MY_TRACE(_T("設定ダイアログをフックします\n"));

					// 設定ダイアログのウィンドウハンドルをグローバル変数に保存しておく。
					g_settingDialog = cwpr->hwnd;
					MY_TRACE_HEX(g_settingDialog);
				}

				break;
			}
		case WM_SHOWWINDOW:
			{
//				MY_TRACE(_T("WM_SHOWWINDOW, 0x%08X, 0x%08X\n"), cwpr->wParam, cwpr->lParam);

				// フォントコンボボックスを取得する。
				HWND fontComboBox = ExEdit_TextObject_GetFontComboBox();
				if (!fontComboBox) break;

				// フォントコンボボックスのドロップダウンリストか調べる。
				COMBOBOXINFO cbi = { sizeof(cbi) };
				::GetComboBoxInfo(fontComboBox, &cbi);
				if (cwpr->hwnd == cbi.hwndList)
				{
					MY_TRACE(_T("ドロップダウンリストの WM_SHOWWINDOW\n"));

					if (cwpr->wParam)
					{
#if 0
						MY_TRACE(_T("ドロップダウンリストが表示されました\n"));
						preview_init(cwpr->hwnd);
						preview_recalcLayout();
						preview_show();
#endif
					}
					else
					{
						MY_TRACE(_T("ドロップダウンリストが非表示になりました\n"));

						preview_hide();
					}
				}

				break;
			}
		case WM_PAINT:
		case WM_PRINT:
//		case WM_VSCROLL:
//		case WM_MOUSEWHEEL:
			{
				// フォントコンボボックスを取得する。
				HWND fontComboBox = ExEdit_TextObject_GetFontComboBox();
				if (!fontComboBox) break;

				// フォントコンボボックスのドロップダウンリストか調べる。
				COMBOBOXINFO cbi = { sizeof(cbi) };
				::GetComboBoxInfo(fontComboBox, &cbi);
				if (cwpr->hwnd == cbi.hwndList)
				{
					MY_TRACE(_T("ドロップダウンリストの WM_PAINT\n"));

					// プレビューを再描画する。
					preview_refresh();
				}

				break;
			}
		}

		if (cwpr->hwnd == g_settingDialog)
		{
			settingDialog_wndProc(cwpr->hwnd, cwpr->message, cwpr->wParam, cwpr->lParam);
		}
	}

	return ::CallNextHookEx(g_hook, code, wParam, lParam);
}

// CallWndProcRet フックを作成する。
void hook()
{
	MY_TRACE(_T("hook()\n"));

	g_hook = ::SetWindowsHookEx(WH_CALLWNDPROCRET, cwprProc, 0, ::GetCurrentThreadId());
}

// CallWndProcRet フックを削除する。
void unhook()
{
	MY_TRACE(_T("unhook()\n"));

	::UnhookWindowsHookEx(g_hook), g_hook = 0;
}

//--------------------------------------------------------------------
