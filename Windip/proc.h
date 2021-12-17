#pragma once

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

#ifdef NULL
	#undef NULL
	#define NULL 0
#endif

LRESULT CALLBACK root_window_procedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);