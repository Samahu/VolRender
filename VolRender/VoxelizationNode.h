#pragma once

#include <string>
#include <vector>

#include "D3DObject.h"
#include "Vector3I.h"
#include "AABB.h"

typedef void (CALLBACK *RENDER_CALLBACK)(IDirect3DDevice9* device, double time, float elapsedTime, void* userContext);

class VoxelizationNode : public D3DObject
{
protected:
	AABB aabb;
	Vector3I resolution;
	bool solid;
	std::wstring name;
	D3DFORMAT textureFormat;
	IDirect3DVolumeTexture9 *volume;	// volume texture
	IDirect3DTexture9 *transferFunctionTexture;

	RENDER_CALLBACK renderCallback;

	bool volumeLocked;
	D3DLOCKED_BOX volumeLockedBOX;

	int onVoxelsCount;

public:
	VoxelizationNode(const Vector3I& resolution, D3DFORMAT textureFormat);
	virtual ~VoxelizationNode();

public:
	const AABB & GetAABB() const { return aabb; }
	void SetAABB(const AABB & value) { aabb = value; }

	const Vector3I & GetResolution() const { return resolution; }

	bool IsSolid() const { return solid; }
	void SetSolid(bool value) { solid = value; }

	const std::wstring & GetName() const { return name; }
	void SetName(const std::wstring& value) { name = value; }

	IDirect3DVolumeTexture9 * GetVolume() { return volume; }

	IDirect3DTexture9 * GetTransferFunctionTexture() { return transferFunctionTexture; }

	D3DXVECTOR3 GetVoxelSize() const;
	D3DXVECTOR3 GetDensityPerUnit() const;

	void SetRenderCallback(RENDER_CALLBACK value) { renderCallback = value; }
	RENDER_CALLBACK GetRenderCallback() { return renderCallback; }

	int GetOnVoxelsCount() const { return onVoxelsCount; }

public:
	static VoxelizationNode * CreateFromRAWFile(IDirect3DDevice9* device, LPCTSTR fileName, const Vector3I& resolution, D3DFORMAT textureFormat);
	static VoxelizationNode * CreateFromDDSFile(IDirect3DDevice9* device, LPCTSTR fileName);

	// Implementation of the D3DObject interface
public:
	/*override*/ void OnCreateDevice(IDirect3DDevice9* device);
	/*override*/ void OnResetDevice(IDirect3DDevice9* device) {}
	/*override*/ void OnLostDevice() {}
	/*override*/ void OnDestroyDevice();

	void SaveVolume(LPCTSTR fileName);

	// Remarks: setting both parameters to to true is invalid.
	void LockVolume(bool discard = true, bool readonly = false);
	void UnlockVolume();
	void InsertSlice(int sliceIndex, int width, int height, int pitch, void *bits);

	int CountOnVoxels();

	void MergeVolume(/*const*/ VoxelizationNode * node);

	void SetTransferFunction(const std::vector< DWORD > &transferValues);
};