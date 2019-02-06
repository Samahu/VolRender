#include "StdAfx.h"
#include "Voxelization.h"

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // DXUT will create and use the best device (either D3D9 or D3D10) 
    // that is available on the system depending on which D3D callbacks are set below

	// Set DXUT callbacks
	DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackKeyboard( OnKeyboard );
    DXUTSetCallbackFrameMove( OnFrameMove );
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
	DXUTSetCallbackShutdown(ExitApp);

    DXUTSetCallbackD3D9DeviceAcceptable( IsD3D9DeviceAcceptable );
    DXUTSetCallbackD3D9DeviceCreated( OnD3D9CreateDevice );
    DXUTSetCallbackD3D9DeviceReset( OnD3D9ResetDevice );
    DXUTSetCallbackD3D9DeviceLost( OnD3D9LostDevice );
    DXUTSetCallbackD3D9DeviceDestroyed( OnD3D9DestroyDevice );
    DXUTSetCallbackD3D9FrameRender( OnD3D9FrameRender );

	// This sample requires a stencil buffer. See the callback function for details.
    CGrowableArray <D3DFORMAT>* pDSFormats = DXUTGetD3D9Enumeration()->GetPossibleDepthStencilFormatList();
    pDSFormats->RemoveAll();
    pDSFormats->Add( D3DFMT_D24S8 );
    pDSFormats->Add( D3DFMT_D24X4S4 );
    pDSFormats->Add( D3DFMT_D15S1 );
    pDSFormats->Add( D3DFMT_D24FS8 );

    InitApp();
    DXUTInit(true, true, NULL); // Parse the command line, show msgboxes on error, no extra command line params
    DXUTSetCursorSettings(true, true);
    
	DXUTCreateWindow(L"Voxelization & Volumetric Rendering Demo");
	DXUTCreateDevice(true, g_BackBufferWidth, g_BackBufferHeight);
    DXUTMainLoop(); // Enter into the DXUT render loop

	return DXUTGetExitCode();
}