#include "proc.h"
#include "utility.h"
#include "resource.h"
#include "dlgproc.h"

#include <stdlib.h>
#include <CommCtrl.h>
#include <Uxtheme.h>
#include <dwmapi.h>
#include <shellapi.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "UxTheme.lib")

struct list_view_data
{
	HWND hListView;
	HIMAGELIST hImgList;
};

struct root_window_data 
{
	struct list_view_data list_data;
};

static struct root_window_data* retrieve_root_window_data(HWND hWnd)
{
	return (struct root_window_data*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
}

static BOOL should_show_process(HWND hWnd)
{
	if (IsWindowVisible(hWnd))
	{
		int length = GetWindowTextLength(hWnd) + 1;

		if (length > 1)
		{
			wchar_t* title = (wchar_t*)malloc(length * sizeof(wchar_t));

			if (title != NULL)
			{
				GetWindowText(hWnd, title, length);

				if (lstrcmp(title, L"Program Manager") != 0)
				{
					INT nCloaked = 0;
					DwmGetWindowAttribute(hWnd, DWMWA_CLOAKED, &nCloaked, sizeof(INT));
					return !nCloaked;
				}

				free(title);
			}
		}
	}

	return FALSE;
}

static BOOL CALLBACK count_enum_callback(HWND hWnd, LPARAM lParam)
{
	if (should_show_process(hWnd))
	{
		int* count = (int*)lParam;

		++(*count);
	}

	return TRUE;
}

static int count_valid_processes(void)
{
	int count = 0;
	EnumWindows(count_enum_callback, (LPARAM)&count);
	return count;
}

static int lv_item_index = 0;

static void add_window_icon_to_list(HWND hWnd, HIMAGELIST hImgList)
{
	HICON hIcon = (HICON)SendMessage(hWnd, WM_GETICON, ICON_BIG, 96);

	if (hIcon == NULL)
	{
		hIcon = GetClassLong(hWnd, GCL_HICON);

		if (hIcon == NULL)
		{
			hIcon = LoadIcon(NULL, IDI_APPLICATION);
		}
	}

	ImageList_AddIcon(hImgList, hIcon);
}

static BOOL CALLBACK refresh_enum_callback(HWND hWnd, LPARAM lParam)
{
	if (should_show_process(hWnd))
	{
		struct list_view_data* data = (struct list_view_data*)lParam;

		add_window_icon_to_list(hWnd, data->hImgList);

		LVITEM lvi;
		ZeroMemory(&lvi, sizeof(lvi));
		lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
		lvi.pszText = utility_get_window_title(hWnd);
		lvi.iItem = lv_item_index;
		lvi.iImage = lv_item_index++;
		lvi.lParam = (LPARAM)hWnd;
		ListView_InsertItem(data->hListView, &lvi);
	}

	return TRUE;
}

static void cleanup_list_data(struct root_window_data* data)
{
	if (data->list_data.hListView != NULL)
	{
		int item_count = ListView_GetItemCount(data->list_data.hListView);

		LVITEM lvi = { 0 };

		for (int i = 0; i < item_count; ++i)
		{
			lvi.iItem = i;
			ListView_GetItem(data->list_data.hListView, &lvi);
			free(lvi.pszText);
		}

		ListView_DeleteAllItems(data->list_data.hListView);
		
		if (data->list_data.hImgList != NULL)
		{
			ImageList_Destroy(data->list_data.hImgList);
		}
	}
}

static void refresh_process_list(HWND hWnd)
{
	struct root_window_data* data = retrieve_root_window_data(hWnd);

	if (data != NULL)
	{
		cleanup_list_data(data);

		int image_size = (int)(util_get_dpi_scale(hWnd) * 20);

		data->list_data.hImgList = ImageList_Create(image_size,
													image_size,
													ILC_COLOR32,
													count_valid_processes(), 0);

		lv_item_index = 0;

		EnumWindows(refresh_enum_callback, (LPARAM)&data->list_data);

		ListView_SetImageList(data->list_data.hListView,
			                  data->list_data.hImgList,
			                  LVSIL_SMALL);
	}
}

static HWND root_window_list_create(HWND hWnd)
{
	RECT rcClient;
	GetClientRect(hWnd, &rcClient);
	
	HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);

	HWND hList = CreateWindowEx(NULL,
								WC_LISTVIEW,
								L"",
								WS_CHILD | LVS_LIST | WS_VISIBLE | LVS_SINGLESEL,
								0, 0,
								rcClient.right,
								rcClient.bottom,
								hWnd, NULL,
								hInstance,
								NULL);

	if (hList == NULL)
	{
		MessageBox(NULL, L"Failed to initialize list view window", L"Error", MB_OK | MB_ICONERROR);

		PostQuitMessage(EXIT_FAILURE);
	}

	else
	{
		SetWindowTheme(hList, L"Explorer", NULL);
	}

	return hList;
}

static LRESULT root_window_create(HWND hWnd)
{
	struct root_window_data* data = (struct root_window_data*)malloc(sizeof(struct root_window_data));

	if (data != NULL)
	{
		ZeroMemory(data, sizeof(struct root_window_data));
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG)data);
		data->list_data.hListView = root_window_list_create(hWnd);
		refresh_process_list(hWnd);
	}
	
	else
	{
		MessageBox(NULL, L"Window data memory allocation failed", L"Error", MB_OK | MB_ICONERROR);
	}

	return 0;
}

static LRESULT root_window_size(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int window_width = LOWORD(lParam);
	int window_height = HIWORD(lParam);

	struct root_window_data* data = retrieve_root_window_data(hWnd);

	if (data != NULL)
	{
		SetWindowPos(data->list_data.hListView,
			         NULL, 0, 0,
			         window_width,
			         window_height,
			         SWP_NOZORDER);

		ListView_SetColumnWidth(data->list_data.hListView, 0, window_width);
	}

	return 0;
}

static void cleanup(HWND hWnd)
{
	struct root_window_data* data = retrieve_root_window_data(hWnd);
	
	cleanup_list_data(data);
	free(data);
}

static LRESULT root_window_command(HWND hWnd, WPARAM wParam)
{
	switch (LOWORD(wParam))
	{
	case IDC_REFRESH:
		refresh_process_list(hWnd);
		break;

	case IDC_ABOUT:
		ShellAbout(hWnd,
			L"Windip",
			L"Developed by Lampros Staikos",
			(HICON)GetClassLong(hWnd, GCL_HICON));
		break;
	}

	return 0;
}

static LRESULT root_window_notify(HWND hWnd, LPARAM lParam)
{
	LPNMHDR info = (LPNMHDR)lParam;
	
	if (info->code == NM_DBLCLK)
	{
		struct root_window_data* data = retrieve_root_window_data(hWnd);

		LPNMITEMACTIVATE activate = (LPNMITEMACTIVATE)lParam;

		LVITEM lvi = { 0 };
		lvi.iItem = activate->iItem;
		lvi.mask = LVIF_PARAM; /* lParam is null in release mode unless this is specified for some reason */
		ListView_GetItem(data->list_data.hListView, &lvi);

		HWND hDlg = CreateDialogParam(GetModuleHandle(NULL), 
									  MAKEINTRESOURCE(IDD_WINDOW_MANIP),
									  hWnd,
									  WindipDialogProc,
									  lvi.lParam);

		ShowWindow(hDlg, SW_SHOW);
	}

	return 0;
}

LRESULT CALLBACK root_window_procedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		return root_window_create(hWnd);

	case WM_SIZE:
		return root_window_size(hWnd, uMsg, wParam, lParam);

	case WM_COMMAND:
		return root_window_command(hWnd, wParam);

	case WM_NOTIFY:
		return root_window_notify(hWnd, lParam);

	case WM_CLOSE:
		cleanup(hWnd);
		PostQuitMessage(EXIT_SUCCESS);
		return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}