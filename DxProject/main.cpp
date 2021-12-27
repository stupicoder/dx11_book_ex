
#include <Windows.h>

// Chapter
#include "Common/D3DApp.h"
// Ch04

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPreInstance, PWSTR pCmdLine, int nCmdShow)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	D3DApp* MainApp = nullptr;
	switch (0)
	{
	case 0:
		MainApp = new D3DApp(hInstance);
		break;
	default:
		break;
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
