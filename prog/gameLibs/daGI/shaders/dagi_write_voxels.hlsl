#ifndef SCENE_VOXELS_WRITE_INCLUDED
#define SCENE_VOXELS_WRITE_INCLUDED 1
#include "dagi_voxels_consts.hlsli"

#if SCENE_VOXELS_COLOR == SCENE_VOXELS_SRGB8_A8
  void writeSceneVoxelData(uint3 coord, uint cascade, float3 color, float alpha)
  {
    voxelsColor[coord+getCascadeShift(cascade)] = float4(encode_scene_voxels_color(color), alpha);
  }
#elif SCENE_VOXELS_COLOR == SCENE_VOXELS_R11G11B10
  void writeSceneVoxelData(uint3 coord, uint cascade, float3 color, float alpha)
  {
    voxelsAlpha[coord+getCascadeShift(cascade)] = alpha;
    voxelsColor[coord+getCascadeShift(cascade)] = encode_scene_voxels_color(color);
  }
  void writeSceneVoxelAlpha(uint3 coord, uint cascade, float alpha)
  {
    voxelsAlpha[coord+getCascadeShift(cascade)] = alpha;
  }
#elif SCENE_VOXELS_COLOR == SCENE_VOXELS_HSV_A8
  float4 decodeRGBAuint(uint value)
  {
    uint ai = value & 0x0000007F;
    uint vi = (value / 0x00000080) & 0x000007FF;
    uint si = (value / 0x00040000) & 0x0000007F;
    uint hi = value / 0x02000000;

    float h = float(hi) / 127.0;
    float s = float(si) / 127.0;
    float v = (float(vi) / 2047.0) * 10.0;
    float a = ai * 2.0;

    v = pow(v, 3.0);

    float3 color = hsv2rgb(float3(h, s, v));

    return float4(color.rgb, a);
  }

  uint encodeRGBAuint(float4 color)
  {
    //7[HHHHHHH] 7[SSSSSSS] 11[VVVVVVVVVVV] 7[AAAAAAAA]
    float3 hsv = rgb2hsv(color.rgb);
    hsv.z = pow(hsv.z, 1.0 / 3.0);

    uint result = 0;

    uint a = min(127, uint(color.a / 2.0));
    uint v = min(2047, uint((hsv.z / 10.0) * 2047));
    uint s = uint(hsv.y * 127);
    uint h = uint(hsv.x * 127);

    result += a;
    result += v * 0x00000080; // << 7
    result += s * 0x00040000; // << 18
    result += h * 0x02000000; // << 25

    return result;
  }
  void writeSceneVoxelData(uint3 coord, uint cascade, float3 color, float alpha)
  {
  }

#endif

#endif
