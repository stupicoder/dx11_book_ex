#include "Hills.h"

#include "../../Common/GeometryGenerator.h"

struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

HillsApp::HillsApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
	, mGridVB(NULL)
	, mGridIB(NULL)
	, mFX(NULL), mTech(NULL)
	, mfxWorldViewProj(NULL)
	, mInputLayout(NULL)
	, mGridIndexCount(0)
	, mTheta(1.5f * MathHelper::Pi)
	, mPhi(0.1f * MathHelper::Pi)
	, mRadius(200.f)
{
	mMainWindowCaption = L"Hills Demo";

	mLastMousePos.x = 0;
	mLastMousePos.y = 0;

	XMMATRIX IdentityMat = XMMatrixIdentity();
	XMStoreFloat4x4(&mGridWorld, IdentityMat);
	XMStoreFloat4x4(&mView, IdentityMat);
	XMStoreFloat4x4(&mProjection, IdentityMat);
}

HillsApp::~HillsApp()
{
	ReleaseCOM(mGridVB);
	ReleaseCOM(mGridIB);
	ReleaseCOM(mFX);
	ReleaseCOM(mInputLayout);
}

bool HillsApp::Init()
{
	if (!D3DApp::Init())
	{
		return false;
	}

	BuildGeometryBuffers();
	BuildFX();
	BuildVertexLayout();

	return true;
}

void HillsApp::OnResize()
{
	D3DApp::OnResize();

	// The window resized. so update the aspect ratio and recompute projection matrix.
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, GetAspectRatio(), 1.f, 1000.f);
	XMStoreFloat4x4(&mProjection, P);
}

void HillsApp::UdapteScene(const float InDeltaTime)
{
	// Convert spherical to cartesian coordinates.
	const float x = mRadius * sinf(mPhi) * cosf(mTheta);
	const float z = mRadius * sinf(mPhi) * sinf(mTheta);
	const float y = mRadius * cosf(mPhi);

	// Build teh view matrix.
	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX V = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, V);
}

void HillsApp::DrawScene()
{
	mD3DImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::LightSteelBlue));
	mD3DImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0.f);

	mD3DImmediateContext->IASetInputLayout(mInputLayout);
	mD3DImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	mD3DImmediateContext->IASetVertexBuffers(0, 1, &mGridVB, &stride, &offset);
	mD3DImmediateContext->IASetIndexBuffer(mGridIB, DXGI_FORMAT_R32_UINT, 0);

	// Set constants

	XMMATRIX View = XMLoadFloat4x4(&mView);
	XMMATRIX Projection = XMLoadFloat4x4(&mProjection);
	XMMATRIX World = XMLoadFloat4x4(&mGridWorld);
	XMMATRIX WorldViewProjection = World * View * Projection;

	D3DX11_TECHNIQUE_DESC TechDesc;
	mTech->GetDesc(&TechDesc);
	for (UINT pass = 0; pass < TechDesc.Passes; ++pass)
	{
		// Draw the grid.
		mfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&WorldViewProjection));
		mTech->GetPassByIndex(pass)->Apply(0, mD3DImmediateContext);
		mD3DImmediateContext->DrawIndexed(mGridIndexCount, 0, 0);
	}

	HR(mSwapChain->Present(0, 0));
}

void HillsApp::OnMouseDown(WPARAM InBtnState, const int X, const int Y)
{
	mLastMousePos.x = X;
	mLastMousePos.y = Y;

	SetCapture(mhMainWindow);
}

void HillsApp::OnMouseUp(WPARAM InBtnState, const int X, const int Y)
{
	ReleaseCapture();
}

void HillsApp::OnMouseMove(WPARAM InBtnState, const int X, const int Y)
{
	if ((InBtnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		const float dx = XMConvertToRadians(0.25f * static_cast<float>(X - mLastMousePos.x));
		const float dy = XMConvertToRadians(0.25f * static_cast<float>(Y - mLastMousePos.y));

		// Update angles based on input to orbit camera around box.
		mTheta += dx;
		mPhi += dy;

		// Restrict the angle mPhi.
		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if((InBtnState & MK_RBUTTON) != 0)
	{
		// Make each pixel correspond to 0.2 unit in the scene.
		const float dx = 0.2f * static_cast<float>(X - mLastMousePos.x);
		const float dy = 0.2f * static_cast<float>(Y - mLastMousePos.y);

		// Update the camera radius based on input.
		mRadius += dx - dy;

		// Restrict the radius.
		mRadius = MathHelper::Clamp(mRadius, 50.0f, 500.0f);
	}

	mLastMousePos.x = X;
	mLastMousePos.y = Y;
}

float HillsApp::GetHeight(const float X, const float Z) const
{
	constexpr float Height = 0.3f;
	constexpr float Width = 0.1f;
	return Height * (Z * sinf(Width * X) + X * cosf(Width * Z));
}

void HillsApp::BuildGeometryBuffers()
{
	GeometryGenerator::MeshData Grid;

	constexpr float Width = 160.f;
	constexpr float Depth = 160.f;
	constexpr UINT M = 50;
	constexpr UINT N = 50;

	GeometryGenerator::CreateGrid(Width, Depth, M, N, Grid);

	mGridIndexCount = Grid.Indices.size();

	//
	// Extract the vertex elements we are interested and apply the height function to
	// each vertex.  In addition, color the vertices based on their height so we have
	// sandy looking beaches, grassy low hills, and snow mountain peaks.
	//

	const size_t GridSize = Grid.Vertices.size();
	std::vector<Vertex> vertices(GridSize);
	for (size_t i = 0; i < GridSize; ++i)
	{
		XMFLOAT3 VertexPos = Grid.Vertices[i].Position;
		VertexPos.y = GetHeight(VertexPos.x, VertexPos.z);
		vertices[i].Pos = VertexPos;

		// Color the vertex based on its height.
		if (VertexPos.y < -10.0f)
		{
			// Sandy beach color.
			vertices[i].Color = XMFLOAT4(1.0f, 0.96f, 0.62f, 1.0f);
		}
		else if (VertexPos.y < 5.0f)
		{
			// Light yellow-green.
			vertices[i].Color = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
		}
		else if (VertexPos.y < 12.0f)
		{
			// Dark yellow-green.
			vertices[i].Color = XMFLOAT4(0.1f, 0.48f, 0.19f, 1.0f);
		}
		else if (VertexPos.y < 20.0f)
		{
			// Dark brown.
			vertices[i].Color = XMFLOAT4(0.45f, 0.39f, 0.34f, 1.0f);
		}
		else
		{
			// White snow.
			vertices[i].Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		}
	}

	D3D11_BUFFER_DESC VBDesc;
	VBDesc.Usage = D3D11_USAGE_IMMUTABLE;
	VBDesc.ByteWidth = sizeof(Vertex) * GridSize;
	VBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	VBDesc.CPUAccessFlags = 0;
	VBDesc.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA VInitData;
	VInitData.pSysMem = &vertices[0];
	HR(mD3DDevice->CreateBuffer(&VBDesc, &VInitData, &mGridVB));

	//
	// Pack the indices of all the meshes into one index buffer.
	//

	D3D11_BUFFER_DESC IBDesc;
	IBDesc.Usage = D3D11_USAGE_IMMUTABLE;
	IBDesc.ByteWidth = sizeof(UINT) * mGridIndexCount;
	IBDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	IBDesc.CPUAccessFlags = 0;
	IBDesc.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA IInitData;
	IInitData.pSysMem = &Grid.Indices[0];
	HR(mD3DDevice->CreateBuffer(&IBDesc, &IInitData, &mGridIB));
}

void HillsApp::BuildFX()
{
	std::ifstream fin("fx/color.fxo", std::ios::binary);

	fin.seekg(0, std::ios_base::end);
	int size = (int)fin.tellg();
	fin.seekg(0, std::ios_base::beg);
	std::vector<char> CompiledShader(size);

	fin.read(&CompiledShader[0], size);
	fin.close();

	HR(D3DX11CreateEffectFromMemory(
		&CompiledShader[0],
		size,
		0,
		mD3DDevice,
		&mFX
	));

	mTech = mFX->GetTechniqueByName("ColorTech");
	mfxWorldViewProj = mFX->GetVariableByName("gWorldViewProjection")->AsMatrix();
}

void HillsApp::BuildVertexLayout()
{
	// Create the vertex input layout.
	D3D11_INPUT_ELEMENT_DESC VertexDesc[] = 
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	// Create the input layout
	D3DX11_PASS_DESC passDesc;
	mTech->GetPassByIndex(0)->GetDesc(&passDesc);
	HR(mD3DDevice->CreateInputLayout(VertexDesc, 2, passDesc.pIAInputSignature,
		passDesc.IAInputSignatureSize, &mInputLayout));
}

