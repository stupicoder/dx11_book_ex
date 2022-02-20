#include "WavesApp.h"

#include "../../Common/GeometryGenerator.h"

struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

WavesApp::WavesApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
	, mLandVB(NULL), mLandIB(NULL)
	, mWavesVB(NULL), mWavesIB(NULL)
	, mFX(NULL), mTech(NULL)
	, mfxWorldViewProj(NULL)
	, mInputLayout(NULL)
	, mWireframeRS(NULL)
	, mGridIndexCount(0)
	, mTheta(1.5f * MathHelper::Pi)
	, mPhi(0.1f * MathHelper::Pi)
	, mRadius(200.f)
{
	mMainWindowCaption = L"Waves Demo";

	mLastMousePos.x = 0;
	mLastMousePos.y = 0;

	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&mGridWorld, I);
	XMStoreFloat4x4(&mWavesWorld, I);
	XMStoreFloat4x4(&mView, I);
	XMStoreFloat4x4(&mProj, I);
}

WavesApp::~WavesApp()
{
	ReleaseCOM(mLandVB);
	ReleaseCOM(mLandIB);
	ReleaseCOM(mWavesVB);
	ReleaseCOM(mWavesIB);
	ReleaseCOM(mFX);
	ReleaseCOM(mInputLayout);
	ReleaseCOM(mWireframeRS);
}

bool WavesApp::Init()
{
	if (!D3DApp::Init())
	{
		return false;
	}

	mWaves.Init(200, 200, 0.8f, 0.03f, 3.25f, 0.4f);

	BuildLandGeometryBuffers();
	BuildWavesGeometryBuffers();
	BuildFX();
	BuildVertexLayout();

	D3D11_RASTERIZER_DESC wireframeDesc;
	ZeroMemory(&wireframeDesc, sizeof(D3D11_RASTERIZER_DESC));
	wireframeDesc.FillMode = D3D11_FILL_WIREFRAME;
	wireframeDesc.CullMode = D3D11_CULL_BACK;
	wireframeDesc.FrontCounterClockwise = false;
	wireframeDesc.DepthClipEnable = true;

	HR(mD3DDevice->CreateRasterizerState(&wireframeDesc, &mWireframeRS));

	return true;
}

void WavesApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, GetAspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void WavesApp::UdapteScene(const float InDeltaTime)
{
	// Convert Spherical to Cartesian coordinates.
	const float x = mRadius * sinf(mPhi) * cosf(mTheta);
	const float z = mRadius * sinf(mPhi) * sinf(mTheta);
	const float y = mRadius * cosf(mPhi);

	// Build the view matrix.
	XMVECTOR pos = XMVectorSet(x, y, z, 1.f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.f, 1.f, 0.f, 0.f);

	XMMATRIX V = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, V);

	//
	// Every quarter second, generate a random wave.
	//
	static float t_base = 0.0f;
	if ((mTimer.GetTotalTime() - t_base) >= 0.25f)
	{
		t_base += 0.25f;

		DWORD i = 5 + rand() % 190;
		DWORD j = 5 + rand() % 190;

		float r = MathHelper::RandF(1.0f, 2.0f);

		mWaves.Disturb(i, j, r);
	}

	mWaves.Update(InDeltaTime);

	//
	// Update the wave vertex buffer with the new solution.
	//

	D3D11_MAPPED_SUBRESOURCE MappedData;
	HR(mD3DImmediateContext->Map(mWavesVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedData));

	Vertex* v = reinterpret_cast<Vertex*>(MappedData.pData);
	for (UINT i = 0; i < mWaves.VertexCount(); ++i)
	{
		v[i].Pos = mWaves[i];
		v[i].Color = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
	}

	mD3DImmediateContext->Unmap(mWavesVB, 0);
}

void WavesApp::DrawScene()
{
	mD3DImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::LightSteelBlue));
	mD3DImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	mD3DImmediateContext->IASetInputLayout(mInputLayout);
	mD3DImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);

	D3DX11_TECHNIQUE_DESC techDesc;
	mTech->GetDesc(&techDesc);
	for (UINT p = 0; p < techDesc.Passes; ++p)
	{
		//
		// Draw the land.
		//
		mD3DImmediateContext->IASetVertexBuffers(0, 1, &mLandVB, &stride, &offset);
		mD3DImmediateContext->IASetIndexBuffer(mLandIB, DXGI_FORMAT_R32_UINT, 0);

		XMMATRIX world = XMLoadFloat4x4(&mGridWorld);
		XMMATRIX worldViewProj = world * view * proj;
		mfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
		mTech->GetPassByIndex(p)->Apply(0, mD3DImmediateContext);
		mD3DImmediateContext->DrawIndexed(mGridIndexCount, 0, 0);

		//
		// Draw the waves.
		//

		mD3DImmediateContext->RSSetState(mWireframeRS);

		mD3DImmediateContext->IASetVertexBuffers(0, 1, &mWavesVB, &stride, &offset);
		mD3DImmediateContext->IASetIndexBuffer(mWavesIB, DXGI_FORMAT_R32_UINT, 0);

		world = XMLoadFloat4x4(&mWavesWorld);
		worldViewProj = world * view * proj;
		mfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
		mTech->GetPassByIndex(p)->Apply(0, mD3DImmediateContext);
		mD3DImmediateContext->DrawIndexed(3 * mWaves.TriangleCount(), 0, 0);

		// Restore default.
		mD3DImmediateContext->RSSetState(0);
	}

	HR(mSwapChain->Present(0, 0));
}

void WavesApp::OnMouseDown(WPARAM InBtnState, const int X, const int Y)
{
	mLastMousePos.x = X;
	mLastMousePos.y = Y;

	SetCapture(mhMainWindow);
}

void WavesApp::OnMouseUp(WPARAM InBtnState, const int X, const int Y)
{
	ReleaseCapture();
}

void WavesApp::OnMouseMove(WPARAM InBtnState, const int X, const int Y)
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
		float dx = 0.2f * static_cast<float>(X - mLastMousePos.x);
		float dy = 0.2f * static_cast<float>(Y - mLastMousePos.y);

		// Update the camera radius based on input.
		mRadius += dx - dy;

		// Restrict the radius.
		mRadius = MathHelper::Clamp(mRadius, 50.0f, 500.0f);
	}

	mLastMousePos.x = X;
	mLastMousePos.y = Y;
}

float WavesApp::GetHeight(const float X, const float Z) const
{
	return 0.3f * (Z * sinf(0.1f * X) + X * cosf(0.1f * Z));
}

void WavesApp::BuildLandGeometryBuffers()
{
	GeometryGenerator::MeshData grid;

	GeometryGenerator::CreateGrid(160.f, 160.f, 50, 50, grid);

	mGridIndexCount = grid.Indices.size();

	//
	// Extract the vertex elements we are interested and apply the height function to
	// each vertex.  In addition, color the vertices based on their height so we have
	// sandy looking beaches, grassy low hills, and snow mountain peaks.
	//

	std::vector<Vertex> vertices(grid.Vertices.size());
	for (size_t i = 0; i < grid.Vertices.size(); ++i)
	{
		XMFLOAT3 p = grid.Vertices[i].Position;

		p.y = GetHeight(p.x, p.z);

		vertices[i].Pos = p;

		// Color the vertex based on its height.
		if (p.y < -10.f)
		{
			// Sandy beach color.
			vertices[i].Color = XMFLOAT4(1.f, 0.98f, 0.62f, 1.f);
		}
		else if (p.y < 5.f)
		{
			// Light yellow-green.
			vertices[i].Color = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.f);
		}
		else if (p.y < 12.0f)
		{
			// Dark yellow-green.
			vertices[i].Color = XMFLOAT4(0.1f, 0.48f, 0.19f, 1.0f);
		}
		else if (p.y < 20.0f)
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
	VBDesc.ByteWidth = sizeof(Vertex) * grid.Vertices.size();
	VBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	VBDesc.CPUAccessFlags = 0;
	VBDesc.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA VInitData;
	VInitData.pSysMem = &vertices[0];
	HR(mD3DDevice->CreateBuffer(&VBDesc, &VInitData, &mLandVB));

	//
	// Pack the indices of all the meshes into one index buffer.
	//

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * mGridIndexCount;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &grid.Indices[0];
	HR(mD3DDevice->CreateBuffer(&ibd, &iinitData, &mLandIB));
}

void WavesApp::BuildWavesGeometryBuffers()
{
	// Create the vertex buffer.  Note that we allocate space only, as
		// we will be updating the data every time step of the simulation.

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_DYNAMIC;
	vbd.ByteWidth = sizeof(Vertex) * mWaves.VertexCount();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vbd.MiscFlags = 0;
	HR(mD3DDevice->CreateBuffer(&vbd, 0, &mWavesVB));


	// Create the index buffer.  The index buffer is fixed, so we only 
	// need to create and set once.

	std::vector<UINT> indices(3 * mWaves.TriangleCount()); // 3 indices per face

	// Iterate over each quad.
	UINT m = mWaves.RowCount();
	UINT n = mWaves.ColumnCount();
	int k = 0;
	for (UINT i = 0; i < m - 1; ++i)
	{
		for (DWORD j = 0; j < n - 1; ++j)
		{
			indices[k] = i * n + j;
			indices[k + 1] = i * n + j + 1;
			indices[k + 2] = (i + 1) * n + j;

			indices[k + 3] = (i + 1) * n + j;
			indices[k + 4] = i * n + j + 1;
			indices[k + 5] = (i + 1) * n + j + 1;

			k += 6; // next quad
		}
	}

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * indices.size();
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &indices[0];
	HR(mD3DDevice->CreateBuffer(&ibd, &iinitData, &mWavesIB));
}

void WavesApp::BuildFX()
{
	std::ifstream fin("fx/color.fxo", std::ios::binary);

	fin.seekg(0, std::ios_base::end);
	int size = (int)fin.tellg();
	fin.seekg(0, std::ios_base::beg);
	std::vector<char> compiledShader(size);

	fin.read(&compiledShader[0], size);
	fin.close();

	HR(D3DX11CreateEffectFromMemory(&compiledShader[0], size,
		0, mD3DDevice, &mFX));

	mTech = mFX->GetTechniqueByName("ColorTech");
	mfxWorldViewProj = mFX->GetVariableByName("gWorldViewProjection")->AsMatrix();
}

void WavesApp::BuildVertexLayout()
{
	// Create the vertex input layout.
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	// Create the input layout
	D3DX11_PASS_DESC passDesc;
	mTech->GetPassByIndex(0)->GetDesc(&passDesc);
	HR(mD3DDevice->CreateInputLayout(vertexDesc, 2, passDesc.pIAInputSignature,
		passDesc.IAInputSignatureSize, &mInputLayout));
}
