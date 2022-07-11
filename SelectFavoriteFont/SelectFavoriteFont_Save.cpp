#include "pch.h"
#include "SelectFavoriteFont.h"

//---------------------------------------------------------------------
// Save

HRESULT saveSettings()
{
	MY_TRACE(_T("saveSettings\n"));

	// 設定ファイルを読み込めていない場合は、保存処理をするとファイルの中の設定値がすべて初期値に戻ってしまう。
	// よって、フラグを確認して設定ファイルを読み込めていない場合は保存処理をしないようにする。
	if (!g_isSettingsFileLoaded)
		return S_FALSE;

	try
	{
		// ドキュメントを作成する。
		MSXML2::IXMLDOMDocumentPtr document(__uuidof(MSXML2::DOMDocument));

		// ドキュメントエレメントを作成する。
		MSXML2::IXMLDOMElementPtr element = appendElement(document, document, L"settings");

		// <setting> を作成する。
		saveSetting(element);

		// <preview> を作成する。
		savePreview(element);

		// <favorite> を作成する。
		saveFavorite(element);

		// <recent> を作成する。
		saveRecent(element);

		return saveXMLDocument(document, g_settingsFileName, L"UTF-16");
	}
	catch (_com_error& e)
	{
		MY_TRACE(_T("%s\n"), e.ErrorMessage());
		return e.Error();
	}
}

// <setting> を作成する。
HRESULT saveSetting(const MSXML2::IXMLDOMElementPtr& element)
{
	MY_TRACE(_T("saveSetting()\n"));

	// <setting> を作成する。
	MSXML2::IXMLDOMElementPtr settingElement = appendElement(element, L"setting");

	// <setting> のアトリビュートを作成する。
	setPrivateProfileString(settingElement, L"labelFormat", g_labelFormat);
	setPrivateProfileString(settingElement, L"separatorFormat", g_separatorFormat);

	return S_OK;
}

// <recent> を作成する。
HRESULT saveRecent(const MSXML2::IXMLDOMElementPtr& element)
{
	MY_TRACE(_T("saveRecent()\n"));

	// <recent> を作成する。
	MSXML2::IXMLDOMElementPtr recentElement = appendElement(element, L"recent");

	int count = ComboBox_GetCount(g_recent);
	for (int i = 0; i < count; i++)
	{
		// <font> を作成する。
		MSXML2::IXMLDOMElementPtr fontElement = appendElement(recentElement, L"font");

		tstring name = ComboBox_GetLBText2(g_recent, i);

		// <font> のアトリビュートを作成する。
		setPrivateProfileString(fontElement, L"name", name.c_str());
	}

	return S_OK;
}

// <favorite> を作成する。
HRESULT saveFavorite(const MSXML2::IXMLDOMElementPtr& element)
{
	MY_TRACE(_T("saveFavorite()\n"));

	// <favorite> を作成する。
	MSXML2::IXMLDOMElementPtr favoriteElement = appendElement(element, L"favorite");

	// 再帰的に <font> を作成する。
	return saveFonts(favoriteElement, TVI_ROOT);
}

// 再帰的に <font> を作成する。
HRESULT saveFonts(const MSXML2::IXMLDOMElementPtr& element, HTREEITEM parentHandle)
{
	MY_TRACE(_T("saveFonts(0x%08X)\n"), parentHandle);

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

			// <font> を作成する。
			MSXML2::IXMLDOMElementPtr fontElement = appendElement(element, L"font");

			// <font> のアトリビュートを作成する。

			// alias を作成する。
			if (item->m_alias.length())
				setPrivateProfileString(fontElement, L"alias", item->m_alias);

			// name を作成する。
			if (item->m_name.length())
				setPrivateProfileString(fontElement, L"name", item->m_name);

			// expand を作成する。
			if (item->m_expand)
				setPrivateProfileBool(fontElement, L"expand", item->m_expand);

			// 再帰的に子要素を作成する。
			saveFonts(fontElement, itemHandle);
		}

		// ツリービュー内の次の子アイテムのハンドルを取得する。
		itemHandle = TreeView_GetNextSibling(g_favorite, itemHandle);
	}

	return S_OK;
}

//---------------------------------------------------------------------
