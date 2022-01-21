#pragma once

#include "../../Common/D3DApp.h"
#include "../../Common/d3dx11effect.h"

class ShapesApp : public D3DApp
{
public:
	ShapesApp(HINSTANCE hInstance);
	virtual ~ShapesApp();

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
	ID3D11Buffer* mVB;
	ID3D11Buffer* mIB;

	ID3DX11Effect* mFX;
	ID3DX11EffectTechnique* mTech;
	ID3DX11EffectMatrixVariable* mfxWorldViewProj;

	ID3D11InputLayout* mInputLayout;

	ID3D11RasterizerState* mWireframeRS;

	// Define transformations form local spaces to world space.
	XMFLOAT4X4 mSphereWorld[10];
	XMFLOAT4X4 mCylWorld[10];
	XMFLOAT4X4 mBoxWorld;
	XMFLOAT4X4 mGridWorld;
	XMFLOAT4X4 mCenterSphere;

	XMFLOAT4X4 mView;
	XMFLOAT4X4 mProj;

	int mBoxVertexOffset;
	int mGridVertexOffset;
	int mSphereVertexOffset;
	int mCylinderVertexOffset;

	UINT mBoxIndexOffset;
	UINT mGridIndexOffset;
	UINT mSphereIndexOffset;
	UINT mCylinderIndexOffset;

	UINT mBoxIndexCount;
	UINT mGridIndexCount;
	UINT mSphereIndexCount;
	UINT mCylinderIndexCount;

	float mTheta;
	float mPhi;
	float mRadius;

	POINT mLastMousePos;
};
