#include "Box.h"

struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

BoxApp::BoxApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
	, mBoxVB(NULL)
	, mBoxIB(NULL)
	, mFX(NULL), mTech(NULL)
	, mfxWorldViewProj(NULL)
	, mInputLayout(NULL)
	, mTheta(1.5f * MathHelper::Pi)
	, mPhi(0.25f * MathHelper::Pi)
	, mRadius(5.f)
{
	mMainWindowCaption = L"Box Demo";

	mLastMousePos.x = 0;
	mLastMousePos.y = 0;

	XMMATRIX IdentityMat = XMMatrixIdentity();
	XMStoreFloat4x4(&mWorld, IdentityMat);
	XMStoreFloat4x4(&mView, IdentityMat);
	XMStoreFloat4x4(&mProjection, IdentityMat);
}

BoxApp::~BoxApp()
{
	ReleaseCOM(mBoxVB);
	ReleaseCOM(mBoxIB);
	ReleaseCOM(mFX);
	ReleaseCOM(mInputLayout);
}

bool BoxApp::Init()
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

void BoxApp::OnResize()
{
	D3DApp::OnResize();

	// The window resized. so update the aspect ratio and recompute projection matrix.
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, GetAspectRatio(), 1.f, 1000.f);
	XMStoreFloat4x4(&mProjection, P);
}

void BoxApp::UdapteScene(const float InDeltaTime)
{
	// Convert spherical to cartesian coordinates.
	const float x = mRadius * sinf(mPhi) * cosf(mTheta);
	const float y = mRadius * sinf(mPhi) * sinf(mTheta);
	const float z = mRadius * cosf(mPhi);

	// Build teh view matrix.
	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX V = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, V);
}

void BoxApp::DrawScene()
{
	assert(mD3DImmediateContext);
	assert(mSwapChain);

	mD3DImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::Blue));
	mD3DImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// 입력 배치 장치에 묶음
	mD3DImmediateContext->IASetInputLayout(mInputLayout);
	// 정점 및 인덱스를 어떤 방식으로 구성할지 지정
	mD3DImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 정점 버퍼 묶음
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	mD3DImmediateContext->IASetVertexBuffers(
		0, // 입력 슬롯 인덱스
		1, // 정점 버퍼 개수
		&mBoxVB, // 정점 버퍼 배열
		&stride, // 정점 버퍼의 원소 사이즈 배열
		&offset); // 정점 버퍼 시작 오프셋 배열

	mD3DImmediateContext->IASetIndexBuffer(
		mBoxIB, // 인덱스 버퍼
		DXGI_FORMAT_R32_UINT, // 인덱스의 형식, 인덱스 범위에 따라 16비트를 사용하면 메모리 절약 가능
		0); // 인덱스 버퍼를 읽기 시작할 오프셋, 인덱스 앞부분을 건너뛰어야되는 경우가 있음


	// Set constants
	XMMATRIX World = XMLoadFloat4x4(&mWorld);
	XMMATRIX View = XMLoadFloat4x4(&mView);
	XMMATRIX Proj = XMLoadFloat4x4(&mProjection);
	XMMATRIX WorldViewProj = World * View * Proj;

	mfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&WorldViewProj));

	// 패스 정보
	D3DX11_TECHNIQUE_DESC TechDesc;
	mTech->GetDesc(&TechDesc);
	for (UINT p = 0; p < TechDesc.Passes; ++p)
	{
		ID3DX11EffectPass* pass = mTech->GetPassByIndex(p);
		pass->Apply( // 상수 버퍼를 갱신 한다
			0, // 다음에 사용, 일단 0
			mD3DImmediateContext); // 패스가 사용할 장치 문맥

		// 기하 구조를 그린다.
		mD3DImmediateContext->DrawIndexed(
			36, // 사용할 인덱스 수
			0, // 버퍼 안에서 사용할 처음 인덱스 위치
			0); // 버텍스 인덱스 버퍼를 합치게되면 인덱스에 시작 버텍스 버퍼의 위치를 전부 더해줘야 함, 그역할을 하는 값

	}

	HR(mSwapChain->Present(0, 0));
}

void BoxApp::OnMouseDown(WPARAM InBtnState, const int X, const int Y)
{
	mLastMousePos.x = X;
	mLastMousePos.y = Y;

	SetCapture(mhMainWindow);
}

void BoxApp::OnMouseUp(WPARAM InBtnState, const int X, const int Y)
{
	ReleaseCapture();
}

void BoxApp::OnMouseMove(WPARAM InBtnState, const int X, const int Y)
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
	else
	{
		// Make each pixel correspond to 0.005 unit in the scene.
		const float dx = 0.005f * static_cast<float>(X - mLastMousePos.x);
		const float dy = 0.005f * static_cast<float>(Y - mLastMousePos.y);

		// Update the camera radius based on input.
		mRadius += dx - dy;

		// Restrict the radius.
		mRadius = MathHelper::Clamp(mRadius, 3.0f, 15.0f);
	}

	mLastMousePos.x = X;
	mLastMousePos.y = Y;
}

void BoxApp::BuildGeometryBuffers()
{
	// Create vertex buffer.
	Vertex vertices[] =
	{
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), (const float*)&Colors::White},
		{ XMFLOAT3(-1.0f, +1.0f, -1.0f), (const float*)&Colors::Black},
		{ XMFLOAT3(+1.0f, +1.0f, -1.0f), (const float*)&Colors::Red},
		{ XMFLOAT3(+1.0f, -1.0f, -1.0f), (const float*)&Colors::Green},
		{ XMFLOAT3(-1.0f, -1.0f, +1.0f), (const float*)&Colors::Blue},
		{ XMFLOAT3(-1.0f, +1.0f, +1.0f), (const float*)&Colors::Yellow},
		{ XMFLOAT3(+1.0f, +1.0f, +1.0f), (const float*)&Colors::Cyan},
		{ XMFLOAT3(+1.0f, -1.0f, +1.0f), (const float*)&Colors::Magenta}
	};

	// ID3D11Buffer 를 서술
	D3D11_BUFFER_DESC VertexBufferDesc;
	VertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE; // 버퍼 내용 불변
	VertexBufferDesc.ByteWidth = sizeof(Vertex) * 8; // Vertex 원소의 사이즈
	VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER; // 정점 버퍼 지정 플래그
	VertexBufferDesc.CPUAccessFlags = 0; // CPU 에서 접근하지 않음 - Immutable 이면 0
	VertexBufferDesc.MiscFlags = 0; // 정점 버퍼는 0
	VertexBufferDesc.StructureByteStride = 0; // Structure 버퍼의 원소 사이즈, 정점 버퍼는 0

	// 버퍼 초기화에 사용될 데이터
	D3D11_SUBRESOURCE_DATA VertexInitData;
	VertexInitData.pSysMem = vertices; // 정점 구조체 배열의 첫 주소
	VertexInitData.SysMemPitch = 0; // 정점 버퍼는 0
	VertexInitData.SysMemSlicePitch = 0; // 정점 버퍼는 0

	// 정점 버퍼 생성
	HR(mD3DDevice->CreateBuffer(&VertexBufferDesc, &VertexInitData, &mBoxVB));

	// Create index buffer.
	UINT indices[] =
	{
		// front face
		0, 1, 2,
		0, 2, 3,
		// back face
		4, 6, 5,
		4, 7, 6,
		// left face
		4, 5, 1,
		4, 1, 0,
		// right face
		3, 2, 6,
		3, 6, 7,
		// top face
		1, 5, 6,
		1, 6, 2,
		// bottom face
		4, 0, 3,
		4, 3, 7
	};

	// 인덱스 버퍼 서술
	D3D11_BUFFER_DESC IndexBufferDesc;
	IndexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE; // 불변 버퍼
	IndexBufferDesc.ByteWidth = sizeof(UINT) * 36; // Index 의 원소 사이즈
	IndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER; // 색인 버퍼 지정
	IndexBufferDesc.CPUAccessFlags = 0;
	IndexBufferDesc.MiscFlags = 0;
	IndexBufferDesc.StructureByteStride = 0;

	// 버퍼 초기화에 사용될 데이터
	D3D11_SUBRESOURCE_DATA IndexInitData;
	IndexInitData.pSysMem = indices;
	IndexInitData.SysMemPitch = 0;
	IndexInitData.SysMemSlicePitch = 0;

	// 인덱스 버퍼 생성
	HR(mD3DDevice->CreateBuffer(&IndexBufferDesc, &IndexInitData, &mBoxIB));
}

void BoxApp::BuildFX()
{
	DWORD shaderFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
	shaderFlags |= D3D10_SHADER_DEBUG;
	shaderFlags |= D3D10_SHADER_SKIP_OPTIMIZATION;
#endif

	/*
		ID3D10Blob : 범용 메모리 블록
		GetBufferPointer : 자료에 대한 void* 반환
		GetBufferSize : 메모리 블록을 바이트 단위로 계산한 값 반환
	*/
	ID3D10Blob* CompiledShader = NULL;
	ID3D10Blob* CompilationMsgs = NULL;

	//HRESULT hr = D3DCompileFromFile(
	//	L"FX/Color.fx", // 컴파일 할 쉐이터 소스 파일 경로
	//	NULL, // CONST D3D10_SHADER_MACRO* pDefines : 고급 옵션
	//	NULL, // LPD3D10INCLUDE pInclude : 고급 옵션
	//	NULL, // LPCSTR pFunctionName : 쉐이더 프로그램 진입점, 주 함수 이름. 개별적으로 컴파일 할 때만 사용. 효과 프레임워크 사용시 NULL
	//	"fx_5_0", // 사용할 쉐이더 버전을 뜻하는 문자열 
	//	shaderFlags, // 쉐이더 코드 컴파일 방식에 영향을 주는 플래그
	//	0, // 고급 옵션
	//	&CompiledShader, // 컴파일된 쉐이더를 담을 구조체 포인터
	//	&CompilationMsgs // 컴파일 오류 메세지를 담은 문자열을 담은 구조체 반환
	//);

	/*
		Header: D3DX11async.h
		Library: D3DX11.lib
	*/
	HRESULT hr = D3DX11CompileFromFile(
		L"FX/Color.fx", // 컴파일 할 쉐이터 소스 파일 경로
		NULL, // CONST D3D10_SHADER_MACRO* pDefines : 고급 옵션
		NULL, // LPD3D10INCLUDE pInclude : 고급 옵션
		NULL, // LPCSTR pFunctionName : 쉐이더 프로그램 진입점, 주 함수 이름. 개별적으로 컴파일 할 때만 사용. 효과 프레임워크 사용시 NULL
		"fx_5_0", // 사용할 쉐이더 버전을 뜻하는 문자열 
		shaderFlags, // 쉐이더 코드 컴파일 방식에 영향을 주는 플래그
		0, // 고급 옵션
		0, // 고급 옵션, 쉐이더를 비동기 적으로 컴파일 하기 위한 옵션
		&CompiledShader, // 컴파일된 쉐이더를 담을 구조체 포인터
		&CompilationMsgs, // 컴파일 오류 메세지를 담은 문자열을 담은 구조체 반환
		0); // 비동기 컴파일 시 오류코드를 조회하는데 사용

	// CompilationMsgs can store errors or warnings.
	if (CompilationMsgs != NULL)
	{
		MessageBoxA(0, (char*)CompilationMsgs->GetBufferPointer(), 0, 0);
		ReleaseCOM(CompilationMsgs);
	}

	// Even if there are no CompilationMsgs, check to make sure there were no other errors.
	if (FAILED(hr))
	{
		// WFILE -> __FILE__ or __FILEW__
		DXTrace(__FILE__, (DWORD)__LINE__, hr, L"D3DX11CompileFromFile", true);
	}

	HR(D3DX11CreateEffectFromMemory(
		CompiledShader->GetBufferPointer(), // 컴파일된 쉐이더 포인터
		CompiledShader->GetBufferSize(), // 컴파일된 쉐이더 사이즈 (바이트)
		0, // 효과 플래그, Flags2 에 지정한 값과 일치해야 함.
		mD3DDevice, // DirectX11 장치를 가리키는 포인터
		&mFX)); // 생성된 효과 파일

	// Done with Compiled shader.
	ReleaseCOM(CompiledShader);

	// 테크닉 객체를 가리키는 포인터 반환
	mTech = mFX->GetTechniqueByName("ColorTech");
	// 상수 버퍼 변수에 대한 포인터를 얻음
	mfxWorldViewProj = mFX->GetVariableByName("gWorldViewProjection")->AsMatrix();
	/*
		AsMatrix, AsVector, AsScalar
		SetMetrix, SetFloatVector, SetFloat, SetInt, SetBool
		특수화 시키지 않고 SetRawValue 로 값을 세팅 가능, 임의 크기 변수 or 구조체 세팅 가능
		위 함수들로 효과 객체의 내부 캐시가 갱신되는 것일 뿐
		실제 GPU 메모리 갱신은 렌더링 패스 수행할 때 갱신된다.
	*/
}

void BoxApp::BuildVertexLayout()
{
	// Create vertex input layout
	D3D11_INPUT_ELEMENT_DESC VertexDesc[] =
	{
		{
			"POSITION", // 매칭될 Semantic 이름
			0, // Semantic 인덱스, texcoord 등에서 사용
			DXGI_FORMAT_R32G32B32_FLOAT, // 정점 성분의 자료형식
			0, // 자료가 입력될 정점 버퍼 슬롯 인덱스 (0 ~ 15)
			0, // ???
			D3D11_INPUT_PER_VERTEX_DATA, // 다른 값은 인스턴싱에 쓰임
			0 // 다른 값은 인스턴싱에 쓰임
		},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	// 현재 패스 정보 서술
	D3DX11_PASS_DESC PassDesc;
	ID3DX11EffectPass* Pass = mTech->GetPassByIndex(0);
	Pass->GetDesc(&PassDesc);

	HR(mD3DDevice->CreateInputLayout(
		VertexDesc, // 정점 구조체 서술 배열
		2, // 배열의 크기
		PassDesc.pIAInputSignature, // 정점 쉐이더 바이트코드 포인터
		PassDesc.IAInputSignatureSize, // 바이트 크기
		&mInputLayout)); // 반환될 InputLayout
}
