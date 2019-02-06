#include "StdAfx.h"
#include "ImageHelper.h"

using namespace Gdiplus;
using namespace ImageHelper;

int ImageEncoder::GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
   UINT  num = 0;          // number of image encoders
   UINT  size = 0;         // size of the image encoder array in bytes

   ImageCodecInfo* pImageCodecInfo = NULL;

   GetImageEncodersSize(&num, &size);
   if(size == 0)
	  return -1;  // Failure

   pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
   if(pImageCodecInfo == NULL)
	  return -1;  // Failure

   GetImageEncoders(num, size, pImageCodecInfo);

   for(UINT j = 0; j < num; ++j)
   {
	  if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
	  {
		 *pClsid = pImageCodecInfo[j].Clsid;
		 free(pImageCodecInfo);
		 return j;  // Success
	  }
   }

   free(pImageCodecInfo);
   return -1;  // Failure
}

ImageEncoder::ImageEncoder()
{
	GetEncoderClsid(L"image/jpeg", &JPEG);
	GetEncoderClsid(L"image/bmp", &BMP);
	GetEncoderClsid(L"image/png", &PNG);
}

GrayScaleColorPalette::GrayScaleColorPalette()
{
	// Allocate the new color table.
	int size;
	size = sizeof(ColorPalette);      // Size of core structure.
	size += sizeof(ARGB) * (256 - 1);   // 256 gray level.

	// Allocate some raw space to back the ColorPalette structure pointer.
	ppal = (ColorPalette *)new BYTE[size];
	ZeroMemory(ppal, size);

	// Initialize a new color table with entries that are determined by
	// some optimal palette finding algorithm; for demonstration 
	// purposes, just do a grayscale. 
	ppal->Flags = PaletteFlagsGrayScale;
	ppal->Count = 256;

	for (UINT i = 0; i < ppal->Count; ++i)
	{
		BYTE v = (BYTE)i;
		ppal->Entries[i] = Color::MakeARGB(255, v, v, v);
	}
}

GrayScaleColorPalette::~GrayScaleColorPalette()
{
	// Clean up after yourself.
	delete [] (BYTE*) ppal;
}

void GrayScaleColorPalette::Set(Bitmap *bitmap)
{
	bitmap->SetPalette(ppal);
}