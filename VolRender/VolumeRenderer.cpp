#include "StdAfx.h"
#include "ShaderFlags.h"
#include "VolumeRenderer.h"
#include "VoxelizationNode.h"

using namespace std;

VolumeRenderer::VolumeRenderer()
: effect(NULL),
techRenderPosition(NULL), techRayCast(NULL), paramWorldViewProj(NULL),
paramFrontTexture(NULL), paramBackTexture(NULL), paramVolumeTexture(NULL)
{
}

VolumeRenderer::~VolumeRenderer()
{
	vector< VoxelizationNodeHolder >::iterator it;
	for (it = voxelizationNodes.begin(); it != voxelizationNodes.end(); ++it)
		delete (*it).node;
}

void VolumeRenderer::OnCreateDevice(IDirect3DDevice9 *device, int backBufferWidth, int backBufferHeight)
{
	HRESULT hr;

	WCHAR strPath[MAX_PATH];
	swprintf_s(strPath, L"../../Models/%s", L"box.x");
	V( uniformBox.Create(device, strPath, false) );
	uniformBox.UseMeshMaterials(false);

	ID3DXBuffer *buffer = NULL;
	// Read the D3DX effect file
	D3DXCreateEffectFromFile(device, L"VolumeRendering.fxo", NULL, NULL, g_dwShaderFlags, NULL, &effect, &buffer);
	if (NULL !=buffer)
	{
		LPCSTR str = (LPCSTR)buffer->GetBufferPointer();
		UNREFERENCED_PARAMETER(str);
		buffer->Release();
	}

	techRenderPosition = effect->GetTechniqueByName("RenderPosition");
	techRayCast = effect->GetTechniqueByName("RayCast");
    paramWorldViewProj = effect->GetParameterByName(NULL, "WorldViewProj");
	paramFrontTexture = effect->GetParameterByName(NULL, "FrontT");
	paramBackTexture = effect->GetParameterByName(NULL, "BackT");
	paramVolumeTexture = effect->GetParameterByName(NULL, "VolumeT");
	paramTransferFunctionTexture = effect->GetParameterByName(NULL, "TransferFunctionT");
	paramIterations = effect->GetParameterByName(NULL, "Iterations");
	paramResolution = effect->GetParameterByName(NULL, "Resolution");
	paramBoxMin = effect->GetParameterByName(NULL, "BoxMin");
	paramBoxMax = effect->GetParameterByName(NULL, "BoxMax");

	vector< VoxelizationNodeHolder >::iterator it;
	for (it = voxelizationNodes.begin(); it != voxelizationNodes.end(); ++it)
	{
		VoxelizationNodeHolder & holder = *it;
		holder.node->OnCreateDevice(device);

		V( device->CreateTexture(backBufferWidth, backBufferHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &holder.textureFrontRT, NULL) );
		V( device->CreateTexture(backBufferWidth, backBufferHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &holder.textureBackRT, NULL) );
	}
}

void VolumeRenderer::OnResetDevice(IDirect3DDevice9 *device)
{
	HRESULT hr;

	V( effect->OnResetDevice() );
	V( uniformBox.RestoreDeviceObjects(device) );

	vector< VoxelizationNodeHolder >::iterator it;
	for (it = voxelizationNodes.begin(); it != voxelizationNodes.end(); ++it)
	{
		VoxelizationNodeHolder & holder = *it;
		holder.node->OnResetDevice(device);
	}
}

void VolumeRenderer::OnLostDevice()
{
	HRESULT hr;

	vector< VoxelizationNodeHolder >::iterator it;
	for (it = voxelizationNodes.begin(); it != voxelizationNodes.end(); ++it)
	{
		VoxelizationNodeHolder & holder = *it;
		holder.node->OnLostDevice();
	}

	V( effect->OnLostDevice() );
	V( uniformBox.InvalidateDeviceObjects() );
}

void VolumeRenderer::OnDestroyDevice()
{
	HRESULT hr;

	vector< VoxelizationNodeHolder >::iterator it;
	for (it = voxelizationNodes.begin(); it != voxelizationNodes.end(); ++it)
	{
		VoxelizationNodeHolder & holder = *it;

		SAFE_RELEASE(holder.textureFrontRT);
		SAFE_RELEASE(holder.textureBackRT);
		holder.node->OnDestroyDevice();
	}

	SAFE_RELEASE(effect);
	V( uniformBox.Destroy() );
}

void VolumeRenderer::SaveFrontBackTextures(IDirect3DTexture9 * textureFrontRT, IDirect3DTexture9 * textureBackRT)
{
	HRESULT hr;

	V( D3DXSaveTextureToFile(L"front.jpg", D3DXIFF_JPG, textureFrontRT, NULL) );
	V( D3DXSaveTextureToFile(L"back.jpg", D3DXIFF_JPG, textureBackRT, NULL) );
}

void VolumeRenderer::RenderUnifromBoxWithEffect(IDirect3DDevice9 *device)
{
	HRESULT hr;
	UINT passes;

	V( effect->Begin(&passes, 0) );
	for (UINT p = 0; p < passes; ++p)
	{
		V( effect->BeginPass(p) );
		V( uniformBox.Render(device) );
		V( effect->EndPass() );
	}
	V( effect->End() );
}

void VolumeRenderer::update(IDirect3DDevice9* device, const D3DXMATRIX& wvp, IDirect3DTexture9 * frontRT, IDirect3DTexture9 * backRT)
{
	HRESULT hr;
	IDirect3DSurface9* tmpRT;

	V( effect->SetMatrix( paramWorldViewProj, &wvp) );

	// Render Front
	V( frontRT->GetSurfaceLevel(0, &tmpRT) );
	V( device->SetRenderTarget(0, tmpRT) );
	tmpRT->Release();
	device->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1.0f, 0);

	if( SUCCEEDED( device->BeginScene() ) )
	{
		device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
		RenderUnifromBoxWithEffect(device);

		V( device->EndScene() );
	}
	
	// Render Back
	V( backRT->GetSurfaceLevel(0, &tmpRT) );
	V( device->SetRenderTarget(0, tmpRT) );
	tmpRT->Release();
	device->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1.0f, 0);

	if( SUCCEEDED( device->BeginScene() ) )
	{
		device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
		RenderUnifromBoxWithEffect(device);

		V( device->EndScene() );
	}
}

void VolumeRenderer::Update(IDirect3DDevice9* device, const D3DXMATRIX& view, const D3DXMATRIX& proj)
{
	HRESULT hr;
	IDirect3DSurface9* oldRT;
	D3DXVECTOR3 c, e;
	D3DXMATRIXA16 world;
	D3DXMATRIXA16 wvp;

	D3DXVECTOR3 vZero(0.0f, 0.0f, 0.0f);
	D3DXQUATERNION qIdentity;
	D3DXQuaternionIdentity(&qIdentity);

	V( device->GetRenderTarget(0, &oldRT) );
	V( effect->SetTechnique(techRenderPosition) );

	vector< VoxelizationNodeHolder >::iterator it;
	for (it = voxelizationNodes.begin(); it != voxelizationNodes.end(); ++it)
	{
		VoxelizationNodeHolder & holder = *it;
		const AABB & aabb = holder.node->GetAABB();

		c = aabb.GetCenter();
		e = aabb.GetExtents();

		D3DXMatrixTransformation(&world, &vZero, &qIdentity, &e, &vZero, &qIdentity, &c);

		wvp = world * view * proj;

		update(device, wvp, holder.textureFrontRT, holder.textureBackRT);
	}

	// Restore previous render target
	V( device->SetRenderTarget(0, oldRT) );
	oldRT->Release();
}

void VolumeRenderer::Render(IDirect3DDevice9* device, const D3DXMATRIX& view, const D3DXMATRIX& proj)
{
	HRESULT hr;

	int iterations;

	D3DXVECTOR3 c, e;
	D3DXMATRIXA16 world;
	D3DXMATRIXA16 wvp;

	D3DXVECTOR3 vZero(0.0f, 0.0f, 0.0f);
	D3DXQUATERNION qIdentity;
	D3DXQuaternionIdentity(&qIdentity);

	V( effect->SetTechnique(techRayCast) );

	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	// Enable Alpha Blending
	device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA); //set the source to be the source's alpha
	device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA); //set the destination to be the inverse of the source's alpha
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE); //enable

	vector< VoxelizationNodeHolder >::iterator it;
	for (it = voxelizationNodes.begin(); it != voxelizationNodes.end(); ++it)
	{
		VoxelizationNodeHolder & holder = *it;

		if (!holder.visible)
			continue;

		const Vector3I & r = holder.node->GetResolution();
		const AABB & aabb = holder.node->GetAABB();

		// D3DXVECTOR4 resolution = D3DXVECTOR4((float)r.x, (float)r.y, (float)r.z, 0.0f);
		iterations = max(max(r.x, r.y), r.z);

		c = aabb.GetCenter();
		e = aabb.GetExtents();

		D3DXMatrixTransformation(&world, &vZero, &qIdentity, &e, &vZero, &qIdentity, &c);

		wvp = world * view * proj;		

		V( effect->SetMatrix(paramWorldViewProj, &wvp) );
		// V( effect->SetVector(paramResolution, &resolution) );
		V( effect->SetInt(paramIterations, iterations) );
		V( effect->SetTexture(paramBackTexture, holder.textureBackRT) );
		V( effect->SetTexture(paramFrontTexture, holder.textureFrontRT) );
		V( effect->SetTexture(paramVolumeTexture, holder.node->GetVolume()) );
		V( effect->SetTexture(paramTransferFunctionTexture, holder.node->GetTransferFunctionTexture()) );

		D3DXVECTOR4 boxMin(aabb.GetMin(), 0.0f);
		D3DXVECTOR4 boxMax(aabb.GetMax(), 0.0f);
		V( effect->SetVector(paramBoxMin, &boxMin) );
		V( effect->SetVector(paramBoxMax, &boxMax) );

		RenderUnifromBoxWithEffect(device);
	}

	device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE); // disable
}

void VolumeRenderer::AddVoxelizationNode(VoxelizationNode * voxelizationNode)
{
	VoxelizationNodeHolder holder;
	holder.node = voxelizationNode;
	voxelizationNodes.push_back(holder);
}

int VolumeRenderer::GetVoxelizationNodesCount() const
{
	return (int)voxelizationNodes.size();
}

VoxelizationNode * VolumeRenderer::GetVoxelizationNode(int idx)
{
	return voxelizationNodes[idx].node;
}

void VolumeRenderer::HideVoxelizationNode(int idx)
{
	voxelizationNodes[idx].visible = false;
}

void VolumeRenderer::GetVoxelizationNodes(std::vector< VoxelizationNode * > & nodes)
{
	nodes.resize(voxelizationNodes.size());

	for (size_t i = 0; i < nodes.size(); ++i)
		nodes[i] = voxelizationNodes[i].node;
}

