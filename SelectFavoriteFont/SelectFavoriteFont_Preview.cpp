#include "pch.h"
#include "SelectFavoriteFont.h"

//---------------------------------------------------------------------

HWND g_preview = 0; // プレビューウィンドウのハンドル。

BOOL g_previewEnable = TRUE; // TRUE ならプレビューを表示する。
BOOL g_previewLeft = TRUE; // TRUE ならプレビューを左側に表示する。
int g_previewItemWidth = 600; // プレビューアイテムの幅。
int g_previewItemHeight = 36; // プレビューアイテムの高さ。
_bstr_t g_previewTextFormat = L"プレビュー(%s)"; // テキストの書式。
COLORREF g_previewFillColor = RGB(0xff, 0xff, 0xff); // 背景の色。
COLORREF g_previewTextColor = RGB(0x00, 0x00, 0x00); // テキストの色。

//---------------------------------------------------------------------

// ドロップダウンリストのウィンドウハンドルを返す。
inline static HWND getDropDownList()
{
	return (HWND)::GetWindowLong(g_preview, WEB_DROP_DOWN_LIST);
}

// リストボックスのテキストを返す。
static tstring ListBox_GetText2(HWND listBox, int index)
{
//	MY_TRACE(_T("ListoBox_GetLBText2(0x%08X, %d)\n"), listBox, index);

	// テキストの長さを取得する。
	int textLength = ListBox_GetTextLen(listBox, index);
//	MY_TRACE_INT(textLength);
	if (textLength <= 0) return _T("");

	// テキストを取得する。
	tstring text(textLength, _T('\0'));
	ListBox_GetText(listBox, index, &text[0]);
//	MY_TRACE_TSTR(text.c_str());

	return text;
}

// プレビューウィンドウを作成する。
void preview_create()
{
	// プレビューウィンドウのウィンドウクラスを登録する。
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(wc);
	wc.lpszClassName = _T("SelectFavoriteFont.Preview");
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = preview_wndProc;
	wc.hInstance = g_instance;
	wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.cbWndExtra = sizeof(HWND);
	::RegisterClassEx(&wc);

	// プレビューウィンドウを作成する。
	g_preview = ::CreateWindowEx(
		WS_EX_NOACTIVATE | WS_EX_TRANSPARENT,
		_T("SelectFavoriteFont.Preview"),
		_T("SelectFavoriteFont.Preview"),
		WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
		WS_POPUP | WS_BORDER,
		0, 0, 100, 100,
		g_exeditObjectDialog,
		0,
		g_instance,
		0);
	MY_TRACE_HEX(g_preview);
}

// プレビューウィンドウを初期化する。
void preview_init(HWND list)
{
	MY_TRACE(_T("preview_init(0x%08X)\n"), list);

	::SetWindowLong(g_preview, WEB_DROP_DOWN_LIST, (LONG)list);
}

// プレビューウィンドウを表示する。
void preview_show()
{
	MY_TRACE(_T("preview_show()\n"));

	if (g_previewEnable)
		::ShowWindow(g_preview, SW_SHOWNA);
}

// プレビューウィンドウを非表示にする。
void preview_hide()
{
	MY_TRACE(_T("preview_term()\n"));

	::ShowWindow(g_preview, SW_HIDE);
}

// プレビューウィンドウのレイアウトを再計算する。
void preview_recalcLayout()
{
	MY_TRACE(_T("preview_recalcLayout()\n"));

	// ドロップダウンリストを取得する。
	HWND list = getDropDownList();

	// ドロップダウンリストのウィンドウ矩形を取得する。
	RECT rc; ::GetWindowRect(list, &rc);
	int x = rc.right;
	int y = rc.top;
	int w = GetWidth(&rc);
	int h = GetHeight(&rc);

	// 非クライアント領域のサイズを取得する。
	DWORD style = GetWindowStyle(g_preview);
	HMENU menu = ::GetMenu(g_preview);
	DWORD exStyle = GetWindowExStyle(g_preview);
	RECT nc = { 0, 0, 0, 0 };
	::AdjustWindowRectEx(&nc, style, !!menu, exStyle);
	int ncWidth = GetWidth(&nc);
	int ncHeight = GetHeight(&nc);

	// スクロール情報を取得する。
	SCROLLINFO si = { sizeof(si) };
	si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE | SIF_TRACKPOS;
	::GetScrollInfo(list, SB_VERT, &si);

	// ユーザー指定のサイズからウィンドウサイズを算出する。
	int w2 = g_previewItemWidth + ncWidth;
	int h2 = g_previewItemHeight * si.nPage + ncHeight;

	if (g_previewItemWidth)
	{
		if (g_previewLeft)
			x = rc.left - w2;
		else
			x = rc.right;
		w = w2;
	}

	if (g_previewItemHeight)
	{
		y += (h - h2) / 2;
		h = h2;
	}

	// プレビューウィンドウを移動する。
	::MoveWindow(g_preview, x, y, w, h, TRUE);
}

// プレビューウィンドウを描画する。
void preview_draw(HDC dc)
{
	MY_TRACE(_T("preview_draw(0x%08X)\n"), dc);

	// クライアント矩形を取得する。
	RECT rc; ::GetClientRect(g_preview, &rc);
	int clientWidth = GetWidth(&rc);
	int clientHeight = GetHeight(&rc);

	// ドロップダウンリストを取得する。
	HWND list = getDropDownList();

	// フォント情報を取得する。
	LOGFONT lf = {};
	HFONT font = GetWindowFont(list);
	::GetObject(font, sizeof(lf), &lf);
	if (g_previewItemHeight) lf.lfHeight = g_previewItemHeight;

	// スクロール情報を取得する。
	SCROLLINFO si = { sizeof(si) };
	si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE | SIF_TRACKPOS;
	::GetScrollInfo(list, SB_VERT, &si);

	// アイテムのサイズを取得する。
	RECT rcItem; ListBox_GetItemRect(list, 0, &rcItem);
	int itemWidth = GetWidth(&rcItem);
	int itemHeight = GetHeight(&rcItem);

	// 項目の描画位置を算出する。
	int x = rc.left;
	int y = rc.top;
	int w = g_previewItemWidth ? g_previewItemWidth : itemWidth;
	int h = g_previewItemHeight ? g_previewItemHeight : itemHeight;

	// 背景を塗りつぶす。
	HBRUSH brush = ::CreateSolidBrush(g_previewFillColor);
	::FillRect(dc, &rc, brush);
	::DeleteObject(brush);

	// カラーを設定する。
	int oldBkMode = ::SetBkMode(dc, TRANSPARENT);
	COLORREF oldTextColor = ::SetTextColor(dc, g_previewTextColor);

	for (int i = 0; i < (int)si.nPage; i++)
	{
		// ドロップダウンリストのテキストを取得する。
		tstring text = ListBox_GetText2(list, i + si.nPos);

		if (!text.empty()) // テキストが取得できなかった場合は何もしない。
		{
			// ディスプレイ用のテキストを取得する。
			TCHAR displayText[MAX_PATH] = {};
			::StringCbPrintf(displayText, sizeof(displayText), g_previewTextFormat, text.c_str());

			// フォントをセットしてディスプレイ用のテキストを描画する。
			::StringCbCopy(lf.lfFaceName, sizeof(lf.lfFaceName), text.c_str());
			HFONT newFont = ::CreateFontIndirect(&lf);
			HFONT oldFont = (HFONT)::SelectObject(dc, newFont);
			::TextOut(dc, x, y, displayText, ::lstrlen(displayText));
			::SelectObject(dc, oldFont);
			::DeleteObject(newFont);
		}

		// 次の描画位置に変更する。
		y += h;
	}

	::SetBkMode(dc, oldBkMode);
	::SetTextColor(dc, oldTextColor);
}

// プレビューウィンドウを再描画する。
void preview_refresh()
{
	MY_TRACE(_T("preview_refresh()\n"));

	::InvalidateRect(g_preview, NULL, FALSE);
}

// プレビューの設定を読み込む。
HRESULT loadPreview(const MSXML2::IXMLDOMElementPtr& element)
{
	MY_TRACE(_T("loadPreview()\n"));

	// <preview> を読み込む。
	MSXML2::IXMLDOMNodeListPtr nodeList = element->selectNodes(L"preview");
	int c = nodeList->length;
	for (int i = 0; i < c; i++)
	{
		MSXML2::IXMLDOMElementPtr previewElement = nodeList->item[i];

		// <preview> のアトリビュートを読み込む。
		getPrivateProfileBool(previewElement, L"enable", g_previewEnable);
		getPrivateProfileBool(previewElement, L"left", g_previewLeft);
		getPrivateProfileInt(previewElement, L"itemWidth", g_previewItemWidth);
		getPrivateProfileInt(previewElement, L"itemHeight", g_previewItemHeight);
		getPrivateProfileString(previewElement, L"textFormat", g_previewTextFormat);
		getPrivateProfileColor(previewElement, L"fillColor", g_previewFillColor);
		getPrivateProfileColor(previewElement, L"textColor", g_previewTextColor);
	}

	return S_OK;
}

// プレビューの設定を保存する。
HRESULT savePreview(const MSXML2::IXMLDOMElementPtr& element)
{
	MY_TRACE(_T("savePreview()\n"));

	// <preview> を作成する。
	MSXML2::IXMLDOMElementPtr previewElement = appendElement(element, L"preview");

	// <preview> のアトリビュートを作成する。
	setPrivateProfileBool(previewElement, L"enable", g_previewEnable);
	setPrivateProfileBool(previewElement, L"left", g_previewLeft);
	setPrivateProfileInt(previewElement, L"itemWidth", g_previewItemWidth);
	setPrivateProfileInt(previewElement, L"itemHeight", g_previewItemHeight);
	setPrivateProfileString(previewElement, L"textFormat", g_previewTextFormat);
	setPrivateProfileColor(previewElement, L"fillColor", g_previewFillColor);
	setPrivateProfileColor(previewElement, L"textColor", g_previewTextColor);

	return S_OK;
}

// プレビューウィンドウのウィンドウプロシージャ。
LRESULT CALLBACK preview_wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_PAINT:
		{
			MY_TRACE(_T("preview_wndProc(WM_PAINT, 0x%08X, 0x%08X)\n"), wParam, lParam);

			// プレビューウィンドウの描画を行う。
			PAINTSTRUCT ps = {};
			HDC dc = ::BeginPaint(hwnd, &ps);
			RECT rc = ps.rcPaint;

			BP_PAINTPARAMS pp = { sizeof(pp) };
			HDC mdc = 0;
			HPAINTBUFFER pb = ::BeginBufferedPaint(dc, &rc, BPBF_COMPATIBLEBITMAP, &pp, &mdc);

			if (pb)
			{
				preview_draw(mdc);

				::EndBufferedPaint(pb, TRUE);
			}

			::EndPaint(hwnd, &ps);

			break;
		}
	}

	return ::DefWindowProc(hwnd, message, wParam, lParam);
}

//---------------------------------------------------------------------
