#pragma once

#define _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS 1
#include <hash_set>

//--------------------------------------------------------------------------------------
// Name: struct D3DXFRAME_DERIVED
// Desc: Structure derived from D3DXFRAME so we can add some app-specific
//       info that will be stored with each frame
//--------------------------------------------------------------------------------------
struct D3DXFRAME_DERIVED : public D3DXFRAME
{
    D3DXMATRIXA16 CombinedTransformationMatrix;
};

//--------------------------------------------------------------------------------------
// Name: struct D3DXMESHCONTAINER_DERIVED
// Desc: Structure derived from D3DXMESHCONTAINER so we can add some app-specific
//       info that will be stored with each mesh
//--------------------------------------------------------------------------------------
struct D3DXMESHCONTAINER_DERIVED : public D3DXMESHCONTAINER
{
    LPDIRECT3DTEXTURE9* ppTextures;       // array of textures, entries are NULL if no texture specified    

    // SkinMesh info
    LPD3DXMESH pOrigMesh;
    LPD3DXATTRIBUTERANGE pAttributeTable;
    DWORD NumAttributeGroups;
    DWORD NumInfl;
    LPD3DXBUFFER pBoneCombinationBuf;
    D3DXMATRIX** ppBoneMatrixPtrs;
    D3DXMATRIX* pBoneOffsetMatrices;
    DWORD NumPaletteEntries;
    bool UseSoftwareVP;
    DWORD iAttributeSW;     // used to denote the split between SW and HW if necessary for non-indexed skinning
};

//--------------------------------------------------------------------------------------
// Name: class CAllocateHierarchy
// Desc: Custom version of ID3DXAllocateHierarchy with custom methods to create
//       frames and meshcontainers.
//--------------------------------------------------------------------------------------
class CAllocateHierarchy : public ID3DXAllocateHierarchy
{
public:
    STDMETHOD( CreateFrame )( THIS_ LPCSTR Name, LPD3DXFRAME *ppNewFrame );
    STDMETHOD( CreateMeshContainer )( THIS_
        LPCSTR Name,
        CONST D3DXMESHDATA *pMeshData,
        CONST D3DXMATERIAL *pMaterials,
        CONST D3DXEFFECTINSTANCE *pEffectInstances,
        DWORD NumMaterials,
        CONST DWORD *pAdjacency,
        LPD3DXSKININFO pSkinInfo,
        LPD3DXMESHCONTAINER *ppNewMeshContainer );
    STDMETHOD( DestroyFrame )( THIS_ LPD3DXFRAME pFrameToFree );
    STDMETHOD( DestroyMeshContainer )( THIS_ LPD3DXMESHCONTAINER pMeshContainerBase );

    CAllocateHierarchy()
    {
    }
};

class AABB;

class SkinnedMesh
{
	LPD3DXFRAME                 g_pFrameRoot;
	ID3DXAnimationController*   g_pAnimController;

	D3DXVECTOR3                 g_vObjectCenter;        // Center of bounding sphere of object
	FLOAT                       g_fObjectRadius;        // Radius of bounding sphere of object

	static D3DXMATRIXA16*       g_pBoneMatrices;
	static UINT					g_NumBoneMatricesMax;

	int verticesCount;
	int facesCount;

	bool frameDirtyFlag;

	stdext::hash_set<int> maskedSubsets;

private:
	void DrawMeshContainer( IDirect3DDevice9* pd3dDevice, LPD3DXMESHCONTAINER pMeshContainerBase, LPD3DXFRAME pFrameBase);
	void DrawFrame( IDirect3DDevice9* pd3dDevice, LPD3DXFRAME pFrame);
	HRESULT SetupBoneMatrixPointersOnMesh( LPD3DXMESHCONTAINER pMeshContainer );
	HRESULT SetupBoneMatrixPointers( LPD3DXFRAME pFrame );
	void UpdateFrameMatrices( LPD3DXFRAME pFrameBase, LPD3DXMATRIX pParentMatrix );
	void ReleaseAttributeTable( LPD3DXFRAME pFrameBase );

public:
	static HRESULT GenerateSkinnedMesh( IDirect3DDevice9* pd3dDevice, D3DXMESHCONTAINER_DERIVED* pMeshContainer );
	static int CountVertices(LPD3DXFRAME pFrameBase, int * maxMeshVrtsCount);
	static int CountFaces(LPD3DXFRAME pFrameBase);

public:
	const D3DXVECTOR3 & GetCenter() const { return g_vObjectCenter; }
	float				GetRadius() const { return g_fObjectRadius; }

	int GetVerticesCount() const { return verticesCount; }
	int GetFacesCount() const { return facesCount; }

public:
	SkinnedMesh();
	~SkinnedMesh();

public:
	void Create(IDirect3DDevice9 *device, LPCTSTR filename, bool computeNormals);
	void RestoreDeviceObjects(IDirect3DDevice9* device);
	void InvalidateDeviceObjects();
	void Destroy();

public:
	void Update(double fTime, float fElapsedTime);
	void Render(IDirect3DDevice9 *device);

	// Vertex Buffer and its vertex declarations used in computing the AABB of a skinned mesh.
	D3DXVECTOR4 * transformedCoords;

private:
	void ComputeMeshAABB(const D3DXMATRIX & transform, D3DXMESHCONTAINER_DERIVED & meshContainer, AABB * aabb);
	void ComputeAABB(const D3DXMATRIX & transform, LPD3DXFRAME pFrameBase, AABB * aabb);

public:
	void ComputeTransformedAABB(const D3DXMATRIX & transform, AABB * aabb);

	void SetMaskedSubset(const stdext::hash_set<int> &masks);
};