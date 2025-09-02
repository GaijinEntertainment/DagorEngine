// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "render/rendererFeatures.h"
#include <ioSys/dag_dataBlock.h>
#include <EASTL/utility.h>
#include <util/dag_string.h>
#include <string.h>
#include <main/settings.h>
#include <startup/dag_globalSettings.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/entityManager.h>
#include <3d/dag_gpuConfig.h>

#define INSIDE_RENDERER 1
#include <render/world/private_worldRenderer.h>

static eastl::pair<FeatureRenderFlags, const char *> feature_render_flags_str_table[] = {{VOLUME_LIGHTS, "volumeLights"},
  {TILED_LIGHTS, "tiledLights"}, {POST_RESOLVE, "postResolve"}, {COMBINED_SHADOWS, "combinedShadows"},
  {DEFERRED_LIGHT, "deferredLight"}, {DECALS, "decals"}, {SPECULAR_CUBES, "specularCubes"},
  {UPSCALE_SAMPLING_TEX, "upscaleSamplingTex"}, {WAKE, "wake"}, {LEVEL_SHADOWS, "levelShadows"}, {FULL_DEFERRED, "fullDeferred"},
  {WATER_PERLIN, "waterPerlin"}, {HAZE, "haze"}, {RIPPLES, "ripples"}, {FOM_SHADOWS, "fomShadows"},
  {DOWNSAMPLED_NORMALS, "downsampledNormals"}, {GPU_RESIDENT_ADAPTATION, "gpuResidentAdaptation"}, {SSAO, "SSAO"},
  {PREV_OPAQUE_TEX, "prevOpaqueTex"}, {DOWNSAMPLED_SHADOWS, "downsampledShadows"}, {ADAPTATION, "adaptation"}, {TAA, "TAA"},
  {WATER_REFLECTION, "waterReflection"}, {BLOOM, "bloom"}, {MICRODETAILS, "microdetails"}, {FORWARD_RENDERING, "forwardRendering"},
  {POSTFX, "postFx"}, {CONTACT_SHADOWS, "contactShadows"}, {STATIC_SHADOWS, "staticShadows"}, {CLUSTERED_LIGHTS, "clusteredLights"},
  {LEVEL_LAND_TEXEL_SIZE, "levelLandTexelSize"}, {DYNAMIC_LIGHTS_SHADOWS, "dynamicLightsShadows"}, {SSR, "SSR"},
  {VOLUME_LIGHTS_PER_PIXEL_FX_APPLY, "volumeLightsPerPixelFxApply"}, {MOBILE_DEFERRED, "mobileDeferred"},
  {MOBILE_SIMPLIFIED_MATERIALS, "mobileSimplifiedMaterials"}, {HIGHRES_PUDDLES, "highresPuddles"},
  {CAMERA_IN_CAMERA, "cameraInCamera"}};

bool forceDefaultRenderingPath;

namespace
{
static FeatureRenderFlagMask current_render_features;
}

void rendering_path::set_force_default(bool v) { forceDefaultRenderingPath = v; }

bool rendering_path::get_force_default() { return forceDefaultRenderingPath; }

static String get_platform_block_name()
{
  String platformBlockName(dgs_get_settings()->getStr("renderFeaturesPlatform", get_platform_string()));

  String renderingPath = get_corrected_rendering_path();
  if (renderingPath != "default" && !forceDefaultRenderingPath)
  {
    String customFeatureBlock(64, "%s_%s", platformBlockName, renderingPath.c_str());
    platformBlockName = customFeatureBlock;
  }

  return platformBlockName;
}

bool load_render_feature(const DataBlock *blk, const FeatureRenderFlags feature)
{
  const String platformBlockName = get_platform_block_name();
  const DataBlock *platformFeaturesBlk = blk->getBlockByNameEx(platformBlockName.c_str());

  const char *featureName = feature_render_flags_str_table[feature].second;

  bool hasFeature = blk->getBool(featureName, is_render_feature_enabled_by_default(feature));
  hasFeature = platformFeaturesBlk->getBool(featureName, hasFeature);

  return hasFeature;
}

FeatureRenderFlagMask load_render_features(const DataBlock *blk)
{
  const String platformBlockName = get_platform_block_name();
  const DataBlock *platformFeaturesBlk = blk->getBlockByNameEx(platformBlockName.c_str());

  FeatureRenderFlagMask featuresMask;
  for (auto &i : feature_render_flags_str_table)
  {
    bool hasFeature = blk->getBool(i.second, is_render_feature_enabled_by_default(i.first));
    hasFeature = platformFeaturesBlk->getBool(i.second, hasFeature);

    if (hasFeature)
      featuresMask.set(i.first);
  }
  return featuresMask;
}

static bool get_render_feature_by_name(const char *name, FeatureRenderFlags &result)
{
  for (auto &i : feature_render_flags_str_table)
    if (!strcmp(i.second, name))
    {
      result = i.first;
      return true;
    }
  return false;
}

FeatureRenderFlagMask apply_render_features_override(const FeatureRenderFlagMask &cur, ecs::Object overrides)
{
  FeatureRenderFlagMask overriden = cur;
  for (const auto &override : overrides)
  {
    FeatureRenderFlags feature;
    if (get_render_feature_by_name(override.first.c_str(), feature))
    {
      if (override.second.is<bool>())
        overriden.set(feature, override.second.get<bool>());
      else
        logerr("Render feature override <%s> should has bool type", override.first.c_str());
    }
    else
      logerr("Unknown render feature <%s>", override.first.c_str());
  }
  return overriden;
}

String render_features_to_string(FeatureRenderFlagMask val)
{
  if (val.none())
    return String("none");

  bool allFeaturesEnabled = true;
  for (auto &i : feature_render_flags_str_table)
    if (val.test(i.first) != is_render_feature_enabled_by_default(i.first))
    {
      allFeaturesEnabled = false;
      break;
    }
  if (allFeaturesEnabled)
    return String("all");

  String featuresString("");

  for (auto &i : feature_render_flags_str_table)
  {
    if (val.test(i.first))
    {
      featuresString += i.second;
      val.reset(i.first);
      if (val.any())
        featuresString += "|";
      else
        break;
    }
  }

  return featuresString;
}

FeatureRenderFlagMask parse_render_features(const char *str)
{
  FeatureRenderFlagMask featuresMask;

  for (auto &i : feature_render_flags_str_table)
  {
    if (strstr(str, i.second))
      featuresMask.set(i.first);
  }

  return featuresMask;
}

bool renderer_has_feature(FeatureRenderFlags feature) { return current_render_features.test(feature); }

const eastl::pair<FeatureRenderFlags, const char *> *get_feature_render_flags_str_table(size_t &cnt)
{
  cnt = countof(feature_render_flags_str_table);
  return cnt ? &feature_render_flags_str_table[0] : nullptr;
}

FeatureRenderFlagMask get_current_render_features() { return current_render_features; }

void set_current_render_features(FeatureRenderFlagMask val) { current_render_features = val; }

String get_corrected_rendering_path()
{
  const char *renderingPath = ::dgs_get_settings()->getBlockByNameEx("graphics")->getStr("renderingPath", "default");
  const DataBlock *fallbackConfig = ::dgs_get_settings()->getBlockByNameEx("renderingPathFallback");
  const char *renderingPathFallback = fallbackConfig->getStr("value", "default");

  bool driverPassingMinCheck = true;

  // driverVersion check
  {
    const GpuUserConfig &gpuCfg = d3d_get_gpu_cfg();
    const char *gpuVendorStr = d3d_get_vendor_name(gpuCfg.primaryVendor);

    const DataBlock *vendorConfig = fallbackConfig->getBlockByNameEx("vendor")->getBlockByNameEx(gpuVendorStr);

    uint32_t cfgDriverMinVer[4] = {(uint32_t)vendorConfig->getInt("driverMinVer0", 0),
      (uint32_t)vendorConfig->getInt("driverMinVer1", 0), (uint32_t)vendorConfig->getInt("driverMinVer2", 0),
      (uint32_t)vendorConfig->getInt("driverMinVer3", 0)};

    for (int i = 0; i < 4; ++i)
      driverPassingMinCheck &= !cfgDriverMinVer[i] || (gpuCfg.driverVersion[i] >= cfgDriverMinVer[i]);
  }

  // log about results
  debug("rendering path correction: using <%s> rendering path", driverPassingMinCheck ? "default" : "fallback");

  return String(driverPassingMinCheck ? renderingPath : renderingPathFallback);
}
