#pragma once

#include "../../Common/D3DApp.h"
#include "../../Common/d3dx11effect.h"

class BoxApp : public D3DApp
{
public:
	BoxApp(HINSTANCE hInstance);
	virtual ~BoxApp();

private:
	virtual bool Init() override;
	virtual void OnResize() override;
	virtual void UdapteScene(const float InDeltaTime) override;
	virtual void DrawScene() override;

	virtual void OnMouseDown(WPARAM InBtnState, const int X, const int Y) override;
	virtual void OnMouseUp(WPARAM InBtnState, const int X, const int Y) override;
	virtual void OnMouseMove(WPARAM InBtnState, const int X, const int Y) override;

private:
	void BuildGeometryBuffers();
	void BuildFX();
	void BuildVertexLayout();

private:
	ID3D11Buffer* mBoxVB;
	ID3D11Buffer* mBoxIB;

	ID3DX11Effect* mFX;
	ID3DX11EffectTechnique* mTech;
	ID3DX11EffectMatrixVariable* mfxWorldViewProj;

	ID3D11InputLayout* mInputLayout;

	XMFLOAT4X4 mWorld;
	XMFLOAT4X4 mView;
	XMFLOAT4X4 mProjection;

	float mTheta;
	float mPhi;
	float mRadius;

	POINT mLastMousePos;
};
