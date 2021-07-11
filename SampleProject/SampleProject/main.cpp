
#include <Windows.h>

#include "Common/D3DApp.h"
#include "Chapter/04_InitDirect3D.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	D3DApp* MainApp = nullptr;

	switch (1)
	{
	case 0:
	{
		MainApp = new D3DApp(hInstance);
		break;
	}
	case 1:
	{
		MainApp = new InitDirect3DApp(hInstance);
		break;
	}
	}

	if (!MainApp->Init())
	{
		delete MainApp;
		return 0;
	}
	
	int Result = MainApp->Run();

	delete MainApp;
	return Result;
}
