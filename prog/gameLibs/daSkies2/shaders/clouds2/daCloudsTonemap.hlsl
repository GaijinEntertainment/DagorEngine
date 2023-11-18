#ifndef DACLOUDS_TONEMAP_HLSL
#define DACLOUDS_TONEMAP_HLSL  1

#define TAA_IN_HDR_SPACE 0
#define TAA_BRIGHTNESS_SCALE 1.
#define TAA_CLOUDS_FRAMES 16

float simple_luma_tonemap(float luma, float exposure) { return rcp(luma * exposure + 1.0); }
float simple_luma_tonemap_inv(float luma, float exposure) { return rcp(max(1.0 - luma * exposure, 0.001)); }

#define CLOUDS_TONEMAPPED 1
#define CLOUDS_TONEMAPPED_TO_SRGB 2
#define ALREADY_TONEMAPPED_SCENE CLOUDS_TONEMAPPED//CLOUDS_TONEMAPPED_TO_SRGB
#define TONEMAPPED_SCENE_EXPOSURE 1.//use average sun light as exposure?
#endif