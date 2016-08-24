#pragma once

#define PRODUCT_NAME "FLIF Codec for Windows"
#define PRODUCT_COMPANY "Private open source build"

#define FILE_VERSION_MAJOR 0
#define FILE_VERSION_MINOR 1
#define FILE_VERSION_MAJOR_STR "0"
#define FILE_VERSION_MINOR_STR "1"
#define PRODUCT_VERSION_MAJOR 0
#define PRODUCT_VERSION_MINOR 1
#define PRODUCT_VERSION_MAJOR_STR "0"
#define PRODUCT_VERSION_MINOR_STR "1"

#define FILE_VERSION_BUILD 0
#define FILE_VERSION_BUILD_STR "0"
#define PRODUCT_VERSION_BUILD 0
#define PRODUCT_VERSION_BUILD_STR "0"

// Builds with a set build id are considered non-private.
#if FILE_VERSION_BUILD
#define VER_PRIVATE 0
#else
#define VER_PRIVATE VS_FF_PRIVATEBUILD
#endif

#ifdef _DEBUG
#define VER_DEBUG VS_FF_DEBUG
#else
#define VER_DEBUG 0
#endif
