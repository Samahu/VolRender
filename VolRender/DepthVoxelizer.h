#pragma once

#include "Voxelizer.h"
#include "VoxelizationNode.h"

class AABB;
class DepthVoxelizationNode;

class DepthVoxelizer : public Voxelizer
{
	struct POST_PROC_VERTEX
	{
		float x, y, z;
		float rhw;

		const static D3DVERTEXELEMENT9 Decl[2];
	};

	struct POST_PROC_ED_VERTEX
	{
		float x, y, z;
		float rhw;

		float u, v;

		const static D3DVERTEXELEMENT9 Decl[3];
	};

private:
	IDirect3DVertexDeclaration9* postProcVertexDecl;	// Vertex declaration for post-process quad rendering
	IDirect3DVertexDeclaration9* postProcEDVertexDecl;	// Vertex declaration for post-process edge detection rendering
	ID3DXEffect* effect;
	D3DXHANDLE techVoxelize;
	D3DXHANDLE techDumpStencil;
	D3DXHANDLE paramWorldViewProj;

private:
	// param width: slice width.
	// param height: slice height
	void CreateSlice(IDirect3DDevice9* device, RENDER_CALLBACK renderCallback, int width, int height, float z);

	bool IsBorderPixel(int width, int height, int pitch, BYTE* src, int row, int col, int thicknesss); 
	void EdgeDetection(DepthVoxelizationNode * node, int width, int height, D3DLOCKED_RECT * srcLockedRC, D3DLOCKED_RECT* destLockedRC);


public:
	// Constructor
	DepthVoxelizer();

	// Implementation of the D3DObject interface
public:
	void OnCreateDevice(IDirect3DDevice9* device);
	void OnResetDevice(IDirect3DDevice9* device);
	void OnLostDevice();
	void OnDestroyDevice();

public:
	/*override*/ VoxelizationNode * CreateVoxelizationNode(const Vector3I & resolution, D3DFORMAT bufferFormat);

	/*override*/ void Voxelize(IDirect3DDevice9* device, const D3DXMATRIX& world, std::vector< VoxelizationNode * > & voxelizationNodes);
};