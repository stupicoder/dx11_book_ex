#include "Skull.h"

#include "../../Common/GeometryGenerator.h"

struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

SkullApp::SkullApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
	, mVB(NULL)
	, mIB(NULL)
	, mFX(NULL), mTech(NULL)
	, mfxWorldViewProj(NULL)
	, mInputLayout(NULL)
	, mWireframeRS(NULL)
	, mSkullIndexCount(0)
	, mTheta(1.5f * MathHelper::Pi)
	, mPhi(0.1f * MathHelper::Pi)
	, mRadius(20.f)
{
	mMainWindowCaption = L"Skull Demo";

	mLastMousePos.x = 0;
	mLastMousePos.y = 0;

	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&mView, I);
	XMStoreFloat4x4(&mProj, I);

	XMMATRIX T = XMMatrixTranslation(0.0f, -2.0f, 0.0f);
	XMStoreFloat4x4(&mSkullWorld, T);
}

SkullApp::~SkullApp()
{
	ReleaseCOM(mVB);
	ReleaseCOM(mIB);
	ReleaseCOM(mFX);
	ReleaseCOM(mInputLayout);
	ReleaseCOM(mWireframeRS);
}

bool SkullApp::Init()
{
	if (!D3DApp::Init())
	{
		return false;
	}

	BuildGeometryBuffers();
	BuildFX();
	BuildVertexLayout();

	D3D11_RASTERIZER_DESC WireframeDesc;
	ZeroMemory(&WireframeDesc, sizeof(D3D11_RASTERIZER_DESC));
	WireframeDesc.FillMode = D3D11_FILL_WIREFRAME;
	WireframeDesc.CullMode = D3D11_CULL_BACK;
	WireframeDesc.FrontCounterClockwise = false;
	WireframeDesc.DepthClipEnable = true;

	HR(mD3DDevice->CreateRasterizerState(&WireframeDesc, &mWireframeRS));

	return true;
}

void SkullApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, GetAspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void SkullApp::UdapteScene(const float InDeltaTime)
{
	// Convert Spherical to Cartesian coordinates.
	float x = mRadius * sinf(mPhi) * cosf(mTheta);
	float z = mRadius * sinf(mPhi) * sinf(mTheta);
	float y = mRadius * cosf(mPhi);

	// Build the view matrix.
	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX V = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, V);
}

void SkullApp::DrawScene()
{
	mD3DImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::LightSteelBlue));
	mD3DImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH || D3D11_CLEAR_STENCIL, 1.f, 0);

	mD3DImmediateContext->IASetInputLayout(mInputLayout);
	mD3DImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	mD3DImmediateContext->RSSetState(mWireframeRS);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	mD3DImmediateContext->IASetVertexBuffers(0, 1, &mVB, &stride, &offset);
	mD3DImmediateContext->IASetIndexBuffer(mIB, DXGI_FORMAT_R32_UINT, 0);

	// Set constants

	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX world = XMLoadFloat4x4(&mSkullWorld);
	XMMATRIX worldViewProj = world * view * proj;

	mfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));

	D3DX11_TECHNIQUE_DESC TechDesc;
	mTech->GetDesc(&TechDesc);
	for (UINT p = 0; p < TechDesc.Passes; ++p)
	{
		mTech->GetPassByIndex(p)->Apply(0, mD3DImmediateContext);
		mD3DImmediateContext->DrawIndexed(mSkullIndexCount, 0, 0);
	}

	HR(mSwapChain->Present(0, 0));
}

void SkullApp::OnMouseDown(WPARAM InBtnState, const int X, const int Y)
{
	mLastMousePos.x = X;
	mLastMousePos.y = Y;

	SetCapture(mhMainWindow);
}

void SkullApp::OnMouseUp(WPARAM InBtnState, const int X, const int Y)
{
	ReleaseCapture();
}

void SkullApp::OnMouseMove(WPARAM InBtnState, const int X, const int Y)
{
	if ((InBtnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f * static_cast<float>(X - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(Y - mLastMousePos.y));

		// Update angles based on input to orbit camera around box.
		mTheta += dx;
		mPhi += dy;

		// Restrict the angle mPhi.
		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((InBtnState & MK_RBUTTON) != 0)
	{
		// Make each pixel correspond to 0.2 unit in the scene.
		float dx = 0.05f * static_cast<float>(X - mLastMousePos.x);
		float dy = 0.05f * static_cast<float>(Y - mLastMousePos.y);

		// Update the camera radius based on input.
		mRadius += dx - dy;

		// Restrict the radius.
		mRadius = MathHelper::Clamp(mRadius, 5.0f, 50.0f);
	}

	mLastMousePos.x = X;
	mLastMousePos.y = Y;
}

void SkullApp::BuildGeometryBuffers()
{
	std::ifstream fin("Models/skull.txt");

	if (!fin)
	{
		MessageBox(0, L"Models/skull.txt not found.", 0, 0);
		return;
	}

	UINT VCount = 0;
	UINT TCount = 0;
	std::string ignore;

	fin >> ignore >> VCount;
	fin >> ignore >> TCount;
	fin >> ignore >> ignore >> ignore >> ignore;

	float nx, ny, nz;
	XMFLOAT4 black(0.f, 0.f, 0.f, 1.f);

	std::vector<Vertex> vertices(VCount);
	for (UINT i = 0; i < VCount; ++i)
	{
		fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;

		vertices[i].Color = black;

		// Normal not used in this demo.
		fin >> nx >> ny >> nz;
	}

	fin >> ignore;
	fin >> ignore;
	fin >> ignore;

	mSkullIndexCount = 3 * TCount;
	std::vector<UINT> Indices(mSkullIndexCount);
	for (UINT i = 0; i < TCount; ++i)
	{
		fin >> Indices[i * 3 + 0] >> Indices[i * 3 + 1] >> Indices[i * 3 + 2];
	}

	fin.close();

	D3D11_BUFFER_DESC VBDesc;
	VBDesc.Usage = D3D11_USAGE_IMMUTABLE;
	VBDesc.ByteWidth = sizeof(Vertex) * VCount;
	VBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	VBDesc.CPUAccessFlags = 0;
	VBDesc.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA VInitData;
	VInitData.pSysMem = &vertices[0];
	HR(mD3DDevice->CreateBuffer(&VBDesc, &VInitData, &mVB));

	//
	// Pack the indices of all the meshes into one index buffer.
	//

	D3D11_BUFFER_DESC IBDesc;
	IBDesc.Usage = D3D11_USAGE_IMMUTABLE;
	IBDesc.ByteWidth = sizeof(UINT) * mSkullIndexCount;
	IBDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	IBDesc.CPUAccessFlags = 0;
	IBDesc.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA IInitData;
	IInitData.pSysMem = &Indices[0];
	HR(mD3DDevice->CreateBuffer(&IBDesc, &IInitData, &mIB));
}

void SkullApp::BuildFX()
{
	std::ifstream fin("fx/color.fxo", std::ios::binary);

	fin.seekg(0, std::ios_base::end);
	int size = (int)fin.tellg();
	fin.seekg(0, std::ios_base::beg);
	std::vector<char> compiledShader(size);

	fin.read(&compiledShader[0], size);
	fin.close();

	HR(D3DX11CreateEffectFromMemory(&compiledShader[0], size, 0, mD3DDevice, &mFX));

	mTech = mFX->GetTechniqueByName("ColorTech");
	mfxWorldViewProj = mFX->GetVariableByName("gWorldViewProjection")->AsMatrix();
}

void SkullApp::BuildVertexLayout()
{
	// Create the vertex input layout.
	D3D11_INPUT_ELEMENT_DESC VertexDesc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	// Create the input layout
	D3DX11_PASS_DESC PassDesc;
	mTech->GetPassByIndex(0)->GetDesc(&PassDesc);
	HR(mD3DDevice->CreateInputLayout(VertexDesc, 2, PassDesc.pIAInputSignature, 
		PassDesc.IAInputSignatureSize, &mInputLayout));
}
