#ifndef RENDER_LIGHTS_INCLUDED
#define RENDER_LIGHTS_INCLUDED 1

#include "renderLightsConsts.hlsli"

struct RenderOmniLight
{
  float4 posRadius;
  float4 colorFlags;
  float4 direction__tex_scale;
  float4 boxR0;
  float4 boxR1;
  float4 boxR2;
  float4 posRelToOrigin_cullRadius;
};

struct RenderSpotLight
{
  float4 lightPosRadius;
  float4 lightColorAngleScale;
  float4 lightDirectionAngleOffset;
  float4 texId_scale_shadow_contactshadow;
};

struct SpotlightShadowDescriptor
{
  float2 decodeDepth;
  float meterToUvAtZfar;
  float hasDynamic; //bool
  float4 uvMinMax;
};

//Implemented here, to avoid needing to pass these two values in a buffer
//inline needed to avoid duplicate symbol link error in c++
inline float2 get_light_shadow_zn_zf(float radius)
{
  return float2(0.001 * radius, radius);
}
#define OOF_GRID_W 8
#define OOF_GRID_VERT 4
#define OOF_GRID_SIZE (OOF_GRID_W*OOF_GRID_W*OOF_GRID_VERT)

#if SHADER_COMPILER_FP16_ENABLED
  inline half2 get_light_shadow_zn_zf(half radius)
  {
    return half2(half(0.001) * radius, radius);
  }
#endif
#endif