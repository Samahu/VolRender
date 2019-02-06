#pragma once

class CDXUTControl;

LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext );
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext );
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext );
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );
bool CALLBACK IsD3D9DeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, bool bWindowed, void* pUserContext );
HRESULT CALLBACK OnD3D9CreateDevice( IDirect3DDevice9* device, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
HRESULT CALLBACK OnD3D9ResetDevice( IDirect3DDevice9* device, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
void CALLBACK OnD3D9FrameRender( IDirect3DDevice9* device, double fTime, float fElapsedTime, void* pUserContext );
void CALLBACK OnD3D9LostDevice( void* pUserContext );
void CALLBACK OnD3D9DestroyDevice( void* pUserContext );

void InitApp();
void CALLBACK ExitApp(void *userContext);
void RenderText();

extern int g_BackBufferWidth;
extern int g_BackBufferHeight;

enum RenderMode { RenderMode_Polygonal, RenderMode_Volumetric };

class Voxelization
{
private:
	static RenderMode renderMode;
	static D3DVIEWPORT9 viewport;

public:
	static RenderMode GetRenderMode() { return renderMode; }
	static void SetRenderMode(RenderMode value) { renderMode = value; }

	static const D3DVIEWPORT9 & GetViewport() { return viewport; }
	static void SetViewport(const D3DVIEWPORT9 & value) { viewport = value; }
};