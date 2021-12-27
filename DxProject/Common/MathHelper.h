#pragma once

#include <Windows.h>
#include <xnamath.h>

class MathHelper
{
public:
	// Returns random float in [0, 1).
	static float RandF()
	{
		return (float)(rand()) / (float)RAND_MAX;
	}

	// Returns random float in [a, b).
	static float RandF(const float a, const float b)
	{
		return a + RandF() * (b - a);
	}

	template<typename T>
	static T Min(const T& a, const T& b)
	{
		return a < b ? a : b;
	}

	template<typename T>
	static T Max(const T& a, const T& b)
	{
		return a > b ? a : b;
	}

	template<typename T>
	static T Lerp(const T& a, const T& b, const float t)
	{
		return a + (b - a) * t;
	}

	template<typename T>
	static T Clamp(const T& x, const T& low, const T& high)
	{
		return x < low ? low : (x > high ? high : x);
	}

	// Returns the polar angle of the point (x, y) in [0, 2 * Pi).
	static float AngleFromXY(const float x, const float y);

	static XMMATRIX InverseTranspose(const CXMMATRIX& M)
	{
		/*
			Inverse-transpose is just applied to normals.
			So zero out translation row so that it doesn't get into our inverse-transpose
			calculation--we don't want the inverse-transpose of the translation.
		*/
		// 역행렬의 전치 행렬을 얻음, 위치 변환은 무시
		XMMATRIX A = M;
		A.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

		XMVECTOR det = XMMatrixDeterminant(A);
		return XMMatrixTranspose(XMMatrixInverse(&det, A));
	}

	static XMVECTOR RandUnitVec3();
	static XMVECTOR RandHemisphereUnitVec3(const XMVECTOR& n);

	static const float Infinity;
	static const float Pi;
};