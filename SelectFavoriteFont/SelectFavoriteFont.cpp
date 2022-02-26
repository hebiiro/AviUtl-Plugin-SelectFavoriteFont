#include "pch.h"
#include "SelectFavoriteFont.h"

//---------------------------------------------------------------------

HINSTANCE g_instance = 0; // この DLL のハンドル。
HFONT g_font = 0; // コントロール用のフォントハンドル。
TCHAR g_recentFileName[MAX_PATH] = {};
TCHAR g_favoriteFileName[MAX_PATH] = {};
TCHAR g_settingsFileName[MAX_PATH] = {};
BOOL g_isSettingsFileLoaded = FALSE;
BOOL g_defaultProcessingOnly = FALSE;

HHOOK g_hook = 0;
HWND g_exeditObjectDialog = 0;
HWND g_container = 0;
HWND g_recent = 0;
HWND g_favorite = 0;

RECT g_containerRect = { 100, 100, 500, 500 };
_bstr_t g_labelFormat = L"%ws --- %ws";
_bstr_t g_separatorFormat = L"---------";

TreeItemContainer g_favoriteItems;

//---------------------------------------------------------------------

// デバッグ用コールバック関数。デバッグメッセージを出力する。
void ___outputLog(LPCTSTR text, LPCTSTR output)
{
	::OutputDebugString(output);
}

//---------------------------------------------------------------------

TreeItem::TreeItem()
{
	m_name = L"";
	m_alias = L"";
	m_label = L"";
	m_expand = FALSE;
}

void TreeItem::updateLabel()
{
	if (m_alias.length())
	{
		if (m_name.length())
		{
			// 別名と実名が指定されているので、ラベルはその組み合わせ。
			WCHAR label[MAX_PATH] = {};
			::StringCbPrintfW(label, sizeof(label),
				g_labelFormat, m_alias.GetBSTR(), m_name.GetBSTR());
			m_label = label;
		}
		else
		{
			// 別名のみが指定されているので、ラベルは別名。
			m_label = m_alias;
		}
	}
	else
	{
		if (m_name.length())
		{
			// 別名が指定されていないので、ラベルは実名。
			m_label = m_name;
		}
		else
		{
			// 別名も実名も指定されていないので、ラベルはセパレータ。
			m_label = g_separatorFormat;
		}
	}

	MY_TRACE_WSTR(m_name.GetBSTR());
	MY_TRACE_WSTR(m_alias.GetBSTR());
	MY_TRACE_WSTR(m_label.GetBSTR());
}

//---------------------------------------------------------------------

HTREEITEM insertFavoriteItem(HTREEITEM parentHandle, HTREEITEM insertAfter, const TreeItemPtr& item)
{
	// アイテムのラベルを更新する。
	item->updateLabel();

	// ツリービューに追加する。同時にマップにも追加する。
	TV_INSERTSTRUCT tv = {};
	tv.hParent = parentHandle;
	tv.hInsertAfter = insertAfter;
	tv.item.mask = TVIF_TEXT;
	tv.item.pszText = (LPTSTR)item->m_label;
	HTREEITEM itemHandle = TreeView_InsertItem(g_favorite, &tv);
	g_favoriteItems.insert(TreeItemContainer::value_type(itemHandle, item));
	return itemHandle;
}

void deleteFavoriteItem(HTREEITEM itemHandle)
{
	TreeView_DeleteItem(g_favorite, itemHandle);
	g_favoriteItems.erase(itemHandle);
}

void updateFavoriteItem(HTREEITEM itemHandle, const TreeItemPtr& item)
{
	// アイテムのラベルを更新する。
	item->updateLabel();

	// ツリービューに追加する。同時にマップにも追加する。
	TV_ITEM tv = {};
	tv.mask = TVIF_TEXT;
	tv.pszText = (LPTSTR)item->m_label;
	tv.hItem = itemHandle;
	TreeView_SetItem(g_favorite, &tv);
}

void expandFavoriteItems(HTREEITEM parentHandle)
{
	// ツリービュー内の親アイテムから最初の子アイテムのハンドルを取得する。
	HTREEITEM itemHandle = TreeView_GetChild(g_favorite, parentHandle);

	while (itemHandle)
	{
		// 「お気に入り」アイテムコンテナから該当するアイテムを取得する。
		TreeItemContainer::iterator it = g_favoriteItems.find(itemHandle);
		if (it != g_favoriteItems.end())
		{
			TreeItemPtr& item = it->second;
			MY_TRACE(_T("%ws, %ws, %ws, %d\n"),
				item->m_name.GetBSTR(),
				item->m_alias.GetBSTR(),
				item->m_label.GetBSTR(),
				item->m_expand);

			// expand が 0 以外ならアイテムを展開する。
			if (item->m_expand)
			{
				BOOL result = TreeView_Expand(g_favorite, itemHandle, TVE_EXPAND);
				MY_TRACE_INT(result);
			}

			// 再帰的に子要素を展開する。
			expandFavoriteItems(itemHandle);
		}

		// ツリービュー内の次の子アイテムのハンドルを取得する。
		itemHandle = TreeView_GetNextSibling(g_favorite, itemHandle);
	}
}

// コンテナウィンドウとコントロール群を作成し、初期設定する。
void createContainer()
{
	MY_TRACE(_T("createContainer()\n"));

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
		WS_POPUP | WS_CAPTION | WS_THICKFRAME,
//		WS_POPUP | WS_BORDER, // コンテナウィンドウに細い枠を付けたいならこっちを使う。
//		WS_POPUP | WS_DLGFRAME, // コンテナウィンドウに立体枠を付けたいならこっちを使う。
		0, 0, 100, 100,
		g_exeditObjectDialog,
		0,
		g_instance,
		0);
	MY_TRACE_HEX(g_container);

	// recent コンボボックスを作成する。
	g_recent = ::CreateWindowEx(
		0,
		WC_COMBOBOX,
		_T("recent"),
		WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL |
//		CBS_AUTOHSCROLL | CBS_DISABLENOSCROLL |
		CBS_DROPDOWNLIST | CBS_HASSTRINGS,
		0, 0, 100, 100,
		g_container,
		(HMENU)ID_RECENT,
		g_instance,
		0);
	MY_TRACE_HEX(g_recent);
	SetWindowFont(g_recent, g_font, TRUE);

	// favorite リストビューを作成する。
	g_favorite = ::CreateWindowEx(
		0,
//		WS_EX_CLIENTEDGE,
		WC_TREEVIEW,
		_T("favorite"),
		WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | WS_BORDER |
		TVS_HASBUTTONS | TVS_LINESATROOT | TVS_SHOWSELALWAYS | TVS_FULLROWSELECT,
//		TVS_HASBUTTONS | TVS_LINESATROOT | TVS_SHOWSELALWAYS | TVS_HASLINES,
		0, 0, 100, 100,
		g_container,
		(HMENU)ID_FAVORITE,
		g_instance,
		0);
	MY_TRACE_HEX(g_favorite);
	SetWindowFont(g_favorite, g_font, TRUE);
#if 1
//	TreeView_SetExtendedStyle(g_favorite, TVS_EX_DOUBLEBUFFER, 0);
	HRESULT hr = ::SetWindowTheme(g_favorite, L"Explorer", 0);
#endif

	preview_create();
}

// コンテナウィンドウを削除する。
void destroyContainer()
{
	MY_TRACE(_T("destroyContainer()\n"));

	::DestroyWindow(g_container);
}

// コンテナウィンドウを表示する。
void showContainer()
{
	MY_TRACE(_T("showContainer()\n"));

	::ShowWindow(g_container, SW_SHOWNA);
}

// コンテナウィンドウを非表示にする。
void hideContainer()
{
	MY_TRACE(_T("hideContainer()\n"));

	::ShowWindow(g_container, SW_HIDE);
}

// コントロール群の表示位置を調整する。
void recalcLayout()
{
	MY_TRACE(_T("recalcLayout()\n"));

	// 基準となる AviUtl (exedit) 側のコントロールとその矩形を取得する。
	HWND base = Exedit_TextObject_GetFontComboBox();
	RECT rcBase; ::GetWindowRect(base, &rcBase);
	int rcBaseWidth = GetWidth(&rcBase);
	int rcBaseHeight = GetHeight(&rcBase);

	// コンテナウィンドウのクライアント矩形を取得する。
	RECT rcContainer; ::GetClientRect(g_container, &rcContainer);
	int rcContainerWidth = GetWidth(&rcContainer);
	int rcContainerHeight = GetHeight(&rcContainer);

	HDWP dwp = ::BeginDeferWindowPos(2); // 引数は動かすウィンドウの数。
	MY_TRACE_HEX(dwp);
	if (!dwp) return;

	{
		// 「最近使った」フォントコンボボックスを動かす。
		int x = rcContainer.left;
		int y = rcContainer.top;
		int w = rcContainerWidth;
		int h = rcContainerHeight;
		int w2 = w + rcBaseWidth;

//		::SendMessage(g_recent, CB_SETDROPPEDWIDTH, w2, 0);
		dwp = ::DeferWindowPos(dwp, g_recent, NULL,
			x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
		MY_TRACE_HEX(dwp);
		if (!dwp) return;
	}

	{
		// 「お気に入り」ツリービューを動かす。
		int x = rcContainer.left;
		int y = rcContainer.top + rcBaseHeight;
		int w = rcContainerWidth;
		int h = rcContainerHeight - rcBaseHeight;

		dwp = ::DeferWindowPos(dwp, g_favorite, NULL,
			x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
		MY_TRACE_HEX(dwp);
		if (!dwp) return;
	}

	BOOL result = ::EndDeferWindowPos(dwp);
	MY_TRACE_INT(result);
}

void showComboBoxContextMenu(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	MY_TRACE(_T("showComboBoxContextMenu(0x%08X, 0x%08X, 0x%08X)\n"), hwnd, wParam, lParam);

	HWND myFontComboBox = (HWND)wParam;
	HWND fontComboBox = Exedit_TextObject_GetFontComboBox();

	tstring fontName = ComboBox_GetCurrentText(fontComboBox);
	tstring myFontName = ComboBox_GetCurrentText(myFontComboBox);

	TCHAR addText[MAX_PATH] = {};
	::StringCbPrintf(addText, sizeof(addText), _T("%s を追加"), fontName.c_str());
	TCHAR deleteText[MAX_PATH] = {};
	::StringCbPrintf(deleteText, sizeof(deleteText), _T("%s を削除"), myFontName.c_str());

	MY_TRACE_TSTR(addText);
	MY_TRACE_TSTR(deleteText);

	const UINT ID_ADD = 1;
	const UINT ID_DELETE = 2;

	HMENU menu = ::CreatePopupMenu();
	if (!fontName.empty() && ComboBox_FindString2(myFontComboBox, fontName.c_str()) < 0)
		::AppendMenu(menu, MF_STRING, ID_ADD, addText);
	if (!myFontName.empty())
		::AppendMenu(menu, MF_STRING, ID_DELETE, deleteText);

	int count = ::GetMenuItemCount(menu);
	MY_TRACE_INT(count);

	if (count == 0)
		return;

	POINTS pt = MAKEPOINTS(lParam);
	::SetForegroundWindow(hwnd);
	UINT id = (UINT)::TrackPopupMenuEx(menu, TPM_NONOTIFY | TPM_RETURNCMD, pt.x, pt.y, hwnd, 0);
	::PostMessage(hwnd, WM_NULL, 0, 0);
	MY_TRACE_INT(id);

	switch (id)
	{
	case ID_ADD:
		{
			ComboBox_AddFontName(myFontComboBox, fontName.c_str());

			break;
		}
	case ID_DELETE:
		{
			ComboBox_DeleteFontName(myFontComboBox, myFontName.c_str());

			break;
		}
	}
}

void showTreeViewContextMenu(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	MY_TRACE(_T("showTreeViewContextMenu(0x%08X, 0x%08X, 0x%08X)\n"), hwnd, wParam, lParam);

	HTREEITEM itemHandle = TreeView_GetSelection(g_favorite);
	MY_TRACE_HEX(itemHandle);

	TreeItemPtr item = NULL;
	_bstr_t label = L"";

	{
		TreeItemContainer::iterator it = g_favoriteItems.find(itemHandle);
		if (it != g_favoriteItems.end())
		{
			item = it->second;
			label = item->m_label;
		}
	}
	MY_TRACE_WSTR(label.GetBSTR());

	HWND fontComboBox = Exedit_TextObject_GetFontComboBox();
	tstring fontName = ComboBox_GetCurrentText(fontComboBox);

	TCHAR addText[MAX_PATH] = {};
	::StringCbPrintf(addText, sizeof(addText), _T("選択中の \"%ws\" の子要素として \"%s\" を追加"), label.GetBSTR(), fontName.c_str());
	MY_TRACE_TSTR(addText);

	TCHAR deleteText[MAX_PATH] = {};
	::StringCbPrintf(deleteText, sizeof(deleteText), _T("選択中の \"%ws\" を削除"), label.GetBSTR());
	MY_TRACE_TSTR(deleteText);

	TCHAR replaceNameText[MAX_PATH] = {};
	::StringCbPrintf(replaceNameText, sizeof(replaceNameText), _T("選択中の \"%ws\" のフォント名を \"%s\" で置き換える"), label.GetBSTR(), fontName.c_str());
	MY_TRACE_TSTR(replaceNameText);

	TCHAR eraseNameText[MAX_PATH] = {};
	::StringCbPrintf(eraseNameText, sizeof(eraseNameText), _T("選択中の \"%ws\" のフォント名を消去する"), label.GetBSTR());
	MY_TRACE_TSTR(eraseNameText);

	const UINT ID_ADD = 1;
	const UINT ID_DELETE = 2;
	const UINT ID_REPLACE_NAME = 3;
	const UINT ID_ERASE_NAME = 4;

	HMENU menu = ::CreatePopupMenu();
	::AppendMenu(menu, MF_STRING, ID_ADD, addText);
	::AppendMenu(menu, MF_STRING, ID_DELETE, deleteText);
	::AppendMenu(menu, MF_STRING, ID_REPLACE_NAME, replaceNameText);
	::AppendMenu(menu, MF_STRING, ID_ERASE_NAME, eraseNameText);

	POINTS pt = MAKEPOINTS(lParam);
	::SetForegroundWindow(hwnd);
	UINT id = (UINT)::TrackPopupMenuEx(menu, TPM_NONOTIFY | TPM_RETURNCMD, pt.x, pt.y, hwnd, 0);
	::PostMessage(hwnd, WM_NULL, 0, 0);
	MY_TRACE_INT(id);

	switch (id)
	{
	case ID_ADD:
		{
			TreeItemPtr item(new TreeItem());
			item->m_name = fontName.c_str();
			item->m_alias = L"";
			insertFavoriteItem(itemHandle, NULL, item);
			TreeView_Expand(g_favorite, itemHandle, TVE_EXPAND);
			::InvalidateRect(g_favorite, NULL, TRUE);

			break;
		}
	case ID_DELETE:
		{
			deleteFavoriteItem(itemHandle);
			::InvalidateRect(g_favorite, NULL, TRUE);

			break;
		}
	case ID_REPLACE_NAME:
		{
			item->m_name = fontName.c_str();
			updateFavoriteItem(itemHandle, item);
			::InvalidateRect(g_favorite, NULL, TRUE);

			break;
		}
	case ID_ERASE_NAME:
		{
			item->m_name = L"";
			updateFavoriteItem(itemHandle, item);
			::InvalidateRect(g_favorite, NULL, TRUE);

			break;
		}
	}
}

//---------------------------------------------------------------------

HWND Exedit_TextObject_GetFontComboBox()
{
	// 同じ ID のコントロールが複数ある場合があるので全検索を行う。
	HWND child = ::GetWindow(g_exeditObjectDialog, GW_CHILD);
	while (child)
	{
		// ID をチェックする。
		if (::GetDlgCtrlID(child) == ID_FONT_COMBO_BOX)
		{
			// クラス名もチェックする。
			TCHAR className[MAX_PATH] = {};
			::GetClassName(child, className, MAX_PATH);
			if (::lstrcmpi(className, WC_COMBOBOX) == 0)
				return child;

		}

		child = ::GetWindow(child, GW_HWNDNEXT);
	}

	return 0;
}

void Exedit_TextObject_SetFont(HWND fontComboBox, LPCTSTR text)
{
	// フォントコンボボックスの選択を切り替える。
	ComboBox_SelectString(fontComboBox, 0, text);

	// デフォルト処理のみ実行する。
	g_defaultProcessingOnly = TRUE;
	::SendMessage(
		g_exeditObjectDialog,
		WM_COMMAND,
		MAKEWPARAM(ID_FONT_COMBO_BOX, CBN_SELCHANGE),
		(LPARAM)fontComboBox);
	g_defaultProcessingOnly = FALSE;
}

//---------------------------------------------------------------------

LRESULT CALLBACK container_wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		{
			MY_TRACE(_T("container_wndProc(WM_DESTROY, 0x%08X, 0x%08X)\n"), wParam, lParam);

			// ファイルに設定を保存する。
			saveSettings();

			break;
		}
	case WM_SIZE:
		{
			MY_TRACE(_T("container_wndProc(WM_SIZE, 0x%08X, 0x%08X)\n"), wParam, lParam);

			// レイアウトを更新する。
			recalcLayout();

			break;
		}
	case WM_WINDOWPOSCHANGED:
		{
			MY_TRACE(_T("container_wndProc(WM_WINDOWPOSCHANGED, 0x%08X, 0x%08X)\n"), wParam, lParam);

			// コンテナウィンドウの位置をグローバル変数に保存する。
			::GetWindowRect(g_container, &g_containerRect);

			break;
		}
	case WM_CONTEXTMENU:
		{
			MY_TRACE(_T("WM_CONTEXTMENU, 0x%08X, 0x%08X\n"), wParam, lParam);

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
			MY_TRACE(_T("container_wndProc(WM_COMMAND, 0x%08X, 0x%08X)\n"), wParam, lParam);

			// WM_COMMAND メッセージの引数を分解する。
			UINT code = HIWORD(wParam);
			UINT id = LOWORD(wParam);
			HWND sender = (HWND)lParam;

			if (code == CBN_SELCHANGE) // コンテナ内のコンボボックスの選択が切り替えられた。
			{
				MY_TRACE(_T("container_wndProc(CBN_SELCHANGE, 0x%08X, 0x%08X)\n"), id, sender);

				// フォントコンボボックスを取得する。
				HWND fontComboBox = Exedit_TextObject_GetFontComboBox();
				if (!fontComboBox) break;

				// 選択されているフォント名を取得する。
				tstring text = ComboBox_GetCurrentText(sender);

				// フォントコンボボックスの選択を切り替える。
				Exedit_TextObject_SetFont(fontComboBox, text.c_str());
			}

			break;
		}
	case WM_NOTIFY:
		{
			//MY_TRACE(_T("container_wndProc(WM_NOTIFY, 0x%08X, 0x%08X)\n"), wParam, lParam);

			switch (wParam)
			{
			case ID_FAVORITE:
				{
					NMHDR* nm = (NMHDR*)lParam;

					static BOOL g_ignoreNotify = FALSE;

					switch (nm->code)
					{
#if 0
					case NM_CLICK:
						{
							MY_TRACE(_T("container_wndProc(NM_CLICK)\n"));

							TVHITTESTINFO hi = {};
							::GetCursorPos(&hi.pt);
							::ScreenToClient(g_favorite, &hi.pt);
							TreeView_HitTest(g_favorite, &hi);
							MY_TRACE_HEX(hi.hItem);
							MY_TRACE_HEX(hi.flags);

							if (hi.hItem && !(hi.flags & (TVHT_ONITEMBUTTON | TVHT_ONITEMLABEL)))
							{
								MY_TRACE(_T("クリック位置がアイテムのボタンとラベル以外の部分ですが、アイテムを選択します\n"));

								TreeView_SelectItem(g_favorite, hi.hItem);
							}

							break;
						}
#endif
#if 0
					case NM_DBLCLK:
						{
							MY_TRACE(_T("container_wndProc(NM_DBLCLK)\n"));

							// TVS_FULLROWSELECT を指定していると、
							// この処理を実行してもダブルクリックでアイテムを展開できない。

							TVHITTESTINFO hi = {};
							::GetCursorPos(&hi.pt);
							::ScreenToClient(g_favorite, &hi.pt);
							TreeView_HitTest(g_favorite, &hi);
							MY_TRACE_HEX(hi.hItem);
							MY_TRACE_HEX(hi.flags);

							if (hi.hItem && !(hi.flags & (TVHT_ONITEMBUTTON | TVHT_ONITEMLABEL)))
							{
								TreeView_Expand(g_favorite, hi.hItem, TVE_TOGGLE);
								::InvalidateRect(g_favorite, NULL, TRUE);
							}

							break;
						}
#endif
					case NM_RCLICK:
						{
							MY_TRACE(_T("container_wndProc(NM_RCLICK)\n"));

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
							MY_TRACE(_T("container_wndProc(TVN_SELCHANGED)\n"));

							if (g_ignoreNotify) // フラグが立っている場合は何もしない。
							{
								MY_TRACE(_T("TVN_SELCHANGED を無視します)\n"));
								break;
							}

							NMTREEVIEW* nm = (NMTREEVIEW*)lParam;

							// コンテナ内のツリービューの選択が切り替えられた。

							// フォントコンボボックスを取得する。
							HWND fontComboBox = Exedit_TextObject_GetFontComboBox();
							if (!fontComboBox) break;

							TreeItemContainer::iterator it = g_favoriteItems.find(nm->itemNew.hItem);
							if (it == g_favoriteItems.end()) break;
							TreeItemPtr& item = it->second;
							MY_TRACE_WSTR((BSTR)item->m_name);
							if (item->m_name.length() == 0) break;

							// フォントコンボボックスの選択を切り替える。
							Exedit_TextObject_SetFont(fontComboBox, item->m_name);

							break;
						}
					}

					break;
				}
			}

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
//			MY_TRACE(_T("hook_exeditObjectDialog_wndProc(WM_COMMAND, 0x%08X, 0x%08X)\n"), wParam, lParam);

			if (g_defaultProcessingOnly)
				break; // フラグが立っている場合は通常処理だけ。独自処理は行わない。

			// WM_COMMAND メッセージの引数を分解する。
			UINT code = HIWORD(wParam);
			UINT id = LOWORD(wParam);
			HWND sender = (HWND)lParam;

			if (code == CBN_SELCHANGE && id == ID_FONT_COMBO_BOX)
			{
				MY_TRACE(_T("hook_exeditObjectDialog_wndProc(CBN_SELCHANGE, 0x%08X, 0x%08X)\n"), id, sender);

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
				MY_TRACE(_T("hook_exeditObjectDialog_wndProc(CBN_DROPDOWN, 0x%08X, 0x%08X)\n"), id, sender);

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
					MY_TRACE(_T("拡張編集のオブジェクト編集ダイアログをフックします\n"));

					// オブジェクト編集ダイアログのウィンドウハンドルをグローバル変数に保存しておく。
					g_exeditObjectDialog = cwpr->hwnd;
					MY_TRACE_HEX(g_exeditObjectDialog);

					// コンテナウィンドウを作成する。
					createContainer();

					// ファイルから設定を読み込む。
					loadSettings();
				}

				break;
			}
		case WM_SHOWWINDOW:
			{
//				MY_TRACE(_T("WM_SHOWWINDOW, 0x%08X, 0x%08X\n"), cwpr->wParam, cwpr->lParam);

				// フォントコンボボックスを取得する。
				HWND fontComboBox = Exedit_TextObject_GetFontComboBox();
				if (!fontComboBox) break;

				// フォントコンボボックスか調べる。
				if (cwpr->hwnd == fontComboBox)
				{
					MY_TRACE(_T("フォントコンボボックスの WM_SHOWWINDOW\n"));

					if (cwpr->wParam)
					{
						MY_TRACE(_T("フォントコンボボックスが表示されました\n"));

						recalcLayout();
						showContainer();
					}
					else
					{
						MY_TRACE(_T("フォントコンボボックスが非表示になりました\n"));

						hideContainer();
					}
				}

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
				HWND fontComboBox = Exedit_TextObject_GetFontComboBox();
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

		if (cwpr->hwnd == g_exeditObjectDialog)
		{
			hook_exeditObjectDialog_wndProc(cwpr->hwnd, cwpr->message, cwpr->wParam, cwpr->lParam);
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

//---------------------------------------------------------------------
//		フィルタ構造体のポインタを渡す関数
//---------------------------------------------------------------------
EXTERN_C FILTER_DLL __declspec(dllexport) * __stdcall GetFilterTable(void)
{
	static TCHAR g_filterName[] = TEXT("お気に入りフォント選択");
	static TCHAR g_filterInformation[] = TEXT("お気に入りフォント選択 version 5.2.1 by 蛇色");

	static FILTER_DLL g_filter =
	{
		FILTER_FLAG_NO_CONFIG |
		FILTER_FLAG_ALWAYS_ACTIVE |
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
//		初期化
//---------------------------------------------------------------------

BOOL func_init(FILTER *fp)
{
	// ロケールを設定する。これをやらないと日本語テキストが文字化けする。
	_tsetlocale(LC_ALL, _T(""));

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

	// フォントリストファイルのパスを取得しておく。
	::GetModuleFileName(g_instance, g_recentFileName, MAX_PATH);
	::PathRenameExtension(g_recentFileName, _T(".recent.txt"));
	::GetModuleFileName(g_instance, g_favoriteFileName, MAX_PATH);
	::PathRenameExtension(g_favoriteFileName, _T(".favorite.txt"));

	// コントロール用のフォントを取得しておく。
	SYS_INFO si = {};
	fp->exfunc->get_sys_info(NULL, &si);
	g_font = si.hfont;
	MY_TRACE_HEX(g_font);

	// CallWndProcRet フックを開始する。
	hook();

	return TRUE;
}

//---------------------------------------------------------------------
//		終了
//---------------------------------------------------------------------
BOOL func_exit(FILTER *fp)
{
	MY_TRACE(_T("func_term()\n"));

	// CallWndProcRet フックを終了する。
	unhook();

	return TRUE;
}

//---------------------------------------------------------------------
