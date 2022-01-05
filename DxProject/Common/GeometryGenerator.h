#pragma once

#include "D3DUtil.h"

class GeometryGenerator
{
public:
	struct Vertex
	{
		Vertex(){}
		Vertex(const XMFLOAT3& P, const XMFLOAT3& N, const XMFLOAT3& T, const XMFLOAT2& UV)
			: Position(P), Normal(N), TangentU(T), Texcoord(UV) {}
		Vertex(const float Px, const float Py, const float Pz,
				const float Nx, const float Ny, const float Nz,
				const float Tx, const float Ty, const float Tz,
				const float U, const float V)
			: Position(Px, Py, Pz), Normal(Nx, Ny, Nz), TangentU(Tx, Ty, Tz), Texcoord(U, V) {}

		XMFLOAT3 Position;
		XMFLOAT3 Normal;
		XMFLOAT3 TangentU;
		XMFLOAT2 Texcoord;
	};

	struct MeshData
	{
		std::vector<Vertex> Vertices;
		std::vector<UINT> Indices;
	};

	///<summary>
	/// Creates an mxn grid in the xz-plane with m rows and n columns, centered
	/// at the origin with the specified width and depth.
	///</summary>
	static void CreateGrid(float width, float depth, UINT m, UINT n, MeshData& meshData);
};
