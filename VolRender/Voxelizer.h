#pragma once

#include <set>
#include <vector>
#include "Vector3I.h"

class VoxelizationListener;
class VoxelizationNode;

class Voxelizer
{
private:
	std::set< VoxelizationListener * > voxelizationListeners;

protected:
	void VoxelizationStarted(int slicesCount);
	void VoxelizationEnded();
	void VoxelizationNewSlice(int sliceIndex, int width, int height, int pitch, void* bits);

public:
	// Constructor
	Voxelizer();

	// Destructor
	virtual ~Voxelizer() {}

public:
	void AddVoxelizationListener(VoxelizationListener *listener);

	void RemoveVoxelizationListener(VoxelizationListener *listener);

public:
	virtual VoxelizationNode * CreateVoxelizationNode(const Vector3I & resolution, D3DFORMAT bufferFormat) = 0;

	virtual void Voxelize(IDirect3DDevice9* device, const D3DXMATRIX& world, std::vector< VoxelizationNode * > & voxelizationNodes) = 0;
};