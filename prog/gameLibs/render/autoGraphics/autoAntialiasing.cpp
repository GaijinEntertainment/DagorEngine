// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/autoGraphics.h>

#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>

#include <drv/3d/dag_drv3d_multi.h>
#include <drv/3d/dag_renderPass.h>
#include <drv/3d/dag_info.h>

#include <render/antialiasing.h>

namespace auto_graphics::auto_antialiasing
{

static bool use_fxaa_amd(uint32_t family)
{
  eastl::pair<const char *, bool> familyData = {"", false};
  switch (family)
  {
    // the default criteria: use FXAA if igpu arch is older than RDNA
    // use config block "auto_antialiasing/force_fxaa_for_igpu_families" to override it for specific families
#define FAMILY_CASE(name, useFxaa) \
  case DeviceAttributesBase::AmdFamily::name: familyData = { #name, useFxaa }; break;
    FAMILY_CASE(PRE_GCN, true);
    FAMILY_CASE(GCN1, true);
    FAMILY_CASE(GCN2, true);
    FAMILY_CASE(GCN3, true);
    FAMILY_CASE(GCN4, true);
    FAMILY_CASE(Vega, true);
    FAMILY_CASE(RDNA, false);
    FAMILY_CASE(RDNA2, false);
    FAMILY_CASE(RDNA3, false);
    FAMILY_CASE(RDNA4, false);
#undef FAMILY_CASE
    default: logwarn("auto_antialiasing: unknown AMD GPU family %u, assume it's a new family and don't use FXAA", family);
  }

  auto [familyName, useFxaaDefaultCriteria] = familyData;
  return dgs_get_settings()
    ->getBlockByNameEx("auto_antialiasing")
    ->getBlockByNameEx("force_fxaa_for_igpu_families")
    ->getBool(familyName, useFxaaDefaultCriteria);
}

static bool use_fxaa_intel(uint32_t family)
{
  eastl::pair<const char *, bool> familyData = {"", false};
  switch (family)
  {
    // the default criteria: use FXAA if igpu arch is older than Xe
    // use config block "auto_antialiasing/force_fxaa_for_igpu_families" to override it for specific families
#define FAMILY_CASE(name, useFxaa) \
  case DeviceAttributesBase::IntelFamily::name: familyData = { #name, useFxaa }; break;
    FAMILY_CASE(SANDYBRIDGE, true);
    FAMILY_CASE(IVYBRIDGE, true);
    FAMILY_CASE(HASWELL, true);
    FAMILY_CASE(VALLEYVIEW, true);
    FAMILY_CASE(BROADWELL, true);
    FAMILY_CASE(CHERRYVIEW, true);
    FAMILY_CASE(SKYLAKE, true);
    FAMILY_CASE(KABYLAKE, true);
    FAMILY_CASE(COFFEELAKE, true);
    FAMILY_CASE(BROXTON, true);
    FAMILY_CASE(GEMINILAKE, true);
    FAMILY_CASE(WHISKEYLAKE, true);
    FAMILY_CASE(CANNONLAKE, true);
    FAMILY_CASE(COMETLAKE, true);
    FAMILY_CASE(ICELAKE, true);
    FAMILY_CASE(ICELAKE_LP, true);
    FAMILY_CASE(LAKEFIELD, true);
    FAMILY_CASE(ELKHARTLAKE, true);
    FAMILY_CASE(JASPERLAKE, true);
    FAMILY_CASE(WILLOWVIEW, false);
    FAMILY_CASE(TIGERLAKE_LP, false);
    FAMILY_CASE(ROCKETLAKE, false);
    FAMILY_CASE(ALDERLAKE_S, false);
    FAMILY_CASE(ALDERLAKE_P, false);
    FAMILY_CASE(ALDERLAKE_N, false);
    FAMILY_CASE(TWINLAKE, false);
    FAMILY_CASE(RAPTORLAKE, false);
    FAMILY_CASE(RAPTORLAKE_S, false);
    FAMILY_CASE(RAPTORLAKE_P, false);
    FAMILY_CASE(DG1, false);
    FAMILY_CASE(XE_HP_SDV, false);
    FAMILY_CASE(ALCHEMIST, false);
    FAMILY_CASE(PONTEVECCHIO, false);
    FAMILY_CASE(METEORLAKE, false);
    FAMILY_CASE(ARROWLAKE_H, false);
    FAMILY_CASE(ARROWLAKE_S, false);
    FAMILY_CASE(BATTLEMAGE, false);
    FAMILY_CASE(LUNARLAKE, false);
    FAMILY_CASE(PANTHERLAKE, false);
    FAMILY_CASE(WILDCATLAKE, false);
#undef FAMILY_CASE
    default: logwarn("auto_antialiasing: unknown Intel GPU family %u, assume it's a new family and don't use FXAA", family);
  }

  auto [familyName, useFxaaDefaultCriteria] = familyData;
  return dgs_get_settings()
    ->getBlockByNameEx("auto_antialiasing")
    ->getBlockByNameEx("force_fxaa_for_igpu_families")
    ->getBool(familyName, useFxaaDefaultCriteria);
}

static const char *get_desired_aa_method(const char *available_methods, bool force_use_upscaling_method)
{
  const auto &gpuInfo = d3d::get_driver_desc().info;
  constexpr const char *DEFAULT_AA = "tsr";
  const char *selectedMethod = DEFAULT_AA;
  auto autoAntialiasingForVendorBlk =
    dgs_get_settings()->getBlockByNameEx("auto_antialiasing")->getBlockByNameEx("desired_aa_for_vendor");

  const bool maybeUseFXAA = !force_use_upscaling_method && gpuInfo.isUMA;
  bool useFXAA = false;
  switch (gpuInfo.vendor)
  {
    case GpuVendor::NVIDIA: selectedMethod = autoAntialiasingForVendorBlk->getStr("nvidia", DEFAULT_AA); break;
    case GpuVendor::AMD:
      // fsr is dx12 only, see render::antialiasing
      selectedMethod = d3d::get_driver_code().is(d3d::dx12) ? autoAntialiasingForVendorBlk->getStr("amd", DEFAULT_AA) : DEFAULT_AA;
      useFXAA = maybeUseFXAA && use_fxaa_amd(gpuInfo.family);
      break;
    case GpuVendor::INTEL:
      // xess is dx12 only, see render::antialiasing
      selectedMethod = d3d::get_driver_code().is(d3d::dx12) ? autoAntialiasingForVendorBlk->getStr("intel", DEFAULT_AA) : DEFAULT_AA;
      useFXAA = maybeUseFXAA && use_fxaa_intel(gpuInfo.family);
      break;
    default: logwarn("auto_antialiasing: unknown GPU vendor, fallback to %s", DEFAULT_AA); useFXAA = false;
  }

  selectedMethod = useFXAA ? "high_fxaa" : selectedMethod;

  return strstr(available_methods, selectedMethod) ? selectedMethod : DEFAULT_AA;
}

bool should_use_upscaling_method(IPoint2 rendering_resolution)
{
  // (0, 0) means it is not used for auto graphics preset
  if (rendering_resolution.x == 0 && rendering_resolution.y == 0)
    return false;

  IPoint2 screenResolution;
  d3d::get_screen_size(screenResolution.x, screenResolution.y);

  const float renderingArea = static_cast<float>(rendering_resolution.x * rendering_resolution.y);
  const float screenArea = static_cast<float>(screenResolution.x * screenResolution.y);

  const float perDimensionScaleFactor = sqrtf(screenArea / renderingArea);

  const float threshold = dgs_get_settings()->getBlockByNameEx("auto_antialiasing")->getReal("scale_factor_threshold", 1.2f);
  return perDimensionScaleFactor >= threshold;
}

const char *get_auto_selected_method(bool force_enable_and_use_upscaling_method)
{
  const bool firstLaunch = dgs_get_settings()->getInt("launchCnt", 1) == 1;
  const bool enableOnFirstLaunch = dgs_get_settings()->getBlockByNameEx("auto_antialiasing")->getBool("enableOnFirstLaunch", false);
  bool useAutoSelectedMethod = firstLaunch && enableOnFirstLaunch || force_enable_and_use_upscaling_method;

#if DAGOR_DBGLEVEL > 0
  const bool forceEnable = dgs_get_settings()->getBlockByNameEx("auto_antialiasing")->getBool("forceEnable", false);
  useAutoSelectedMethod = useAutoSelectedMethod || forceEnable;
#endif

  if (!useAutoSelectedMethod)
    return nullptr;

  auto availableMethods = render::antialiasing::get_available_methods(false, false);
  if (strcmp(availableMethods, "off") == 0)
  {
    logerr("auto_antialiasing: render::antialiasing must be initialized first");
    return nullptr;
  }

  return get_desired_aa_method(availableMethods, force_enable_and_use_upscaling_method);
}

} // namespace auto_graphics::auto_antialiasing
