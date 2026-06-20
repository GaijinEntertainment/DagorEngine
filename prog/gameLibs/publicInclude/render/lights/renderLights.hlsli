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
  float4 shadowZnZf_pad;
};

struct RenderSpotLight
{
  float4 lightPos_halfRadius_halfCullRadius;
  float4 lightColorAngleScale;
  float4 lightDirectionAngleOffset;
  float4 texId_scale_illuminatingplane_packedDataBits;
};

#ifndef __cplusplus
float decode_light_roll_angle(RenderSpotLight sl)
{
  return float((asuint(sl.texId_scale_illuminatingplane_packedDataBits.w) & SPOT_LIGHT_ROLL_MASK) >> SPOT_LIGHT_ROLL_BIT_OFFSET) / SPOT_LIGHT_ROLL_MAX_VALUE;
}

float2 decode_spot_light_radius_and_culling_radius(float encoded_value)
{
  return float2(f16tof32(asuint(encoded_value)), f16tof32(asuint(encoded_value) >> 16));
}

float2 decode_spot_light_radius_and_culling_radius(RenderSpotLight sl)
{
  return decode_spot_light_radius_and_culling_radius(sl.lightPos_halfRadius_halfCullRadius.w);
}

float4 decode_spot_light_pos_radius(float4 encoded_value)
{
  float4 posRadius;
  posRadius.xyz = encoded_value.xyz;
  posRadius.w = f16tof32(asuint(encoded_value.w));
  return posRadius;
}

float4 decode_spot_light_pos_radius(RenderSpotLight sl)
{
  return decode_spot_light_pos_radius(sl.lightPos_halfRadius_halfCullRadius);
}

#endif

struct SpotlightShadowDescriptor
{
  float2 decodeDepth;
  float meterToUvAtZfar;
  float hasDynamic; //bool
  float4 uvMinMax;
};

#ifndef __cplusplus
float4 get_omni_light_color(RenderOmniLight light)
{
  return float4(abs(light.colorFlags.rgb), light.colorFlags.a);
}
#endif

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