#include "StdAfx.h"
#include "TransferFunctionControls.h"
#include "TransferFunction.h"
#include "VoxelizationNode.h"
#include "VolumeRenderer.h"

extern VolumeRenderer* g_VolumeRenderer;

//--------------------------------------------------------------------------------------
// CDXUTStatic class
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
CDXUTAlphaEditBox::CDXUTAlphaEditBox( CDXUTDialog* pDialog )
: m_bPressed(false), m_ControlPoints(), m_bDragging(false), m_DraggedPoint(m_ControlPoints.end())
{
    m_Type = DXUT_CONTROL_BUTTON;
    m_pDialog = pDialog;

    ZeroMemory( &m_strText, sizeof( m_strText ) );

    for( int i = 0; i < m_Elements.GetSize(); i++ )
    {
        CDXUTElement* pElement = m_Elements.GetAt( i );
        SAFE_DELETE( pElement );
    }

    m_Elements.RemoveAll();

	POINT pt;

	pt.x = 0, pt.y = 0;
	m_ControlPoints.push_back(pt);
	pt.x = 255, pt.y = 0;
	m_ControlPoints.push_back(pt);
}

//--------------------------------------------------------------------------------------
void CDXUTAlphaEditBox::Render( float fElapsedTime )
{
    if( m_bVisible == false )
        return;

	// Draw the background
	m_pDialog->DrawRect(&m_rcBoundingBox, D3DCOLOR_RGBA(255, 255, 255, 125));
	
	std::vector< POINT > controlPointsArray;
	controlPointsArray.resize(m_ControlPoints.size());
	int ctr = 0;
	POINT pt;
	RECT rc;

	std::list< POINT >::const_iterator i;
	for (i = m_ControlPoints.begin(); i != m_ControlPoints.end(); ++i)
	{
		TransformControlPoint(&pt, *i);

		SetRect(&rc, pt.x - 2, pt.y - 2, pt.x + 2, pt.y + 2);
		m_pDialog->DrawRect(&rc, D3DCOLOR_RGBA(0, 0, 0, 255));
		controlPointsArray[ctr++] = pt;
	}

	m_pDialog->DrawPolyLine(&controlPointsArray[0], (int)controlPointsArray.size(), D3DCOLOR_RGBA(0, 0, 0, 255));


    DXUT_CONTROL_STATE iState = DXUT_STATE_NORMAL;

    if( m_bEnabled == false )
        iState = DXUT_STATE_DISABLED;

    CDXUTElement* pElement = m_Elements.GetAt( 0 );

    pElement->FontColor.Blend( iState, fElapsedTime );

    m_pDialog->DrawText( m_strText, pElement, &m_rcBoundingBox, true );
}

//--------------------------------------------------------------------------------------
HRESULT CDXUTAlphaEditBox::GetTextCopy( __out_ecount(bufferCount) LPWSTR strDest, 
                                  UINT bufferCount )
{
    // Validate incoming parameters
    if( strDest == NULL || bufferCount == 0 )
        return E_INVALIDARG;

    // Copy the window text
    wcscpy_s( strDest, bufferCount, m_strText );

    return S_OK;
}

//--------------------------------------------------------------------------------------
HRESULT CDXUTAlphaEditBox::SetText( LPCWSTR strText )
{
    if( strText == NULL )
    {
        m_strText[0] = 0;
        return S_OK;
    }

    wcscpy_s( m_strText, MAX_PATH, strText );
    return S_OK;
}

//--------------------------------------------------------------------------------------
std::list< POINT >::iterator CDXUTAlphaEditBox::FindPoint(const POINT &pt)
{
	RECT rc;
	POINT p;
	std::list< POINT >::iterator i;

	for (i = m_ControlPoints.begin(); i != m_ControlPoints.end(); ++i)
	{
		p = *i;
		SetRect(&rc, p.x - 2, p.y - 2, p.x + 2, p.y + 2);
		if (PtInRect(&rc, pt))
			return i;
	}

	return m_ControlPoints.end();
}

//--------------------------------------------------------------------------------------
bool CDXUTAlphaEditBox::HandleMouse(UINT uMsg, POINT pt, WPARAM wParam, LPARAM lParam)
{
    if( !m_bEnabled || !m_bVisible )
        return false;

	if (!ContainsPoint(pt))
		return false;

	if( !m_bHasFocus )
        m_pDialog->RequestFocus( this );

    switch( uMsg )
    {
    case WM_LBUTTONDOWN:
        {
            // Pressed while inside the control
            SetCapture(DXUTGetHWND());
			m_bPressed = true;

			POINT out;
			UnTransformControlPoint(&out, pt);

			std::list< POINT >::iterator i = FindPoint(out);

			if (i == m_ControlPoints.end())	// No match was found
			{
				for (i = m_ControlPoints.begin(); i != m_ControlPoints.end() && out.x > i->x; ++i)
					;

				if (i->x == out.x)
					i->y = out.y;
				else
					m_ControlPoints.insert(i, out);

				m_pDialog->SendEvent(ConfigureTransferFunctionDlg::IDC_APPLY, false, this);
			}
			else
			{
				// Drag the point around.
				m_bDragging = true;
				m_DraggedPoint = i;
			}

            return true;
        }
		break;

	case WM_MOUSEMOVE:
		{
			if (m_bDragging && m_DraggedPoint != m_ControlPoints.end())
			{
				POINT out;

				UnTransformControlPoint(&out, pt);

				if (m_DraggedPoint->x == 0 || m_DraggedPoint->x == 255)
					m_DraggedPoint->y = out.y;	// Restrict movement only to y value
				else
				{
					// it is an in-between point, make sure the user no be able to by pass its neighbours.
					std::list< POINT >::const_iterator left, right;
					left = right = m_DraggedPoint;
					--left, ++right;

					if (out.x <= left->x)
						out.x = left->x + 1;
					else if (out.x >= right->x)
						out.x = right->x - 1;

					*m_DraggedPoint = out;
				}

				m_pDialog->SendEvent(ConfigureTransferFunctionDlg::IDC_APPLY, false, this);
			}
		}
		break;

    case WM_LBUTTONUP:
		{
			if (m_bDragging)
			{
				m_bDragging = false;
				m_DraggedPoint = m_ControlPoints.end();
			}

			if( m_bPressed )
			{
			    m_bPressed = false;
			    ReleaseCapture();

				if( !m_pDialog->m_bKeyboardInput )
					m_pDialog->ClearFocus();
			}

			return true;
		}
		break;

	case WM_RBUTTONDOWN:
		{
			POINT out;
			UnTransformControlPoint(&out, pt);

			std::list< POINT >::const_iterator i = FindPoint(out);

			if (i != m_ControlPoints.end())
			{
				POINT p = *i;

				// Don't erase first and last points.
				if (p.x != 0 && p.x != 255)
				{
					m_ControlPoints.erase(i);

					m_pDialog->SendEvent(ConfigureTransferFunctionDlg::IDC_APPLY, false, this);
				}
			}
		}
		break;
    };

    return false;
}

//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------

ConfigureTransferFunctionDlg::ConfigureTransferFunctionDlg()
: active(false), dialog(), pAlphaEditBox(NULL)
{
}

HRESULT ConfigureTransferFunctionDlg::Init(CDXUTDialogResourceManager* manager)
{
	dialog.Init(manager);
	dialog.SetCallback(StaticOnEvent, this);
	dialog.EnableNonUserEvents(true);

	const int CONTROL_WIDTH = 100;
	const int CONTROL_HEIGHT = 20;
	const int CONTROLS_SPACING = 24;
	int iY = 10;

	CDXUTElement element;
	element.SetFont(0, D3DCOLOR_ARGB( 255, 255, 255, 255 ), DT_LEFT | DT_TOP);
    element.FontColor.States[ DXUT_STATE_DISABLED ] = D3DCOLOR_RGBA( 200, 200, 200, 200 );

	// Add the alpha edit box
    pAlphaEditBox = new CDXUTAlphaEditBox(&dialog);

    if( pAlphaEditBox == NULL )
        return E_OUTOFMEMORY;

	HRESULT hr;

    hr = dialog.AddControl(pAlphaEditBox);
    if( FAILED(hr) )
        return hr;

    // Set the ID and list index
    pAlphaEditBox->SetID(IDC_ALPHAEDITBOX);
    pAlphaEditBox->SetText(L"Edit Alpha Interpolation");
    pAlphaEditBox->SetLocation(10, iY);
    pAlphaEditBox->SetSize(256, 100);
    pAlphaEditBox->m_bIsDefault = false;

	pAlphaEditBox->SetElement(0, &element);

	iY += pAlphaEditBox->m_height + CONTROLS_SPACING;
	dialog.AddButton(IDC_APPLY, L"Apply", 10, iY, CONTROL_WIDTH, CONTROL_HEIGHT);

    return S_OK;
}

void ConfigureTransferFunctionDlg::StaticOnEvent(UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserData)
{
	ConfigureTransferFunctionDlg* ccDialog = static_cast< ConfigureTransferFunctionDlg * >(pUserData);

    if(ccDialog)
        ccDialog->OnEvent(nEvent, nControlID, pControl);
}

void ConfigureTransferFunctionDlg::OnRender(float elapsedTime)
{
	dialog.OnRender(elapsedTime);
}

void ConfigureTransferFunctionDlg::OnCreateDevice(IDirect3DDevice9* device)
{
}

void ConfigureTransferFunctionDlg::OnResetDevice(int backBufferWidth, int backBufferHeight)
{
	dialog.SetLocation( 10, 300 );
    dialog.SetSize( backBufferWidth, 400 );
}

void ConfigureTransferFunctionDlg::OnLostDevice()
{
}

void ConfigureTransferFunctionDlg::OnDestroyDevice()
{
}

bool ConfigureTransferFunctionDlg::MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return dialog.MsgProc(hWnd, uMsg, wParam, lParam);
}

void ConfigureTransferFunctionDlg::ApplyTransferFunction()
{
	TransferFunction tf;

	tf.InsertColorCP(0, 0.0f, 1.0f, 0.0f);
	// tf.InsertColorCP(1, 0.0f, 1.0f, 0.0f);
	tf.InsertColorCP(255, 0.0f, 1.0f, 0.0f);

	const std::list< POINT > & lst = pAlphaEditBox->GetControlPoints();
	std::list< POINT >::const_iterator i;

	for (i = lst.begin(); i != lst.end(); ++i)
		tf.InsertAlphaCP(i->x, i->y / 100.0f);

	tf.Construct();

	// Apply the voxelization nodes.
	std::vector< VoxelizationNode * > nodes;
	g_VolumeRenderer->GetVoxelizationNodes(nodes);
	nodes[0]->SetTransferFunction(tf.GetTransferValues());
}

void ConfigureTransferFunctionDlg::OnEvent(UINT nEvent, int nControlID, CDXUTControl * pControl)
{
	switch(nControlID)
	{
	case IDC_ALPHAEDITBOX:
		if (nEvent == IDC_APPLY)
			ApplyTransferFunction();
		break;

	case IDC_APPLY:
		ApplyTransferFunction();
		break;
	}
}