#include "StdAfx.h"
#include "Voxelizer.h"
#include "VoxelizationListener.h"

using namespace std;

void Voxelizer::VoxelizationStarted(int slicesCount)
{
	set< VoxelizationListener * >::iterator itr;
	for (itr = voxelizationListeners.begin(); itr != voxelizationListeners.end(); ++itr)
		(*itr)->OnVoxelizationStarted(slicesCount);
}

void Voxelizer::VoxelizationEnded()
{
	set< VoxelizationListener * >::iterator itr;
	for (itr = voxelizationListeners.begin(); itr != voxelizationListeners.end(); ++itr)
		(*itr)->OnVoxelizationEnded();
}

void Voxelizer::VoxelizationNewSlice(int sliceIndex, int width, int height, int pitch, void* bits)
{
	set< VoxelizationListener * >::iterator itr;
	for (itr = voxelizationListeners.begin(); itr != voxelizationListeners.end(); ++itr)
		(*itr)->OnVoxelizationNewSlice(sliceIndex, width, height, pitch, bits);
}

Voxelizer::Voxelizer()
{
}

void Voxelizer::AddVoxelizationListener(VoxelizationListener *listener)
{
	voxelizationListeners.insert(listener);
}

void Voxelizer::RemoveVoxelizationListener(VoxelizationListener *listener)
{
	voxelizationListeners.erase(listener);
}