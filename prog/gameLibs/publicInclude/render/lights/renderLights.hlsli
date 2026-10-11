#ifndef RENDER_LIGHTS_INCLUDED
#define RENDER_LIGHTS_INCLUDED 1

#include "renderLightsConsts.hlsli"
#include "spot_light_shadow_flags.hlsli"

struct RenderOmniLight
{
  float4 posRadius;
  float4 colorFlags;
  float4 direction__tex_scale;
  float4 boxR0;
  float4 boxR1;
  float4 boxR2;
  float4 posRelToOrigin_cullRadius;
  float4 shadowZnZf_flags_packedDataBits;
};

struct RenderSpotLight
{
  float4 lightPos_halfRadius_halfCullRadius;
  float4 lightColorAngleScale;
  float4 lightDirectionAngleOffset;
  float4 texId_scale_illuminatingplane_packedDataBits;
};

#define RENDER_OMNI_LIGHT_STRIDE_SIZE 128
#define RENDER_SPOT_LIGHT_STRIDE_SIZE 64

#define RENDER_OMNI_LIGHT_POS_RADIUS_OFFSET 0
#define RENDER_OMNI_LIGHT_POS_REL_TO_ORIGIN_CULL_RADIUS_OFFSET 96

#define RENDER_SPOT_LIGHT_POS_HALF_RADIUS_HALF_CULL_RADIUS_OFFSET 0
#define RENDER_SPOT_LIGHT_COLOR_ANGLE_SCALE_OFFSET 16
#define RENDER_SPOT_LIGHT_DIRECTION_ANGLE_OFFSET_OFFSET 32

#ifdef __cplusplus
  static_assert(RENDER_OMNI_LIGHT_STRIDE_SIZE == sizeof(RenderOmniLight), "RENDER_OMNI_LIGHT_STRIDE_SIZE has invalid size, please update");
  static_assert(RENDER_SPOT_LIGHT_STRIDE_SIZE == sizeof(RenderSpotLight), "RENDER_SPOT_LIGHT_STRIDE_SIZE has invalid size, please update");

  static_assert(RENDER_OMNI_LIGHT_POS_RADIUS_OFFSET == offsetof(RenderOmniLight, posRadius),
    "RENDER_OMNI_LIGHT_POS_RADIUS_OFFSET has invalid offset, please update");
  static_assert(RENDER_OMNI_LIGHT_POS_REL_TO_ORIGIN_CULL_RADIUS_OFFSET == offsetof(RenderOmniLight, posRelToOrigin_cullRadius),
    "RENDER_OMNI_LIGHT_POS_REL_TO_ORIGIN_CULL_RADIUS_OFFSET has invalid offset, please update");

  static_assert(RENDER_SPOT_LIGHT_POS_HALF_RADIUS_HALF_CULL_RADIUS_OFFSET == offsetof(RenderSpotLight, lightPos_halfRadius_halfCullRadius),
    "RENDER_SPOT_LIGHT_POS_HALF_RADIUS_HALF_CULL_RADIUS_OFFSET has invalid offset, please update");
  static_assert(RENDER_SPOT_LIGHT_COLOR_ANGLE_SCALE_OFFSET == offsetof(RenderSpotLight, lightColorAngleScale),
    "RENDER_SPOT_LIGHT_COLOR_ANGLE_SCALE_OFFSET has invalid offset, please update");
  static_assert(RENDER_SPOT_LIGHT_DIRECTION_ANGLE_OFFSET_OFFSET == offsetof(RenderSpotLight, lightDirectionAngleOffset),
    "RENDER_SPOT_LIGHT_DIRECTION_ANGLE_OFFSET_OFFSET has invalid offset, please update");
#endif

#ifndef __cplusplus
float decode_light_roll_angle(RenderSpotLight sl)
{
  return float((asuint(sl.texId_scale_illuminatingplane_packedDataBits.w) & SPOT_LIGHT_ROLL_MASK) >> SPOT_LIGHT_ROLL_BIT_OFFSET) / SPOT_LIGHT_ROLL_MAX_VALUE;
}

uint get_spot_light_shadow_data_bits(RenderSpotLight sl)
{
  return asuint(sl.texId_scale_illuminatingplane_packedDataBits.w);
}

bool spot_light_has_shadow(uint shadow_data_bits)
{
  return shadow_data_bits & SPOT_LIGHT_HAS_SHADOW_MASK;
}

bool spot_light_needs_contact_shadows(uint shadow_data_bits)
{
  return shadow_data_bits & SPOT_LIGHT_NEEDS_CONTACT_SHADOWS_MASK;
}

float2 decode_spot_light_radius_and_culling_radius(float encoded_value)
{
  return float2(f16tof32(asuint(encoded_value)), f16tof32(asuint(encoded_value) >> 16));
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

uint get_omni_light_flags(RenderOmniLight light)
{
  return uint(light.shadowZnZf_flags_packedDataBits.z + 0.5);
}

uint get_omni_light_id(RenderOmniLight light)
{
  return asuint(light.shadowZnZf_flags_packedDataBits.w) & OMNI_LIGHT_ID_MASK;
}

uint get_spot_light_id(RenderSpotLight sl)
{
  return (asuint(sl.texId_scale_illuminatingplane_packedDataBits.w) & SPOT_LIGHT_ID_MASK) >> SPOT_LIGHT_ID_BIT_OFFSET;
}

float get_omni_light_source_radius(RenderOmniLight light)
{
  return float((asuint(light.shadowZnZf_flags_packedDataBits.w) >> OMNI_LIGHT_SOURCE_RADIUS_BIT_OFFSET) & LIGHT_SOURCE_RADIUS_MASK) / LIGHT_SOURCE_RADIUS_CM_PER_M;
}

float get_spot_light_source_radius(RenderSpotLight sl)
{
  return float((asuint(sl.texId_scale_illuminatingplane_packedDataBits.w) >> SPOT_LIGHT_SOURCE_RADIUS_BIT_OFFSET) & LIGHT_SOURCE_RADIUS_MASK) / LIGHT_SOURCE_RADIUS_CM_PER_M;
}

#else

// At least on win7 we have a limit for 64k of cb buffer size
// But drivers requires to keep cb buffer size under 64k on all platforms.
// So we limit it for all platforms.
// Reserve one Point4 for the element count constant that ReallocatableLightsConstBuffer
// prepends when store_elems_count is true (see reallocate()).
static_assert(MAX_VISIBLE_FAR_LIGHTS <= (65536 - 16) / max(sizeof(RenderSpotLight), sizeof(RenderOmniLight)), "MAX_VISIBLE_FAR_LIGHTS has too high value - please update const");


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