#pragma once

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <CommCtrl.h>

float util_get_dpi_scale(HWND hWnd);

HTREEITEM 
utility_add_item_to_tree(HWND hTreeView,
						 HTREEITEM hParent,
						 LPWSTR lpszItem,
						 LPARAM lParam);
HTREEITEM 
utility_set_tree_root(HWND hTreeView,
					  LPWSTR lpszItem,
					  LPARAM lParam);

wchar_t* utility_get_window_title(HWND hWnd);