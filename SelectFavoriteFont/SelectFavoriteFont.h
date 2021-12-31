﻿#pragma once

//---------------------------------------------------------------------
// Define and Const

const UINT ID_FONT_COMBO_BOX = 0x5654;

const UINT ID_RECENT = 2021;
const UINT ID_FAVORITE= 2022;

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

void Exedit_TextObject_SetFont(HWND fontComboBox, LPCTSTR text);

LRESULT CALLBACK container_wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK hook_exeditObjectDialog_wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK cwprProc(int code, WPARAM wParam, LPARAM lParam);
void hook();
void unhook();

//---------------------------------------------------------------------