#pragma once

namespace ImageHelper
{
	class ImageEncoder
	{
	public:
		CLSID JPEG;
		CLSID BMP;
		CLSID PNG;

	private:
		static int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);

	public:
		ImageEncoder();
	};

	class GrayScaleColorPalette
	{
		Gdiplus::ColorPalette *ppal;

	public:
		GrayScaleColorPalette();
		~GrayScaleColorPalette();

	public:
		void Set(Gdiplus::Bitmap *bitmap);
	};
}