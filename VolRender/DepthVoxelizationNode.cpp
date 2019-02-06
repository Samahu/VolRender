#include "StdAfx.h"
#include "DepthVoxelizationNode.h"

DepthVoxelizationNode::DepthVoxelizationNode(const Vector3I& resolution_, D3DFORMAT textureFormat_)
: VoxelizationNode(resolution_, textureFormat_),
surfaceRenderTargetVid(NULL), surfaceRenderTargetSys(NULL),
surfaceRenderTargetSysForSurfaceVoxelization(NULL)
{
}

void DepthVoxelizationNode::OnCreateDevice(IDirect3DDevice9* device)
{
	VoxelizationNode::OnCreateDevice(device);

	HRESULT hr;

	IDirect3DSurface9* origTarget = NULL;
	D3DSURFACE_DESC desc;
	device->GetRenderTarget(0, &origTarget);
	origTarget->GetDesc(&desc);
	origTarget->Release();

	V( device->CreateRenderTarget(resolution.x, resolution.y, textureFormat, desc.MultiSampleType, desc.MultiSampleQuality, FALSE, &surfaceRenderTargetVid, NULL) );

	// Create a lockable system memory surface to copy render target to it.
	V( device->CreateOffscreenPlainSurface(resolution.x, resolution.y, textureFormat, D3DPOOL_SYSTEMMEM, &surfaceRenderTargetSys, NULL) );

	V( device->CreateOffscreenPlainSurface(resolution.x, resolution.y, textureFormat, D3DPOOL_SYSTEMMEM, &surfaceRenderTargetSysForSurfaceVoxelization, NULL) );
}

void DepthVoxelizationNode::OnDestroyDevice()
{
	SAFE_RELEASE(surfaceRenderTargetSysForSurfaceVoxelization);

	SAFE_RELEASE(surfaceRenderTargetSys);
	SAFE_RELEASE(surfaceRenderTargetVid);

	VoxelizationNode::OnDestroyDevice();
}