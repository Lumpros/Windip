#include "utility.h"

#include <CommCtrl.h>

float util_get_dpi_scale(HWND hWnd)
{
	return GetDpiForWindow(hWnd) / 96.0f;
}

HTREEITEM utility_add_item_to_tree(HWND hTreeView, HTREEITEM hParent, LPWSTR lpszItem, LPARAM lParam)
{
	TVITEM tvItem = {0};
	tvItem.mask = TVIF_TEXT | TVIF_PARAM;
	tvItem.pszText = lpszItem;
	tvItem.cchTextMax = lstrlen(lpszItem);
	tvItem.lParam = lParam;

	TVINSERTSTRUCT tvInsert = {0};
	tvInsert.item = tvItem;
	tvInsert.hInsertAfter = TreeView_GetPrevSibling(hTreeView, TreeView_GetChild(hTreeView, hParent));
	tvInsert.hParent = hParent;

	return TreeView_InsertItem(hTreeView, &tvInsert);
}

HTREEITEM utility_set_tree_root(HWND hTreeView, LPWSTR lpszItem, LPARAM lParam)
{
	TVITEM tvItem = {0};
	tvItem.mask = TVIF_TEXT | TVIF_PARAM;
	tvItem.cchTextMax = lstrlen(lpszItem);
	tvItem.pszText = lpszItem;
	tvItem.lParam = lParam;

	TVINSERTSTRUCT tvInsert = {0};
	tvInsert.hParent = TVI_ROOT;
	tvInsert.hInsertAfter = TVI_FIRST;
	tvInsert.item = tvItem;

	return TreeView_InsertItem(hTreeView, &tvInsert);
}

/* Must be freed by the caller */
wchar_t* utility_get_window_title(HWND hWnd)
{
	int length = GetWindowTextLength(hWnd) + 1;

	if (length > 1)
	{
		wchar_t* title = (wchar_t*)malloc(length * sizeof(wchar_t));

		if (title != NULL)
		{
			GetWindowText(hWnd, title, length);

			return title;
		}
	}

	return NULL;
}