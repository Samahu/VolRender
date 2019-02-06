#include "StdAfx.h"
#include "VoxelizationDumper.h"
#include "ImageHelper.h"

using namespace Gdiplus;

void VoxelizationDumper::OnVoxelizationNewSlice(int sliceIndex, int width, int height, int pitch, void* bits)
{
	Bitmap bitmap(width, height, PixelFormat16bppRGB565);
	// Copy into bitmap
	BitmapData bmpData;
	Rect rc(0, 0, width, height);
	bitmap.LockBits(&rc, ImageLockModeWrite, PixelFormat16bppRGB565, &bmpData);
	assert(bmpData.Stride % bmpData.Width == 0);
	int BytesPerPixel = bmpData.Stride / bmpData.Width;

	BYTE *dst = (BYTE *)bmpData.Scan0;

	assert(pitch % width == 0);
	int SrcBytesPerPixel = pitch / width;

	BYTE *src = (BYTE*)bits;
	
	int index;
	register int row, col;
	for(row = 0; row < height; ++row)
		for(col = 0; col < width; ++col)
		{
			index = row * width + col;
			dst[BytesPerPixel * index] = src[SrcBytesPerPixel * index];	// Consider the first byte of each pixel (pixel triple or quad).
		}

	bitmap.UnlockBits(&bmpData);

	ImageHelper::ImageEncoder imageEncoder;
	WCHAR buffer[32];
	swprintf_s(buffer, L"Slices/imaging_slice_%d.png", sliceIndex);
	bitmap.Save(buffer, &imageEncoder.PNG);
}