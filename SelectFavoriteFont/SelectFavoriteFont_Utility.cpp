#include "pch.h"
#include "SelectFavoriteFont.h"

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

// クライアント矩形が指定された位置に来るようにウィンドウの位置を調整する
void SetClientRect(HWND hwnd, int x, int y, int w, int h)
{
	// 現在のウィンドウ矩形とクライアント矩形の座標を取得する。
	RECT rcWindow; ::GetWindowRect(hwnd, &rcWindow);
	RECT rcClient; ::GetClientRect(hwnd, &rcClient);

	// クライアント矩形をスクリーン座標に変換する。
	::MapWindowPoints(hwnd, NULL, (LPPOINT)&rcClient, 2);

	// 幅と高さを取得する。
	int rcWindowWidth = GetWidth(&rcWindow);
	int rcWindowHeight = GetHeight(&rcWindow);
	int rcClientWidth = GetWidth(&rcClient);
	int rcClientHeight = GetHeight(&rcClient);

	// 指定されたクライアント矩形の座標から新しいウィンドウ矩形の座標を算出する。
	x += rcWindow.left - rcClient.left;
	y += rcWindow.top - rcClient.top;
	w += rcWindowWidth - rcClientWidth;
	h += rcWindowHeight - rcClientHeight;

	::SetWindowPos(hwnd, NULL,
		x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
}

//---------------------------------------------------------------------

// コンボボックス用関数
// 指定されたインデックスに該当するアイテムのテキストを返す
tstring ComboBox_GetLBText2(HWND comboBox, int index)
{
//	MY_TRACE(_T("ComboBox_GetLBText2(0x%08X, %d)\n"), comboBox, index);

	// テキストの長さを取得する。
	int textLength = ComboBox_GetLBTextLen(comboBox, index);
//	MY_TRACE_INT(textLength);
	if (textLength <= 0) return _T("");

	// テキストを取得する。
	tstring text(textLength, _T('\0'));
	ComboBox_GetLBText(comboBox, index, &text[0]);
//	MY_TRACE_TSTR(text.c_str());

	return text;
}

// コンボボックスのリストから fontName を見つけてそのインデックスを返す
// (CB_FINDSTRING は部分一致でもヒットするので使用できない)
int ComboBox_FindString2(HWND comboBox, LPCTSTR fontName)
{
//	MY_TRACE(_T("ComboBox_FindString2(0x%08X, %s)\n"), comboBox, fontName);

	int count = ComboBox_GetCount(comboBox);
	for (int i = 0; i < count; i++)
	{
		// テキストを取得する。
		tstring text = ComboBox_GetLBText2(comboBox, i);
		MY_TRACE_TSTR(text.c_str());
		if (text.empty()) continue;

		// テキストが完全一致するかチェックする。
		if (::lstrcmp(text.c_str(), fontName) == 0)
			return i;
	}

	return CB_ERR;
}

// コンボボックスで現在選択されているアイテムのテキストを返す
tstring ComboBox_GetCurrentText(HWND comboBox)
{
	// 選択インデックスを取得する。
	int sel = ComboBox_GetCurSel(comboBox);
	MY_TRACE_INT(sel);
	if (sel < 0) return _T("");

	// テキストを取得する。
	tstring text = ComboBox_GetLBText2(comboBox, sel);
	MY_TRACE_TSTR(text.c_str());

	// テキストを返す。
	return text;
}

// コンボボックスにテキストを追加する。ただし、すでに存在する場合は追加しない
void ComboBox_AddFontName(HWND comboBox, LPCTSTR fontName)
{
	// 指定されたフォント名がすでに存在するか調べる。
	int index = ComboBox_FindString2(comboBox, fontName);
	if (index >= 0)
	{
		// 一旦このフォント名を削除する。
		ComboBox_DeleteString(comboBox, index);
	}

	// リストの先頭にフォント名を追加する。
	ComboBox_InsertString(comboBox, 0, fontName);
	ComboBox_SetCurSel(comboBox, 0);
}

// コンボボックスからテキストを削除する
void ComboBox_DeleteFontName(HWND comboBox, LPCTSTR fontName)
{
	// 指定されたフォント名がすでに存在するか調べる。
	int index = ComboBox_FindString2(comboBox, fontName);
	if (index >= 0)
	{
		// 一旦このフォント名を削除する。
		ComboBox_DeleteString(comboBox, index);

		// 要素数が 0 になったとき再描画が発行されないので、ここで明示的に発行する。
		::InvalidateRect(comboBox, NULL, TRUE);
	}
}

// 指定されたテキストを持つアイテムを選択肢し、そのインデックスを返す
int ComboBox_SelectFontName(HWND comboBox, LPCTSTR fontName)
{
	// 指定されたフォント名が存在するか調べる。
	int index = ComboBox_FindString2(comboBox, fontName);
	if (index >= 0)
	{
		// 指定されたフォント名が存在する場合は選択する。
		ComboBox_SetCurSel(comboBox, index);
	}

	// インデックスを返す。
	return index;
}

//---------------------------------------------------------------------
