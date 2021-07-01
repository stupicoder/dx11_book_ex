#include "D3DApp.h"

#include <WindowsX.h>

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
	// Get a pointer to the application object so we can forward 
	// Windows messages to the object's window procedure through
	// the global window procedure.
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

	mTimer.Reset();

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
			mTimer.Tick();

			if (!mbAppPaused)
			{
				CalculateFrameStats();
				UdapteScene(mTimer.GetDeltaTime());
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
			mTimer.Stop();
		}
		else
		{
			mbAppPaused = false;
			mTimer.Start();
		}
	}
		return 0;

	// WM_SIZE is sent when the user resizes the windpw.
	case WM_SIZE:
	{
		// Save the new client area dimensions.
		mClientWidth = LOWORD(lParam);
		mClientHeight = HIWORD(lParam);

	}
		return 0;

	// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
	{
		mbAppPaused = true;
		mbResizing = true;
		mTimer.Stop();
	}
		return 0;

	// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
	// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
	{
		mbAppPaused = false;
		mbResizing = false;
		mTimer.Start();
		OnResize();
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

void D3DApp::CalculateFrameStats()
{
	/*
		Code computes the average frames per second, 
		and also the average time it takes to render one frame. 
		These stats are appended to the window caption bar.

		이 메서드는 평균 FPS를 계산하며, 하나의 프레임을 렌더링하는데 걸리는 평균 시간도 계산한다.
		또한 이 통계치들을 창의 제목 줄에 추가한다.
	*/
	static int FrameCount = 0;
	static float TimeElapsed = 0.0f;

	++FrameCount;

	// Compute average over one second period.
	// 1초 동안의 평균 프레임 수를 계산
	if ((mTimer.GetTotalTime() - TimeElapsed) >= 1.0f)
	{
		const float FPS = (float)FrameCount; // FPS = FrameCount / 1
		const float MSPF = 1000.0f / FPS;

		std::wostringstream outs;
		outs.precision(6);
		outs << mMainWindowCaption << L" - FPS: " << FPS << L" - FrameTime: " << MSPF << L" (ms)";
		SetWindowText(mhMainWindow, outs.str().c_str());

		// Reset for next average.
		// 다음 평균을 위해 다시 초기화
		FrameCount = 0;
		TimeElapsed += 1.0f;
	}
}
