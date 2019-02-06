#pragma once

class D3DObject
{
public:
	virtual ~D3DObject() {}

public: 
	virtual void OnCreateDevice(IDirect3DDevice9* device) = 0;
	virtual void OnResetDevice(IDirect3DDevice9* device) = 0;
	virtual void OnLostDevice() = 0;
	virtual void OnDestroyDevice() = 0;
};