#pragma once

#include "VoxelizationNode.h"

class DepthVoxelizationNode : public VoxelizationNode
{
	friend class DepthVoxelizer;

private:
	IDirect3DSurface9* surfaceRenderTargetVid;			// the d3d surface of the Render Target Texture
	IDirect3DSurface9* surfaceRenderTargetSys;			// a d3d surface located on system memory

	// TODO: Surface Rendering Stuff
	IDirect3DSurface9* surfaceRenderTargetSysForSurfaceVoxelization;	// a d3d surface located on system memory

public:
	// Constructor
	DepthVoxelizationNode(const Vector3I& resolution, D3DFORMAT textureFormat);

	// Implementation of D3DObject interface
public:
	/*override*/ void OnCreateDevice(IDirect3DDevice9* device);
	/*override*/ void OnDestroyDevice();
};