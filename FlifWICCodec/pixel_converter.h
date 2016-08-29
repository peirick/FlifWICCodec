#pragma once

void BGRA8ToRGBA8Row(size_t width, void* row);
void BGR8ToRGB8Row(size_t width, void* row);
void BGRX8ToRGB8Row(size_t width, void* row);
void RGBX8ToRGB8Row(size_t width, void* row);
void CopyRGBA8ToRGBA8Row(size_t width, void* sourceRow, void* destRow);