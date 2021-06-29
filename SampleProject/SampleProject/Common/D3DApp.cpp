#include "D3DApp.h"

#include<windowsx.h>

namespace
{
	/*
		This is just used to forward Windows messages from a global window
		procedure to our member function window procedure because we cannot
		assign a member function to WNDCLASS::lpfnWndProc.
	*/
	D3DApp* gD3DApp = nullptr;
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Forward hwnd on because we can get message (e.g., WM_CREATE)
	// before CreateWindow returns, and thus before mhMainWnd is valid.
	return gD3DApp->MsgProc(hWnd, msg, wParam, lParam);
}

D3DApp::D3DApp(HINSTANCE hInstance)
	: mhInstance(hInstance)
	, mhMainWindow(NULL)
	, mClientWidth(800)
	, mClientHeight(600)
	, mMainWindowCaption(L"D3D11 Application")
	, mbAppPaused(false)
	, mbMinimized(false)
	, mbMaximized(false)
	, mbResizing(false)
	, mbEnable4xMSAA(false)
{
	gD3DApp = this;
}

D3DApp::~D3DApp()
{
	gD3DApp = nullptr;
}

HINSTANCE D3DApp::GetAppInstance()
{
	return mhInstance;
}

HWND D3DApp::GetMainWindow()
{
	return mhMainWindow;
}

const float D3DApp::GetAspectRatio()
{
	return (float)mClientWidth / (float)mClientHeight;
}

int D3DApp::Run()
{
	MSG msg = {0};

	while (msg.message != WM_QUIT)
	{
		// If there are Window messages then process them.
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// Otherwise, do animation / game stuff.
		else
		{
			if (!mbAppPaused)
			{
				UdapteScene(0.f);
				DrawScene();
			}
			else
			{
				Sleep(100);
			}
		}
	}

	return (int)msg.wParam;
}

bool D3DApp::Init()
{
	if (!InitMainWindow())
	{
		return false;
	}

	if (!InitDirect3D())
	{
		return false;
	}

	return true;
}

void D3DApp::OnResize()
{
}

LRESULT D3DApp::MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	/*
		WM_ACTIVATE is sent when the window is activated or deactivated.
		We pause the game when the window is deactivated and unpause it
		when it becomes active.
	*/
	case WM_ACTIVATE:
	{
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			mbAppPaused = true;
			
		}
		else
		{
			mbAppPaused = false;
		}
	}
		return 0;

	// WM_DESTROY is sent when window is being destroyed.
	case WM_DESTROY:
	{
		PostQuitMessage(0);
	}
		return 0;

	/*
		The WM_MENUCHAR message is sent when a menu is active and the user presses
		a key that does not correspond to any mnemonic or accelerator key.
	*/
	case WM_MENUCHAR:
	// Don't beep when we alt-enter.
		return MAKELRESULT(0, MNC_CLOSE);

	// Catch this message so to prevent the window form becoming too small.
	case WM_GETMINMAXINFO:
	{
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
	}
		return 0;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	{
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
	}
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	{
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
	}
		return 0;
	case WM_MOUSEMOVE:
	{
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
	}
		return 0;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool D3DApp::InitMainWindow()
{
	WNDCLASS WindowClass;
	WindowClass.style = CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = MainWndProc;
	WindowClass.cbClsExtra = 0;
	WindowClass.cbWndExtra = 0;
	WindowClass.hInstance = mhInstance;
	WindowClass.hIcon = LoadIcon(0, IDI_APPLICATION);
	WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
	WindowClass.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	WindowClass.lpszMenuName = NULL;
	WindowClass.lpszClassName = L"D3DWindowClassName";

	if (!RegisterClass(&WindowClass))
	{
		MessageBox(0, L"RegisterClass Failed.", 0, 0);
		return false;
	}

	// Compute window rectangle dimensions based on requested client area dimensions.
	RECT rect = {0, 0, mClientWidth, mClientHeight};
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
	const int Width = rect.right - rect.left;
	const int Height = rect.bottom - rect.top;

	mhMainWindow = CreateWindow(
		L"D3DWindowClassName", mMainWindowCaption.c_str(),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, Width, Height, NULL, NULL, mhInstance, NULL);
	if (!mhMainWindow)
	{
		MessageBox(0, L"CreateWindow Falied.", 0, 0);
		return false;
	}

	ShowWindow(mhMainWindow, SW_SHOW);
	UpdateWindow(mhMainWindow);

	return true;
}

bool D3DApp::InitDirect3D()
{
	return true;
}
