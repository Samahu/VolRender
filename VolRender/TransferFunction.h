#pragma once

#include <vector>
#include <map>

class TransferFunction
{
private:
	std::map<int, D3DXVECTOR3> colorsMap;
	std::map<int, float> alphasMap;

	std::vector< D3DXVECTOR4 > transferValuesTemp;
	std::vector< DWORD > transferValues;

private:
	void ConstructColor();
	void ConstructAlpha();

public:
	// Constructor
	TransferFunction();

	void InsertColorCP(int isoValue, const D3DXVECTOR3& color);
	void InsertColorCP(int isoValue, float r, float g, float b);
	void InsertAlphaCP(int isoValue, float alpha);

	void Construct();
	const std::vector< DWORD > GetTransferValues() const { return transferValues; }
};