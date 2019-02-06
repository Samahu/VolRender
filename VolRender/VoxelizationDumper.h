#pragma once

#include "VoxelizationListener.h"

interface VoxelizationDumper : public VoxelizationListener
{
	/*override*/ void OnVoxelizationStarted(int slicesCount) {};
	/*override*/ void OnVoxelizationEnded() {};
	/*override*/ void OnVoxelizationNewSlice(int sliceIndex, int width, int height, int pitch, void* bits);
};