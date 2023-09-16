#ifndef CLOUDS_SETTINGS_HLSL
#define CLOUDS_SETTINGS_HLSL 1

#define MAX_CLOUD_SHAPE_MIPS 6
#define CLOUD_WARP_SIZE 4
#define CLOUD_WARP_SIZE_2D 8
#define CLOUD_TRACE_WARP_X 16
#define CLOUD_TRACE_WARP_Y 8
#define CLOUD_SHADOWS_RES 768
#define CLOUD_SHADOWS_WARP_SIZE_XZ 4
#define CLOUD_SHADOWS_WARP_SIZE_Y 4

#define CLOUD_CURL_RES 128

//volume shadows optimization
#define CLOUD_SHADOWS_VOLUME_RES_XZ 256
#define CLOUD_SHADOWS_VOLUME_RES_Y 64
//#define CLOUDS_SHADOW_VOL_TEXEL 128//for additional texture with better quality just around
#define CLOUDS_VOLUME_SHADOWS_AND_STEPS 0//enhanced quality, probably doesn't worth it

#define CLOUDS_HOLE_DOWNSAMPLE 4
#define CLOUDS_HOLE_GEN_RES (CLOUD_SHADOWS_VOLUME_RES_XZ / CLOUDS_HOLE_DOWNSAMPLE)

//#define BASE_WEATHER_SIZE (65536.0)
//#define INV_BASE_WEATHER_SIZE (1./WEATHER_SIZE)
//#define WEATHER_SCALE 1.0
//#define WEATHER_SIZE (BASE_WEATHER_SIZE*WEATHER_SCALE)
//#define INV_WEATHER_SIZE (1./WEATHER_SIZE)

#define CLOUDS_LIGHT_TEXTURE_WIDTH 4//4 gradations should be enough (each ~two kilometers)
#define MAX_CLOUDS_DIST_LIGHT_KM 280.

#define CLOUDS_HAS_EMPTY 0//has empty tiles
#define CLOUDS_NO_EMPTY 1//has no empty tiles, and has no close clouds layer
#define CLOUDS_HAS_CLOSE_LAYER 2//has no empty tiles, and has no close clouds layer
#define CLOUDS_APPLY_COUNT_PS 3

#define CLOUDS_APPLY_COUNT (CLOUDS_APPLY_COUNT_PS*2)
#define CLOUDS_TILE_W 8//matches closest_tiled_dist_packed() bitshift!
#define CLOUDS_TILE_H CLOUDS_TILE_W//matches closest_tiled_dist_packed() bitshift!

#endif