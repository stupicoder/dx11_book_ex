

/*
    HLSL(High Level Shading Language)
*/

/*
    상수버퍼 Constant Buffer
    상수 버퍼를 갱신할 때는 모든 변수를 갱신해야함
    변수를 업데이트 주기에 따라 나누는 것이 필요
*/
cbuffer cbPerObject
{
    float4x4 gWorldViewProjection;
};

/*
*   시맨틱
    : POSITION, COLOR 는 입력 매개변수와 출력 매개변수를 대응시킴
    SV_ : System Value 를 의미
    D3D11_INPUT_ELEMENT_DESC 배열에 의해 정점의 성분마다 시맨틱이 설정됨
*/
struct VertexIn
{
    float3 PosL  : POSITION;
    float4 Color : COLOR;
};

struct VertexOut
{
    float4 PosH  : SV_POSITION;
    float4 Color : COLOR;
};

/*
    정점 쉐이더 함수
*/
// out 은 출력 매개변수를 의미, 아니면 구조체로 만들어 return 시킴
//void VS(float3 iPosL : POSITION,
//        float4 iColor : COLOR,
//        out float4 oPosH : SV_POSITION,
//        out float4 oColor : COLOR)
//{ ... }
VertexOut VS(VertexIn vin)
{
    VertexOut vout;

    // Transform to homogeneous clip space. 동차 절단 공간으로 변환
    // vector 가 아닌 point를 의미하므로 w=1.f 로 설정
    // mul : 벡터 행렬 곱 수행
    vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProjection);
    // Geometry Shader 를 사용하지 않으면 투영변환까지 수행해야함
    // 원근나누기는 수행하지 말아야함 (하드웨어에서 수행)

    // Just pass vertex color into the pixel shader. 정점 컬러 출력
    vout.Color = vin.Color;

    return vout;
}

/*
    픽셀 쉐이더 함수
    Rasterizer Stage 에서 Vertex Output의 결과를 보간시킨 픽셀 별로 수행
    각 Pixel Fragment 별로 1번 수행
    중간에 제거 되어 후면버퍼에 기록되지 못하는 경우도 있다. (clip 함수도 존재)
    깊이 스텐실에 의해 제거 될 수 있음
    픽셀 : 프레임 버퍼에 기록된 결과
    픽셀 프래그먼트 : 프레임 버퍼에 기록될 수 있는 후보
*/
// SV_Target : 반환값이 렌더 대상 형식과 일치해야함
float4 PS(VertexOut pin) : SV_Target
{
    return pin.Color;
}

/*
    하나의 technique 은 하나 이상의 pass 를 포함
    다중 pass를 포함하고 다중패스로 렌더링하는 경우도 있음
    여러 기법을 하나의 효과 그룹으로 묶을 수도 있음
*/
technique11 ColorTech
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }
}
