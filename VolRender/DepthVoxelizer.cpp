#include "StdAfx.h"
#include "DepthVoxelizer.h"
#include "ShaderFlags.h"
#include "SDKmesh.h"
#include "VolumeRenderer.h"
#include "AABB.h"
#include "VoxelizationNode.h"
#include "DepthVoxelizationNode.h"

using namespace std;

const D3DVERTEXELEMENT9 DepthVoxelizer::POST_PROC_VERTEX::Decl[2] =
{
	{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0 },
	D3DDECL_END()
};

const D3DVERTEXELEMENT9 DepthVoxelizer::POST_PROC_ED_VERTEX::Decl[3] =
{
	{ 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0 },
	{ 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
	D3DDECL_END()
};

DepthVoxelizer::DepthVoxelizer()
: effect(NULL),
techVoxelize(NULL), techDumpStencil(NULL), paramWorldViewProj(NULL)
{
}

void DepthVoxelizer::OnCreateDevice(IDirect3DDevice9 *device)
{
	HRESULT hr;

	// Initialize the vertex declaration
	V( device->CreateVertexDeclaration(POST_PROC_VERTEX::Decl, &postProcVertexDecl) );

	V( device->CreateVertexDeclaration(POST_PROC_ED_VERTEX::Decl, &postProcEDVertexDecl) );

	ID3DXBuffer *buffer = NULL;
	// Read the D3DX effect file
	V( D3DXCreateEffectFromFile(device, L"Voxelization.fxo", NULL, NULL, g_dwShaderFlags, NULL, &effect, &buffer) );
	if (NULL !=buffer)
	{
		LPCSTR str = (LPCSTR)buffer->GetBufferPointer();
		UNREFERENCED_PARAMETER(str);
		buffer->Release();
	}

	techVoxelize = effect->GetTechniqueByName("Voxelize");
	techDumpStencil = effect->GetTechniqueByName("DumpStencil");
    paramWorldViewProj = effect->GetParameterByName(NULL, "g_mWorldViewProjection");
}

void DepthVoxelizer::OnResetDevice(IDirect3DDevice9* device)
{
	HRESULT hr;
	V( effect->OnResetDevice() );
}

void DepthVoxelizer::OnLostDevice()
{
	HRESULT hr;
	V( effect->OnLostDevice() );
}

void DepthVoxelizer::OnDestroyDevice()
{
	SAFE_RELEASE(postProcVertexDecl);
	SAFE_RELEASE(postProcEDVertexDecl);
	SAFE_RELEASE(effect);
}

VoxelizationNode * DepthVoxelizer::CreateVoxelizationNode(const Vector3I & resolution, D3DFORMAT bufferFormat)
{
	return new DepthVoxelizationNode(resolution, bufferFormat);
}

/*
Algorithm 3.1: Solid Slice Creating
CreateSolidSlice()
Input:
L ← a display list of a closed surface-based geometric object
z ← the z position of the solid slice
Nx, Ny ← the size of the solid slice
fs ← a specified solid slice function
Output:
S← the solid slice
Begin
1 Clear the color, stencil, and depth buffers
2 Set the depth buffer using the slice position with the size of Nx by Ny
3 Call the display list L to draw the geometry object
4 Check surface parity
5 Apply the solid slice function fs to the slice in the color buffer
6 Use stencil mask to mask out the solid slice
6.1 If stencil buffer value is 1 then
Corresponding portion of ith slice is updated by slice function in color buffer
6.2 Else cleared to background color
End
7 Get final voxelized solid slice S from the framebuffer
8 Output the entire voxelized solid slice S
End
*/

void DepthVoxelizer::CreateSlice(IDirect3DDevice9 *device, RENDER_CALLBACK renderCallback, int width, int height, float z)
{
	HRESULT hr;
	
	D3DCOLOR color = D3DCOLOR_ARGB(0, 0, 0, 0);	//alpha [0 .. 255] = [fully transparent .. fully opaque]
	device->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, color, z, 0);

	device->BeginScene();

	V( effect->SetTechnique(techVoxelize) );

	UINT passes;
	V( effect->Begin(&passes, 0) );
	for (UINT i = 0; i < passes; ++i)
	{
		V( effect->BeginPass(i) );
		if (renderCallback)
			renderCallback(device, 0.0f, 0.0f, NULL);
		V( effect->EndPass() );
	}
	V( effect->End() );

	POST_PROC_VERTEX quad[4] =
	{
		{ 0.0f,			0.0f,			0.5f, 1.0f },
		{ (float)width,	0.0f,			0.5f, 1.0f },
		{ 0.0f,			(float)height,	0.5f, 1.0f },
		{ (float)width,	(float)height,	0.5f, 1.0f }
	};

	device->SetVertexDeclaration(postProcVertexDecl);
	effect->SetTechnique(techDumpStencil);
	V( effect->Begin(&passes, 0) );
	for (UINT i = 0; i < passes; ++i)
	{
		V( effect->BeginPass(i) );
		device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(POST_PROC_VERTEX));
		V( effect->EndPass() );
	}
	V( effect->End() );

	device->EndScene();

	// 7 Get final voxelized solid slice S from the framebuffer
}

/*
Algorithm 3.2: Real-time Solid Voxelization
SolidVoxelization()
Input:
O ← a closed surface-based object
Nx, Ny, Nz ← size of the output volume
Bmin, Bmax ← minimum and maximum 3D vectors (x, y, z) of the bounding box
fs ← a specified solid slice function
Output:
V← the voxelized solid volume
Begin
1 Set the viewport (0, 0, Nx, Ny)
2 Set orthogonal projection
3 Set background color to be (0, 0, 0, 0)
4 Do transformation for the object, O, to make its orientation specified for
voxelization consistent with Z axis. Generate the display list L for the geometric
object O drawing.
5 Initialize or preprocess the slice function fs
6 For each slice i = 0 to Nz – 1 do
6.1 If slice i insides the bounding box (Bmin, Bmax) then
6.1.1 ith solidSlice ← CreateSolidSlice(L, z(i), fs)
6.1.2 Insert ith solidSlice into the 3D texture volume V
End
End
7 Output the entire volume V
End
*/

void DepthVoxelizer::Voxelize(IDirect3DDevice9* device, const D3DXMATRIX& world, std::vector< VoxelizationNode * > & voxelizationNodes)
{
	assert(device);

	HRESULT hr;
	IDirect3DSurface9 *oldRenderTarget;

	V( device->GetRenderTarget(0, &oldRenderTarget) );

	D3DXMATRIXA16 view;
    D3DXMATRIXA16 proj;
    D3DXMATRIXA16 wvp;
	D3DLOCKED_RECT lockedRC;
	D3DXVECTOR3 c, e;
	D3DXVECTOR3 eye, at, up;

	for (size_t i = 0; i < voxelizationNodes.size(); ++i)
	{
		DepthVoxelizationNode * node = dynamic_cast< DepthVoxelizationNode * >(voxelizationNodes[i]);

		V( device->SetRenderTarget(0, node->surfaceRenderTargetVid) );	// render-to-surface

		const AABB& aabb = node->GetAABB();
		const Vector3I & r = node->GetResolution();

		c = aabb.GetCenter();
		e = aabb.GetExtents();

		eye.x = c.x, eye.y = c.y, eye.z = aabb.GetMin().z;
		at.x = c.x, at.y = c.y, at.z = c.z;					// look to the center of the object
		up.x = 0.0f, up.y = 1.0f, up.z = 0.0f;

		D3DXMatrixLookAtLH(&view, &eye, &at, &up);

		// create an orthogonal the proj matrix
		D3DXMatrixOrthoLH(&proj, e.x, e.y, 0.0f, e.z);

		// Calc the world x view x proj matrix
		wvp = world * view * proj;

		// Update the effect.
		V( effect->SetMatrix(paramWorldViewProj, &wvp) );

		node->LockVolume(true, false);

		for (int z = 0; z < r.z; ++z)
		{
			// 6.1 If slice i insides the bounding box (Bmin, Bmax) then
			// 6.1.1 ith solidSlice ← CreateSolidSlice(L, z(i), fs)
			CreateSlice(device, node->GetRenderCallback(), r.x, r.y, (float)(z + 1) / (float)r.z);

			// 6.1.2 Insert ith solidSlice into the 3D texture volume V
			V( device->GetRenderTargetData(node->surfaceRenderTargetVid, node->surfaceRenderTargetSys) );

			V( node->surfaceRenderTargetSys->LockRect(&lockedRC, NULL, D3DLOCK_READONLY) );

			if (!node->IsSolid())
			{
				D3DLOCKED_RECT dstLockedRC;
				EdgeDetection(node, r.x, r.y, &lockedRC, &dstLockedRC);
				lockedRC = dstLockedRC;
			}

			node->InsertSlice(z, r.x, r.y, lockedRC.Pitch, lockedRC.pBits);

			V( node->surfaceRenderTargetSys->UnlockRect() );
		}

		node->UnlockVolume();
	}

	// Restore old buffer
	V( device->SetRenderTarget(0, oldRenderTarget) );
	oldRenderTarget->Release();
}

bool DepthVoxelizer::IsBorderPixel(int width, int height, int pitch, BYTE* src, int row, int col, int thickness)
{
	int bytesPerPixel = pitch / width;

	int index = row * width + col;

	if (0 == src[bytesPerPixel * index])
		return false;

	for (int i = -thickness; i <= thickness; ++i)
		for (int j = -thickness; j <= thickness; ++j)
		{
			if (0 == i && 0 == j)
				continue;

			if (row + i < 0 || row + i >= height || col + j < 0 || col + j >= width)
				continue;

			index = (row + i) * width + (col + j);
			if (0 == src[bytesPerPixel * index])
				return true;
		}

	return false;
}

void DepthVoxelizer::EdgeDetection(DepthVoxelizationNode * node, int width, int height, D3DLOCKED_RECT * srcLockedRC, D3DLOCKED_RECT* destLockedRC)
{
	HRESULT hr;

	V( node->surfaceRenderTargetSysForSurfaceVoxelization->LockRect(destLockedRC, NULL, D3DLOCK_DISCARD) );	// Lock slice for read/write

	int bytesPerPixel = destLockedRC->Pitch / width;
	BYTE *src = (BYTE*)srcLockedRC->pBits;
	BYTE *dst = (BYTE*)destLockedRC->pBits;
	int index;
	register int row, col;
	for (row = 0; row < height; ++row)
		for (col = 0; col < width; ++col)
		{
			index = row * width + col;
			BYTE value = IsBorderPixel(width, height, srcLockedRC->Pitch, src, row, col, 1) ? 255 : 0;
			dst[bytesPerPixel * index + 0] = value;
			dst[bytesPerPixel * index + 1] = value;
		}

	V( node->surfaceRenderTargetSysForSurfaceVoxelization->UnlockRect() );
}