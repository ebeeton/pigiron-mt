// Entry point for cv.
//
// Copyright Evan Beeton 2/1/2005

#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifdef _DEBUG
	#include <crtdbg.h> // Used for automatic heap leak detection.
#endif

#include "Game.h"
#include "Timer.h"
#include "PI_Utils.h"

// Globals
const char *szTitle			= "Evan Beeton's CV",
			*szWindowClass	= "PigIronWindow";
HINSTANCE hInst = 0;
HWND hWnd = 0;
CRITICAL_SECTION g_cs;

// Forward Declarations
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	srand(GetTickCount());	

	MSG msg;

#ifdef _DEBUG // Auto-detect memory leakage.
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	// Register the window class.
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX); 
	wcex.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInst = hInstance;
	wcex.hIcon			= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= 0;//(HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= 0;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= NULL;

	if (!RegisterClassEx(&wcex))
	{
		ERRORBOX("RegisterClassEx failed");
		return FALSE;
	}

#ifdef _DEBUG
	if (!(hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPED,
							  0, 0, 800, 600, NULL, NULL, GetModuleHandle(0), NULL)))
	{
		ERRORBOX("CreateWindow failed");
		return FALSE;
	}
#else
	WORD width = GetSystemMetrics(SM_CXSCREEN), height = GetSystemMetrics(SM_CYSCREEN);
	if (!(hWnd = CreateWindow(szWindowClass, szTitle, WS_POPUP,
							  0, 0, width, height, NULL, NULL, GetModuleHandle(0), NULL)))
	{
		ERRORBOX("CreateWindow failed");
		return FALSE;
	}
#endif

	InitializeCriticalSection(&g_cs);
	//InitializeCriticalSectionAndSpinCount(&g_cs, 0xFFFFFFFF);
	

	Game &cv = Game::GetInstance();
	if (!cv.Init())
	{
		ERRORBOX("The main game object failed to initialize!");
		PostQuitMessage(-1);
	}

	ShowWindow(hWnd, SW_SHOWNORMAL);
#ifndef _DEBUG
	// The WS_POPUP window style seems to not post a WM_SIZE when the window is shown.
	// Force one so that the renderer's viewport is properly sized.
	SendMessage(hWnd, WM_SIZE, SIZE_RESTORED, MAKELPARAM(width, height));
#endif
	UpdateWindow(hWnd);
	

	Timer timer;
	timer.Start();

	while (true)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) 
		{
			if (KEYDOWN(VK_ESCAPE))
				PostQuitMessage(0);

			if (msg.message == WM_QUIT)
				break;

			TranslateMessage(&msg); // Is this really necessary?
			DispatchMessage(&msg);
		}

		// Process the main game loop.
		cv.Update(timer.ElapsedSecondsReset());

		// TODO : Why does this change CPU usage and frame rate so drastically?
		//		  Processor usage drops from 50 to 0 and frame rate drops from 400-500 to 64 (800x600 debug)!
		//Sleep(1);
	}

	// Shut down the game.
	cv.Shutdown();

	DeleteCriticalSection(&g_cs);

	return (int) msg.wParam;
}

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	/*int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;*/


	switch (message) 
	{
	//case WM_COMMAND:
	//	wmId    = LOWORD(wParam); 
	//	wmEvent = HIWORD(wParam); 
	//	// Parse the menu selections:
	//	/*switch (wmId)
	//	{
	//	case IDM_EXIT:
	//		DestroyWindow(hWnd);
	//		break;
	//	default:
	//		return DefWindowProc(hWnd, message, wParam, lParam);
	//	}*/
	//	break;
	case WM_ERASEBKGND:
		break;
	case WM_PAINT:
		/*hdc = BeginPaint(hWnd, &ps);

		EndPaint(hWnd, &ps);*/
		break;

	/*case WM_SIZE:
			NOT THREADSAFE!!!
			PI_Render::GetInstance().UpdateViewportSize(LOWORD(lParam), HIWORD(lParam));
		break;*/

	case WM_CLOSE:
		{
			PostQuitMessage(0);
		}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
