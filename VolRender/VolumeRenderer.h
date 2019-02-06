#pragma once

#include <SDKmesh.h>
#include <vector>

class VoxelizationNode;
class AABB;

class VolumeRenderer
{
	struct VoxelizationNodeHolder
	{
		VoxelizationNode * node;

		IDirect3DTexture9* textureFrontRT;
		IDirect3DTexture9* textureBackRT;
		bool visible;

		VoxelizationNodeHolder() : node(NULL), textureFrontRT(NULL), textureBackRT(NULL), visible(true) { }
	};

private:
	CDXUTXFileMesh uniformBox;
	ID3DXEffect* effect;
	D3DXHANDLE techRenderPosition;
	D3DXHANDLE techRayCast;
	D3DXHANDLE paramWorldViewProj;
	D3DXHANDLE paramFrontTexture;
	D3DXHANDLE paramBackTexture;
	D3DXHANDLE paramVolumeTexture;
	D3DXHANDLE paramTransferFunctionTexture;
	D3DXHANDLE paramIterations;
	D3DXHANDLE paramResolution;
	D3DXHANDLE paramBoxMin;
	D3DXHANDLE paramBoxMax;

	int voxelSkipping;

	std::vector< VoxelizationNodeHolder > voxelizationNodes;

private:
	void RenderUnifromBoxWithEffect(IDirect3DDevice9* device);
	void update(IDirect3DDevice9* device, const D3DXMATRIX& wvp, IDirect3DTexture9 * frontRT, IDirect3DTexture9 * backRT);

public:
	void SetVoxelSkipping(int value) { voxelSkipping = value; }

public:
	// Constructor
	VolumeRenderer();
	~VolumeRenderer();

	void OnCreateDevice(IDirect3DDevice9* device, int backBufferWidth, int backBufferHeight);
	void OnResetDevice(IDirect3DDevice9 *device);
	void OnLostDevice();
	void OnDestroyDevice();

	void Update(IDirect3DDevice9* device, const D3DXMATRIX& view, const D3DXMATRIX& proj);
	void Render(IDirect3DDevice9* device, const D3DXMATRIX& view, const D3DXMATRIX& proj);

	void SaveFrontBackTextures(IDirect3DTexture9 * textureFrontRT, IDirect3DTexture9 * textureBackRT);

public:
	void AddVoxelizationNode(VoxelizationNode * voxelizationNode);
	int GetVoxelizationNodesCount() const;
	VoxelizationNode * GetVoxelizationNode(int idx);
	void HideVoxelizationNode(int idx);

	void GetVoxelizationNodes(std::vector< VoxelizationNode * > & nodes);
};