#include "MathHelper.h"
#include <float.h>
#include <cmath>


const float MathHelper::Infinity = FLT_MAX;
const float MathHelper::Pi = 3.1415926535f;

float MathHelper::AngleFromXY(const float x, const float y)
{
	float theta = 0.0f;

	// 근데 이거 Normal 안된것도 상관 없나??

	// Quadrant I or IV (1,4 사분면)
	if (x >= 0.0f)
	{
		// if x = 0, then atanf(y / x) = +Pi / 2 if y > 0
		//                atanf(y / x) = -Pi / 2 if y < 0
		theta = atanf(y / x); // in [-pi / 2, +pi / 2]

		if (theta < 0.0f)
		{
			theta += 2.f * Pi; // in [0, 2 * Pi]
		}
	}
	// Quadrant II or III (2, 3 사분면)
	else
	{
		theta = atanf(y / x) + Pi; // in [0, 2 * Pi).
	}

	return theta;
}

XMVECTOR MathHelper::RandUnitVec3()
{
	XMVECTOR One = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
	XMVECTOR Zero = XMVectorZero();

	// Keep trying until we get a point on/in the hemisphere.
	while (true)
	{
		// Generate random point in the cube [-1,1]^3.
		XMVECTOR v = XMVectorSet(MathHelper::RandF(-1.0f, 1.0f), MathHelper::RandF(-1.0f, 1.0f), MathHelper::RandF(-1.0f, 1.0f), 0.0f);

		// Ignore points outside the unit sphere in order to get an even distribution 
		// over the unit sphere.  Otherwise points will clump more on the sphere near 
		// the corners of the cube.

		if (XMVector3Greater(XMVector3LengthSq(v), One))
			continue;

		return XMVector3Normalize(v);
	}
}

XMVECTOR MathHelper::RandHemisphereUnitVec3(const XMVECTOR& n)
{
	const XMVECTOR One = XMVectorSet(1.f, 1.f, 1.f, 1.f);
	const XMVECTOR Zero = XMVectorZero();

	// Keep trying until we get a point on / in the hemisphere.
	while (true)
	{
		// Generate random point in the cube [-1, 1]^3.
		XMVECTOR v = XMVectorSet(
			MathHelper::RandF(-1.f, 1.f),
			MathHelper::RandF(-1.f, 1.f),
			MathHelper::RandF(-1.f, 1.f),
			0.0f);

		/*
			Ignore points outside the unit sphere in order to get an even distribution over the unit sphere.
			Otherwise points will clump more on the sphere near the corners of the cube.
		*/

		if (XMVector3Greater(XMVector3LengthSq(v), One))
		{
			continue;
		}

		// Ignore points in the bottom hemisphere.
		if (XMVector3Less(XMVector3Dot(n, v), Zero))
		{
			continue;
		}

		return XMVector3Normalize(v);
	}
}
