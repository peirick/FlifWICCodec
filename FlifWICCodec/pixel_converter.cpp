#include "pixel_converter.h"

#pragma pack(push, 1)
struct RGBA {
	unsigned char r, g, b, a;
};
#pragma pack(pop)

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