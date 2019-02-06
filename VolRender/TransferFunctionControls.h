#pragma once

#include <vector>
#include <list>

class CDXUTAlphaEditBox : public CDXUTControl
{
public:
	CDXUTAlphaEditBox( CDXUTDialog* pDialog = NULL );

	/*override*/ bool CanHaveFocus() { return m_bVisible && m_bEnabled; }
	/*override*/ bool HandleMouse(UINT uMsg, POINT pt, WPARAM wParam, LPARAM lParam);
	/*override*/ void Render(float fElapsedTime);

	HRESULT	GetTextCopy(__out_ecount(bufferCount) LPWSTR strDest, UINT bufferCount);
	LPCWSTR	GetText() { return m_strText; }
	HRESULT	SetText(LPCWSTR strText);

	const std::list< POINT > & GetControlPoints() const { return m_ControlPoints; }

	void TransformControlPoint(POINT *out, const POINT &pt) const
	{
		assert(NULL != out);

		out->x = m_x + pt.x;
		out->y = m_y + m_height - pt.y;
	}

	void UnTransformControlPoint(POINT *out, const POINT &pt) const
	{
		assert(NULL != out);

		out->x = pt.x - m_x;
		out->y = m_y + m_height - pt.y;
	}

	std::list< POINT >::iterator FindPoint(const POINT &pt);

protected:
	bool m_bPressed;
    WCHAR m_strText[MAX_PATH];      // Window text
	std::list< POINT > m_ControlPoints;
	bool m_bDragging;
	std::list< POINT >::iterator m_DraggedPoint;
};

class ConfigureTransferFunctionDlg
{
private:
	bool active;
	CDXUTDialog dialog;
	CDXUTAlphaEditBox* pAlphaEditBox;

public:
	enum
	{
		IDC_ALPHAEDITBOX,
		IDC_COLOREDITBOX,
		IDC_APPLY
	};

private:
	static void WINAPI  StaticOnEvent(UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserData);
	void OnEvent(UINT nEvent, int nControlID, CDXUTControl* pControl);

public:
	bool IsActive() { return active; }
	void SetActive(bool value) { active = value; }
	void ApplyTransferFunction();

public:
	ConfigureTransferFunctionDlg();

	HRESULT Init(CDXUTDialogResourceManager* manager);
	void OnRender(float elapsedTime);

	void OnCreateDevice(IDirect3DDevice9* device);
    void OnResetDevice(int backBufferWidth, int backBufferHeight);
    void OnLostDevice();
    void OnDestroyDevice();

	bool MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void SetCSpace(class CSpaceDX *cspaceDX);
};