// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_RENDERER
#include "private_worldRenderer.h"
#include <perfMon/dag_statDrv.h>
#include <render/resolution.h>
#include <drv/3d/dag_renderTarget.h>
#if _TARGET_PC
#include <3d/dag_gpuConfig.h>
#endif

#define STATIC_UPSAMPLE_GLOBAL_VARS \
  VAR(upsampling_quality)           \
  VAR(upsampling_target_size)       \
  VAR(upsampling_source_size)

#define VAR(a) static int a##VarId = -1;
STATIC_UPSAMPLE_GLOBAL_VARS
#undef VAR


void WorldRenderer::closeStaticUpsample() { resolutionScaleMode = RESOLUTION_SCALE_NATIVE; }

// It's a noop if the two input parameters are the same
void WorldRenderer::initStaticUpsample(const IPoint2 &origResolution, const IPoint2 &scaledResolution)
{
  resolutionScaleMode = RESOLUTION_SCALE_NATIVE;

  if (isSSAAEnabled())
    return;


  if (scaledResolution != origResolution)
  {
    if (dgs_get_settings()->getBlockByName("video")->getBool("upscaleInPostfx", false))
    {
      debug("Static resolution scaling will be done in postfx shader.");
      resolutionScaleMode = RESOLUTION_SCALE_POSTFX;
      return;
    }

    G_ASSERT(scaledResolution.x <= origResolution.x && scaledResolution.y <= origResolution.y);

#define VAR(a) a##VarId = get_shader_variable_id(#a);
    STATIC_UPSAMPLE_GLOBAL_VARS
#undef VAR

    applyStaticUpsampleQuality();

    ShaderGlobal::set_color4(upsampling_target_sizeVarId, origResolution.x, origResolution.y, 1.0f / origResolution.x,
      1.0f / origResolution.y);
    ShaderGlobal::set_color4(upsampling_source_sizeVarId, scaledResolution.x, scaledResolution.y, 1.0f / scaledResolution.x,
      1.0f / scaledResolution.y);

    resolutionScaleMode = RESOLUTION_SCALE_SUB;

    debug("upscaling initialized upscaling from %ix%i to %ix%i", scaledResolution.x, scaledResolution.y, origResolution.x,
      origResolution.y);
  }
}

float get_default_static_resolution_scale()
{
// TODO should be determined by benchmark result instead
#if _TARGET_PC
  auto gpuCfg = d3d_get_gpu_cfg();
  if (gpuCfg.integrated && gpuCfg.primaryVendor == D3D_VENDOR_INTEL)
  {
    int w, h;
    d3d::get_screen_size(w, h);
    float ratio = safediv(900.0f, h) * 100.0f;
    float quantizedRatio = floor(ratio / 5) * 5;
    return clamp(quantizedRatio, 50.0f, 100.0f);
  }
#endif
  return 100.0f;
}

IPoint2 WorldRenderer::getStaticScaledResolution(const IPoint2 &orig_resolution) const
{
  float staticResolutionScale =
    ((currentAntiAliasingMode == AntiAliasingMode::OFF || currentAntiAliasingMode == AntiAliasingMode::FXAA) &&
      !forceStaticResolutionOff)
      ? cachedStaticResolutionScale
      : 100.0f;
  if (staticResolutionScale > 100.f)
  {
    logerr("staticResolutionScale was %f, clamping to 100", staticResolutionScale);
    staticResolutionScale = 100.f;
  }

  IPoint2 scaledResolution = (orig_resolution * staticResolutionScale) / 100;

  if (scaledResolution.x < 32 || scaledResolution.y < 32)
  {
    logerr("specified staticResolutionScale %f can't be used at resolution %ix%i", staticResolutionScale, orig_resolution.x,
      orig_resolution.y);
    return orig_resolution;
  }

  // Make the target size divisible by 16 for better FX and clouds.
  if (scaledResolution.x != orig_resolution.x || scaledResolution.y != orig_resolution.y)
  {
    scaledResolution.x -= scaledResolution.x % 16; // Grant a 4 pixel alignment for quarter-res targets.
    scaledResolution.y -= scaledResolution.y % 16;
  }

  return scaledResolution;
}

bool WorldRenderer::isStaticUpsampleEnabled() const { return resolutionScaleMode == RESOLUTION_SCALE_SUB; }

void WorldRenderer::applyStaticUpsampleQuality()
{
  const char *settingName = ::dgs_get_settings()->getBlockByNameEx("graphics")->getStr("staticUpsampleQuality", "catmullrom");
  int upsamplingQuality = 0;
  if (strcmp(settingName, "catmullrom") == 0)
    upsamplingQuality = 1;
  else if (strcmp(settingName, "sharpen") == 0)
    upsamplingQuality = 2;
  else if (strcmp(settingName, "bilinear") != 0)
    logerr("graphics/staticUpsampleQuality invalid value: %s", settingName);
  ShaderGlobal::set_int(upsampling_qualityVarId, upsamplingQuality);
}