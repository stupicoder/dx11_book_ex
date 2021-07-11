#pragma once

#include "../Common/D3DApp.h"

class InitDirect3DApp : public D3DApp
{
public:
	InitDirect3DApp(HINSTANCE hInstance);
	virtual ~InitDirect3DApp();

private:
	virtual void UdapteScene(const float InDeltaTime) override;
	virtual void DrawScene() override;
};

InitDirect3DApp::InitDirect3DApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

InitDirect3DApp::~InitDirect3DApp()
{

}

void InitDirect3DApp::UdapteScene(const float InDeltaTime) 
{

}

void InitDirect3DApp::DrawScene()
{
	assert(mD3DImmediateContext);
	assert(mSwapChain);

	mD3DImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::Blue));
	mD3DImmediateContext->ClearDepthStencilView(
		mDepthStencilView, 
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, // Clear 플래그
		1.0f, // Clear Depth 값 
		0 // Clear Stencil 값
	); 

	HR(mSwapChain->Present(
		0, 
		0
	));
}