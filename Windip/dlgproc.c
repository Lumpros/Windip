#include "dlgproc.h"
#include "resource.h"
#include "utility.h"

#include <stdlib.h>
#include <CommCtrl.h>
#include <dwmapi.h>
#include <wchar.h>

#ifdef NULL
	#undef NULL
	#define NULL 0
#endif

struct tree_data
{
	HWND hTree;
	HTREEITEM hItem;
};

static void add_children_to_treeitem(HWND hTree, HTREEITEM hParent);

static HWND get_target_handle(HWND hDialog)
{
	return (HWND)GetWindowLongPtr(hDialog, GWLP_USERDATA);
}

static void refresh_dialog_title(HWND hDlg)
{
	HWND hTarget = get_target_handle(hDlg);

	const wchar_t windip_indicator[] = L" (Windip Dialog)";

	int length = GetWindowTextLength(hTarget) + 1;

	if (length > 1)
	{
		int total_length = length + lstrlen(windip_indicator);

		wchar_t* title = (wchar_t*)malloc(total_length * sizeof(wchar_t));
		
		if (title != NULL)
		{
			GetWindowText(hTarget, title, length);

			lstrcat(title, windip_indicator);

			SetWindowText(hDlg, title);
		}

		free(title);
	}

	else {
		wchar_t class_name[256];
		GetClassName(hTarget, class_name, 256);

	}
}

static void set_icon_to_target_icon(HWND hDlg, HWND hTarget)
{
	HICON hIcon = (HICON)SendMessage(hTarget, WM_GETICON, ICON_BIG, 96);

	SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
}

static void update_target_style(HWND hTarget)
{
	DWORD dwExStyle = GetWindowLongPtr(hTarget, GWL_EXSTYLE);

	if ((dwExStyle & WS_EX_LAYERED) == 0)
	{
		SetWindowLongPtr(hTarget, GWL_EXSTYLE, dwExStyle | WS_EX_LAYERED);
		SetLayeredWindowAttributes(hTarget, NULL, 255, LWA_ALPHA);
	}
}

static int get_dlg_number(HWND hDlg, int id)
{
	wchar_t buf[16];
	GetDlgItemText(hDlg, id, buf, 16);
	return _wtoi(buf);
}

static void on_apply_position(HWND hDlg, HWND hTarget)
{
	int x = get_dlg_number(hDlg, IDC_X_EDIT);
	int y = get_dlg_number(hDlg, IDC_Y_EDIT);
	int width = get_dlg_number(hDlg, IDC_WIDTH_EDIT);
	int height = get_dlg_number(hDlg, IDC_HEIGHT_EDIT);

	SetWindowPos(hTarget,
				 NULL,
				 x, y,
				 width,
				 height,
				 SWP_NOZORDER);
}

static void apply_blur(HWND hDlg, HWND hTarget)
{
	HWND hSlider = GetDlgItem(hDlg, IDC_OPACITY_SLIDER);
	BYTE opacity = (BYTE)SendMessage(hSlider, TBM_GETPOS, NULL, NULL);
	SetLayeredWindowAttributes(hTarget, NULL, opacity, LWA_ALPHA);
}

static void apply_opacity(HWND hDlg, HWND hTarget)
{
	DWM_BLURBEHIND bb = { 0 };
	bb.dwFlags = DWM_BB_ENABLE;
	bb.fEnable = !!SendDlgItemMessage(hDlg, IDC_BLUR_CHECK, BM_GETCHECK, 0, 0);
	bb.hRgnBlur = NULL;
	DwmEnableBlurBehindWindow(hTarget, &bb);
}

static void on_apply_miscellaneous(HWND hDlg, HWND hTarget)
{
	apply_blur(hDlg, hTarget);
	apply_opacity(hDlg, hTarget);
}

static void on_apply_title(HWND hWnd, HWND hTarget)
{
	HWND hTitleEdit = GetDlgItem(hWnd, IDC_TITLE_EDIT);

	int title_length = GetWindowTextLength(hTitleEdit) + 1;

	if (title_length > 1)
	{
		wchar_t* new_title = (wchar_t*)malloc(title_length * sizeof(wchar_t));

		if (new_title)
		{
			GetWindowText(hTitleEdit, new_title, title_length);

			SetWindowText(hTarget, new_title);

			free(new_title);
		}
	}

	refresh_dialog_title(hWnd);
}

static void on_apply(HWND hDlg)
{
	HWND hTarget = get_target_handle(hDlg);
	on_apply_position(hDlg, hTarget);
	on_apply_miscellaneous(hDlg, hTarget);
	on_apply_title(hDlg, hTarget);
}

static void initialize_position_values(HWND hDlg, HWND hTarget)
{
	RECT rcWindow;
	GetWindowRect(hTarget, &rcWindow);

	HWND hParent = GetParent(hTarget);

	if (hParent != NULL)
	{
		RECT rcParentWindow = {0}, rcParentClient = {0};
		GetWindowRect(hParent, &rcParentWindow);
		GetClientRect(hParent, &rcParentClient);

		MARGINS margins = {0};
		margins.cxLeftWidth = ((rcParentWindow.right - rcParentWindow.left) - rcParentClient.right) / 2;
		margins.cyTopHeight = (rcParentWindow.bottom - rcParentWindow.top) - rcParentClient.bottom - margins.cxLeftWidth;
		OffsetRect(&rcWindow, 
				   -rcParentWindow.left - margins.cxLeftWidth,
				   -rcParentWindow.top - margins.cyTopHeight);
	}

	wchar_t buffer[16];
	_itow_s(rcWindow.left, buffer, 16, 10);
	SetDlgItemText(hDlg, IDC_X_EDIT, buffer);

	_itow_s(rcWindow.top, buffer, 16, 10);
	SetDlgItemText(hDlg, IDC_Y_EDIT, buffer);

	_itow_s(rcWindow.right - rcWindow.left, buffer, 16, 10);
	SetDlgItemText(hDlg, IDC_WIDTH_EDIT, buffer);

	_itow_s(rcWindow.bottom - rcWindow.top, buffer, 16, 10);
	SetDlgItemText(hDlg, IDC_HEIGHT_EDIT, buffer);
}

static void initialize_miscellaneous_values(HWND hDlg, HWND hTarget)
{
	BYTE bAlpha = 0xFF;
	GetLayeredWindowAttributes(hTarget, NULL, &bAlpha, NULL);

	HWND hSlider = GetDlgItem(hDlg, IDC_OPACITY_SLIDER);
	SendMessage(hSlider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 255));
	SendMessage(hSlider, TBM_SETPOS, TRUE, bAlpha);
}

BOOL CALLBACK child_window_enum(HWND hWnd, LPARAM lParam)
{
	struct tree_data* data = (struct tree_data*)lParam;

	wchar_t class_name[256], window_text[32];
	GetClassName(hWnd, class_name, 221);
	GetWindowText(hWnd, window_text, 32);
	swprintf_s(class_name, 256, L"%ls \"%ls\"", class_name, window_text);

	HTREEITEM hItem = utility_add_item_to_tree(data->hTree, data->hItem, class_name, (LPARAM)hWnd);
	add_children_to_treeitem(data->hTree, hItem);

	return TRUE;
}

static void add_children_to_treeitem(HWND hTree, HTREEITEM hParent)
{
	/* Gets the lParam, because it's the handle to the target window */
	TVITEM tvi;
	ZeroMemory(&tvi, sizeof(tvi));
	tvi.hItem = hParent;
	tvi.mask = TVIF_HANDLE;
	TreeView_GetItem(hTree, &tvi);

	struct tree_data data = {0};
	data.hItem = hParent;
	data.hTree = hTree;
	EnumChildWindows((HWND)tvi.lParam, child_window_enum, &data);
}

static void initialize_window_tree(HWND hDlg, HWND hTarget)
{
	HWND hTree = GetDlgItem(hDlg, IDC_CHILDREN_TREE);

	if (hTree != NULL)
	{
		TreeView_DeleteAllItems(hTree);

		wchar_t* title = utility_get_window_title(hTarget);

		if (title != NULL)
		{
			HTREEITEM hRoot = utility_set_tree_root(hTree, title, (LPARAM)hTarget);
			add_children_to_treeitem(hTree, hRoot);
			TreeView_Expand(hTree, hRoot, TVM_EXPAND);
			free(title);
		}
	}
}

static void initialize_title_edit(HWND hDlg, HWND hTarget)
{
	wchar_t* title = utility_get_window_title(hTarget);
	SetDlgItemText(hDlg, IDC_TITLE_EDIT, title);
	free(title);
}

static void initialize_control_values(HWND hDlg, HWND hTarget)
{
	initialize_position_values(hDlg, hTarget);
	initialize_miscellaneous_values(hDlg, hTarget);
	initialize_window_tree(hDlg, hTarget);
	initialize_title_edit(hDlg, hTarget);
}

static HTREEITEM get_clicked_item(HWND hTree)
{
	POINT ptCursor;
	GetCursorPos(&ptCursor);
	ScreenToClient(hTree, &ptCursor);

	TVHITTESTINFO info = { 0 };
	info.pt = ptCursor;
	TreeView_HitTest(hTree, &info);
	
	return info.hItem;
}

static void on_notify(HWND hDlg, LPARAM lParam)
{
	LPNMHDR hdr = (LPNMHDR)lParam;

	if (hdr->code == NM_DBLCLK)
	{
		HWND hTree = GetDlgItem(hDlg, IDC_CHILDREN_TREE);
		HTREEITEM hItem = get_clicked_item(hTree);

		if (hItem)
		{
			TVITEM tvi;
			ZeroMemory(&tvi, sizeof(tvi));
			tvi.mask = TVIF_HANDLE;
			tvi.hItem = hItem;
			TreeView_GetItem(hTree, &tvi);

			HWND hModeless = CreateDialogParam(GetModuleHandle(NULL),
											   MAKEINTRESOURCE(IDD_WINDOW_MANIP),
											   GetParent(hDlg),
											   WindipDialogProc,
											   tvi.lParam);

			ShowWindow(hModeless, SW_SHOW);
		}
	}
}

static void on_update(HWND hWnd)
{
	initialize_control_values(hWnd, get_target_handle(hWnd));
}

INT_PTR CALLBACK WindipDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);
		refresh_dialog_title(hWnd);
		set_icon_to_target_icon(hWnd, (HWND)lParam);
		update_target_style((HWND)lParam);
		initialize_control_values(hWnd, (HWND)lParam);
		SetTimer(hWnd, 0, 200, NULL);
		return TRUE;

	case WM_TIMER:
		if (!IsWindow(get_target_handle(hWnd)))
		{
			EndDialog(hWnd, 0);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{ 
		case IDC_APPLY:
			on_apply(hWnd);
			break;

		case IDC_UPDATE:
			on_update(hWnd);
			break;

		case IDC_DESTROY:
			PostMessage(get_target_handle(hWnd), WM_CLOSE, NULL, NULL);
			PostMessage(GetParent(hWnd), WM_COMMAND, IDC_REFRESH, NULL);
			break;
		}
		return TRUE;
			
	case WM_NOTIFY:
		on_notify(hWnd, lParam);
		return TRUE;

	case WM_CLOSE:
		EndDialog(hWnd, 0);
		return TRUE;
	}

	return FALSE;
}