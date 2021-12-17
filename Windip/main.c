#include <Windows.h>
#include <CommCtrl.h>

#include "resource.h"
#include "proc.h"

#pragma comment(lib, "Comctl32.lib")

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' " \
	"version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

static LPCWSTR g_WindipClass = L"WindipClass";

static BOOLEAN register_windip_class(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;
	ZeroMemory(&wcex, sizeof(wcex));
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.lpszClassName = g_WindipClass;
	wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = root_window_procedure;
	wcex.cbWndExtra = sizeof(void*);
	
	return RegisterClassEx(&wcex) != NULL;
}

INT APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	              _In_opt_ HINSTANCE hPrevInstance,
	                  _In_ LPWSTR    lpCmdLine,
	                  _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	if (register_windip_class(hInstance))
	{
		INITCOMMONCONTROLSEX iccex = { 0 };
		iccex.dwSize = sizeof(iccex);
		iccex.dwICC = ICC_LISTVIEW_CLASSES;
		InitCommonControlsEx(&iccex);

		HWND hWnd = CreateWindowEx(NULL,
					g_WindipClass,
					L"Windip",
					WS_OVERLAPPEDWINDOW,
					CW_USEDEFAULT,
					CW_USEDEFAULT,
					640,
					480,
					NULL, NULL,
					hInstance,
					NULL);

		SetMenu(hWnd, LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU1)));

		ShowWindow(hWnd, nCmdShow);
		UpdateWindow(hWnd);

		MSG msg;

		while (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	else 
	{
		MessageBox(NULL, L"Unable to register Windip's window class", L"Error", MB_OK | MB_ICONERROR);

		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}