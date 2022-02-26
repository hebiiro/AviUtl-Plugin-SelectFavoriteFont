#pragma once

//---------------------------------------------------------------------
// Define and Const

// フォントコンボボックスに割り当てられている ID。
// ただし、他のコントロールにも同じ ID が
// 割り当てられている場合があるので注意が必要。(例えばグループ制御の対象レイヤー数)
const UINT ID_FONT_COMBO_BOX = 0x5654;

// このプラグインで作成するコントロールの ID。
const UINT ID_RECENT = 2021;
const UINT ID_FAVORITE= 2022;

const UINT WEB_DROP_DOWN_LIST = 0; // プレビュー用の Window Extra Bytes。
const UINT WM_MY_DROPDOWN = WM_APP + 100; // フォントコンボボックスのドロップダウンリストが表示されたときに使用するメッセージ。

//---------------------------------------------------------------------
// Class

class TreeItem
{
public:
	_bstr_t m_name;
	_bstr_t m_alias;
	_bstr_t m_label;
	BOOL m_expand;

	TreeItem();
	void updateLabel();
};

typedef std::shared_ptr<TreeItem> TreeItemPtr;
typedef std::map<HTREEITEM, TreeItemPtr> TreeItemContainer;

//---------------------------------------------------------------------
// Variable

extern HINSTANCE g_instance;
extern HFONT g_font;
extern TCHAR g_recentFileName[MAX_PATH];
extern TCHAR g_favoriteFileName[MAX_PATH];
extern TCHAR g_settingsFileName[MAX_PATH];
extern BOOL g_isSettingsFileLoaded;
extern BOOL g_defaultProcessingOnly;

extern HHOOK g_hook;
extern HWND g_exeditObjectDialog;
extern HWND g_container;
extern HWND g_recent;
extern HWND g_favorite;

extern RECT g_containerRect;
extern _bstr_t g_labelFormat;
extern _bstr_t g_separatorFormat;

extern TreeItemContainer g_favoriteItems;

//---------------------------------------------------------------------
// Function

HRESULT loadSettings();
HRESULT loadSetting(const MSXML2::IXMLDOMElementPtr& element);
HRESULT loadRecent(const MSXML2::IXMLDOMElementPtr& element);
HRESULT loadFavorite(const MSXML2::IXMLDOMElementPtr& element);
HRESULT loadFonts(const MSXML2::IXMLDOMElementPtr& element, HTREEITEM parentHandle);

HRESULT saveSettings();
HRESULT saveSetting(const MSXML2::IXMLDOMElementPtr& element);
HRESULT saveRecent(const MSXML2::IXMLDOMElementPtr& element);
HRESULT saveFavorite(const MSXML2::IXMLDOMElementPtr& element);
HRESULT saveFonts(const MSXML2::IXMLDOMElementPtr& element, HTREEITEM parentHandle);

int GetWidth(LPCRECT rc);
int GetHeight(LPCRECT rc);
void SetClientRect(HWND hwnd, int x, int y, int w, int h);

tstring ComboBox_GetLBText2(HWND comboBox, int index);
int ComboBox_FindString2(HWND comboBox, LPCTSTR fontName);
tstring ComboBox_GetCurrentText(HWND comboBox);
void ComboBox_AddFontName(HWND comboBox, LPCTSTR fontName);
void ComboBox_DeleteFontName(HWND comboBox, LPCTSTR fontName);
int ComboBox_SelectFontName(HWND comboBox, LPCTSTR fontName);

HTREEITEM insertFavoriteItem(HTREEITEM parentHandle, HTREEITEM insertAfter, const TreeItemPtr& item);
void deleteFavoriteItem(HTREEITEM itemHandle);
void updateFavoriteItem(HTREEITEM itemHandle, const TreeItemPtr& item);
void expandFavoriteItems(HTREEITEM parentHandle);

void createContainer();
void destroyContainer();
void showContainer();
void hideContainer();
void showComboBoxContextMenu(HWND hwnd, WPARAM wParam, LPARAM lParam);
void showTreeViewContextMenu(HWND hwnd, WPARAM wParam, LPARAM lParam);

HWND Exedit_TextObject_GetFontComboBox();
void Exedit_TextObject_SetFont(HWND fontComboBox, LPCTSTR text);

LRESULT CALLBACK container_wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK hook_exeditObjectDialog_wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK cwprProc(int code, WPARAM wParam, LPARAM lParam);
void hook();
void unhook();

void preview_create();
void preview_init(HWND dropDownList);
void preview_show();
void preview_hide();
void preview_recalcLayout();
void preview_draw(HDC dc);
void preview_refresh();
void preview_dropDown(HWND fontComboBox, HWND dropDownList);
HRESULT loadPreview(const MSXML2::IXMLDOMElementPtr& element);
HRESULT savePreview(const MSXML2::IXMLDOMElementPtr& element);
LRESULT CALLBACK preview_wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

//---------------------------------------------------------------------
