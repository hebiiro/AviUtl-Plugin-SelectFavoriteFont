#include "pch.h"
#include "SelectFavoriteFont.h"

//---------------------------------------------------------------------
// Load

HRESULT loadSettings()
{
	MY_TRACE(_T("loadSettings()\n"));

	try
	{
		// MSXML を使用する。
		MSXML2::IXMLDOMDocumentPtr document(__uuidof(MSXML2::DOMDocument));

		// 設定ファイルを開く。
		if (document->load(g_settingsFileName) == VARIANT_FALSE)
		{
			MY_TRACE(_T("%s を開けませんでした\n"), g_settingsFileName);

			return S_FALSE;
		}

		g_isSettingsFileLoaded = FALSE;

		// これからファイルからアイテムを読み込むので、その前に既存の全アイテムを削除する。
		ComboBox_ResetContent(g_recent);
		TreeView_DeleteAllItems(g_favorite);
		g_favoriteItems.clear();

		MSXML2::IXMLDOMElementPtr element = document->documentElement;

		// <setting> を読み込む。
		loadSetting(element);

		// <recent> を読み込む。
		loadRecent(element);

		// <favorite> を読み込む。
		loadFavorite(element);

		MY_TRACE(_T("設定ファイルの読み込みに成功しました\n"));

		// 設定ファイルを読み込めたのでフラグを立てておく。
		g_isSettingsFileLoaded = TRUE;

		expandFavoriteItems(TVI_ROOT);

		return S_OK;
	}
	catch (_com_error& e)
	{
		MY_TRACE(_T("設定ファイルの読み込みに失敗しました\n"));
		MY_TRACE(_T("%s\n"), e.ErrorMessage());
		return e.Error();
	}
}

// <setting> を読み込む。
HRESULT loadSetting(const MSXML2::IXMLDOMElementPtr& element)
{
	MY_TRACE(_T("loadSetting()\n"));

	// <setting> を読み込む。
	MSXML2::IXMLDOMNodeListPtr nodeList = element->selectNodes(L"setting");
	int c = nodeList->length;
	for (int i = 0; i < c; i++)
	{
		MSXML2::IXMLDOMElementPtr settingElement = nodeList->item[i];

		// <setting> のアトリビュートを読み込む。

		int x = g_containerRect.left;
		int y = g_containerRect.top;
		int w = GetWidth(&g_containerRect);
		int h = GetHeight(&g_containerRect);

		getPrivateProfileReal(settingElement, L"x", x);
		getPrivateProfileReal(settingElement, L"y", y);
		getPrivateProfileReal(settingElement, L"w", w);
		getPrivateProfileReal(settingElement, L"h", h);
		getPrivateProfileString(settingElement, L"labelFormat", g_labelFormat);
		getPrivateProfileString(settingElement, L"separatorFormat", g_separatorFormat);

		MY_TRACE_INT(x);
		MY_TRACE_INT(y);
		MY_TRACE_INT(w);
		MY_TRACE_INT(h);
		MY_TRACE_WSTR(g_labelFormat.GetBSTR());
		MY_TRACE_WSTR(g_separatorFormat.GetBSTR());

		g_containerRect.left = x;
		g_containerRect.top = y;
		g_containerRect.right = x + w;
		g_containerRect.bottom = y + h;

		::MoveWindow(g_container, x, y, w, h, TRUE);
	}

	return S_OK;
}

// <recent> を読み込む。
HRESULT loadRecent(const MSXML2::IXMLDOMElementPtr& element)
{
	MY_TRACE(_T("loadRecent()\n"));

	// <recent> を読み込む。
	MSXML2::IXMLDOMNodeListPtr nodeList = element->selectNodes(L"recent");
	int c = nodeList->length;
	for (int i = 0; i < c; i++)
	{
		MSXML2::IXMLDOMElementPtr recentElement = nodeList->item[i];

		// <recent> 内の <font> を読み込む。
		MSXML2::IXMLDOMNodeListPtr nodeList = recentElement->selectNodes(L"font");
		int c = nodeList->length;
		for (int i = 0; i < c; i++)
		{
			MSXML2::IXMLDOMElementPtr fontElement = nodeList->item[i];

			// <recent> 内の <font> のアトリビュートを読み込む。

			// <font> の name を読み込む。
			_bstr_t name;
			getPrivateProfileString(fontElement, L"name", name);
			MY_TRACE_WSTR(name.GetBSTR());

			if (!name.GetBSTR() || !*name.GetBSTR())
				continue; // name が存在しない場合は読み込まない。

			ComboBox_AddString(g_recent, name);
		}
	}

	return S_OK;
}

// <favorite> を読み込む。
HRESULT loadFavorite(const MSXML2::IXMLDOMElementPtr& element)
{
	MY_TRACE(_T("loadFavorite()\n"));

	// <favorite> を読み込む。
	MSXML2::IXMLDOMNodeListPtr nodeList = element->selectNodes(L"favorite");
	int c = nodeList->length;
	for (int i = 0; i < c; i++)
	{
		MSXML2::IXMLDOMElementPtr favoriteElement = nodeList->item[i];

		// 再帰的に <font> を読み込む。
		loadFonts(favoriteElement, TVI_ROOT);
	}

	return S_OK;
}

// 再帰的に <font> を読み込む。
HRESULT loadFonts(const MSXML2::IXMLDOMElementPtr& element, HTREEITEM parentHandle)
{
	MY_TRACE(_T("loadFonts()\n"));

	// <font> を読み込む。
	MSXML2::IXMLDOMNodeListPtr nodeList = element->selectNodes(L"font");
	int c = nodeList->length;
	for (int i = 0; i < c; i++)
	{
		MSXML2::IXMLDOMElementPtr fontElement = nodeList->item[i];

		TreeItemPtr item(new TreeItem());

		// <font> のアトリビュートを読み込む。

		// <font> の name を読み込む。
		getPrivateProfileString(fontElement, L"name", item->m_name);
		MY_TRACE_WSTR(item->m_name.GetBSTR());

		// <font> の alias を読み込む。
		getPrivateProfileString(fontElement, L"alias", item->m_alias);
		MY_TRACE_WSTR(item->m_alias.GetBSTR());

		// <font> の expand を読み込む。
		getPrivateProfileBool(fontElement, L"expand", item->m_expand);
		MY_TRACE_INT(item->m_expand);

		// parent に新規ツリーアイテムを追加する。
		HTREEITEM itemHandle = insertFavoriteItem(parentHandle, TVI_LAST, item);

		// 再帰的に子要素を読み込む。
		loadFonts(fontElement, itemHandle);
	}

	return S_OK;
}

//---------------------------------------------------------------------
