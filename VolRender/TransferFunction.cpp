#include "StdAfx.h"
#include "TransferFunction.h"

TransferFunction::TransferFunction()
: transferValuesTemp(256), transferValues(256)
{
}

void TransferFunction::InsertColorCP(int isoValue, const D3DXVECTOR3& color)
{
	if (isoValue >= 0 && isoValue < 256)
		colorsMap[isoValue] = color;
}

void TransferFunction::InsertColorCP(int isoValue, float r, float g, float b)
{
	if (isoValue >= 0 && isoValue < 256)
		colorsMap[isoValue] = D3DXVECTOR3(r, g, b);
}

void TransferFunction::InsertAlphaCP(int isoValue, float alpha)
{
	if (isoValue >= 0 && isoValue < 256)
		alphasMap[isoValue] = alpha;
}

void TransferFunction::ConstructColor()
{
	std::map<int, D3DXVECTOR3>::const_iterator i, j;

	i = colorsMap.begin();
	if (0 != i->first)
		colorsMap[0] = i->second;

	i = colorsMap.end();
	--i;
	if (255 != i->first)
		colorsMap[255] = i->second;

	int k, steps;
	float s;
	D3DXVECTOR3 p;

	for (i = j = colorsMap.begin(), ++j; j != colorsMap.end(); i = j++)
	{
		steps = j->first - i->first;

		for (k = 0; k <= steps; ++k)
		{
			s = (float)k / (float)steps;
			D3DXVec3Lerp(&p, &i->second, &j->second, s);	// Linear Interpolation
			transferValuesTemp[i->first + k] = D3DXVECTOR4(p, 0);
		}
	}
}

void TransferFunction::ConstructAlpha()
{
	std::map<int, float>::const_iterator i, j;

	int k, steps;
	float s;

	for (i = j = alphasMap.begin(), ++j; j != alphasMap.end(); i = j++)
	{
		steps = j->first - i->first;

		for (k = 0; k <= steps; ++k)
		{
			s = (float)k / (float)steps;
			transferValuesTemp[i->first + k].w = i->second + s * (j->second - i->second);	// Linear Interpolation
		}
	}
}

void TransferFunction::Construct()
{
	ConstructColor();
	ConstructAlpha();

	D3DXVECTOR4 m;
	for (int i = 0; i < 256; ++i)
	{
		m = 255.0f * transferValuesTemp[i];
		transferValues[i] = D3DCOLOR_ARGB((byte)m.w, (byte)m.x, (byte)m.y, (byte)m.z);	// TODO: you may need to perform a round operation before casting.
	}
}