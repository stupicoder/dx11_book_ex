#pragma once

#include "../../Common/D3DApp.h"
#include "../../Common/d3dx11effect.h"

#include "Waves.h"

class WavesApp : public D3DApp
{
public:
	WavesApp(HINSTANCE hInstance);
	virtual ~WavesApp();

private:
	virtual bool Init() override;
	virtual void OnResize() override;
	virtual void UdapteScene(const float InDeltaTime) override;
	virtual void DrawScene() override;

	virtual void OnMouseDown(WPARAM InBtnState, const int X, const int Y) override;
	virtual void OnMouseUp(WPARAM InBtnState, const int X, const int Y) override;
	virtual void OnMouseMove(WPARAM InBtnState, const int X, const int Y) override;

private:
	float GetHeight(const float X, const float Z) const;
	void BuildLandGeometryBuffers();
	void BuildWavesGeometryBuffers();
	void BuildFX();
	void BuildVertexLayout();

private:
	ID3D11Buffer* mLandVB;
	ID3D11Buffer* mLandIB;
	ID3D11Buffer* mWavesVB;
	ID3D11Buffer* mWavesIB;

	ID3DX11Effect* mFX;
	ID3DX11EffectTechnique* mTech;
	ID3DX11EffectMatrixVariable* mfxWorldViewProj;

	ID3D11InputLayout* mInputLayout;

	ID3D11RasterizerState* mWireframeRS;

	// Define transformations from local spaces to world space.
	XMFLOAT4X4 mGridWorld;
	XMFLOAT4X4 mWavesWorld;

	UINT mGridIndexCount;

	Waves mWaves;

	XMFLOAT4X4 mView;
	XMFLOAT4X4 mProj;

	float mTheta;
	float mPhi;
	float mRadius;

	POINT mLastMousePos;
};
