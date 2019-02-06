#pragma once

class VoxelizationListener
{
public:
	virtual ~VoxelizationListener() {}

	virtual void OnVoxelizationStarted(int slicesCount) = 0;
	virtual void OnVoxelizationEnded() = 0;
	virtual void OnVoxelizationNewSlice(int sliceIndex, int width, int height, int pitch, void* bits) = 0;
};