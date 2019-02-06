#include "StdAfx.h"
#include "SkinnedMesh.h"
#include "AABB.h"

//--------------------------------------------------------------------------------------
// Name: AllocateName()
// Desc: Allocates memory for a string to hold the name of a frame or mesh
//--------------------------------------------------------------------------------------
HRESULT AllocateName( LPCSTR Name, LPSTR* pNewName )
{
    UINT cbLength;

    if( Name != NULL )
    {
        cbLength = ( UINT )strlen( Name ) + 1;
        *pNewName = new CHAR[cbLength];
        if( *pNewName == NULL )
            return E_OUTOFMEMORY;
        memcpy( *pNewName, Name, cbLength * sizeof( CHAR ) );
    }
    else
    {
        *pNewName = NULL;
    }

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: CAllocateHierarchy::CreateFrame()
// Desc: 
//--------------------------------------------------------------------------------------
HRESULT CAllocateHierarchy::CreateFrame( LPCSTR Name, LPD3DXFRAME* ppNewFrame )
{
    HRESULT hr = S_OK;
    D3DXFRAME_DERIVED* pFrame;

    *ppNewFrame = NULL;

    pFrame = new D3DXFRAME_DERIVED;
    if( pFrame == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto e_Exit;
    }

    hr = AllocateName( Name, &pFrame->Name );
    if( FAILED( hr ) )
        goto e_Exit;

    // initialize other data members of the frame
    D3DXMatrixIdentity( &pFrame->TransformationMatrix );
    D3DXMatrixIdentity( &pFrame->CombinedTransformationMatrix );

    pFrame->pMeshContainer = NULL;
    pFrame->pFrameSibling = NULL;
    pFrame->pFrameFirstChild = NULL;

    *ppNewFrame = pFrame;
    pFrame = NULL;

e_Exit:
    delete pFrame;
    return hr;
}

//--------------------------------------------------------------------------------------
// Name: CAllocateHierarchy::CreateMeshContainer()
// Desc: 
//--------------------------------------------------------------------------------------
HRESULT CAllocateHierarchy::CreateMeshContainer(
    LPCSTR Name,
    CONST D3DXMESHDATA *pMeshData,
    CONST D3DXMATERIAL *pMaterials,
    CONST D3DXEFFECTINSTANCE *pEffectInstances,
    DWORD NumMaterials,
    CONST DWORD *pAdjacency,
    LPD3DXSKININFO pSkinInfo,
    LPD3DXMESHCONTAINER *ppNewMeshContainer )
 {
    HRESULT hr;
    D3DXMESHCONTAINER_DERIVED *pMeshContainer = NULL;
    UINT NumFaces;
    UINT iMaterial;
    UINT iBone, cBones;
    LPDIRECT3DDEVICE9 pd3dDevice = NULL;

    LPD3DXMESH pMesh = NULL;

    *ppNewMeshContainer = NULL;

    // this sample does not handle patch meshes, so fail when one is found
    if( pMeshData->Type != D3DXMESHTYPE_MESH )
	{
        hr = E_FAIL;
        goto e_Exit;
    }

    // get the pMesh interface pointer out of the mesh data structure
    pMesh = pMeshData->pMesh;

    // this sample does not FVF compatible meshes, so fail when one is found
    if( pMesh->GetFVF() == 0 )
	{
        hr = E_FAIL;
        goto e_Exit;
    }

    // allocate the overloaded structure to return as a D3DXMESHCONTAINER
    pMeshContainer = new D3DXMESHCONTAINER_DERIVED;
    if( pMeshContainer == NULL )
	{
        hr = E_OUTOFMEMORY;
        goto e_Exit;
    }
    memset( pMeshContainer, 0, sizeof( D3DXMESHCONTAINER_DERIVED ) );

    // make sure and copy the name.  All memory as input belongs to caller, interfaces can be addref'd though
    hr = AllocateName( Name, &pMeshContainer->Name );
    if( FAILED( hr ) )
        goto e_Exit;

    pMesh->GetDevice( &pd3dDevice );
    NumFaces = pMesh->GetNumFaces();

    // if no normals are in the mesh, add them
    if( !( pMesh->GetFVF() & D3DFVF_NORMAL ) )
	{
        pMeshContainer->MeshData.Type = D3DXMESHTYPE_MESH;

		// clone the mesh to make room for the normals
        hr = pMesh->CloneMeshFVF( pMesh->GetOptions(),
                                    pMesh->GetFVF() | D3DFVF_NORMAL,
                                    pd3dDevice, &pMeshContainer->MeshData.pMesh );
        if( FAILED( hr ) )
            goto e_Exit;

		// get the new pMesh pointer back out of the mesh container to use
		// NOTE: we do not release pMesh because we do not have a reference to it yet
        pMesh = pMeshContainer->MeshData.pMesh;

		// now generate the normals for the pmesh
        D3DXComputeNormals( pMesh, NULL );
    }
    else  // if no normals, just add a reference to the mesh for the mesh container
	{
        pMeshContainer->MeshData.pMesh = pMesh;
        pMeshContainer->MeshData.Type = D3DXMESHTYPE_MESH;

        pMesh->AddRef();
    }

    // allocate memory to contain the material information.  This sample uses
    // the D3D9 materials and texture names instead of the EffectInstance style materials
    pMeshContainer->NumMaterials = max( 1, NumMaterials );
    pMeshContainer->pMaterials = new D3DXMATERIAL[pMeshContainer->NumMaterials];
    pMeshContainer->ppTextures = new LPDIRECT3DTEXTURE9[pMeshContainer->NumMaterials];
    pMeshContainer->pAdjacency = new DWORD[NumFaces*3];
    if( ( pMeshContainer->pAdjacency == NULL ) || ( pMeshContainer->pMaterials == NULL ) )
	{
        hr = E_OUTOFMEMORY;
        goto e_Exit;
    }

    memcpy( pMeshContainer->pAdjacency, pAdjacency, sizeof( DWORD ) * NumFaces*3 );
    memset( pMeshContainer->ppTextures, 0, sizeof( LPDIRECT3DTEXTURE9 ) * pMeshContainer->NumMaterials );

    // if materials provided, copy them
    if( NumMaterials > 0 )
	{
        memcpy( pMeshContainer->pMaterials, pMaterials, sizeof( D3DXMATERIAL ) * NumMaterials );

        for( iMaterial = 0; iMaterial < NumMaterials; iMaterial++ )
		{
            if( pMeshContainer->pMaterials[iMaterial].pTextureFilename != NULL )
			{
                WCHAR strTexturePath[MAX_PATH];
                MultiByteToWideChar( CP_ACP, 0, pMeshContainer->pMaterials[iMaterial].pTextureFilename, -1, strTexturePath, MAX_PATH );
                if( FAILED( D3DXCreateTextureFromFile( pd3dDevice, strTexturePath,
                                                        &pMeshContainer->ppTextures[iMaterial] ) ) )
                    pMeshContainer->ppTextures[iMaterial] = NULL;

				// don't remember a pointer into the dynamic memory, just forget the name after loading
                pMeshContainer->pMaterials[iMaterial].pTextureFilename = NULL;
            }
        }
    }
    else // if no materials provided, use a default one
	{
        pMeshContainer->pMaterials[0].pTextureFilename = NULL;
        memset( &pMeshContainer->pMaterials[0].MatD3D, 0, sizeof( D3DMATERIAL9 ) );
        pMeshContainer->pMaterials[0].MatD3D.Diffuse.r = 0.5f;
        pMeshContainer->pMaterials[0].MatD3D.Diffuse.g = 0.5f;
        pMeshContainer->pMaterials[0].MatD3D.Diffuse.b = 0.5f;
        pMeshContainer->pMaterials[0].MatD3D.Specular = pMeshContainer->pMaterials[0].MatD3D.Diffuse;
    }

    // if there is skinning information, save off the required data and then setup for HW skinning
    if( pSkinInfo != NULL )
	{
		// first save off the SkinInfo and original mesh data
        pMeshContainer->pSkinInfo = pSkinInfo;
        pSkinInfo->AddRef();

        pMeshContainer->pOrigMesh = pMesh;
        pMesh->AddRef();

		// Will need an array of offset matrices to move the vertices from the figure space to the bone's space
        cBones = pSkinInfo->GetNumBones();
        pMeshContainer->pBoneOffsetMatrices = new D3DXMATRIX[cBones];
        if( pMeshContainer->pBoneOffsetMatrices == NULL )
		{
            hr = E_OUTOFMEMORY;
            goto e_Exit;
        }

		// get each of the bone offset matrices so that we don't need to get them later
        for( iBone = 0; iBone < cBones; iBone++ )
		{
            pMeshContainer->pBoneOffsetMatrices[iBone] = *( pMeshContainer->pSkinInfo->GetBoneOffsetMatrix( iBone ) );
        }

		// GenerateSkinnedMesh will take the general skinning information and transform it to a HW friendly version
		hr = SkinnedMesh::GenerateSkinnedMesh( pd3dDevice, pMeshContainer );
        if( FAILED( hr ) )
            goto e_Exit;
    }

    *ppNewMeshContainer = pMeshContainer;
    pMeshContainer = NULL;

e_Exit:
    SAFE_RELEASE( pd3dDevice );

    // call Destroy function to properly clean up the memory allocated 
    if( pMeshContainer != NULL )
        DestroyMeshContainer( pMeshContainer );

    return hr;
}

//--------------------------------------------------------------------------------------
// Name: CAllocateHierarchy::DestroyFrame()
// Desc: 
//--------------------------------------------------------------------------------------
HRESULT CAllocateHierarchy::DestroyFrame( LPD3DXFRAME pFrameToFree )
{
    SAFE_DELETE_ARRAY( pFrameToFree->Name );
    SAFE_DELETE( pFrameToFree );
    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: CAllocateHierarchy::DestroyMeshContainer()
// Desc: 
//--------------------------------------------------------------------------------------
HRESULT CAllocateHierarchy::DestroyMeshContainer( LPD3DXMESHCONTAINER pMeshContainerBase )
{
    UINT iMaterial;
    D3DXMESHCONTAINER_DERIVED* pMeshContainer = ( D3DXMESHCONTAINER_DERIVED* )pMeshContainerBase;

    SAFE_DELETE_ARRAY( pMeshContainer->Name );
    SAFE_DELETE_ARRAY( pMeshContainer->pAdjacency );
    SAFE_DELETE_ARRAY( pMeshContainer->pMaterials );
    SAFE_DELETE_ARRAY( pMeshContainer->pBoneOffsetMatrices );

    // release all the allocated textures
    if( pMeshContainer->ppTextures != NULL )
    {
        for( iMaterial = 0; iMaterial < pMeshContainer->NumMaterials; iMaterial++ )
        {
            SAFE_RELEASE( pMeshContainer->ppTextures[iMaterial] );
        }
    }

    SAFE_DELETE_ARRAY( pMeshContainer->ppTextures );
    SAFE_DELETE_ARRAY( pMeshContainer->ppBoneMatrixPtrs );
    SAFE_RELEASE( pMeshContainer->pBoneCombinationBuf );
    SAFE_RELEASE( pMeshContainer->MeshData.pMesh );
    SAFE_RELEASE( pMeshContainer->pSkinInfo );
    SAFE_RELEASE( pMeshContainer->pOrigMesh );
    SAFE_DELETE( pMeshContainer );
    return S_OK;
}

//--------------------------------------------------------------------------------------
//
//
//--------------------------------------------------------------------------------------

D3DXMATRIXA16*	SkinnedMesh::g_pBoneMatrices = NULL;
UINT			SkinnedMesh::g_NumBoneMatricesMax = 0;

//--------------------------------------------------------------------------------------
// Called either by CreateMeshContainer when loading a skin mesh, or when 
// changing methods.  This function uses the pSkinInfo of the mesh 
// container to generate the desired drawable mesh and bone combination 
// table.
//--------------------------------------------------------------------------------------
HRESULT SkinnedMesh::GenerateSkinnedMesh( IDirect3DDevice9* pd3dDevice, D3DXMESHCONTAINER_DERIVED* pMeshContainer )
{
    HRESULT hr = S_OK;
    D3DCAPS9 d3dCaps;
    pd3dDevice->GetDeviceCaps( &d3dCaps );

    if( pMeshContainer->pSkinInfo == NULL )
        return hr;

    SAFE_RELEASE( pMeshContainer->MeshData.pMesh );
    SAFE_RELEASE( pMeshContainer->pBoneCombinationBuf );

	{
        hr = pMeshContainer->pOrigMesh->CloneMeshFVF( D3DXMESH_MANAGED, pMeshContainer->pOrigMesh->GetFVF(),
                                                      pd3dDevice, &pMeshContainer->MeshData.pMesh );
        if( FAILED( hr ) )
            goto e_Exit;

        hr = pMeshContainer->MeshData.pMesh->GetAttributeTable( NULL, &pMeshContainer->NumAttributeGroups );
        if( FAILED( hr ) )
            goto e_Exit;

        delete[] pMeshContainer->pAttributeTable;
        pMeshContainer->pAttributeTable = new D3DXATTRIBUTERANGE[pMeshContainer->NumAttributeGroups];
        if( pMeshContainer->pAttributeTable == NULL )
        {
            hr = E_OUTOFMEMORY;
            goto e_Exit;
        }

        hr = pMeshContainer->MeshData.pMesh->GetAttributeTable( pMeshContainer->pAttributeTable, NULL );
        if( FAILED( hr ) )
            goto e_Exit;

        // allocate a buffer for bone matrices, but only if another mesh has not allocated one of the same size or larger
        if( g_NumBoneMatricesMax < pMeshContainer->pSkinInfo->GetNumBones() )
        {
            g_NumBoneMatricesMax = pMeshContainer->pSkinInfo->GetNumBones();

            // Allocate space for blend matrices
            delete[] g_pBoneMatrices;
            g_pBoneMatrices = new D3DXMATRIXA16[g_NumBoneMatricesMax];
            if( g_pBoneMatrices == NULL )
            {
                hr = E_OUTOFMEMORY;
                goto e_Exit;
            }
        }
    }

e_Exit:
    return hr;
}

//--------------------------------------------------------------------------------------
// Called to render a mesh in the hierarchy
//--------------------------------------------------------------------------------------
void SkinnedMesh::DrawMeshContainer( IDirect3DDevice9* pd3dDevice, LPD3DXMESHCONTAINER pMeshContainerBase, LPD3DXFRAME pFrameBase)
{
    HRESULT hr;
    D3DXMESHCONTAINER_DERIVED* pMeshContainer = ( D3DXMESHCONTAINER_DERIVED* )pMeshContainerBase;
	// Commented out by me
    // D3DXFRAME_DERIVED* pFrame = ( D3DXFRAME_DERIVED* )pFrameBase;
    UINT iMaterial;
    UINT iAttrib;

    D3DXMATRIXA16 matTemp;
    D3DCAPS9 d3dCaps;
    pd3dDevice->GetDeviceCaps( &d3dCaps );

    // first check for skinning
    if( pMeshContainer->pSkinInfo != NULL )
    {
        {
			if (frameDirtyFlag)
			{
				D3DXMATRIX Identity;
				DWORD cBones = pMeshContainer->pSkinInfo->GetNumBones();
				DWORD iBone;
				PBYTE pbVerticesSrc;
				PBYTE pbVerticesDest;

				// set up bone transforms
				for( iBone = 0; iBone < cBones; ++iBone )
				{
					D3DXMatrixMultiply(
						&g_pBoneMatrices[iBone],                 // output
						&pMeshContainer->pBoneOffsetMatrices[iBone],
						pMeshContainer->ppBoneMatrixPtrs[iBone]
						);
				}

				// Commented out by me
				// set world transform
				//D3DXMatrixIdentity( &Identity );
				//V( pd3dDevice->SetTransform( D3DTS_WORLD, &Identity ) );

				V( pMeshContainer->pOrigMesh->LockVertexBuffer( D3DLOCK_READONLY, ( LPVOID* )&pbVerticesSrc ) );
				V( pMeshContainer->MeshData.pMesh->LockVertexBuffer( 0, ( LPVOID* )&pbVerticesDest ) );

				// generate skinned mesh
				pMeshContainer->pSkinInfo->UpdateSkinnedMesh( g_pBoneMatrices, NULL, pbVerticesSrc, pbVerticesDest );

				V( pMeshContainer->pOrigMesh->UnlockVertexBuffer() );
				V( pMeshContainer->MeshData.pMesh->UnlockVertexBuffer() );

				frameDirtyFlag = false;
			}

            for( iAttrib = 0; iAttrib < pMeshContainer->NumAttributeGroups; iAttrib++ )
            {
                V( pd3dDevice->SetMaterial(&(pMeshContainer->pMaterials[pMeshContainer->pAttributeTable[iAttrib].AttribId].MatD3D) ) );
                V( pd3dDevice->SetTexture(0, pMeshContainer->ppTextures[pMeshContainer->pAttributeTable[iAttrib].AttribId]) );
                V( pMeshContainer->MeshData.pMesh->DrawSubset(pMeshContainer->pAttributeTable[iAttrib].AttribId) );
            }
        }
    }
    else  // standard mesh, just draw it after setting material properties
    {
		// Commented out by me
        // V( pd3dDevice->SetTransform( D3DTS_WORLD, &pFrame->CombinedTransformationMatrix ) );

		stdext::hash_set<int>::const_iterator itr;

        for( iMaterial = 0; iMaterial < pMeshContainer->NumMaterials; iMaterial++ )
        {
			itr = maskedSubsets.find(iMaterial);
			if (itr != maskedSubsets.end())	// match was found this subset is masked.
				continue;

            V( pd3dDevice->SetMaterial( &pMeshContainer->pMaterials[iMaterial].MatD3D ) );
            V( pd3dDevice->SetTexture( 0, pMeshContainer->ppTextures[iMaterial] ) );
            V( pMeshContainer->MeshData.pMesh->DrawSubset( iMaterial ) );
        }
    }
}

//--------------------------------------------------------------------------------------
// Called to render a frame in the hierarchy
//--------------------------------------------------------------------------------------
void SkinnedMesh::DrawFrame( IDirect3DDevice9* pd3dDevice, LPD3DXFRAME pFrame)
{
    LPD3DXMESHCONTAINER pMeshContainer;

    pMeshContainer = pFrame->pMeshContainer;
    while( pMeshContainer != NULL )
    {
        DrawMeshContainer(pd3dDevice, pMeshContainer, pFrame);

        pMeshContainer = pMeshContainer->pNextMeshContainer;
    }

    if( pFrame->pFrameSibling != NULL )
    {
        DrawFrame( pd3dDevice, pFrame->pFrameSibling);
    }

    if( pFrame->pFrameFirstChild != NULL )
    {
        DrawFrame( pd3dDevice, pFrame->pFrameFirstChild);
    }
}

//--------------------------------------------------------------------------------------
// Called to setup the pointers for a given bone to its transformation matrix
//--------------------------------------------------------------------------------------
HRESULT SkinnedMesh::SetupBoneMatrixPointersOnMesh( LPD3DXMESHCONTAINER pMeshContainerBase )
{
    UINT iBone, cBones;
    D3DXFRAME_DERIVED* pFrame;

    D3DXMESHCONTAINER_DERIVED* pMeshContainer = ( D3DXMESHCONTAINER_DERIVED* )pMeshContainerBase;

    // if there is a skinmesh, then setup the bone matrices
    if( pMeshContainer->pSkinInfo != NULL )
    {
        cBones = pMeshContainer->pSkinInfo->GetNumBones();

        pMeshContainer->ppBoneMatrixPtrs = new D3DXMATRIX*[cBones];
        if( pMeshContainer->ppBoneMatrixPtrs == NULL )
            return E_OUTOFMEMORY;

        for( iBone = 0; iBone < cBones; iBone++ )
        {
            pFrame = ( D3DXFRAME_DERIVED* )D3DXFrameFind( g_pFrameRoot,
                                                          pMeshContainer->pSkinInfo->GetBoneName( iBone ) );
            if( pFrame == NULL )
                return E_FAIL;

            pMeshContainer->ppBoneMatrixPtrs[iBone] = &pFrame->CombinedTransformationMatrix;
        }
    }

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Called to setup the pointers for a given bone to its transformation matrix
//--------------------------------------------------------------------------------------
HRESULT SkinnedMesh::SetupBoneMatrixPointers( LPD3DXFRAME pFrame )
{
    HRESULT hr;

    if( pFrame->pMeshContainer != NULL )
    {
        hr = SetupBoneMatrixPointersOnMesh( pFrame->pMeshContainer );
        if( FAILED( hr ) )
            return hr;
    }

    if( pFrame->pFrameSibling != NULL )
    {
        hr = SetupBoneMatrixPointers( pFrame->pFrameSibling );
        if( FAILED( hr ) )
            return hr;
    }

    if( pFrame->pFrameFirstChild != NULL )
    {
        hr = SetupBoneMatrixPointers( pFrame->pFrameFirstChild );
        if( FAILED( hr ) )
            return hr;
    }

    return S_OK;
}




//--------------------------------------------------------------------------------------
// update the frame matrices
//--------------------------------------------------------------------------------------
void SkinnedMesh::UpdateFrameMatrices( LPD3DXFRAME pFrameBase, LPD3DXMATRIX pParentMatrix )
{
    D3DXFRAME_DERIVED* pFrame = ( D3DXFRAME_DERIVED* )pFrameBase;

    if( pParentMatrix != NULL )
        D3DXMatrixMultiply( &pFrame->CombinedTransformationMatrix, &pFrame->TransformationMatrix, pParentMatrix );
    else
        pFrame->CombinedTransformationMatrix = pFrame->TransformationMatrix;

    if( pFrame->pFrameSibling != NULL )
    {
        UpdateFrameMatrices( pFrame->pFrameSibling, pParentMatrix );
    }

    if( pFrame->pFrameFirstChild != NULL )
    {
        UpdateFrameMatrices( pFrame->pFrameFirstChild, &pFrame->CombinedTransformationMatrix );
    }
}

void SkinnedMesh::ReleaseAttributeTable( LPD3DXFRAME pFrameBase )
{
    D3DXFRAME_DERIVED* pFrame = ( D3DXFRAME_DERIVED* )pFrameBase;
    D3DXMESHCONTAINER_DERIVED* pMeshContainer;

    pMeshContainer = ( D3DXMESHCONTAINER_DERIVED* )pFrame->pMeshContainer;

    while( pMeshContainer != NULL )
    {
        delete[] pMeshContainer->pAttributeTable;

        pMeshContainer = ( D3DXMESHCONTAINER_DERIVED* )pMeshContainer->pNextMeshContainer;
    }

    if( pFrame->pFrameSibling != NULL )
    {
        ReleaseAttributeTable( pFrame->pFrameSibling );
    }

    if( pFrame->pFrameFirstChild != NULL )
    {
        ReleaseAttributeTable( pFrame->pFrameFirstChild );
    }
}

int SkinnedMesh::CountVertices(LPD3DXFRAME pFrameBase, int * maxMeshVrtsCount)
{
    D3DXFRAME_DERIVED* pFrame = ( D3DXFRAME_DERIVED* )pFrameBase;
    D3DXMESHCONTAINER_DERIVED* pMeshContainer;

    pMeshContainer = ( D3DXMESHCONTAINER_DERIVED* )pFrame->pMeshContainer;

	int totalVerticesCount = 0;
	int verticesCount;
    while (pMeshContainer != NULL)
    {
		verticesCount = pMeshContainer->MeshData.pMesh->GetNumVertices();
		totalVerticesCount += verticesCount;

		if (NULL != maxMeshVrtsCount)
			*maxMeshVrtsCount = max(*maxMeshVrtsCount, verticesCount);
		
        pMeshContainer = ( D3DXMESHCONTAINER_DERIVED* )pMeshContainer->pNextMeshContainer;
    }

    if (pFrame->pFrameSibling != NULL)
	{
        verticesCount = CountVertices(pFrame->pFrameSibling, maxMeshVrtsCount);
		totalVerticesCount += verticesCount;

		if (NULL != maxMeshVrtsCount)
			*maxMeshVrtsCount = max(*maxMeshVrtsCount, verticesCount);
	}

    if (pFrame->pFrameFirstChild != NULL)
	{
        verticesCount = CountVertices(pFrame->pFrameFirstChild, maxMeshVrtsCount);
		totalVerticesCount += verticesCount;

		if (NULL != maxMeshVrtsCount)
			*maxMeshVrtsCount = max(*maxMeshVrtsCount, verticesCount);
	}

	return totalVerticesCount;
}

int SkinnedMesh::CountFaces( LPD3DXFRAME pFrameBase )
{
    D3DXFRAME_DERIVED* pFrame = ( D3DXFRAME_DERIVED* )pFrameBase;
    D3DXMESHCONTAINER_DERIVED* pMeshContainer;

    pMeshContainer = ( D3DXMESHCONTAINER_DERIVED* )pFrame->pMeshContainer;

	int facesCount = 0;
    while (pMeshContainer != NULL)
    {
		facesCount += pMeshContainer->MeshData.pMesh->GetNumFaces();
        pMeshContainer = ( D3DXMESHCONTAINER_DERIVED* )pMeshContainer->pNextMeshContainer;
    }

    if (pFrame->pFrameSibling != NULL)
        facesCount += CountFaces( pFrame->pFrameSibling );

    if (pFrame->pFrameFirstChild != NULL)
        facesCount += CountFaces( pFrame->pFrameFirstChild );

	return facesCount;
}

//--------------------------------------------------------------------------------------
// Name: SkinnedMesh::SkinnedMesh()
// Desc: 
//--------------------------------------------------------------------------------------
SkinnedMesh::SkinnedMesh()
: g_pFrameRoot(NULL), g_pAnimController(NULL),
frameDirtyFlag(true), transformedCoords(NULL)
{
}

//--------------------------------------------------------------------------------------
// Name: SkinnedMesh::~SkinnedMesh()
// Desc: 
//--------------------------------------------------------------------------------------
SkinnedMesh::~SkinnedMesh()
{
}

//--------------------------------------------------------------------------------------
// Name: SkinnedMesh::Create()
// Desc: 
//--------------------------------------------------------------------------------------
void SkinnedMesh::Create(IDirect3DDevice9 *device, LPCTSTR filename, bool computeNormals)
{
	HRESULT hr;
	CAllocateHierarchy Alloc;

    V( D3DXLoadMeshHierarchyFromX(filename, D3DXMESH_MANAGED, device, &Alloc, NULL, &g_pFrameRoot, &g_pAnimController) );
    V( SetupBoneMatrixPointers(g_pFrameRoot) );
    V( D3DXFrameCalculateBoundingSphere(g_pFrameRoot, &g_vObjectCenter, &g_fObjectRadius) );

	int maxMeshVrtsCount = 0;
	verticesCount = CountVertices(g_pFrameRoot, &maxMeshVrtsCount);
	facesCount = CountFaces(g_pFrameRoot);

	transformedCoords = new D3DXVECTOR4[maxMeshVrtsCount];
}

void SkinnedMesh::RestoreDeviceObjects(IDirect3DDevice9* device)
{
}

void SkinnedMesh::InvalidateDeviceObjects()
{
}

void SkinnedMesh::Destroy()
{
	SAFE_DELETE_ARRAY(transformedCoords);

	if (NULL != g_pFrameRoot)
		ReleaseAttributeTable( g_pFrameRoot );

	if (NULL != g_pBoneMatrices)
	{
		delete[] g_pBoneMatrices;
		g_pBoneMatrices = NULL;
		g_NumBoneMatricesMax = 0;
	}

	CAllocateHierarchy Alloc;
    D3DXFrameDestroy( g_pFrameRoot, &Alloc );
	g_pFrameRoot = NULL;
    SAFE_RELEASE( g_pAnimController );
}

void SkinnedMesh::Update(double fTime, float fElapsedTime)
{
	if(g_pAnimController != NULL)
        g_pAnimController->AdvanceTime(fElapsedTime, NULL);

    UpdateFrameMatrices(g_pFrameRoot, NULL);
	frameDirtyFlag = true;
}

void SkinnedMesh::Render(IDirect3DDevice9 *device)
{
	DrawFrame(device, g_pFrameRoot);
}

void SkinnedMesh::ComputeMeshAABB(const D3DXMATRIX & transform, D3DXMESHCONTAINER_DERIVED & meshContainer, AABB * aabb)
{
	HRESULT hr;
	ID3DXMesh * mesh = meshContainer.MeshData.pMesh;
	DWORD vCount = mesh->GetNumVertices();
	DWORD vSize = mesh->GetNumBytesPerVertex();
	D3DXVECTOR3 *data = NULL;
	mesh->LockVertexBuffer(D3DLOCK_READONLY, (void**)&data);
	D3DXVec3TransformArray(transformedCoords, sizeof(D3DXVECTOR4), data, vSize, &transform, vCount);
	mesh->UnlockVertexBuffer();

	D3DXVECTOR3 min, max;
	V( D3DXComputeBoundingBox((D3DXVECTOR3*)transformedCoords, vCount, sizeof(D3DXVECTOR4), &min, &max) );
	aabb->SetMinMax(min, max);
}

void SkinnedMesh::ComputeAABB(const D3DXMATRIX & transform, LPD3DXFRAME pFrameBase, AABB * aabb)
{
	D3DXFRAME_DERIVED* pFrame = ( D3DXFRAME_DERIVED* )pFrameBase;
    D3DXMESHCONTAINER_DERIVED* pMeshContainer;

    pMeshContainer = ( D3DXMESHCONTAINER_DERIVED* )pFrame->pMeshContainer;

    while (pMeshContainer != NULL)
    {
		AABB aabb2;
		ComputeMeshAABB(transform, *pMeshContainer, &aabb2);
		aabb->Merge(aabb2);
        pMeshContainer = ( D3DXMESHCONTAINER_DERIVED* )pMeshContainer->pNextMeshContainer;
    }

    if (pFrame->pFrameSibling != NULL)
	{
		AABB aabb2;
		ComputeAABB(transform, pFrame->pFrameSibling, &aabb2);
		aabb->Merge(aabb2);
	}

    if (pFrame->pFrameFirstChild != NULL)
	{
		AABB aabb2;
		ComputeAABB(transform, pFrame->pFrameFirstChild, &aabb2);
		aabb->Merge(aabb2);
	}
}

void SkinnedMesh::ComputeTransformedAABB(const D3DXMATRIX & transform, AABB * aabb)
{
	ComputeAABB(transform, g_pFrameRoot, aabb);
}

void SkinnedMesh::SetMaskedSubset(const stdext::hash_set<int> &masks)
{
	maskedSubsets = masks;
}