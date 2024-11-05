// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/array.h>
#include <math/dag_hlsl_floatx.h>

namespace plod
{

struct ProcessedMaterial
{
  float3 albedo;
  float ao;
  float3 normal;
  float material;
  float smoothness;
  float reflectance;
  float metallTranslucency;
  float shadow;
};

inline constexpr int SIMPLE_MATERIAL_CHANNELS = sizeof(ProcessedMaterial) / sizeof(float4);

template <int n>
struct RawMaterial
{
  eastl::array<float4, n> diffuse;
  eastl::array<float4, n> packedNormalMap;
};

inline constexpr int PERLIN_MATERIAL_CHANNELS = sizeof(RawMaterial<3>) / sizeof(float4);

} // namespace plod