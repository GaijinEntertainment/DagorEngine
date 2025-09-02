// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/bitset.h>

class DataBlock;
class String;
namespace ecs
{
class Object;
class ComponentsInitializer;
} // namespace ecs

enum FeatureRenderFlags : uint32_t
{
  VOLUME_LIGHTS,
  TILED_LIGHTS,
  POST_RESOLVE,
  COMBINED_SHADOWS,
  DEFERRED_LIGHT,
  DECALS,
  SPECULAR_CUBES,
  UPSCALE_SAMPLING_TEX,
  WAKE,
  LEVEL_SHADOWS,
  FULL_DEFERRED,
  WATER_PERLIN,
  HAZE,
  RIPPLES,
  FOM_SHADOWS,
  DOWNSAMPLED_NORMALS,
  GPU_RESIDENT_ADAPTATION,
  SSAO,
  PREV_OPAQUE_TEX,
  DOWNSAMPLED_SHADOWS,
  ADAPTATION,
  TAA,
  WATER_REFLECTION,
  BLOOM,
  MICRODETAILS,
  FORWARD_RENDERING,
  POSTFX,
  CONTACT_SHADOWS,
  STATIC_SHADOWS,
  CLUSTERED_LIGHTS,
  LEVEL_LAND_TEXEL_SIZE,
  DYNAMIC_LIGHTS_SHADOWS,
  SSR,
  VOLUME_LIGHTS_PER_PIXEL_FX_APPLY,
  MOBILE_DEFERRED,
  MOBILE_SIMPLIFIED_MATERIALS,
  HIGHRES_PUDDLES,
  CAMERA_IN_CAMERA,
  RENDER_FEATURES_NUM
};

using FeatureRenderFlagMask = eastl::bitset<RENDER_FEATURES_NUM>;

namespace rendering_path
{
void set_force_default(bool v);
bool get_force_default();
} // namespace rendering_path

static inline bool is_render_feature_enabled_by_default(FeatureRenderFlags f)
{
  return (f != FORWARD_RENDERING) && (f != MOBILE_DEFERRED) && (f != MOBILE_SIMPLIFIED_MATERIALS) && (f != CAMERA_IN_CAMERA);
}

bool load_render_feature(const DataBlock *blk, const FeatureRenderFlags feature);
FeatureRenderFlagMask load_render_features(const DataBlock *blk);
FeatureRenderFlagMask apply_render_features_override(const FeatureRenderFlagMask &cur, ecs::Object overrides);
String render_features_to_string(FeatureRenderFlagMask val);
FeatureRenderFlagMask parse_render_features(const char *str);
bool renderer_has_feature(FeatureRenderFlags feature);
FeatureRenderFlagMask get_current_render_features();
void set_current_render_features(FeatureRenderFlagMask val);

// corrects config supplied rendering path to fail-safe one if needed
String get_corrected_rendering_path();
