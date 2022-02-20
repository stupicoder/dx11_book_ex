
#include <Windows.h>

// Chapter
#include "Chapter/Ch04/InitDirect3D.h"
#include "Chapter/Ch06/Box.h"
#include "Chapter/Ch06/Hills.h"
#include "Chapter/Ch06/Shapes.h"
#include "Chapter/Ch06/Skull.h"
#include "Chapter/Ch06/WavesApp.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPreInstance, PWSTR pCmdLine, int nCmdShow)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	D3DApp* MainApp = nullptr;
	switch (0)
	{
	case 0:
		MainApp = new WavesApp(hInstance);
		break;
	case 1:
		MainApp = new SkullApp(hInstance);
		break;
	case 2:
		MainApp = new ShapesApp(hInstance);
		break;
	case 3:
		MainApp = new HillsApp(hInstance);
		break;
	case 4:
		MainApp = new BoxApp(hInstance);
		break;
	case 5:
		MainApp = new InitDirect3DApp(hInstance);
		break;
	default:
		MainApp = new D3DApp(hInstance);
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
