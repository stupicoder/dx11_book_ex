#pragma once

#include <Windows.h>
#include "D3DUtil.h"
#include "GameTimer.h"

class D3DApp
{
public:
	D3DApp(HINSTANCE hInstance);
	virtual ~D3DApp();

	HINSTANCE GetAppInstance();
	HWND GetMainWindow();
	const float GetAspectRatio();

	int Run();

	// 프레임 워크 매서드들. 
	// 파생 클라이언트 클래스는 이 메서드들을 자신의 요구에 맞게 재정의

	virtual bool Init();
	virtual void OnResize();
	virtual void UdapteScene(const float InDeltaTime) {}// = 0;
	virtual void DrawScene() {}// = 0;
	virtual LRESULT MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	// 마우스 입력의 처리를 위한 편의용 가상 메서드들.
	virtual void OnMouseDown(WPARAM InBtnState, const int X, const int Y) {}
	virtual void OnMouseUp(WPARAM InBtnState, const int X, const int Y) {}
	virtual void OnMouseMove(WPARAM InBtnState, const int X, const int Y) {}

protected:
	bool InitMainWindow();
	bool InitDirect3D();

	void CalculateFrameStats();

protected:
	HINSTANCE mhInstance; // 응용프로그램 인스턴스 핸들
	HWND mhMainWindow; // 주 창 핸들
	
	// 클라이언트 창 초기 크기 (800x600), 이후 창 크기를 조절하면 동적으로 변함 
	int mClientWidth;
	int mClientHeight;

	// 창의 제목
	std::wstring mMainWindowCaption;
	
	// 경과 시간과 게임 전체 시간을 측정하는데 쓰인다 (4.3)
	GameTimer mTimer;

	BYTE mbAppPaused:1; // 정지 상태 체크
	BYTE mbMinimized:1; // 최소화 상태 체크
	BYTE mbMaximized:1; // 최대화 상태 체크
	BYTE mbResizing:1; // 창 사이즈 조절 상태 체크

	BYTE mbEnable4xMSAA:1; // 4xMSAA 사용여부, 기본 값은 false (4.1.8)

};