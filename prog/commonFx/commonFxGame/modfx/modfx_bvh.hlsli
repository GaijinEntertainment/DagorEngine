#ifndef DAFX_MODFX_BVH_HLSL
#define DAFX_MODFX_BVH_HLSL

#include "modfx_consts.hlsli"

#ifdef __cplusplus
  #define float4 Point4
  #define float3 Point3
  #define uint   uint32_t
#endif

// TODO: Compress me!

struct ModfxBVHParticleData
{
  uint4 colorMatrix;

  float4 texcoord0;
  float4 texcoord1;
  float4 texcoord2;
  float4 texcoord3;

  uint colorRemapArr[MODFX_PREBAKE_GRAD_STEPS_LIMIT]; // TODO: we can bake a less accurate curve for it

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
  float3 atmosphereLoss;
  uint colorRemapStepCnt;
  float3 atmosphereInscatter;
  float lifeNorm;
  uint3 padding1;
  float gradScaleRcp;
};

#define BVH_MODFX_RFLAG_COLOR_USE_ALPHA_THRESHOLD (1 << 0)
#define BVH_MODFX_RMOD_LIGHTING_INIT (1 << 1)
#define BVH_MODFX_RMOD_TEX_COLOR_REMAP (1 << 2)
#define BVH_MODFX_RMOD_TEX_COLOR_MATRIX (1 << 3)
#define BVH_MODFX_RFLAG_TEX_COLOR_REMAP_APPLY_BASE_COLOR (1 << 4)
#define BVH_MODFX_RFLAG_TEX_COLOR_REMAP_SECOND_MASK (1 << 5)
#define BVH_MODFX_RFLAG_TEX_COLOR_REMAP_SECOND_MASK_APPLY_BASE_COLOR (1 << 6)
#define BVH_MODFX_RMOD_TEX_COLOR_REMAP_DYNAMIC (1 << 7)
#define BVH_MODFX_RFLAG_BLEND_ABLEND (1 << 8)
#define BVH_MODFX_RFLAG_BLEND_ADD (1 << 9)


#endif