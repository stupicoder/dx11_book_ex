#include "Shapes.h"

#include "../../Common/GeometryGenerator.h"

struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

ShapesApp::ShapesApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
	, mVB(NULL), mIB(NULL)
	, mFX(NULL), mTech(NULL)
	, mfxWorldViewProj(NULL)
	, mInputLayout(NULL), mWireframeRS(NULL)
	, mTheta(1.5f * MathHelper::Pi), mPhi(0.1f * MathHelper::Pi), mRadius(15.f)
{
	mMainWindowCaption = L"Shapes Demo";

	mLastMousePos.x = 0;
	mLastMousePos.y = 0;

	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&mGridWorld, I);
	XMStoreFloat4x4(&mView, I);
	XMStoreFloat4x4(&mProj, I);

	XMMATRIX boxScale = XMMatrixScaling(2.0f, 1.0f, 2.0f);
	XMMATRIX boxOffset = XMMatrixTranslation(0.0f, 0.5f, 0.0f);
	XMStoreFloat4x4(&mBoxWorld, XMMatrixMultiply(boxScale, boxOffset));

	XMMATRIX centerSphereScale = XMMatrixScaling(2.0f, 2.0f, 2.0f);
	XMMATRIX centerSphereOffset = XMMatrixTranslation(0.0f, 2.0f, 0.0f);
	XMStoreFloat4x4(&mCenterSphere, XMMatrixMultiply(centerSphereScale, centerSphereOffset));

	for (int i = 0; i < 5; ++i)
	{
		XMStoreFloat4x4(&mCylWorld[i * 2 + 0], XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f));
		XMStoreFloat4x4(&mCylWorld[i * 2 + 1], XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f));

		XMStoreFloat4x4(&mSphereWorld[i * 2 + 0], XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f));
		XMStoreFloat4x4(&mSphereWorld[i * 2 + 1], XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f));
	}
}

ShapesApp::~ShapesApp()
{
	ReleaseCOM(mVB);
	ReleaseCOM(mIB);
	ReleaseCOM(mFX);
	ReleaseCOM(mInputLayout);
	ReleaseCOM(mWireframeRS);
}

bool ShapesApp::Init()
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

void ShapesApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, GetAspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void ShapesApp::UdapteScene(const float InDeltaTime)
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

void ShapesApp::DrawScene()
{
	mD3DImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::LightSteelBlue));
	mD3DImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

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
	XMMATRIX viewProj = view * proj;

	D3DX11_TECHNIQUE_DESC TechDesc;
	mTech->GetDesc(&TechDesc);
	for (UINT p = 0; p < TechDesc.Passes; ++p)
	{
		// Draw the grid.
		XMMATRIX world = XMLoadFloat4x4(&mGridWorld);
		XMMATRIX worldViewProj = world * viewProj;
		mfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
		mTech->GetPassByIndex(p)->Apply(0, mD3DImmediateContext);
		mD3DImmediateContext->DrawIndexed(mGridIndexCount, mGridIndexOffset, mGridVertexOffset);

		// Draw the box.
		world = XMLoadFloat4x4(&mBoxWorld);
		worldViewProj = world * viewProj;
		mfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
		mTech->GetPassByIndex(p)->Apply(0, mD3DImmediateContext);
		mD3DImmediateContext->DrawIndexed(mBoxIndexCount, mBoxIndexOffset, mBoxVertexOffset);

		// Draw center sphere.
		world = XMLoadFloat4x4(&mCenterSphere);
		worldViewProj = world * viewProj;
		mfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
		mTech->GetPassByIndex(p)->Apply(0, mD3DImmediateContext);
		mD3DImmediateContext->DrawIndexed(mSphereIndexCount, mSphereIndexOffset, mSphereVertexOffset);

		// Draw the cylinders.
		for (int i = 0; i < 10; ++i)
		{
			world = XMLoadFloat4x4(&mCylWorld[i]);
			worldViewProj = world * viewProj;
			mfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
			mTech->GetPassByIndex(p)->Apply(0, mD3DImmediateContext);
			mD3DImmediateContext->DrawIndexed(mCylinderIndexCount, mCylinderIndexOffset, mCylinderVertexOffset);
		}

		// Draw the spheres.
		for (int i = 0; i < 10; ++i)
		{
			world = XMLoadFloat4x4(&mSphereWorld[i]);
			worldViewProj = world * viewProj;
			mfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
			mTech->GetPassByIndex(p)->Apply(0, mD3DImmediateContext);
			mD3DImmediateContext->DrawIndexed(mSphereIndexCount, mSphereIndexOffset, mSphereVertexOffset);
		}
	}

	HR(mSwapChain->Present(0, 0));
}

void ShapesApp::OnMouseDown(WPARAM InBtnState, const int X, const int Y)
{
	mLastMousePos.x = X;
	mLastMousePos.y = Y;

	SetCapture(mhMainWindow);
}

void ShapesApp::OnMouseUp(WPARAM InBtnState, const int X, const int Y)
{
	ReleaseCapture();
}

void ShapesApp::OnMouseMove(WPARAM InBtnState, const int X, const int Y)
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
		// Make each pixel correspond to 0.01 unit in the scene.
		float dx = 0.01f * static_cast<float>(X - mLastMousePos.x);
		float dy = 0.01f * static_cast<float>(Y - mLastMousePos.y);

		// Update the camera radius based on input.
		mRadius += dx - dy;

		// Restrict the radius.
		mRadius = MathHelper::Clamp(mRadius, 3.0f, 200.0f);
	}

	mLastMousePos.x = X;
	mLastMousePos.y = Y;
}

void ShapesApp::BuildGeometryBuffers()
{
	GeometryGenerator::MeshData Box;
	GeometryGenerator::MeshData Grid;
	GeometryGenerator::MeshData Sphere;
	GeometryGenerator::MeshData Cylinder;

	GeometryGenerator::CreateBox(1.f, 1.f, 1.f, Box);
	GeometryGenerator::CreateGrid(20.f, 30.f, 60, 40, Grid);
	GeometryGenerator::CreateSphere(0.5f, 20, 20, Sphere);
	//GeometryGenerator::CreateGeosphere(0.5f, 2, Sphere);
	GeometryGenerator::CreateCylinder(0.5f, 0.3f, 3.f, 20, 20, Cylinder);

	// Cache the vertex offsets to each object in the concatenated vertex buffer.
	mBoxVertexOffset = 0;
	mGridVertexOffset = Box.Vertices.size();
	mSphereVertexOffset = mGridVertexOffset + Grid.Vertices.size();
	mCylinderVertexOffset = mSphereVertexOffset + Sphere.Vertices.size();

	// Cache the index count of each object.
	mBoxIndexCount = Box.Indices.size();
	mGridIndexCount = Grid.Indices.size();
	mSphereIndexCount = Sphere.Indices.size();
	mCylinderIndexCount = Cylinder.Indices.size();

	// Cache the starting index for each object in the concatenated index buffer.
	mBoxIndexOffset = 0;
	mGridIndexOffset = mBoxIndexCount;
	mSphereIndexOffset = mGridIndexOffset + mGridIndexCount;
	mCylinderIndexOffset = mSphereIndexOffset + mSphereIndexCount;

	const UINT TotalVertexCount = 
		Box.Vertices.size() +
		Grid.Vertices.size() +
		Sphere.Vertices.size() + 
		Cylinder.Vertices.size();

	const UINT TotalIndexCount = 
		mBoxIndexCount + mGridIndexCount + mSphereIndexCount + mCylinderIndexCount;

	//
	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.
	//

	std::vector<Vertex> Vertices(TotalVertexCount);

	XMFLOAT4 Black(0.f, 0.f, 0.f, 1.f);

	UINT k = 0;
	for (size_t i = 0; i < Box.Vertices.size(); ++i, ++k)
	{
		Vertices[k].Pos = Box.Vertices[i].Position;
		Vertices[k].Color = Black;
	}

	for (size_t i = 0; i < Grid.Vertices.size(); ++i, ++k)
	{
		Vertices[k].Pos = Grid.Vertices[i].Position;
		Vertices[k].Color = Black;
	}

	for (size_t i = 0; i < Sphere.Vertices.size(); ++i, ++k)
	{
		Vertices[k].Pos = Sphere.Vertices[i].Position;
		Vertices[k].Color = Black;
	}

	for (size_t i = 0; i < Cylinder.Vertices.size(); ++i, ++k)
	{
		Vertices[k].Pos = Cylinder.Vertices[i].Position;
		Vertices[k].Color = Black;
	}

	D3D11_BUFFER_DESC VBDesc;
	VBDesc.Usage = D3D11_USAGE_IMMUTABLE;
	VBDesc.ByteWidth = sizeof(Vertex) * TotalVertexCount;
	VBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	VBDesc.CPUAccessFlags = 0;
	VBDesc.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA VInitData;
	VInitData.pSysMem = &Vertices[0];
	HR(mD3DDevice->CreateBuffer(&VBDesc, &VInitData, &mVB));

	//
	// Pack the indices of all the meshes into one index buffer.
	//

	std::vector<UINT> Indices;
	Indices.insert(Indices.end(), Box.Indices.begin(), Box.Indices.end());
	Indices.insert(Indices.end(), Grid.Indices.begin(), Grid.Indices.end());
	Indices.insert(Indices.end(), Sphere.Indices.begin(), Sphere.Indices.end());
	Indices.insert(Indices.end(), Cylinder.Indices.begin(), Cylinder.Indices.end());

	D3D11_BUFFER_DESC IBDesc;
	IBDesc.Usage = D3D11_USAGE_IMMUTABLE;
	IBDesc.ByteWidth = sizeof(UINT) * TotalIndexCount;
	IBDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	IBDesc.CPUAccessFlags = 0;
	IBDesc.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA IInitData;
	IInitData.pSysMem = &Indices[0];
	HR(mD3DDevice->CreateBuffer(&IBDesc, &IInitData, &mIB));
}

void ShapesApp::BuildFX()
{
	std::ifstream fin("fx/color.fxo", std::ios::binary);

	fin.seekg(0, std::ios_base::end);
	int size = (int)fin.tellg();
	fin.seekg(0, std::ios_base::beg);
	std::vector<char> CompiledShader(size);

	fin.read(&CompiledShader[0], size);
	fin.close();

	HR(D3DX11CreateEffectFromMemory(&CompiledShader[0], size,
		0, mD3DDevice, &mFX));

	mTech = mFX->GetTechniqueByName("ColorTech");
	mfxWorldViewProj = mFX->GetVariableByName("gWorldViewProjection")->AsMatrix();
}

void ShapesApp::BuildVertexLayout()
{
	// Create the vertex input layout.
	D3D11_INPUT_ELEMENT_DESC VertexDesc[]=
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
