#ifndef VOXELS_CONSTS_INCLUDED
#define VOXELS_CONSTS_INCLUDED 1

#define SCENE_VOXELS_ALPHA_SCALE 1.003937007874016
#define MAX_SCENE_VOXELS_ALPHA_TRANSPARENT 0.9921568627450981

#define SCENE_VOXELS_SRGB8_A8 1//srgb8_a8 allows are to not store separate texture, and so speed up everthing. However, it limits overbright to 4, we need to evaluate that on real data
#define SCENE_VOXELS_R11G11B10 2
#define SCENE_VOXELS_HSV_A8 2
#define SCENE_VOXELS_COLOR SCENE_VOXELS_R11G11B10
//SCENE_VOXELS_SRGB8_A8
//SCENE_VOXELS_R11G11B10
//SCENE_VOXELS_SRGB8_A8
//SCENE_VOXELS_HSV_A8
//SCENE_VOXELS_R11G11B10//SCENE_VOXELS_SRGB8_A8


#define VOXEL_RESOLUTION_CASCADES 3
#define VOXEL_RESOLUTION_X 128
#define VOXEL_RESOLUTION_Z VOXEL_RESOLUTION_X
#define VOXEL_RESOLUTION_Y 64
#define VOXEL_RESOLUTION float3(VOXEL_RESOLUTION_X, VOXEL_RESOLUTION_Z, VOXEL_RESOLUTION_Y)

#ifndef __cplusplus
  #include <pixelPacking/ColorSpaceUtility.hlsl>
  #if SCENE_VOXELS_COLOR == SCENE_VOXELS_SRGB8_A8
    #define MIN_REPRESENTABLE_SCENE_VALUE (1./255)
    #define SCENE_VOXELS_OVERBRIGHT 4.
    float3 decode_scene_voxels_color(float3 color) {return color*SCENE_VOXELS_OVERBRIGHT;}
    float3 encode_scene_voxels_color(float3 color) {return floor(255*ApplySRGBCurve(saturate(color/SCENE_VOXELS_OVERBRIGHT))+0.25)/255;}//instead of round factorization, to keep it darker
  #elif SCENE_VOXELS_COLOR == SCENE_VOXELS_R11G11B10
    #define MIN_REPRESENTABLE_SCENE_VALUE 0.000030517578125
    float3 decode_scene_voxels_color(float3 color) {return color;}
    float3 encode_scene_voxels_color(float3 color) {return color;}
  #elif SCENE_VOXELS_COLOR == SCENE_VOXELS_HSV_A8
    #define MIN_REPRESENTABLE_SCENE_VALUE 0.000030517578125
    float3 rgb2hsv(float3 c)
    {
      float4 k = float4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
      float4 p = lerp(float4(c.bg, k.wz), float4(c.gb, k.xy), step(c.b, c.g));
      float4 q = lerp(float4(p.xyw, c.r), float4(c.r, p.yzx), step(p.x, c.r));

      float d = q.x - min(q.w, q.y);
      float e = 1.0e-10;

      return float3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
    }

    float3 hsv2rgb(float3 c)
    {
      float4 k = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
      float3 p = abs(frac(c.xxx + k.xyz) * 6.0 - k.www);
      return c.z * lerp(k.xxx, saturate(p - k.xxx), c.y);
    }

    float3 decode_scene_voxels_color(float3 color) {return hsv2rgb(color);}
    float3 encode_scene_voxels_color(float3 color) {return rgb2hsv(color);}
  #endif
#endif

#endif