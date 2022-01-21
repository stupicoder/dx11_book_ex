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
	/// Creates a box centered at the origin with the given dimensions.
	///</summary>
	static void CreateBox(float width, float height, float depth, MeshData& meshData);

	///<summary>
	/// Creates a sphere centered at the origin with the given radius.  The
	/// slices and stacks parameters control the degree of tessellation.
	///</summary>
	static void CreateSphere(float radius, UINT sliceCount, UINT stackCount, MeshData& meshData);

	///<summary>
	/// Creates a geosphere centered at the origin with the given radius.  The
	/// depth controls the level of tessellation.
	///</summary>
	static void CreateGeosphere(float radius, UINT numSubdivisions, MeshData& meshData);

	///<summary>
	/// Creates a cylinder parallel to the y-axis, and centered about the origin.  
	/// The bottom and top radius can vary to form various cone shapes rather than true
	// cylinders.  The slices and stacks parameters control the degree of tessellation.
	///</summary>
	static void CreateCylinder(float bottomRadius, float topRadius, float height, UINT sliceCount, UINT stackCount, MeshData& meshData);

	///<summary>
	/// Creates an mxn grid in the xz-plane with m rows and n columns, centered
	/// at the origin with the specified width and depth.
	///</summary>
	static void CreateGrid(float width, float depth, UINT m, UINT n, MeshData& meshData);

private:
	static void Subdivide(MeshData& meshData);
	static void BuildCylinderTopCap(float bottomRadius, float topRadius, float height, UINT sliceCount, UINT stackCount, MeshData& meshData);
	static void BuildCylinderBottomCap(float bottomRadius, float topRadius, float height, UINT sliceCount, UINT stackCount, MeshData& meshData);
};
