//--------------------------------------------------------------------------------------
// File: Voxelization.cpp
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "StdAfx.h"
#include "DepthVoxelizer.h"
#include "VolumeRenderer.h"
#include "ShaderFlags.h"
#include "SkinnedMesh.h"
#include "VoxelizationDumper.h"
#include "AABB.h"
#include "Voxelization.h"
#include "VoxelizationNode.h"
#include "TransferFunctionControls.h"

//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLE_FULLSCREEN			1001
#define IDC_TOGGLE_REF          		1002
#define IDC_CHANGE_DEVICE				1003
#define IDC_CONFIG_VOXELIZATION			1004
#define IDC_CONFIG_TRANSFER_FUNCTION	1005
#define IDC_TOGGLE_RENDER_MODE			1011
#define IDC_SELECT_MESH					1012
#define IDC_SELECT_RESOLUTION			1013
#define IDC_AUTO_FIT_VOXELIZATION_BOX	1014
#define IDC_VOXELIZE					1015
#define IDC_DUMP_SLICES					1016
#define IDC_SAVE_VOXELIZATION			1017
#define IDC_ADJUST_VOXEL_TRANSPARENCY	1018

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
CDXUTDialogResourceManager  g_DialogResourceManager;	// manager for shared resources of dialogs
CD3DSettingsDlg             g_SettingsDlg;				// Device settings dialog

CDXUTTextHelper*            g_pTxtHelper = NULL;
CDXUTDialog                 g_HUD;                  	// dialog for standard controls
CDXUTDialog                 g_SampleUI;             	// dialog for sample specific controls
CDXUTComboBox*				g_comboBoxSelectMesh;
CDXUTComboBox*				g_comboBoxSelectVoxelizationFormat;
// Direct3D 9 resources
ID3DXFont*                  g_pFont9 = NULL;
ID3DXSprite*                g_pSprite9 = NULL;
CModelViewerCamera			g_Camera;

// Effect Resources
ID3DXEffect* g_Effect;
D3DXHANDLE g_TechRenderScene;
D3DXHANDLE g_ParamWorldViewProj;
D3DXHANDLE g_ParamWorld;
D3DXHANDLE g_ParamMeshTexture;
DWORD g_dwShaderFlags = 0;

ConfigureTransferFunctionDlg g_ConfigureTransferFunctionDlg;

static float g_UniformScale = 1.0f;
static D3DXMATRIXA16 g_matUniformScale;
static D3DXMATRIXA16 g_matWorld;
static D3DXMATRIXA16 g_matView;
static D3DXMATRIXA16 g_matProj;

//--------------------------------------------------------------------------------------
// Voxelization variables
//--------------------------------------------------------------------------------------
float g_VoxelizationTime = 0.0f;
float g_EdgeDetectionTime = 0.0f;
float g_OrthoZoom = 1.0f;
Vector3I g_VoxelizationFormat[] =
{
	Vector3I(4, 4, 4),
	Vector3I(8, 8, 8),
	Vector3I(16, 16, 16),
	Vector3I(32, 32, 32),
	Vector3I(64, 64, 64),
	Vector3I(128, 128, 128),
	Vector3I(256, 256, 256),
	Vector3I(512, 512, 512),
	Vector3I(1024, 768, 16),
	Vector3I(1024, 768, 32),
	Vector3I(1024, 768, 64),
	Vector3I(1024, 768, 128),
	Vector3I(1024, 768, 256),
	Vector3I(1024, 768, 512),
	Vector3I(1024, 768, 768)
};

float g_Scale = 1.0f;	// 4.42f;	// TODO: remove

int g_BackBufferWidth = 1024;
int g_BackBufferHeight = 768;
float g_fAspectRatio = 1.0f;
extern const D3DFORMAT g_VolumeFormat = D3DFMT_A1R5G5B5;
const D3DFORMAT g_SurfaceFormat = D3DFMT_R5G6B5;	//D3DFMT_R16F;	//D3DFMT_R5G6B5; // D3DFMT_A8R8G8B8;
bool g_bVoxelize = false;
bool g_Solid = true;
bool g_AutoFitVoxelizationBox = true;
bool g_DumpSlices = false;
RenderMode Voxelization::renderMode = RenderMode_Polygonal;
D3DVIEWPORT9 Voxelization::viewport;

DepthVoxelizer* g_Voxelizer = NULL;
VolumeRenderer* g_VolumeRenderer = NULL;
VoxelizationDumper g_VoxelizationDumper;

bool g_ConfigVoxelization = true;

//--------------------------------------------------------------------------------------
// Mesh variables
//--------------------------------------------------------------------------------------

std::vector< SkinnedMesh* > g_SkinnedMeshes(2, NULL);

ULONG_PTR g_gdiplusToken;

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
void CenterizeMesh(LPD3DXMESH mesh, D3DXVECTOR3* min, D3DXVECTOR3* max);
void ScaleMesh(LPD3DXMESH mesh, float scale);
void LoadMesh(IDirect3DDevice9* device, int meshIndex, LPCTSTR filename);
void ReconstructVoxelizers(const Vector3I & resolution, D3DFORMAT surfaceFormat, bool solid);

void CALLBACK VoxelizationFrameRender( IDirect3DDevice9* device, double fTime, float fElapsedTime, void* pUserContext )
{
	g_SkinnedMeshes[0]->Render(device);
}

void CALLBACK VoxelizationFrameRender1( IDirect3DDevice9* device, double fTime, float fElapsedTime, void* pUserContext )
{
	stdext::hash_set<int> masks;
	for (int i = 10; i < 20; ++i)
		masks.insert(i);

	g_SkinnedMeshes[0]->SetMaskedSubset(masks);

	g_SkinnedMeshes[0]->Render(device);

	masks.clear();
	g_SkinnedMeshes[0]->SetMaskedSubset(masks);
}

void CALLBACK VoxelizationFrameRender2( IDirect3DDevice9* device, double fTime, float fElapsedTime, void* pUserContext )
{
	stdext::hash_set<int> masks;
	for (int i = 0; i < 10; ++i)
		masks.insert(i);

	g_SkinnedMeshes[0]->SetMaskedSubset(masks);

	
	g_SkinnedMeshes[0]->Render(device);

	masks.clear();
	g_SkinnedMeshes[0]->SetMaskedSubset(masks);
}

//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);	// Initialize GDI+.

	g_dwShaderFlags = D3DXFX_NOT_CLONEABLE | D3DXFX_LARGEADDRESSAWARE;

#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3DXSHADER_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    g_dwShaderFlags |= D3DXSHADER_DEBUG;
    #endif

#ifdef DEBUG_VS
        g_dwShaderFlags |= D3DXSHADER_FORCE_VS_SOFTWARE_NOOPT;
    #endif
#ifdef DEBUG_PS
        g_dwShaderFlags |= D3DXSHADER_FORCE_PS_SOFTWARE_NOOPT;
    #endif

    g_dwShaderFlags |= D3DXSHADER_NO_PRESHADER;

    g_SettingsDlg.Init(&g_DialogResourceManager);
	g_HUD.Init(&g_DialogResourceManager);
    g_SampleUI.Init(&g_DialogResourceManager);
	g_ConfigureTransferFunctionDlg.Init(&g_DialogResourceManager);

	int iY = 10;

    g_HUD.SetCallback(OnGUIEvent);
    g_HUD.AddButton(IDC_TOGGLE_FULLSCREEN, L"Toggle full screen", 35, iY, 125, 22);
    g_HUD.AddButton(IDC_TOGGLE_REF, L"Toggle REF (F3)", 35, iY += 24, 125, 22, VK_F3);
    g_HUD.AddButton(IDC_CHANGE_DEVICE, L"Change device (F2)", 35, iY += 24, 125, 22, VK_F2);
	g_HUD.AddButton(IDC_CONFIG_VOXELIZATION, L"Config Voxelization", 35, iY += 24, 125, 22);
	g_HUD.AddButton(IDC_CONFIG_TRANSFER_FUNCTION, L"Config Transfer Function", 35, iY += 24, 125, 22);

	const int CONTROLS_LEFT = 30;

	iY = 0;
	g_SampleUI.AddButton(IDC_TOGGLE_RENDER_MODE, L"Toggle Render Mode (M)", CONTROLS_LEFT, iY += 24, 125, 22, 'M');
	
	g_SampleUI.AddComboBox(IDC_SELECT_MESH, CONTROLS_LEFT, iY += 48, 125, 22, 0, false, &g_comboBoxSelectMesh);
	g_comboBoxSelectMesh->AddItem(L"Tiger", L"Tiger.x");
	g_comboBoxSelectMesh->AddItem(L"Water Tight Sphere", L"WaterTightSphere.x");
	g_comboBoxSelectMesh->AddItem(L"Incomplete Sphere", L"IncompleteSphere.x");
	g_comboBoxSelectMesh->AddItem(L"Tiny", L"Tiny.x");
	g_comboBoxSelectMesh->AddItem(L"Monkey", L"Monkey.x");
	g_comboBoxSelectMesh->AddItem(L"Dragon2", L"Dragon2.x");
	g_comboBoxSelectMesh->AddItem(L"dragon_res4", L"dragon_vrip_res4_bin.x");
	g_comboBoxSelectMesh->AddItem(L"dragon_res3", L"dragon_vrip_res3_bin.x");
	g_comboBoxSelectMesh->AddItem(L"dragon_res2", L"dragon_vrip_res2_bin.x");
	g_comboBoxSelectMesh->AddItem(L"dragon", L"dragon_vrip_bin.x");
	g_comboBoxSelectMesh->AddItem(L"FuelValve", L"FuelValve.x");
	g_comboBoxSelectMesh->SetSelectedByIndex(1);

	WCHAR buffer[32];
	g_SampleUI.AddComboBox(IDC_SELECT_RESOLUTION, CONTROLS_LEFT, iY += 24, 125, 22, 0, false, &g_comboBoxSelectVoxelizationFormat);
	int formatsCount = sizeof(g_VoxelizationFormat)/sizeof(g_VoxelizationFormat[0]);
	for (int i = 0; i < formatsCount; ++i)
	{
		Vector3I *v = &g_VoxelizationFormat[i];
		swprintf_s(buffer, L"%dx%dx%d", v->x, v->y, v->z);
		g_comboBoxSelectVoxelizationFormat->AddItem(buffer, v);
	}
	g_comboBoxSelectVoxelizationFormat->SetSelectedByIndex(8);

	g_SampleUI.AddCheckBox(IDC_AUTO_FIT_VOXELIZATION_BOX, L"Auto Fit Voxelization Box", CONTROLS_LEFT, iY += 24, 150, 22, g_AutoFitVoxelizationBox);
	g_SampleUI.AddButton(IDC_VOXELIZE, L"Voxelize (V)", CONTROLS_LEFT, iY += 48, 125, 22, 'V');
	g_SampleUI.AddCheckBox(IDC_DUMP_SLICES, L"Save Slices", CONTROLS_LEFT, iY += 24, 125, 22, g_DumpSlices);
	g_SampleUI.AddButton(IDC_SAVE_VOXELIZATION, L"Save Voxelization", CONTROLS_LEFT, iY += 48, 125, 22);

	g_SampleUI.AddSlider(IDC_ADJUST_VOXEL_TRANSPARENCY, CONTROLS_LEFT, iY += 24 , 125, 22, 0, 100, 20);

    g_SampleUI.SetCallback(OnGUIEvent);

	int idx = g_comboBoxSelectVoxelizationFormat->GetSelectedIndex();
	ReconstructVoxelizers(g_VoxelizationFormat[idx], g_SurfaceFormat, g_Solid);
}

void CALLBACK ExitApp(void* userContext)
{
	for (size_t i = 0; i < g_SkinnedMeshes.size(); ++i)
		SAFE_DELETE(g_SkinnedMeshes[i]);

	SAFE_DELETE(g_VolumeRenderer);
	SAFE_DELETE(g_Voxelizer);

	Gdiplus::GdiplusShutdown(g_gdiplusToken);
}

//--------------------------------------------------------------------------------------
// Render the help and statistics text. This function uses the ID3DXFont interface for 
// efficient text rendering.
//--------------------------------------------------------------------------------------
void RenderText()
{
	WCHAR buffer[64];

    g_pTxtHelper->Begin();
    g_pTxtHelper->SetInsertionPos( 5, 5 );
    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
    g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );

	g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );

	for (size_t i = 0; i < g_SkinnedMeshes.size(); ++i)
	{
		if (NULL == g_SkinnedMeshes[i])
			continue;

		const SkinnedMesh * sm = g_SkinnedMeshes[i];

		const D3DXVECTOR3 &c = sm->GetCenter();
		swprintf_s(buffer, L"center(%.3f, %.3f, %.3f), Radius(%.3f)", c.x, c.y, c.z, sm->GetRadius());
		g_pTxtHelper->DrawTextLine(buffer);

		swprintf_s(buffer, L"vertices: %d, faces: %d", sm->GetVerticesCount(), sm->GetFacesCount());
		g_pTxtHelper->DrawTextLine(buffer);
	}

	if (g_VoxelizationTime != 0.0f)
	{
		g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 0.0f, 0.0f, 1.0f ) );
		swprintf_s(buffer, L"Voxelization Time: %0.2fms", 1000.0f * g_VoxelizationTime);
		g_pTxtHelper->DrawTextLine(buffer);
	}

    g_pTxtHelper->End();
}

//--------------------------------------------------------------------------------------
// Returns true if a particular depth-stencil format can be created and used with
// an adapter format and backbuffer format combination.
BOOL IsDepthFormatOk( D3DFORMAT DepthFormat,
                      D3DFORMAT AdapterFormat,
                      D3DFORMAT BackBufferFormat )
{
    // Verify that the depth format exists
    HRESULT hr = DXUTGetD3D9Object()->CheckDeviceFormat( D3DADAPTER_DEFAULT,
                                                         D3DDEVTYPE_HAL,
                                                         AdapterFormat,
                                                         D3DUSAGE_DEPTHSTENCIL,
                                                         D3DRTYPE_SURFACE,
                                                         DepthFormat );
    if( FAILED( hr ) ) return FALSE;

    // Verify that the backbuffer format is valid
    hr = DXUTGetD3D9Object()->CheckDeviceFormat( D3DADAPTER_DEFAULT,
                                                 D3DDEVTYPE_HAL,
                                                 AdapterFormat,
                                                 D3DUSAGE_RENDERTARGET,
                                                 D3DRTYPE_SURFACE,
                                                 BackBufferFormat );
    if( FAILED( hr ) ) return FALSE;

    // Verify that the depth format is compatible
    hr = DXUTGetD3D9Object()->CheckDepthStencilMatch( D3DADAPTER_DEFAULT,
                                                      D3DDEVTYPE_HAL,
                                                      AdapterFormat,
                                                      BackBufferFormat,
                                                      DepthFormat );

    return SUCCEEDED( hr );
}


//--------------------------------------------------------------------------------------
// Rejects any D3D9 devices that aren't acceptable to the app by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D9DeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat,
                                      D3DFORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    // Skip backbuffer formats that don't support alpha blending
    IDirect3D9* pD3D = DXUTGetD3D9Object();
    if( FAILED( pD3D->CheckDeviceFormat( pCaps->AdapterOrdinal, pCaps->DeviceType,
                                         AdapterFormat, D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING,
                                         D3DRTYPE_TEXTURE, BackBufferFormat ) ) )
        return false;

    // Must support pixel shader 2.0
    if( pCaps->PixelShaderVersion < D3DPS_VERSION( 2, 0 ) )
        return false;

    // Must support stencil buffer
    if( !IsDepthFormatOk( D3DFMT_D24S8,
                          AdapterFormat,
                          BackBufferFormat ) &&
        !IsDepthFormatOk( D3DFMT_D24X4S4,
                          AdapterFormat,
                          BackBufferFormat ) &&
        !IsDepthFormatOk( D3DFMT_D15S1,
                          AdapterFormat,
                          BackBufferFormat ) &&
        !IsDepthFormatOk( D3DFMT_D24FS8,
                          AdapterFormat,
                          BackBufferFormat ) )
        return false;

    return true;
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D10 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    if( pDeviceSettings->ver == DXUT_D3D9_DEVICE )
    {
        IDirect3D9* pD3D = DXUTGetD3D9Object();
        D3DCAPS9 Caps;
        pD3D->GetDeviceCaps( pDeviceSettings->d3d9.AdapterOrdinal, pDeviceSettings->d3d9.DeviceType, &Caps );

        // If device doesn't support HW T&L or doesn't support 1.1 vertex shaders in HW 
        // then switch to SWVP.
        if( ( Caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT ) == 0 ||
            Caps.VertexShaderVersion < D3DVS_VERSION( 1, 1 ) )
        {
            pDeviceSettings->d3d9.BehaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
        }

        // Debugging vertex shaders requires either REF or software vertex processing 
        // and debugging pixel shaders requires REF.  
#ifdef DEBUG_VS
        if( pDeviceSettings->d3d9.DeviceType != D3DDEVTYPE_REF )
        {
            pDeviceSettings->d3d9.BehaviorFlags &= ~D3DCREATE_HARDWARE_VERTEXPROCESSING;
            pDeviceSettings->d3d9.BehaviorFlags &= ~D3DCREATE_PUREDEVICE;
            pDeviceSettings->d3d9.BehaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
        }
#endif
#ifdef DEBUG_PS
        pDeviceSettings->d3d9.DeviceType = D3DDEVTYPE_REF;
#endif
    }

	// This sample requires a stencil buffer.
    if( IsDepthFormatOk( D3DFMT_D24S8,
                         pDeviceSettings->d3d9.AdapterFormat,
                         pDeviceSettings->d3d9.pp.BackBufferFormat ) )
        pDeviceSettings->d3d9.pp.AutoDepthStencilFormat = D3DFMT_D24S8;
    else if( IsDepthFormatOk( D3DFMT_D24X4S4,
                              pDeviceSettings->d3d9.AdapterFormat,
                              pDeviceSettings->d3d9.pp.BackBufferFormat ) )
        pDeviceSettings->d3d9.pp.AutoDepthStencilFormat = D3DFMT_D24X4S4;
    else if( IsDepthFormatOk( D3DFMT_D24FS8,
                              pDeviceSettings->d3d9.AdapterFormat,
                              pDeviceSettings->d3d9.pp.BackBufferFormat ) )
        pDeviceSettings->d3d9.pp.AutoDepthStencilFormat = D3DFMT_D24FS8;
    else if( IsDepthFormatOk( D3DFMT_D15S1,
                              pDeviceSettings->d3d9.AdapterFormat,
                              pDeviceSettings->d3d9.pp.BackBufferFormat ) )
        pDeviceSettings->d3d9.pp.AutoDepthStencilFormat = D3DFMT_D15S1;

    // For the first device created if its a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        s_bFirstTime = false;
        if( ( DXUT_D3D9_DEVICE == pDeviceSettings->ver && pDeviceSettings->d3d9.DeviceType == D3DDEVTYPE_REF ) ||
            ( DXUT_D3D10_DEVICE == pDeviceSettings->ver &&
              pDeviceSettings->d3d10.DriverType == D3D10_DRIVER_TYPE_REFERENCE ) )
            DXUTDisplaySwitchingToREFWarning( pDeviceSettings->ver );
    }

    return true;
}


//--------------------------------------------------------------------------------------
// Create any D3D9 resources that will live through a device reset (D3DPOOL_MANAGED)
// and aren't tied to the back buffer size
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D9CreateDevice( IDirect3DDevice9* device, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
                                     void* pUserContext )
{
    HRESULT hr;

	g_BackBufferWidth = pBackBufferSurfaceDesc->Width;
	g_BackBufferHeight = pBackBufferSurfaceDesc->Height;

    V_RETURN( g_DialogResourceManager.OnD3D9CreateDevice( device ) );
    V_RETURN( g_SettingsDlg.OnD3D9CreateDevice( device ) );
	g_ConfigureTransferFunctionDlg.OnCreateDevice(device);

    V_RETURN( D3DXCreateFont( device, 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET,
                              OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                              L"Arial", &g_pFont9 ) );

	ID3DXBuffer *buffer = NULL;
	// Read the D3DX effect file
	V( D3DXCreateEffectFromFile(device, L"RenderScene.fxo", NULL, NULL, g_dwShaderFlags, NULL, &g_Effect, &buffer) );
	if (NULL !=buffer)
	{
		LPCSTR str = (LPCSTR)buffer->GetBufferPointer();
		UNREFERENCED_PARAMETER(str);
		buffer->Release();
	}

	g_TechRenderScene = g_Effect->GetTechniqueByName("RenderScene");
    g_ParamWorldViewProj = g_Effect->GetParameterByName(NULL, "g_mWorldViewProjection");
    g_ParamWorld = g_Effect->GetParameterByName(NULL, "g_mWorld");
	g_ParamMeshTexture = g_Effect->GetParameterByName(NULL, "g_MeshTexture");

// 	LoadMesh(device, 1, L"WaterTightSphere.x");

	LPCTSTR xfilename = (LPCTSTR)g_comboBoxSelectMesh->GetSelectedData();
	LoadMesh(device, 0, xfilename);

	if (NULL != g_Voxelizer)
		g_Voxelizer->OnCreateDevice(device);

	if (NULL != g_VolumeRenderer)
		g_VolumeRenderer->OnCreateDevice(device, g_BackBufferWidth, g_BackBufferHeight);

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D9 resources that won't live through a device reset (D3DPOOL_DEFAULT) 
// or that are tied to the back buffer size
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D9ResetDevice( IDirect3DDevice9* device,
                                    const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnD3D9ResetDevice() );
    V_RETURN( g_SettingsDlg.OnD3D9ResetDevice() );
	g_ConfigureTransferFunctionDlg.OnResetDevice(pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height);

    V_RETURN( g_pFont9->OnResetDevice() );
	V_RETURN( g_Effect->OnResetDevice() );

    V_RETURN( D3DXCreateSprite( device, &g_pSprite9 ) );
    g_pTxtHelper = new CDXUTTextHelper( g_pFont9, g_pSprite9, NULL, NULL, 15 );

	g_fAspectRatio = (float)pBackBufferSurfaceDesc->Width / (float)pBackBufferSurfaceDesc->Height;
	D3DXMatrixPerspectiveFovLH(&g_matProj, 0.25f * D3DX_PI, g_fAspectRatio, 0.1f, g_SkinnedMeshes[0]->GetRadius() * 200.0f );
	device->SetTransform(D3DTS_PROJECTION, &g_matProj);

    g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - 170, 0 );
    g_HUD.SetSize( 170, 170 );
    g_SampleUI.SetLocation( pBackBufferSurfaceDesc->Width - 170, 200 );
    g_SampleUI.SetSize( 170, 300 );

	if (NULL != g_Voxelizer)
		g_Voxelizer->OnResetDevice(device);

	if (NULL != g_VolumeRenderer)
		g_VolumeRenderer->OnResetDevice(device);

	for (size_t i = 0; i < g_SkinnedMeshes.size(); ++i)
		if (NULL != g_SkinnedMeshes[i])
			g_SkinnedMeshes[i]->RestoreDeviceObjects(device);

	g_Camera.SetWindow(pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height, 0.85f);

	const SkinnedMesh * sm = g_SkinnedMeshes[0];

	const D3DXVECTOR3 & vCenter = sm->GetCenter();
	D3DXVECTOR3 vEye(vCenter.x, vCenter.y, vCenter.z - 5 * sm->GetRadius());
	D3DXVECTOR3 vAt(vCenter);
    D3DXVECTOR3 vUp(0.0f, 1.0f, 0.0f);

	g_Camera.SetViewParams(&vEye, &vAt);

	// Set Colors and Lights
	D3DXVECTOR4 value;
	D3DXHANDLE param;
	// Set Material's ambient color
	param = g_Effect->GetParameterByName(NULL, "g_MaterialAmbientColor");
	value.x = 0.3f, value.y = 0.3f, value.z = 0.3f, value.w = 0.0f;
	g_Effect->SetVector(param, &value);
	// Set Material's diffuse color
	param = g_Effect->GetParameterByName(NULL, "g_MaterialDiffuseColor");
	value.x = 1.0f, value.y = 1.0f, value.z = 1.0f, value.w = 0.0f;
	g_Effect->SetVector(param, &value);
	// Set Light's direction in world space
	param = g_Effect->GetParameterByName(NULL, "g_LightDir");
	value.x = -1.0f, value.y = -1.0f, value.z = -0.3f, value.w = 0.0f;
	g_Effect->SetVector(param, &value);
	// Set Light's diffuse color
	param = g_Effect->GetParameterByName(NULL, "g_LightDiffuse");
	value.x = 1.0f, value.y = 1.0f, value.z = 1.0f, value.w = 0.0f;
	g_Effect->SetVector(param, &value);	

	// Setup the light
	D3DLIGHT9 light;
	D3DXVECTOR3 vecLightDirUnnormalized( 0.0f, -1.0f, 1.0f );
	ZeroMemory( &light, sizeof( D3DLIGHT9 ) );
	light.Type = D3DLIGHT_DIRECTIONAL;
	light.Diffuse.r = 1.0f;
	light.Diffuse.g = 1.0f;
	light.Diffuse.b = 1.0f;
	D3DXVec3Normalize( ( D3DXVECTOR3* )&light.Direction, &vecLightDirUnnormalized );
	light.Position.x = 0.5f;
	light.Position.y = -1.0f;
	light.Position.z = 1.0f;
	light.Range = 1000.0f;
	V( device->SetLight( 0, &light ) );
	V( device->LightEnable( 0, TRUE ) );

	D3DVIEWPORT9 viewport;
	device->GetViewport(&viewport);
	Voxelization::SetViewport(viewport);

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
	g_Camera.FrameMove(fElapsedTime);

	for (size_t i = 0; i < g_SkinnedMeshes.size(); ++i)
		if (NULL != g_SkinnedMeshes[i])
			g_SkinnedMeshes[i]->Update(fTime, fElapsedTime);
}

//--------------------------------------------------------------------------------------
// Render the scene using the D3D9 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D9FrameRender(IDirect3DDevice9* device, double fTime, float fElapsedTime, void* pUserContext)
{
    HRESULT hr;

	const D3DVIEWPORT9 & viewport = Voxelization::GetViewport();
	device->SetViewport(&viewport);

    // If the settings dialog is being shown, then render it instead of rendering the app's scene
    if( g_SettingsDlg.IsActive() )
    {
        g_SettingsDlg.OnRender( fElapsedTime );
        return;
    }

	g_matWorld = *g_Camera.GetWorldMatrix();
	D3DXMATRIX scaling;
	D3DXMatrixScaling(&scaling, g_Scale, g_Scale, g_Scale);
	g_matWorld *= scaling;
	g_matView = *g_Camera.GetViewMatrix();

	if (g_bVoxelize)
	{
		g_bVoxelize = false;

		static CDXUTTimer gtimer;

		std::vector< VoxelizationNode * > nodes;
		g_VolumeRenderer->GetVoxelizationNodes(nodes);

		AABB aabb;
		if (g_AutoFitVoxelizationBox)
		{
			g_SkinnedMeshes[0]->ComputeTransformedAABB(g_matWorld, &aabb);
			aabb.Scale(1.1f);

			for (size_t i = 0; i < nodes.size(); ++i)
				nodes[i]->SetAABB(aabb);
			
			//g_SkinnedMeshes[1]->ComputeTransformedAABB(g_matWorld, &aabb);
			//aabb.Scale(1.1f);
			//nodes[1]->SetAABB(aabb);
		}
		else
		{
			const D3DXVECTOR3 &c = g_SkinnedMeshes[0]->GetCenter();
			float r = g_SkinnedMeshes[0]->GetRadius();

			D3DXVECTOR3 _min = D3DXVECTOR3(c.x - r, c.y - r, c.z - r);
			D3DXVECTOR3 _max = D3DXVECTOR3(c.x + r, c.y + r, c.z + r);

			aabb.SetMinMax(_min, _max);
		}

		gtimer.GetElapsedTime();
		g_Voxelizer->Voxelize(device, g_matWorld, nodes);
		g_VoxelizationTime = gtimer.GetElapsedTime();

//#pragma message ("Hack 1")
		//nodes[0]->MergeVolume(nodes[1]);
		//g_VolumeRenderer->HideVoxelizationNode(1);

		Voxelization::SetRenderMode(RenderMode_Volumetric);
	}

	D3DCOLOR clearColor = D3DCOLOR_ARGB( 0, 45, 50, 170 );

	if (RenderMode_Volumetric == Voxelization::GetRenderMode())
	{
		g_VolumeRenderer->Update(device, g_matView, g_matProj);
		clearColor = 0;
	}

    // Clear the render target and the zbuffer 
    V( device->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clearColor, 1.0f, 0 ) );

    // Render the scene
    if( SUCCEEDED( device->BeginScene() ) )
    {
		switch (Voxelization::GetRenderMode())
		{
		case RenderMode_Polygonal:
			{
				device->SetTransform(D3DTS_WORLD, &g_matWorld);
				device->SetTransform(D3DTS_VIEW, &g_matView);
				device->SetRenderState(D3DRS_LIGHTING, TRUE);
				for (size_t i = 0; i < g_SkinnedMeshes.size(); ++i)
					if (NULL != g_SkinnedMeshes[i])
						g_SkinnedMeshes[i]->Render(device);
			}
			break;

		case RenderMode_Volumetric:
			{
				g_VolumeRenderer->Render(device, g_matView, g_matProj);
			}
			break;
		}

        DXUT_BeginPerfEvent(DXUT_PERFEVENTCOLOR, L"HUD / Stats"); // These events are to help PIX identify what the code is doing

        RenderText();
        V( g_HUD.OnRender(fElapsedTime) );

		if (g_ConfigVoxelization)
			V( g_SampleUI.OnRender(fElapsedTime) );

		if (g_ConfigureTransferFunctionDlg.IsActive())
			g_ConfigureTransferFunctionDlg.OnRender(fElapsedTime);

        DXUT_EndPerfEvent();

        V( device->EndScene() );
    }
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext )
{
    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass messages to settings dialog if its active
    if(g_SettingsDlg.IsActive())
    {
        g_SettingsDlg.MsgProc(hWnd, uMsg, wParam, lParam);
        return 0;
    }


	if (g_ConfigureTransferFunctionDlg.IsActive())
	{
		*pbNoFurtherProcessing = g_ConfigureTransferFunctionDlg.MsgProc(hWnd, uMsg, wParam, lParam);
		if( *pbNoFurtherProcessing )
			return 0;
	}

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    *pbNoFurtherProcessing = g_SampleUI.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

	g_Camera.HandleMessages(hWnd, uMsg, wParam, lParam);

	//switch (uMsg)
	//{
	//case WM_MOUSEWHEEL:
	//	int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	//	g_UniformScale += 0.001f * zDelta;
	//	if (g_UniformScale <= 0.1f)
	//		g_UniformScale = 0.1f;
	//	break;
	//}

    return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
	if(bKeyDown)
    {
        switch(nChar)
        {
			case 'R':
				g_UniformScale = 1.0f;
				g_Camera.Reset();
				g_Camera.SetRadius(2 * g_SkinnedMeshes[0]->GetRadius(), g_SkinnedMeshes[0]->GetRadius());
				g_Camera.SetModelCenter(g_SkinnedMeshes[0]->GetCenter());
				break;

			case 'X':
				break;
        }
    }
}


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
    switch( nControlID )
    {
        case IDC_TOGGLE_FULLSCREEN:
            DXUTToggleFullScreen(); 
			break;

        case IDC_TOGGLE_REF:
            DXUTToggleREF(); 
			break;

        case IDC_CHANGE_DEVICE:
            g_SettingsDlg.SetActive(!g_SettingsDlg.IsActive());
			break;

		case IDC_CONFIG_VOXELIZATION:
			g_ConfigVoxelization = !g_ConfigVoxelization;
			break;

		case IDC_CONFIG_TRANSFER_FUNCTION:
			g_ConfigureTransferFunctionDlg.SetActive(!g_ConfigureTransferFunctionDlg.IsActive());
			break;

		case IDC_TOGGLE_RENDER_MODE:
			{
				int mode = (int)Voxelization::GetRenderMode();
				Voxelization::SetRenderMode((RenderMode)((mode + 1) % 2));
			}
			break;

		case IDC_SELECT_MESH:
			{
				IDirect3DDevice9* device = DXUTGetD3D9Device();
				LPCTSTR filename = (LPCTSTR)g_comboBoxSelectMesh->GetSelectedData();
				LoadMesh(device, 0, filename);
				D3DXMatrixPerspectiveFovLH(&g_matProj, 0.25f * D3DX_PI, g_fAspectRatio, 0.1f, g_SkinnedMeshes[0]->GetRadius() * 200.0f );
				device->SetTransform(D3DTS_PROJECTION, &g_matProj);
			}
			break;

		case IDC_SELECT_RESOLUTION:
			{
				int idx = g_comboBoxSelectVoxelizationFormat->GetSelectedIndex();

				if (NULL != g_Voxelizer)
				{
					g_Voxelizer->OnLostDevice();
					g_Voxelizer->OnDestroyDevice();
				}

				if (NULL != g_VolumeRenderer)
				{
					g_VolumeRenderer->OnLostDevice();
					g_VolumeRenderer->OnDestroyDevice();
				}

				ReconstructVoxelizers(g_VoxelizationFormat[idx], g_SurfaceFormat, g_Solid);

				IDirect3DDevice9* device = DXUTGetD3D9Device();

				if (NULL != g_VolumeRenderer)
				{
					g_VolumeRenderer->OnCreateDevice(device, g_BackBufferWidth, g_BackBufferHeight);				
					g_VolumeRenderer->OnResetDevice(device);
				}

				if (NULL != g_Voxelizer)
				{
					g_Voxelizer->OnCreateDevice(device);
					g_Voxelizer->OnResetDevice(device);
				}
			}
			break;

		case IDC_AUTO_FIT_VOXELIZATION_BOX:
			g_AutoFitVoxelizationBox = !g_AutoFitVoxelizationBox;
			break;

		case IDC_VOXELIZE:
			g_bVoxelize = true;
			break;

		case IDC_DUMP_SLICES:
			{
				g_DumpSlices = !g_DumpSlices;

				if (g_DumpSlices)
					g_Voxelizer->AddVoxelizationListener(&g_VoxelizationDumper);
				else
					g_Voxelizer->RemoveVoxelizationListener(&g_VoxelizationDumper);
			}
			break;

		case IDC_SAVE_VOXELIZATION:
			{
				LPCTSTR strModel = (LPCTSTR)g_comboBoxSelectMesh->GetSelectedItem()->strText;
				WCHAR buffer[MAX_PATH];
				
				std::vector< VoxelizationNode * > nodes;
				g_VolumeRenderer->GetVoxelizationNodes(nodes);

				for (size_t i = 0; i < nodes.size(); ++i)
				{
					VoxelizationNode * node = nodes[i];

					swprintf_s(buffer, L"Volumes/%s_%s.dds", strModel, node->GetName().c_str());
					node->SaveVolume(buffer);
				}
			}
			break;

		case IDC_ADJUST_VOXEL_TRANSPARENCY:
			{
				CDXUTSlider * slider = dynamic_cast< CDXUTSlider * >(pControl);
				float voxelTransparency = 0.01f * slider->GetValue();

				std::vector< VoxelizationNode * > nodes;
				g_VolumeRenderer->GetVoxelizationNodes(nodes);

				for (size_t i = 0; i < nodes.size(); ++i)
				{
					VoxelizationNode * node = nodes[i];
					// node->SetVoxelTransparency(voxelTransparency);
				}
			}
			break;
    }
}


//--------------------------------------------------------------------------------------
// Release D3D9 resources created in the OnD3D9ResetDevice callback 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D9LostDevice( void* pUserContext )
{
    g_DialogResourceManager.OnD3D9LostDevice();
    g_SettingsDlg.OnD3D9LostDevice();
	g_ConfigureTransferFunctionDlg.OnLostDevice();

    g_pFont9->OnLostDevice();
	g_Effect->OnLostDevice();
    SAFE_RELEASE( g_pSprite9 );
    SAFE_DELETE( g_pTxtHelper );

	if (NULL != g_Voxelizer)
		g_Voxelizer->OnLostDevice();

	if (NULL != g_VolumeRenderer)
		g_VolumeRenderer->OnLostDevice();

	for (size_t i = 0; i < g_SkinnedMeshes.size(); ++i)
		if (NULL != g_SkinnedMeshes[i])
			g_SkinnedMeshes[i]->InvalidateDeviceObjects();
}


//--------------------------------------------------------------------------------------
// Release D3D9 resources created in the OnD3D9CreateDevice callback 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D9DestroyDevice( void* pUserContext )
{
	if (NULL != g_Voxelizer)
		g_Voxelizer->OnDestroyDevice();

	if (NULL != g_VolumeRenderer)
		g_VolumeRenderer->OnDestroyDevice();

	for (size_t i = 0; i < g_SkinnedMeshes.size(); ++i)
		if (NULL != g_SkinnedMeshes[i])
			g_SkinnedMeshes[i]->Destroy();

    g_DialogResourceManager.OnD3D9DestroyDevice();
    g_SettingsDlg.OnD3D9DestroyDevice();
	g_ConfigureTransferFunctionDlg.OnDestroyDevice();

	SAFE_RELEASE(g_Effect);
    SAFE_RELEASE(g_pFont9);
}

void CenterizeMesh(LPD3DXMESH mesh, D3DXVECTOR3* min, D3DXVECTOR3* max)
{
	D3DXVECTOR3 center = 0.5f * (*min + *max);
	D3DXVECTOR3 *data = NULL;
	DWORD vSize = mesh->GetNumBytesPerVertex();
	DWORD vCount = mesh->GetNumVertices();
	mesh->LockVertexBuffer(0, (void**)&data);

	for (DWORD i = 0; i < vCount; ++i)
	{
		*data -= center;
		data = (D3DXVECTOR3*)((BYTE*)data + vSize);
	}

	mesh->UnlockVertexBuffer();

	*min -= center;
	*max -= center;
}

void ScaleMesh(LPD3DXMESH mesh, float scale)
{
	D3DXVECTOR3 *data = NULL;
	DWORD vSize = mesh->GetNumBytesPerVertex();
	DWORD vCount = mesh->GetNumVertices();
	mesh->LockVertexBuffer(0, (void**)&data);

	for (DWORD i = 0; i < vCount; ++i)
	{
		*data *= scale;
		data = (D3DXVECTOR3*)((BYTE*)data + vSize);
	}

	mesh->UnlockVertexBuffer();
}

void LoadMesh(IDirect3DDevice9* device, int meshIndex, LPCTSTR filename)
{
	SkinnedMesh *sm = g_SkinnedMeshes[meshIndex];

	if (NULL != sm)
	{
		sm->InvalidateDeviceObjects();
		sm->Destroy();
		delete sm;
	}

	g_SkinnedMeshes[meshIndex] = sm = new SkinnedMesh();
	
	WCHAR strPath[MAX_PATH];
	swprintf_s(strPath, L"../../Models/%s", filename);
	sm->Create(device, strPath, true);

	g_UniformScale = 1.0f;
	g_Camera.Reset();
	g_Camera.SetRadius(2 * sm->GetRadius(), sm->GetRadius());
	g_Camera.SetModelCenter(sm->GetCenter());
}

void ReconstructVoxelizers(const Vector3I & resolution, D3DFORMAT surfaceFormat, bool solid)
{
	SAFE_DELETE(g_VolumeRenderer);
	SAFE_DELETE(g_Voxelizer);

	g_VolumeRenderer = new VolumeRenderer();
	g_Voxelizer = new DepthVoxelizer();

	Vector3I res;
	D3DXVECTOR3 _min, _max;
	AABB aabb;
	VoxelizationNode * vnode;

	D3DXVECTOR3 c, e;	// center, extents

	// Head
	res.x = res.y = res.z = 128;
	// res.x = 1024, res.y = 768, res.z = 768;
	c.x = -0.05f; c.y = 0.058f; c.z = 0.002f;
	e.x = 0.101f; e.y = 0.055f; e.z = 0.067f;
	c *= g_Scale;	e *= g_Scale;
	aabb.SetCenterExtents(c, e);
	vnode = g_Voxelizer->CreateVoxelizationNode(res, surfaceFormat);
	vnode->SetName(L"512^3");
	vnode->SetAABB(aabb);
	vnode->SetSolid(true);
	vnode->SetRenderCallback(VoxelizationFrameRender);
	g_VolumeRenderer->AddVoxelizationNode(vnode);
}