#ifndef DAFX_MODFX_BVH_HLSL
#define DAFX_MODFX_BVH_HLSL

#ifdef __cplusplus
  #define float4 Point4
  #define float3 Point3
  #define uint   uint32_t
#endif

// TODO: Compress me!

struct ModfxBVHParticleData
{
  float4 color_matrix_r;
  float4 color_matrix_g;
  float4 color_matrix_b;
  float4 color_matrix_a;

  float4 texcoord0;
  float4 texcoord1;
  float4 texcoord2;
  float4 texcoord3;
  float4 color;
  float3 emission;
  float radius;
  float3 lighting;
  float shadow;
  float3 ambient;
  float frameBlend;
  float3 upDir;
  uint flags;
  float3 rightDir;
  uint lighting_type;
  float lighting_translucency;
  float sphere_normal_power;
  float sphere_normal_softness;
  float sphere_normal_radius;
};

static const uint UseAlphaTreshold = 1 << 0;
static const uint UseLighting      = 1 << 1;
static const uint UseColorRemap    = 1 << 2;
static const uint UseColorMatrix   = 1 << 3;

#endif