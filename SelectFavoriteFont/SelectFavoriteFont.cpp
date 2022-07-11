#include "pch.h"
#include "SelectFavoriteFont.h"

//--------------------------------------------------------------------

HINSTANCE g_instance = 0; // この DLL のハンドル。
HFONT g_font = 0; // コントロール用のフォントハンドル。
TCHAR g_settingsFileName[MAX_PATH] = {};
BOOL g_isSettingsFileLoaded = FALSE;
BOOL g_defaultProcessingOnly = FALSE;

HHOOK g_hook = 0;
HWND g_settingDialog = 0;
HWND g_container = 0;
HWND g_recent = 0;
HWND g_favorite = 0;

_bstr_t g_labelFormat = L"%ws --- %ws";
_bstr_t g_separatorFormat = L"---------";

TreeItemContainer g_favoriteItems;

//--------------------------------------------------------------------

// デバッグ用コールバック関数。デバッグメッセージを出力する。
void ___outputLog(LPCTSTR text, LPCTSTR output)
{
	::OutputDebugString(output);
}

//--------------------------------------------------------------------

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

//--------------------------------------------------------------------

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
	g_favoriteItems[itemHandle] = item;
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

// コントロール群を作成し、初期設定する。
void createControls(HWND hwnd)
{
	MY_TRACE(_T("createControls(0x%08X)\n"), hwnd);

	g_container = hwnd;

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

// コントロール群の表示位置を調整する。
void recalcLayout()
{
	MY_TRACE(_T("recalcLayout()\n"));

	// 基準となる AviUtl (exedit) 側のコントロールとその矩形を取得する。
	HWND base = g_recent;
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
	HWND fontComboBox = ExEdit_TextObject_GetFontComboBox();

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

	HWND fontComboBox = ExEdit_TextObject_GetFontComboBox();
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

//--------------------------------------------------------------------

HWND ExEdit_TextObject_GetFontComboBox()
{
	// 同じ ID のコントロールが複数ある場合があるので全検索を行う。
	HWND child = ::GetWindow(g_settingDialog, GW_CHILD);
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

void ExEdit_TextObject_SetFont(HWND fontComboBox, LPCTSTR text)
{
	// フォントコンボボックスの選択を切り替える。
	ComboBox_SelectString(fontComboBox, 0, text);

	// デフォルト処理のみ実行する。
	g_defaultProcessingOnly = TRUE;
	::SendMessage(
		g_settingDialog,
		WM_COMMAND,
		MAKEWPARAM(ID_FONT_COMBO_BOX, CBN_SELCHANGE),
		(LPARAM)fontComboBox);
	g_defaultProcessingOnly = FALSE;
}

//--------------------------------------------------------------------
