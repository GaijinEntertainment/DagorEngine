// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/autoGraphics.h>

#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>

#include <drv/3d/dag_info.h>

#include <render/antialiasing.h>

namespace auto_graphics::auto_resolution
{

static const char *validate_upscaling(const char *method, const char *upscaling)
{
  // antialiasing lib proposes a list of available upscaling options for each method with ; separator
  // they are sorted in descending quality order, so we will search our selected option in this string
  // or fallback to last available option
  const char *availableUpscalingOptions = render::antialiasing::get_available_upscaling_options(method);
  G_FAST_ASSERT(availableUpscalingOptions);

  if (!strstr(availableUpscalingOptions, upscaling))
  {
    const char *failedUpscaling = eastl::exchange(upscaling, availableUpscalingOptions);
    const char *separatorIt = strchr(availableUpscalingOptions, ';');
    while (separatorIt != nullptr)
    {
      upscaling = separatorIt + 1;
      separatorIt = strchr(upscaling, ';');
    }

    debug("auto_resolution: upscaling option '%s' is not available for method '%s', fallback to '%s'", failedUpscaling, method,
      upscaling);
  }

  return upscaling;
}

using AntialiasingMethod = render::antialiasing::AntialiasingMethod;

static eastl::optional<AntialiasingMethod> get_method_from_string(const char *antialiasing_method)
{
  if (!antialiasing_method)
    return {};

  auto method = render::antialiasing::get_method_from_name(antialiasing_method);
  switch (method)
  {
    case AntialiasingMethod::TSR: [[fallthrough]];
    case AntialiasingMethod::FSR: [[fallthrough]];
    case AntialiasingMethod::DLSS: [[fallthrough]];
    case AntialiasingMethod::XeSS: [[fallthrough]];
    case AntialiasingMethod::FXAAHigh: break;
    default: debug("auto_resolution: method '%s' is not used for auto resolution", antialiasing_method); return {};
  }

  auto availableMethods = render::antialiasing::get_available_methods(false, false);
  if (strcmp(availableMethods, "off") == 0)
  {
    logerr("auto_resolution: render::antialiasing must be initialized first");
    return {};
  }
  if (strstr(availableMethods, antialiasing_method) == nullptr)
  {
    // not an error - e.g. if previously launched with dx12+xess, and now with dx11+xess.
    // render::antialiasing will use "off" then. Later UI will select tsr.
    // This case is not related to auto resolution, so just log it and ignore.
    debug("auto_resolution: antialiasing method <%s> is not available, ignoring", antialiasing_method);
    return {};
  }

  return method;
}

DataBlock get_antialiasing_settings(const char *antialiasing_method, IPoint2 rendering_resolution)
{
  DataBlock autoResolutionSettingsBlk{};

  auto maybeMethod = get_method_from_string(antialiasing_method);
  if (!maybeMethod.has_value())
    return autoResolutionSettingsBlk;

  const bool overrideSettingsForAutoPreset = auto_antialiasing::should_use_upscaling_method(rendering_resolution);

  const auto &video = *::dgs_get_settings()->getBlockByNameEx("video");
  const bool isAutoResolution = strcmp(video.getStr("resolution", "auto"), "auto") == 0;

  // TODO: uncomment all code related to launchCnt when each project supports AA override

  // If some users have a config without overrideAAforAutoResolution, we can't change their AA settings unexpectedly
  // Use launchCnt to apply AA override for new users only
  const bool firstLaunch = dgs_get_settings()->getInt("launchCnt", 1) == 1;
  const bool overrideAAForAutoResolution = /* firstLaunch ||  */ video.getBool("overrideAAforAutoResolution", false);

  // Don't use DPI based scaling if auto preset requires to override AA settings
  const bool useDPIBasedScaling = isAutoResolution && overrideAAForAutoResolution && !overrideSettingsForAutoPreset;

  if (!useDPIBasedScaling && !overrideSettingsForAutoPreset)
    return autoResolutionSettingsBlk;

  auto &overriddenVideo = *autoResolutionSettingsBlk.addBlock("video");
  overriddenVideo.setStr("antialiasing_mode", antialiasing_method);

  // this is required when settings.blk has no overrideAAforAutoResolution and launchCnt == 0
  // if (useDPIBasedScaling)
  //   overriddenVideo.setBool("overrideAAforAutoResolution", true);

  int w, h;
  d3d::get_screen_size(w, h);
  const float screenArea = static_cast<float>(w * h);

  float perDimensionScaleFactor = 1.f; // no scaling by default

  if (useDPIBasedScaling)
  {
    // use it if the display area is larger than full hd in upscalerThreshold times
    constexpr float fullHdArea = 1920 * 1080;
    const float upscalerThreshold = video.getReal("autoResolutionUpscalerThreshold");

    if (screenArea / fullHdArea >= upscalerThreshold)
    {
      // DPI based scaling
      // By default user can choose between 100% 125% 150% 175% 200% scaling in OS settings,
      // but advanced OS settings provides up to 500%, so clamp to 2.0
      perDimensionScaleFactor = clamp(d3d::get_display_scale(), 1.f, 2.f);
    }
  }
  else if (overrideSettingsForAutoPreset)
  {
    const float renderingArea = static_cast<float>(rendering_resolution.x * rendering_resolution.y);
    perDimensionScaleFactor = clamp(screenArea / renderingArea, 1.f, 2.f);
  }

  const auto method = maybeMethod.value();
  const bool isTsr = method == AntialiasingMethod::TSR;
  const bool isVendorBased =
    method == AntialiasingMethod::DLSS || method == AntialiasingMethod::FSR || method == AntialiasingMethod::XeSS;
  const bool isFxaa = method == AntialiasingMethod::FXAAHigh;

  const char *upscalingStr = nullptr;
  if (isTsr || isVendorBased)
  {
    String tmp;
    if (isTsr)
    {
      // convert perDimensionScaling to tsr upscaling percentage
      // UI provides values in range [50, 100] with step 5
      const int rawUpscaling = eastl::max(static_cast<int>(100.f / perDimensionScaleFactor), 50);
      const float upscaling = static_cast<int>((rawUpscaling + 2) / 5 * 5);

      // quirrel uses "%g" format for ToString
      tmp = String(16, "%g", 10000.f / upscaling);
      upscalingStr = tmp.c_str();
    }
    else
    {
      // scale factor presets for DLSS and FSR are:
      // native - 1.0x, quality - 1.5x, balanced - 1.7x, performance - 2.0x
      // Scale factor presets for XeSS are:
      // native - 1.0x, quality - 1.7x, balanced - 2.0x, performance - 2.3x

      const bool isXess = method == AntialiasingMethod::XeSS;
      auto scaleFactorBoundsBlk = dgs_get_settings()
                                    ->getBlockByNameEx("auto_resolution")
                                    ->getBlockByNameEx(isXess ? "xess_scale_factor_bounds" : "dlss_fsr_scale_factor_bounds");

      const float SCALE_FACTOR_BOUND_QUALITY_DEFAULT = isXess ? 1.75 : 1.5;
      const float SCALE_FACTOR_BOUND_BALANCED_DEFAULT = isXess ? 2.0 : 1.75;
      const float SCALE_FACTOR_BOUND_PERFORMANCE_DEFAULT = 2.0;

      if (perDimensionScaleFactor <= scaleFactorBoundsBlk->getReal("quality", SCALE_FACTOR_BOUND_QUALITY_DEFAULT))
      {
        upscalingStr = "quality";
      }
      else if (perDimensionScaleFactor <= scaleFactorBoundsBlk->getReal("balanced", SCALE_FACTOR_BOUND_BALANCED_DEFAULT))
      {
        upscalingStr = "balanced";
      }
      else
      {
        G_UNUSED(SCALE_FACTOR_BOUND_PERFORMANCE_DEFAULT);
        G_ASSERT(perDimensionScaleFactor <= scaleFactorBoundsBlk->getReal("performance", SCALE_FACTOR_BOUND_PERFORMANCE_DEFAULT));
        upscalingStr = "performance";
      }

      upscalingStr = validate_upscaling(antialiasing_method, upscalingStr);
    }
    overriddenVideo.setStr("antialiasing_upscaling", upscalingStr);
  }
  else if (isFxaa)
  {
    if (!firstLaunch)
      return DataBlock{};
    // TODO: new UI behavior will allow selecting the AA method for auto resolution.
    // In this case, antialiasing_upscaling and fxaaQuality are selected by this function (with a freezed slider in UI).
    // Should any heuristics be used for fxaaQuality instead of a fixed value?
    const char *fxaaQuality = dgs_get_settings()->getBlockByNameEx("auto_resolution")->getStr("fxaaQuality", "medium");
    autoResolutionSettingsBlk.addBlock("graphics")->setStr("fxaaQuality", fxaaQuality);
  }

  return autoResolutionSettingsBlk;
}

} // namespace auto_graphics::auto_resolution
