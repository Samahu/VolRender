#include "StdAfx.h"
#include "VoxelizationNode.h"
#include "TransferFunction.h"

extern const D3DFORMAT g_VolumeFormat;

VoxelizationNode::VoxelizationNode(const Vector3I& resolution_, D3DFORMAT textureFormat_)
: aabb(), resolution(resolution_), textureFormat(textureFormat_), volume(NULL), transferFunctionTexture(NULL), volumeLocked(false),
onVoxelsCount(0), renderCallback(NULL)
{
}

VoxelizationNode::~VoxelizationNode()
{
}

D3DXVECTOR3 VoxelizationNode::GetVoxelSize() const
{
	D3DXVECTOR3 extents = aabb.GetExtents();
	return D3DXVECTOR3(extents.x / resolution.x, extents.y / resolution.y, extents.z / resolution.z);
}

D3DXVECTOR3 VoxelizationNode::GetDensityPerUnit() const
{
	D3DXVECTOR3 extents = aabb.GetExtents();
	return D3DXVECTOR3(resolution.x / extents.x, resolution.y / extents.y, resolution.z / extents.z);
}

void VoxelizationNode::OnCreateDevice(IDirect3DDevice9 *device)
{
	HRESULT hr;

//#pragma message("Hack 2")
//	int res_z = (resolution.z > 128 ? 128 : resolution.z);

	V( device->CreateVolumeTexture(resolution.x, resolution.y, resolution.z, 1, D3DUSAGE_DYNAMIC, g_VolumeFormat, D3DPOOL_DEFAULT, &volume, NULL) );

	// copy transfer array into the texture1D.
	V( device->CreateTexture(256, 1, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &transferFunctionTexture, NULL) );
}

void VoxelizationNode::OnDestroyDevice()
{
	SAFE_RELEASE(transferFunctionTexture);
	SAFE_RELEASE(volume);
}

VoxelizationNode * VoxelizationNode::CreateFromRAWFile(IDirect3DDevice9* device, LPCTSTR fileName, const Vector3I& resolution, D3DFORMAT textureFormat)
{
	HRESULT hr;
	FILE* file = NULL;

	if (0 != _wfopen_s(&file, fileName, L"rb") )
	{
		V( E_FAIL );
		return NULL;
	}

	// Read the file into a raw buffer.
	size_t vSize = resolution.x * resolution.y * resolution.z;
	byte* buffer = new byte[vSize];
	fread_s(buffer, vSize, 1, vSize, file);
	fclose(file);

	VoxelizationNode * node = new VoxelizationNode(resolution, textureFormat);
	node->OnCreateDevice(device);
	node->OnResetDevice(device);

	IDirect3DVolume9* volume = NULL;	

	// write the raw buffer into the volume buffer.
	V( node->volume->GetVolumeLevel(0, &volume) );
	if (volume)
	{
		D3DLOCKED_BOX lockedBOX;

		V( volume->LockBox(&lockedBOX, NULL, D3DLOCK_DISCARD) );
		float *ptr = (float*)lockedBOX.pBits;

		for (size_t i = 0; i < vSize; ++i)
			ptr[i] = (float)buffer[i] / UCHAR_MAX;

		volume->UnlockBox();
		volume->Release();
	}

	delete buffer;

	return node;
}

VoxelizationNode * VoxelizationNode::CreateFromDDSFile(IDirect3DDevice9* device, LPCTSTR fileName)
{
	D3DXIMAGE_INFO imageInfo;
	IDirect3DVolumeTexture9 * volumeTexture;

	D3DXCreateVolumeTextureFromFileEx(
		device, fileName, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 1, 0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, &imageInfo, NULL, &volumeTexture);

	Vector3I resolution(imageInfo.Width, imageInfo.Height, imageInfo.Depth);
	VoxelizationNode * node = new VoxelizationNode(resolution, imageInfo.Format);

	node->volume = volumeTexture;

	return node;
}

void VoxelizationNode::SaveVolume(LPCTSTR fileName)
{
	HRESULT hr;

	IDirect3DVolume9* level = NULL;
	V( volume->GetVolumeLevel(0, &level) );
	if (level)
	{
		D3DXSaveVolumeToFile(fileName, D3DXIFF_DDS, level, NULL, NULL);
		level->Release();
	}
}

void VoxelizationNode::LockVolume(bool discard, bool readonly)
{
	assert(!(discard &&readonly));	// both values can't be true

	HRESULT hr;

	DWORD flags = 0;

	if (discard)
		flags |= D3DLOCK_DISCARD;

	if (readonly)
		flags |= D3DLOCK_READONLY;

	assert(!volumeLocked);
	V( volume->LockBox(0, &volumeLockedBOX, NULL, flags) );
	volumeLocked = true;
}

void VoxelizationNode::UnlockVolume()
{
	HRESULT hr;

	assert(volumeLocked);
	V( volume->UnlockBox(0) );
	volumeLocked = false;
}

void VoxelizationNode::InsertSlice(int sliceIndex, int width, int height, int pitch, void *bits)
{
	assert(volumeLocked);

//#pragma message("Hack 2")
//	sliceIndex = (sliceIndex < 128 ? sliceIndex : 127);

	BYTE *dest = (BYTE*)volumeLockedBOX.pBits + sliceIndex * volumeLockedBOX.SlicePitch;
	memcpy_s(dest, volumeLockedBOX.SlicePitch, bits, height * pitch);
}

int VoxelizationNode::CountOnVoxels()
{
	int count = 0;

	LockVolume(false, true);

	BYTE* volume = (BYTE*)volumeLockedBOX.pBits;
	int bytesPerPixel = volumeLockedBOX.RowPitch / resolution.x;

	for (int z = 0; z < resolution.z; ++z)
	{
		BYTE *slice = &volume[z * volumeLockedBOX.SlicePitch];

		for (int y = 0; y < resolution.y; ++y)
		{
			BYTE *row = &slice[y * volumeLockedBOX.RowPitch];

			for (int x = 0; x < resolution.x; ++x)
			{
				BYTE value = row[x * bytesPerPixel];

				if (value > 0)
					++count;
			}
		}
	}

	UnlockVolume();

	onVoxelsCount = count;
	return count;
}

void VoxelizationNode::MergeVolume(VoxelizationNode * node)
{
	assert(this->resolution == node->resolution);

	this->LockVolume(false, false);
	node->LockVolume(false, true);

	// Make sure volumes formats match.
	assert(this->volumeLockedBOX.RowPitch == node->volumeLockedBOX.RowPitch &&
		node->volumeLockedBOX.SlicePitch == this->volumeLockedBOX.SlicePitch);

	int bytesPerPixel = volumeLockedBOX.RowPitch / resolution.x;

	BYTE* dstVolume = (BYTE*)this->volumeLockedBOX.pBits;
	BYTE* srcVolume = (BYTE*)node->volumeLockedBOX.pBits;

	for (int z = 0; z < resolution.z; ++z)
	{
		BYTE *dstSlice = &dstVolume[z * volumeLockedBOX.SlicePitch];
		BYTE *srcSlice = &srcVolume[z * volumeLockedBOX.SlicePitch];

		for (int y = 0; y < resolution.y; ++y)
		{
			BYTE *dstRow = &dstSlice[y * volumeLockedBOX.RowPitch];
			BYTE *srcRow = &srcSlice[y * volumeLockedBOX.RowPitch];

			for (int x = 0; x < resolution.x; ++x)
			{
				BYTE dstValue = dstRow[x * bytesPerPixel];
				BYTE srcValue = srcRow[x * bytesPerPixel];
				dstValue = max(srcValue, dstValue);
				dstRow[x * bytesPerPixel + 0] = dstValue;
				dstRow[x * bytesPerPixel + 1] = dstValue;
			}
		}
	}

	this->UnlockVolume();
	node->UnlockVolume();
}

void VoxelizationNode::SetTransferFunction(const std::vector< DWORD > &transferValues)
{
	assert(transferValues.size() == 256);

	D3DLOCKED_RECT rc;
	transferFunctionTexture->LockRect(0, &rc, NULL, D3DLOCK_DISCARD);
	memcpy_s(rc.pBits, 256 * sizeof(DWORD), &transferValues[0], 256 * sizeof(DWORD));
	transferFunctionTexture->UnlockRect(0);
}