
#include <Windows.h>

#include "Common/D3DApp.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	D3DApp MainApp(hInstance);
	if (!MainApp.Init())
	{
		return 0;
	}
	
	return MainApp.Run();
}
