#include "D3DApp.h"

#include <windowsx.h>

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

	, mD3DDevice(NULL)
	, mD3DImmediateContext(NULL)
	, mSwapChain(NULL)
	, mDepthStencilBuffer(NULL)
	, mRenderTargetView(NULL)
	, mDepthStencilView(NULL)
	, md3dDriverType(D3D_DRIVER_TYPE_HARDWARE)
{
	ZeroMemory(&mScreenViewport, sizeof(D3D11_VIEWPORT));

	// Get a pointer to the application object so we can forward 
	// Windows messages to the object's window procedure through
	// the global window procedure.
	gD3DApp = this;
}

D3DApp::~D3DApp()
{
	ReleaseCOM(mRenderTargetView);
	ReleaseCOM(mDepthStencilView);
	ReleaseCOM(mSwapChain);
	ReleaseCOM(mDepthStencilBuffer);

	// Restore all default settings.
	if (mD3DImmediateContext)
	{
		mD3DImmediateContext->ClearState();
	}

	ReleaseCOM(mD3DImmediateContext);
	ReleaseCOM(mD3DDevice);

	gD3DApp = nullptr;
}

HINSTANCE D3DApp::GetAppInstance() const
{
	return mhInstance;
}

HWND D3DApp::GetMainWindow() const
{
	return mhMainWindow;
}

float D3DApp::GetAspectRatio() const
{
	return (float)mClientWidth / (float)mClientHeight;
}

int D3DApp::Run()
{
	MSG msg = { 0 };

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
	assert(mD3DDevice);
	assert(mD3DImmediateContext);
	assert(mSwapChain);

	// Release the old views, as they hold references to the buffers we
	// will be destroying.  Also release the old depth/stencil buffer.

	ReleaseCOM(mRenderTargetView);
	ReleaseCOM(mDepthStencilView);
	ReleaseCOM(mDepthStencilBuffer);

	// Resize the swap chain and recreate the render target view.

	HR(mSwapChain->ResizeBuffers(
		1, // Resize 할 버퍼 수
		mClientWidth, mClientHeight, // 사이즈
		DXGI_FORMAT_R8G8B8A8_UNORM, // 포맷
		0
	));

	// 4.2.5 렌더 대상 뷰의 생성
	// 백버퍼 자원은 SwapChain을 통해 이미 할당 되어 있는듯
	ID3D11Texture2D* BackBuffer;
	HR(mSwapChain->GetBuffer(
		0, // 얻고자 하는 후면 버퍼의 인덱스
		__uuidof(ID3D11Texture2D), // 버퍼 인터페이스 형식
		reinterpret_cast<void**>(&BackBuffer) // 후면버퍼의 포인터
	));
	// 렌더대상 뷰 생성
	HR(mD3DDevice->CreateRenderTargetView(
		BackBuffer, // 대상 RenderResource
		NULL, // D3D11_RENDER_TARGET_VIEW_DESC 구조체 포인터, 무형식이 아니라면 NULL 해줘도 무방
		&mRenderTargetView // 렌더 타겟 뷰, 첫번째 밉맵 수준의 뷰 (후에 8장에서 자세히 서술)
	));
	ReleaseCOM(BackBuffer);

	// 4.2.6 깊이, 스텐실 버퍼와 뷰의 생성
	// Create the depth/stencil buffer and view.

	D3D11_TEXTURE2D_DESC DepthStencilDesc;
	DepthStencilDesc.Width = mClientWidth;
	DepthStencilDesc.Height = mClientHeight;
	DepthStencilDesc.MipLevels = 1; // 깊이 스텐실 버퍼는 밉이 1
	DepthStencilDesc.ArraySize = 1; // 텍스쳐 배열의 텍스쳐 개수, CubeMap 텍스쳐 같은 자원을 위한 것인 듯
	DepthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// Use 4X MSAA? --must match swap chain MSAA values.
	if (mbEnable4xMSAA)
	{
		// 이전 설정 값과 동일해야함 (SwapChain 생성시 설정 값)
		DepthStencilDesc.SampleDesc.Count = 4;
		DepthStencilDesc.SampleDesc.Quality = m4xMSAAQuality - 1;
	}
	else
	{
		DepthStencilDesc.SampleDesc.Count = 1;
		DepthStencilDesc.SampleDesc.Quality = 0;
	}

	DepthStencilDesc.Usage = D3D11_USAGE_DEFAULT; // GPU 가 자원을 읽고 씀, CPU 는 X
	DepthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL; // 자원을 깊이 스텐실 단계에 묶는다. 그외 RENDERTARGET, SHADERRESOURCE 등이 있음
	DepthStencilDesc.CPUAccessFlags = 0; // CPU 가 자원을 접근하는 방식
	DepthStencilDesc.MiscFlags = 0; // 기타 플래그

	HR(mD3DDevice->CreateTexture2D(
		&DepthStencilDesc, // Desc
		NULL, // 텍스쳐를 채울 초기 값
		&mDepthStencilBuffer
	));
	HR(mD3DDevice->CreateDepthStencilView(
		mDepthStencilBuffer,
		NULL, // Buffer 생성시 무형식이 아니라면 NULL 가능,  D3D11_DEPTH_STENCIL_VIEW_DESC
		&mDepthStencilView
	));

	// 4.2.7 뷰들을 출력 병합기 단계에 묶기

	// Bind the render target view and depth/stencil view to the pipeline.

	// Output Merge Stage
	mD3DImmediateContext->OMSetRenderTargets(
		1, // 묶을 렌더타겟 뷰 수, 여러 뷰들을 사용하는 것은 제 3장에 안내.
		&mRenderTargetView, // 묶을 렌더타겟 뷰들의 포인터
		mDepthStencilView // 깊이 스텐실은 하나만 묶을 수 있다.
	);

	/*
		장면을 그려넣을 후면 버퍼 일부의 직사각형을 뷰포트라 함
		후면 버퍼 좌표를 기준으로 좌상단이 0,0.
	*/

	// Set the viewport transform.

	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width = static_cast<float>(mClientWidth);
	mScreenViewport.Height = static_cast<float>(mClientHeight);
	mScreenViewport.MinDepth = 0.f;
	mScreenViewport.MaxDepth = 1.f;

	// 4.7 연습문제 6
	/*mScreenViewport.TopLeftX = 100.f;
	mScreenViewport.TopLeftY = 100.f;
	mScreenViewport.Width = 500.f;
	mScreenViewport.Height = 400.f;
	mScreenViewport.MinDepth = 0.f;
	mScreenViewport.MaxDepth = 1.f;*/

	// Rasterize Stage
	mD3DImmediateContext->RSSetViewports(1, &mScreenViewport);
	// Viewport 를 여러개 설정하면 플레이어 별로 다른 뷰를 보여줄 수 있음.

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
		if (mD3DDevice)
		{
			if (wParam == SIZE_MINIMIZED)
			{
				mbAppPaused = true;
				mbMinimized = true;
				mbMaximized = false;
			}
			else if (wParam == SIZE_MAXIMIZED)
			{
				mbAppPaused = false;
				mbMinimized = false;
				mbMaximized = true;
				OnResize();
			}
			else if (wParam == SIZE_RESTORED)
			{
				// Restoring from minimized state?
				if (mbMinimized)
				{
					mbAppPaused = false;
					mbMinimized = false;
					OnResize();
				}
				// Restoring from maximized state?
				else if (mbMaximized)
				{
					mbAppPaused = false;
					mbMaximized = false;
					OnResize();
				}
				else if (mbResizing)
				{
					// If user is dragging the resize bars, we do not resize 
					// the buffers here because as the user continuously 
					// drags the resize bars, a stream of WM_SIZE messages are
					// sent to the window, and it would be pointless (and slow)
					// to resize for each WM_SIZE message received from dragging
					// the resize bars.  So instead, we reset after the user is 
					// done resizing the window and releases the resize bars, which 
					// sends a WM_EXITSIZEMOVE message.
				}
				else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
				{
					OnResize();
				}
			}
		}
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
	RECT rect = { 0, 0, mClientWidth, mClientHeight };
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
	// Create the device and device context.

	UINT CreateDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
	CreateDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL FeatureLevel;
	HRESULT hr = D3D11CreateDevice(
		0, // default adapter
		md3dDriverType,
		0, // no software device
		CreateDeviceFlags,
		0, 0, // default feature level array
		D3D11_SDK_VERSION,
		&mD3DDevice,
		&FeatureLevel,
		&mD3DImmediateContext);

	if (FAILED(hr))
	{
		MessageBox(0, L"D3D11CreateDevice Failed.", 0, 0);
		return false;
	}

	if (FeatureLevel != D3D_FEATURE_LEVEL_11_0)
	{
		MessageBox(0, L"Direct3D Feature Level 11 unsupported.", 0, 0);
		return false;
	}

	// Check 4X MSAA quality support for our back buffer format.
	// All Direct3D 11 capable devices support 4X MSAA for all render 
	// target formats, so we only need to check quality support.
	HR(mD3DDevice->CheckMultisampleQualityLevels(
		DXGI_FORMAT_R8G8B8A8_UNORM, // 텍스쳐 형식
		4,							// 픽셀당 표본 개수
		&m4xMSAAQuality));			// 텍스쳐 형식과 표본 개수의 조합에 대한 품질 수준의 수
	assert(m4xMSAAQuality > 0);
	// 지원하지 않으면 0 을 반환, [0 ~ NumQualityLevels)
	// 최대 표본 개수 : D3D11_MAX_MULTISAMPLE_COUNT ( 32 )

	// 4.2.3 교환 사슬의 설정
	// Fill out a DXGI_SWAP_CHAIN_DESC to describe our swap chain.
	DXGI_SWAP_CHAIN_DESC SwapChainDesc;
	SwapChainDesc.BufferDesc.Width = mClientWidth;
	SwapChainDesc.BufferDesc.Height = mClientHeight;
	// RefreshRate : 디스플레이 모드 갱신률
	SwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	SwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	SwapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	SwapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Use 4x MSAA?
	if (mbEnable4xMSAA)
	{
		SwapChainDesc.SampleDesc.Count = 4;
		SwapChainDesc.SampleDesc.Quality = m4xMSAAQuality - 1;
	}
	// No MSAA
	else
	{
		SwapChainDesc.SampleDesc.Count = 1;
		SwapChainDesc.SampleDesc.Quality = 0;
	}

	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 버퍼의 용도
	SwapChainDesc.BufferCount = 1; // 후면 버퍼의 개수, 1 => 이중버퍼링, 2 => 삼중 버퍼링
	SwapChainDesc.OutputWindow = mhMainWindow;
	SwapChainDesc.Windowed = true;
	SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; // 교환 효과 : 디스플레이 드라이버에서 자동으로 선택
	SwapChainDesc.Flags = 0;
	/*
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH : 전체화면으로 전활 될 떄, 후면 버퍼 설정에 가장 적합한 디스플레이 모드가 설정
		0 은 아무런 효과 없이 전환

		실행 시점에서 다중표본화 설정을 변경하고 싶으면 교환 사슬을 삭제하고 다시 생성하는 것이 한가지 방법
	*/

	// 4.2.4 교환 사슬의 생성
	// To correctly create the swap chain, we must use the IDXGIFactory that was
	// used to create the device.  If we tried to use a different IDXGIFactory instance
	// (by calling CreateDXGIFactory), we get an error: "IDXGIFactory::CreateSwapChain: 
	// This function is being called with a device from a different IDXGIFactory."

	/*
		DXGI ( DirectX Graphics Infrastructure ) : Direct3D 와는 개별적은 API
		교환사슬 설정, 그래픽 하드웨어 나열, 창 모드와 전체화면 모드 전환
		Direct3D 와 분리한 이유는 Direct2D 등 에서도 해당 기능이 사용되기 때문에
	*/
	IDXGIDevice* DXGIDevice = NULL;
	HR(mD3DDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&DXGIDevice));

	IDXGIAdapter* DXGIAdapter = NULL;
	HR(DXGIDevice->GetParent(__uuidof(IDXGIAdapter*), (void**)&DXGIAdapter));

	IDXGIFactory* DXGIFactory = NULL;
	HR(DXGIAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&DXGIFactory));

	// 교환 사슬을 생성
	HR(DXGIFactory->CreateSwapChain(mD3DDevice, &SwapChainDesc, &mSwapChain));


	// 4.7 연습문제 1
	/*UINT MWAFlags = DXGI_MWA_NO_WINDOW_CHANGES;
	DXGIFactory->MakeWindowAssociation(mhMainWindow, MWAFlags);*/

	// 4.7 연습문제 2
	/*UINT AdapterNum = 0;
	IDXGIAdapter* CurrentAdapter = NULL;
	while (!FAILED(DXGIFactory->EnumAdapters(AdapterNum, &CurrentAdapter)))
	{
		if (CurrentAdapter == NULL)
		{
			break;
		}
		++AdapterNum;
	}
	{
		std::wstring Msg = L"AdapterNum : " + std::to_wstring(AdapterNum);
		MessageBox(0, Msg.c_str(), 0, 0);
	}*/

	// 4.7 연습문제 3
	/*UINT AdapterNum = 0;
	IDXGIAdapter* CurrentAdapter = NULL;
	std::wstring Msg;
	while (!FAILED(DXGIFactory->EnumAdapters(AdapterNum, &CurrentAdapter)))
	{
		if (CurrentAdapter == NULL)
		{
			break;
		}
		++AdapterNum;

		LARGE_INTEGER UMDVersion;
		HRESULT Result = CurrentAdapter->CheckInterfaceSupport(__uuidof(ID3D11Device), &UMDVersion);
		Msg += std::to_wstring(AdapterNum - 1) + L"_Adpater: "
			+ (FAILED(hr) ? L" DX11 Failed" : L"DX11 Support") + L"\n";
	}
	{
		Msg += L"AdapterNum : " + std::to_wstring(AdapterNum);
		MessageBox(0, Msg.c_str(), 0, 0);
	}*/

	// 4.7 연습문제 4
	/*UINT OutputNum = 0;
	IDXGIAdapter* CurrentAdapter = NULL;
	std::wstring Msg;
	if (!FAILED(DXGIFactory->EnumAdapters(0, &CurrentAdapter)))
	{
		IDXGIOutput* CurrentOutput = NULL;
		while (!FAILED(CurrentAdapter->EnumOutputs(OutputNum, &CurrentOutput)))
		{
			if (CurrentOutput == NULL)
			{
				break;
			}

			++OutputNum;

			ReleaseCOM(CurrentOutput);
		}

		ReleaseCOM(CurrentAdapter);
	}
	{
		Msg += L"OutputNum : " + std::to_wstring(OutputNum);
		MessageBox(0, Msg.c_str(), 0, 0);
	}*/

	// 4.7 연습문제 5
	/*IDXGIAdapter* CurrentAdapter = NULL;
	std::wstring Msg;
	if (!FAILED(DXGIFactory->EnumAdapters(0, &CurrentAdapter)))
	{
		UINT OutputNum = 0;
		IDXGIOutput* CurrentOutput = NULL;
		while (!FAILED(CurrentAdapter->EnumOutputs(OutputNum, &CurrentOutput)))
		{
			UINT ModeNum = 0;
			HRESULT hr = CurrentOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, 0, &ModeNum, NULL);
			if (FAILED(hr))
			{
				break;
			}

			DXGI_MODE_DESC* ModeList = new DXGI_MODE_DESC[ModeNum];
			hr = CurrentOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, 0, &ModeNum, ModeList);
			for (int i = 0; i < ModeNum; ++i)
			{
				Msg += std::to_wstring(OutputNum) + L" Output, " + std::to_wstring(i) + L" Mode, ";
				Msg += L"Width : " + std::to_wstring(ModeList[i].Width);
				Msg += L", Height : " + std::to_wstring(ModeList[i].Height);
				Msg += L", Refresh : " + std::to_wstring(ModeList[i].RefreshRate.Numerator);
				Msg += L"/ " + std::to_wstring(ModeList[i].RefreshRate.Denominator) + L"\n";
			}

			++OutputNum;

			delete[] ModeList;
			ReleaseCOM(CurrentOutput);
		}

		ReleaseCOM(CurrentAdapter);
	}
	{
		MessageBox(0, Msg.c_str(), 0, 0);
	}*/

	// 획득했던 COM 인터페이스를 해제 (다 사용했으므로)
	ReleaseCOM(DXGIDevice);
	ReleaseCOM(DXGIAdapter);
	ReleaseCOM(DXGIFactory);

	// The remaining steps that need to be carried out for d3d creation
	// also need to be executed every time the window is resized.  So
	// just call the OnResize method here to avoid code duplication.

	OnResize();

	return true;
}

void D3DApp::CalculateFrameStats()
{
	// 4.4.4 프레임 통계치

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
