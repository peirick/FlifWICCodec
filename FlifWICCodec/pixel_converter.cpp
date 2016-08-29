#include "pixel_converter.h"

#pragma pack(push, 1)
struct RGB {
	unsigned char r, g, b;
};

struct RGBA {
	unsigned char r, g, b, a;
};

struct BGR {
	unsigned char b, g, r;
};

struct BGRA {
	unsigned char b, g, r, a;
};
#pragma pack(pop)

void BGR8ToRGB8Row(size_t width, void* row)
{
	BGR* src = (BGR*)row;
	RGB* dst = (RGB*)row;
	for (size_t i = 0; i < width; ++i) {
		const BGR temp = src[i];
		dst[i].r = temp.r;
		dst[i].g = temp.g;
		dst[i].b = temp.b;
	}
}

void BGRA8ToRGBA8Row(size_t width, void* row)
{
	BGRA* src = (BGRA*)row;
	RGBA* dst = (RGBA*)row;
	for (size_t i = 0; i < width; ++i) {
		const BGRA temp = src[i];
		dst[i].r = temp.r;
		dst[i].g = temp.g;
		dst[i].b = temp.b;
		dst[i].a = temp.a;
	}
}

void BGRX8ToRGB8Row(size_t width, void* row) {
	BGRA* src = (BGRA*)row;
	RGB* dst = (RGB*)row;
	for (size_t i = 0; i < width; ++i) {
		const BGRA temp = src[i];
		dst[i].r = temp.r;
		dst[i].g = temp.g;
		dst[i].b = temp.b;
	}
}

void RGBX8ToRGB8Row(size_t width, void* row) {
	RGBA* src = (RGBA*)row;
	RGB* dst = (RGB*)row;
	for (size_t i = 0; i < width; ++i) {
		const RGBA temp = src[i];
		dst[i].r = temp.r;
		dst[i].g = temp.g;
		dst[i].b = temp.b;
	}
}

void CopyRGBA8ToRGBA8Row(size_t width, void* sourceRow, void* destRow) {
	RGBA* src = (RGBA*)sourceRow;
	RGBA* dst = (RGBA*)destRow;
	for (size_t i = 0; i < width; ++i) {
		const RGBA temp = src[i];
		//Don't copy fully transparent pixel
		if (temp.a == 0)
			continue;
		dst[i].r = temp.r;
		dst[i].g = temp.g;
		dst[i].b = temp.b;
		dst[i].a = temp.a;
	}
}