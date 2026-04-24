// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/temporalSuperResolution.h>
#include <render/antialiasing.h>
#include <3d/dag_nvFeatures.h>
#include <3d/dag_amdFsr.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_consts.h>
#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_renderPass.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_shaders.h>
#include <generic/dag_initOnDemand.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <startup/dag_loadSettings.h>
#include <math/random/dag_halton.h>
#include <render/temporalAA.h>
#include <render/fxaa.h>
#include <render/smaa.h>
#include <render/hdrRender.h>
#include <render/motionVectorAccess.h>
#include <memory/dag_framemem.h>
#include <util/dag_convar.h>
#include <perfMon/dag_statDrv.h>
#include <workCycle/dag_workCycle.h>
#include <osApiWrappers/dag_atomic.h>
#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>
#if _TARGET_C2

#endif
#include <fast_float/fast_float.h>
#include <workCycle/dag_wcHooks.h>

#if MOBILE_ANTIALIASING && !_TARGET_IOS
#define HAS_SGSR 1
#endif

#if HAS_SGSR
#include <SnapdragonSuperResolution/SnapdragonSuperResolution.h>
#endif

static bool dlss_without_streamline()
{
#if _TARGET_PC_LINUX || (_TARGET_PC_WIN && _M_ARM64)
  return true;
#else
  return false;
#endif
}

CONSOLE_BOOL_VAL("render", amdfsr_debug_view, false);

CONSOLE_BOOL_VAL("render", flagMetalfxUpscale, true);

extern bool grs_draw_wire;

namespace render::antialiasing
{

static AppGlue *app_glue_ptr = nullptr;
inline AppGlue *app_glue()
{
  G_ASSERT(app_glue_ptr);
  return app_glue_ptr;
};

enum class XessQuality
{
  OFF = -1,
  PERFORMANCE,
  BALANCED,
  QUALITY,
  ULTRA_QUALITY,
  ULTRA_QUALITY_PLUS,
  SETTING_AA,
  ULTRA_PERFORMANCE
};

static const char *convert(AntialiasingMethod method)
{
  switch (method)
  {
    case AntialiasingMethod::None: return "off";
    case AntialiasingMethod::MSAA: return "msaa";
    case AntialiasingMethod::FXAALow: return "low_fxaa";
    case AntialiasingMethod::FXAAHigh: return "high_fxaa";
    case AntialiasingMethod::TAA: return "taa";
    case AntialiasingMethod::MobileTAA: return "mobile_taa";
    case AntialiasingMethod::MobileTAALow: return "mobile_taa_low";
    case AntialiasingMethod::TSR: return "tsr";
    case AntialiasingMethod::DLSS: return "dlss";
    case AntialiasingMethod::XeSS: return "xess";
    case AntialiasingMethod::FSR: return "fsr";
    case AntialiasingMethod::SGSR: return "sgsr";
    case AntialiasingMethod::METALFX: return "metalfx";
    case AntialiasingMethod::MOBILE_MSAA: return "mobile_msaa";
    case AntialiasingMethod::SGSR2: return "sgsr2";
    case AntialiasingMethod::SMAA: return "smaa";
    case AntialiasingMethod::SSAA: return "ssaa";
#if _TARGET_C2

#endif
  }
  return "off";
}

static const char *convert(UpscalingQuality name, bool prefix = false)
{
  const char *result = ";native";

  switch (name)
  {
    case UpscalingQuality::Native: result = ";native"; break;
    case UpscalingQuality::UltraQualityPlus: result = ";ultra_quality_plus"; break;
    case UpscalingQuality::UltraQuality: result = ";ultra_quality"; break;
    case UpscalingQuality::Quality: result = ";quality"; break;
    case UpscalingQuality::Balanced: result = ";balanced"; break;
    case UpscalingQuality::Performance: result = ";performance"; break;
    case UpscalingQuality::UltraPerformance: result = ";ultra_performance"; break;
  }
  return prefix ? result : result + 1;
}

static UpscalingQuality convert_quality(const char *name, bool &is_valid)
{
  is_valid = true;
  if (stricmp(name, "native") == 0)
    return UpscalingQuality::Native;
  if (stricmp(name, "ultra_quality_plus") == 0)
    return UpscalingQuality::UltraQualityPlus;
  if (stricmp(name, "ultra_quality") == 0)
    return UpscalingQuality::UltraQuality;
  if (stricmp(name, "quality") == 0)
    return UpscalingQuality::Quality;
  if (stricmp(name, "balanced") == 0)
    return UpscalingQuality::Balanced;
  if (stricmp(name, "performance") == 0)
    return UpscalingQuality::Performance;
  if (stricmp(name, "ultra_performance") == 0)
    return UpscalingQuality::UltraPerformance;

  is_valid = false;
  return UpscalingQuality::Native;
}

static UpscalingQuality convert_quality(const char *name)
{
  bool dummy;
  return convert_quality(name, dummy);
}

static nv::DLSS::Mode convert_to_dlss(UpscalingQuality q)
{
  switch (q)
  {
    case UpscalingQuality::Native: return nv::DLSS::Mode::DLAA;
    case UpscalingQuality::UltraQualityPlus:
    case UpscalingQuality::UltraQuality: // these presets are not supported for DLSS, fallback to Quality
    case UpscalingQuality::Quality: return nv::DLSS::Mode::MaxQuality;
    case UpscalingQuality::Balanced: return nv::DLSS::Mode::Balanced;
    case UpscalingQuality::Performance: return nv::DLSS::Mode::MaxPerformance;
    case UpscalingQuality::UltraPerformance: return nv::DLSS::Mode::UltraPerformance;
  }

  G_ASSERT(false);

  return nv::DLSS::Mode::Off;
}

static XessQuality convert_to_xess(UpscalingQuality q)
{
  switch (q)
  {
    case UpscalingQuality::Native: return XessQuality::SETTING_AA;
    case UpscalingQuality::UltraQualityPlus: return XessQuality::ULTRA_QUALITY_PLUS;
    case UpscalingQuality::UltraQuality: return XessQuality::ULTRA_QUALITY;
    case UpscalingQuality::Quality: return XessQuality::QUALITY;
    case UpscalingQuality::Balanced: return XessQuality::BALANCED;
    case UpscalingQuality::Performance: return XessQuality::PERFORMANCE;
    case UpscalingQuality::UltraPerformance: return XessQuality::ULTRA_PERFORMANCE;
  }

  G_ASSERT(false);

  return XessQuality::OFF;
}

static amd::FSR::UpscalingMode convert_to_fsr(UpscalingQuality q)
{
  switch (q)
  {
    case UpscalingQuality::Native: return amd::FSR::UpscalingMode::NativeAA;
    case UpscalingQuality::UltraQualityPlus:
    case UpscalingQuality::UltraQuality: // these presets are not supported for FSR, fallback to Quality
    case UpscalingQuality::Quality: return amd::FSR::UpscalingMode::Quality;
    case UpscalingQuality::Balanced: return amd::FSR::UpscalingMode::Balanced;
    case UpscalingQuality::Performance: return amd::FSR::UpscalingMode::Performance;
    case UpscalingQuality::UltraPerformance: return amd::FSR::UpscalingMode::UltraPerformance;
  }

  G_ASSERT(false);

  return amd::FSR::UpscalingMode::Off;
}

static AntialiasingMethod convert_method(const char *name, bool &is_valid)
{
  is_valid = true;
  if (stricmp(name, "off") == 0)
    return AntialiasingMethod::None;
  if (stricmp(name, "msaa") == 0)
    return AntialiasingMethod::MSAA;
  if (stricmp(name, "low_fxaa") == 0)
    return AntialiasingMethod::FXAALow;
  if (stricmp(name, "high_fxaa") == 0)
    return AntialiasingMethod::FXAAHigh;
  if (stricmp(name, "taa") == 0)
    return AntialiasingMethod::TAA;
  if (stricmp(name, "mobile_taa") == 0)
    return AntialiasingMethod::MobileTAA;
  if (stricmp(name, "mobile_taa_low") == 0)
    return AntialiasingMethod::MobileTAALow;
  if (stricmp(name, "tsr") == 0)
    return AntialiasingMethod::TSR;
  if (stricmp(name, "dlss") == 0)
    return AntialiasingMethod::DLSS;
  if (stricmp(name, "xess") == 0)
    return AntialiasingMethod::XeSS;
  if (stricmp(name, "fsr") == 0)
    return AntialiasingMethod::FSR;
  if (stricmp(name, "sgsr") == 0)
    return AntialiasingMethod::SGSR;
  if (stricmp(name, "metalfx") == 0)
    return AntialiasingMethod::METALFX;
  if (stricmp(name, "metalfx_fxaa") == 0)
    return AntialiasingMethod::METALFX;
  if (stricmp(name, "mobile_msaa") == 0)
    return AntialiasingMethod::MOBILE_MSAA;
  if (stricmp(name, "sgsr2") == 0)
    return AntialiasingMethod::SGSR2;
  if (stricmp(name, "smaa") == 0)
    return AntialiasingMethod::SMAA;
  if (stricmp(name, "ssaa") == 0)
    return AntialiasingMethod::SSAA;
#if _TARGET_C2


#endif

  is_valid = false;
  return AntialiasingMethod::None;
}

static AntialiasingMethod convert_method(const char *name)
{
  bool dummy;
  return convert_method(name, dummy);
}

static const char *get_method_name_from_config()
{
  return dgs_get_settings()->getBlockByNameEx("video")->getStr("antialiasing_mode", "off");
}

static const char *get_upscaling_name_from_config()
{
  return dgs_get_settings()->getBlockByNameEx("video")->getStr("antialiasing_upscaling", "native");
}

static bool is_xess_quality_available_at_resolution(int request_quality, int screen_width, int screen_height)
{
  XessState currentState = (XessState)d3d::driver_command(Drv3dCommand::GET_XESS_STATE);
  if (currentState != XessState::READY && currentState != XessState::SUPPORTED)
    return false;

  if (screen_width == 0 || screen_height == 0)
  {
    const IPoint2 &postfxResolution = app_glue()->getOutputResolution();
    if (postfxResolution == IPoint2::ZERO)
      return false;

    screen_width = postfxResolution.x;
    screen_height = postfxResolution.y;
  }

  IPoint2 targetResolution(screen_width, screen_height);
  return d3d::driver_command(Drv3dCommand::IS_XESS_QUALITY_AVAILABLE_AT_RESOLUTION, &targetResolution, &request_quality);
}

static bool is_dlss_quality_available_at_resolution(int request_quality, int screen_width, int screen_height)
{
  if (screen_width == 0 || screen_height == 0)
  {
    const IPoint2 &postfxResolution = app_glue()->getOutputResolution();
    if (postfxResolution == IPoint2::ZERO)
      return false;

    screen_width = postfxResolution.x;
    screen_height = postfxResolution.y;
  }

  if (dlss_without_streamline())
  {
    nv::DLSS *dlss = nullptr;
    d3d::driver_command(Drv3dCommand::GET_DLSS, &dlss);
    if (!dlss)
      return false;

    auto optimalSettings = dlss->getOptimalSettings(nv::DLSS::Mode(request_quality), IPoint2(screen_width, screen_height));
    return optimalSettings.has_value();
  }
  else
  {
    nv::Streamline *streamline{};
    d3d::driver_command(Drv3dCommand::GET_STREAMLINE, &streamline);
    if (!streamline)
      return false;

    return streamline->isDlssModeAvailableAtResolution(nv::DLSS::Mode(request_quality), IPoint2(screen_width, screen_height));
  }
}

static IPoint2 get_min_dynamic_resolution(const IPoint2 &rr)
{
  float aspect = float(rr.x) / rr.y;
  return IPoint2(64 * aspect, 64);
}

struct Context
{
  AntialiasingMethod method = AntialiasingMethod::None;
  UpscalingQuality quality = UpscalingQuality::Balanced;
  int qualityOverride = -1;

  int qualityOffset = 0;
  float percentage = 0;
  bool dlssLegacyMode = false;

  eastl::unique_ptr<amd::FSR> fsr;
  eastl::unique_ptr<TemporalAA> taa;
  eastl::unique_ptr<TemporalSuperResolution> tsr;
  eastl::unique_ptr<Antialiasing> antialiasing;
  eastl::unique_ptr<ComputeShader> prepareDlssRayReconstruction;

  eastl::optional<dafg::NodeHandle> aaApplyNode;

#if HAS_SGSR
  eastl::unique_ptr<SnapdragonSuperResolution> sgsr;
  eastl::unique_ptr<AppGlue::SGSR2Interface> sgsr2;
#endif

  eastl::unique_ptr<SMAA> smaa;

  RTargetPool::Ptr ssTargetPool;
  RTargetPool::Ptr ssInputTexturePool;
  RTargetPool::Ptr rTargetMetalfxUpscale;

  float mipBias = 0.f;
  int subsamples = 1;

  Point2 velocityScale = Point2::ZERO;

  int dlssGCount = 0;

  PostFxRenderer uiBlend;

  unsigned int textureFlg = TEXFMT_A16B16G16R16F | TEXCF_RTARGET;

  ~Context()
  {
    // Explicit cleanup needed: close() destroys Context via demandDestroy() without
    // going through reset(), so we must release ShaderGlobal's managed-res reference
    // on tsr_kernel_weights here to ensure RMGR entry is evicted before tsr is destroyed.
    if (tsr)
      tsr->releaseShaderResources();
  }

  void init()
  {
    if (!fsr)
      fsr.reset(amd::createFSR());
    if (fsr && (!fsr->isLoaded() || !fsr->isUpscalingSupported()))
      fsr.reset();

    if (shader_exists("antialiasing"))
      antialiasing.reset(new Antialiasing());

    uiBlend.init("ui_blend", true);

    parseSettings(true);
  }

  void reset()
  {
    ssTargetPool.reset();
    ssInputTexturePool.reset();
    rTargetMetalfxUpscale.reset();

    if (fsr)
      fsr->teardownUpscaling();
    taa.reset();
    // Release ShaderGlobal's managed-res reference before tsr is destroyed.
    // Without this, ShaderGlobal holds a reference (refCount stays >= 1) which
    // prevents eviction of the "tsr_kernel_weights" RMGR entry. A subsequent
    // TSR re-init fails with "texture id already used" when it tries to register
    // a new buffer under the same name (repro: benchmark scene reload on PS4/PS5).
    if (tsr)
      tsr->releaseShaderResources();
    tsr.reset();
    antialiasing.reset();
    prepareDlssRayReconstruction.reset();

    uiBlend.clear();

#if HAS_SGSR
    sgsr.reset();
    sgsr2.reset();
#endif

    smaa.reset();

    if (dlss_without_streamline())
    {
      nv::DLSS *dlss = nullptr;
      d3d::driver_command(Drv3dCommand::GET_DLSS, &dlss);
      if (dlss)
        dlss->DeleteFeature();
    }

    aaApplyNode.reset();
  }

  int parseSettings(bool save_state)
  {
    if (app_glue()->hasMinimumRenderFeatures() || app_glue()->useMobileCodepath())
    {
      auto methodName = get_method_name_from_config();
      auto qualityName = get_upscaling_name_from_config();

#if _TARGET_PC_WIN
      bool isDx12 = d3d::get_driver_code().is(d3d::dx12);
      if (!isDx12 && (stricmp(methodName, "fsr") == 0 || stricmp(methodName, "xess") == 0))
      {
        methodName = "off";
        qualityName = "native";
      }
#endif

      auto newDlssGCount = getCfgFrameGenCount();

      auto newMethod = convert_method(methodName);
      auto newQuality = convert_quality(qualityName);
      float newPercentage = 0.0f;
      fast_float::from_chars(qualityName, qualityName + strlen(qualityName), newPercentage);
      newPercentage *= 0.01f;

      if (qualityOverride >= 0)
      {
        if (qualityOverride == 0) // forced native res
          newQuality = (UpscalingQuality)qualityOverride;
        else // none native, choose best performance one
          newQuality = (UpscalingQuality)max(qualityOverride, (int)newQuality);
        newPercentage = 0.f; // override quality made a priority
      }

      static bool fallbackFxaaLowToBilinear =
        ::dgs_get_settings()->getBlockByNameEx("graphics")->getBool("fallbackFxaaLowToBilinear", false);
      if (newMethod == AntialiasingMethod::FXAALow && fallbackFxaaLowToBilinear) // This if first, we want to override the UI settings
        newMethod = AntialiasingMethod::None;
      else if (newMethod == AntialiasingMethod::FXAAHigh && d3d::get_driver_desc().issues.hasBrokenShadersAfterAppSwitch)
        newMethod = AntialiasingMethod::FXAALow;

      bool newDlssLegacyMode = getdlssLegacyModeConfig();

      bool needRecreate = method != newMethod || quality != newQuality || percentage != newPercentage || dlssGCount != newDlssGCount ||
                          dlssLegacyMode != newDlssLegacyMode;
      auto needReset = needRecreate && (method == AntialiasingMethod::DLSS || method == AntialiasingMethod::XeSS ||
                                         method == AntialiasingMethod::MSAA || newMethod == AntialiasingMethod::DLSS ||
                                         newMethod == AntialiasingMethod::XeSS || newMethod == AntialiasingMethod::MSAA);

      if (save_state)
      {
        method = newMethod;
        quality = newQuality;
        percentage = newPercentage;
        dlssGCount = newDlssGCount;
        dlssLegacyMode = newDlssLegacyMode;
        debug("using AA method %s", convert(method));
      }

      int r = 0;
      r |= needReset ? (int)SettingsChange::NeedDeviceReset : 0;
      r |= needRecreate ? (int)SettingsChange::NeedRecreate : 0;
      return r;
    }

    method = AntialiasingMethod::None;
    quality = UpscalingQuality::Native;
    percentage = 0;

    return 0;
  }

  void initCommonUpscaling(const char *name, int quality_msg, const IPoint2 inputResolution, const IPoint2 outputResolution)
  {
    textureFlg = TEXFMT_A16B16G16R16F | TEXCF_UNORDERED | TEXCF_RTARGET;
    ssTargetPool = RTargetPool::get(outputResolution.x, outputResolution.y, textureFlg, 1);

    const int dlssInputTextureFlags = TEXFMT_A16B16G16R16F | TEXCF_UNORDERED | TEXCF_RTARGET;
    ssInputTexturePool = RTargetPool::get(inputResolution.x, inputResolution.y, dlssInputTextureFlags, 1);

    mipBias = log2((float)inputResolution.y / (float)outputResolution.y);

    const int jitterSequenceBaseLength = app_glue()->isVrHmdEnabled() ? 16 : 8;
    float exponent = ceil(log2(jitterSequenceBaseLength * sqr(float(outputResolution.y) / float(inputResolution.y))));
    subsamples = int(exp2(exponent));

    debug("%s quality=%d, w=%d, h=%d, mipBias=%f, subsamples=%d", name, quality_msg, inputResolution.x, inputResolution.y, mipBias,
      subsamples);
  }

  static int getCfgFrameGenCount() { return dgs_get_settings()->getBlockByNameEx("video")->getInt("antialiasing_fgc", 0); }
  static int getRRConfig() { return dgs_get_settings()->getBlockByNameEx("video")->getBool("rayReconstruction", false); }
  static bool getdlssLegacyModeConfig()
  {
    const char *configStr = dgs_get_settings()->getBlockByNameEx("video")->getStr("dlssLegacyMode", "auto");
    if (stricmp(configStr, "on") == 0)
      return true;
    else if (stricmp(configStr, "off") == 0)
      return false;

    // auto mode - enable legacy mode based on GPU architecture, as newer modes may be slower on older GPUs
    uint32_t gpuFamily = d3d::get_driver_desc().info.family;
    bool result = gpuFamily < DeviceAttributesBase::NvidiaFamily::AD100;
    logdbg("DLSS legacy mode config is set to auto, detected GPU family: 0x%08x, using %s", gpuFamily,
      result ? "legacy mode" : "default mode");
    return result;
  }

  bool tryInitDlss(const IPoint2 &outputResolution, IPoint2 &inputResolution, IPoint2 &min_dynamic_resolution,
    IPoint2 &max_dynamic_resolution, const char *input_name)
  {
    if (method != AntialiasingMethod::DLSS)
      return false;

    nv::DLSS::Mode dlssQuality;
    eastl::optional<nv::DLSS::OptimalSettings> optimalSettings;

    if (dlss_without_streamline())
    {
      nv::DLSS *dlss = nullptr;
      d3d::driver_command(Drv3dCommand::GET_DLSS, &dlss);
      if (!dlss)
        return false;

      dlssQuality = convert_to_dlss(quality);
      if (dlssQuality == nv::DLSS::Mode::Off)
        return false;

      optimalSettings = dlss->getOptimalSettings(dlssQuality, outputResolution);
      if (!optimalSettings.has_value())
        return false;

      if (!dlss->setOptions(dlssQuality, outputResolution, getRRConfig(), dlssLegacyMode))
        return false;

      inputResolution = IPoint2(optimalSettings->renderWidth, optimalSettings->renderHeight);
      min_dynamic_resolution = IPoint2(optimalSettings->renderMinWidth, optimalSettings->renderMinHeight);
      max_dynamic_resolution = IPoint2(optimalSettings->renderMaxWidth, optimalSettings->renderMaxHeight);

      if (getRRConfig() && dlss->supportRayReconstruction())
        prepareDlssRayReconstruction = eastl::make_unique<ComputeShader>("prepare_ray_reconstruction");
    }
    else
    {
      nv::Streamline *streamline = nullptr;
      d3d::driver_command(Drv3dCommand::GET_STREAMLINE, &streamline);
      if (!streamline)
        return false;

      bool dlssReady = streamline->getDlssState() == nv::DLSS::State::READY;
      if (!dlssReady)
        return false;

      dlssQuality = convert_to_dlss(quality);
      if (dlssQuality == nv::DLSS::Mode::Off)
        return false;

      optimalSettings = streamline->getDlssFeature(0)->getOptimalSettings(dlssQuality, outputResolution);
      if (!optimalSettings)
        return false;

      inputResolution = IPoint2(optimalSettings->renderWidth, optimalSettings->renderHeight);
      min_dynamic_resolution = IPoint2(optimalSettings->renderMinWidth, optimalSettings->renderMinHeight);
      max_dynamic_resolution = IPoint2(optimalSettings->renderMaxWidth, optimalSettings->renderMaxHeight);

      if (optimalSettings->rayReconstruction)
        prepareDlssRayReconstruction = eastl::make_unique<ComputeShader>("prepare_ray_reconstruction");

      streamline->getDlssFeature(0)->setOptions(dlssQuality, outputResolution, false, dlssLegacyMode);
    }

    dlssGCount = getCfgFrameGenCount();

    initCommonUpscaling("DLSS", int(dlssQuality), inputResolution, outputResolution);

    if (input_name)
    {
      aaApplyNode = dafg::register_node("dlss", DAFG_PP_NODE_SRC, [this, outputResolution, input_name](dafg::Registry registry) {
        textureFlg = TEXFMT_A16B16G16R16F | TEXCF_UNORDERED | TEXCF_RTARGET;
        auto antialiasedHndl = registry.createTexture2d("frame_after_aa", {textureFlg, outputResolution})
                                 .atStage(dafg::Stage::PS_OR_CS)
                                 .useAs(dafg::Usage::COLOR_ATTACHMENT)
                                 .handle();

        auto frameHndl =
          registry.read(input_name).texture().atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
        auto applyCtxHndl = registry.readBlob<ApplyContext>("aa_apply_context").handle();

        return
          [this, frameHndl, applyCtxHndl, antialiasedHndl] { applyDlss(frameHndl.get(), applyCtxHndl.ref(), antialiasedHndl.get()); };
      });
    }

    return true;
  }

  void applyDlss(Texture *in_color, const ApplyContext &apply_context, Texture *target)
  {
    TIME_D3D_PROFILE(applyDlss);

    G_ASSERT(in_color);
    G_ASSERT(apply_context.depthTexture);
    G_ASSERT(apply_context.motionTexture);

    G_ASSERT_RETURN(target, );

    const IPoint2 &renderingResolution = app_glue()->getInputResolution();
    nv::DlssParams dlssParams = {};
    dlssParams.inColor = in_color;
    dlssParams.inDepth = apply_context.depthTexture;
    dlssParams.inMotionVectors = apply_context.motionTexture;
    dlssParams.inExposure = apply_context.exposureTexture;
    dlssParams.inSsssGuide = apply_context.ssssGuideTexture;
    dlssParams.inColorBeforeTransparency = apply_context.colorBeforeTransparencyTexture;
    dlssParams.inAlbedo = apply_context.albedoTexture;
    dlssParams.inHitDist = apply_context.hitDistTexture;
    dlssParams.inNormalRoughness = apply_context.normalRoughnessTexture;
    dlssParams.inSpecularAlbedo = apply_context.specularAlbedoTexture;
    dlssParams.inJitterOffsetX = apply_context.jitterPixelOffset.x;
    dlssParams.inJitterOffsetY = apply_context.jitterPixelOffset.y;
    dlssParams.outColor = target;
    dlssParams.inMVScaleX = 1;
    dlssParams.inMVScaleY = 1;
    dlssParams.inColorDepthOffsetX = apply_context.inputOffset.x;
    dlssParams.inColorDepthOffsetY = apply_context.inputOffset.y;
    dlssParams.inWidth = renderingResolution.x;
    dlssParams.inHeight = renderingResolution.y;
    dlssParams.frameId = dagor_get_global_frame_id();
    dlssParams.camera.projection = apply_context.projection;
    dlssParams.camera.projectionInverse = apply_context.projectionInverse;
    dlssParams.camera.reprojection = apply_context.reprojection;
    dlssParams.camera.reprojectionInverse = apply_context.reprojectionInverse;
    dlssParams.camera.right = apply_context.right;
    dlssParams.camera.up = apply_context.up;
    dlssParams.camera.forward = apply_context.forward;
    dlssParams.camera.position = apply_context.position;
    dlssParams.camera.nearZ = apply_context.nearZ;
    dlssParams.camera.farZ = apply_context.farZ;
    dlssParams.camera.fov = apply_context.fov;
    dlssParams.camera.aspect = apply_context.aspect;
    dlssParams.camera.worldToView = apply_context.worldToView;
    dlssParams.camera.viewToWorld = apply_context.viewToWorld;

    RTarget::Ptr normalRoughness, specularAlbedo;

    if (prepareDlssRayReconstruction && (dlssParams.inNormalRoughness == nullptr || dlssParams.inSpecularAlbedo == nullptr))
    {
      normalRoughness = ssInputTexturePool->acquire();
      specularAlbedo = ssInputTexturePool->acquire();

      static int dlss_normal_roughnessVarId = get_shader_variable_id("dlss_normal_roughness", true);
      static int dlss_specular_albedoVarId = get_shader_variable_id("dlss_specular_albedo", true);

      ShaderGlobal::set_texture(dlss_normal_roughnessVarId, normalRoughness->getTexId());
      ShaderGlobal::set_texture(dlss_specular_albedoVarId, specularAlbedo->getTexId());

      TextureInfo texinfo;
      specularAlbedo->getTex2D()->getinfo(texinfo);

      prepareDlssRayReconstruction->dispatchThreads(texinfo.w, texinfo.h, 1);

      dlssParams.inNormalRoughness = normalRoughness->getTex2D();
      dlssParams.inSpecularAlbedo = specularAlbedo->getTex2D();
    }

    if (dlss_without_streamline())
      d3d::driver_command(Drv3dCommand::EXECUTE_DLSS_NO_STREAMLINE, &dlssParams);
    else
    {
      int viewIndex = apply_context.viewIndex;
      d3d::driver_command(Drv3dCommand::EXECUTE_DLSS, &dlssParams, &viewIndex);
    }
  }

  bool tryInitFsr(const IPoint2 &outputResolution, IPoint2 &inputResolution, IPoint2 &min_dynamic_resolution,
    IPoint2 &max_dynamic_resolution, const char *input_name)
  {
    if (method != AntialiasingMethod::FSR || !fsr)
      return false;

    amd::FSR::ContextArgs args;
    args.enableHdr = hdrrender::is_hdr_enabled();
    args.outputWidth = outputResolution.x;
    args.outputHeight = outputResolution.y;
    args.mode = convert_to_fsr(quality);
    if (!fsr->initUpscaling(args))
      return false;

    inputResolution = fsr->getRenderingResolution(args.mode, outputResolution);

    max_dynamic_resolution = inputResolution;
    min_dynamic_resolution = get_min_dynamic_resolution(inputResolution);

    initCommonUpscaling("FSR", int(args.mode), inputResolution, outputResolution);

    if (input_name)
    {
      aaApplyNode = dafg::register_node("fsr", DAFG_PP_NODE_SRC, [this, outputResolution, input_name](dafg::Registry registry) {
        textureFlg = TEXFMT_A16B16G16R16F | TEXCF_UNORDERED | TEXCF_RTARGET;
        auto antialiasedHndl = registry.createTexture2d("frame_after_aa", {textureFlg, outputResolution})
                                 .atStage(dafg::Stage::PS_OR_CS)
                                 .useAs(dafg::Usage::COLOR_ATTACHMENT)
                                 .handle();

        auto applyCtxHndl = registry.readBlob<ApplyContext>("aa_apply_context").handle();
        auto frameHndl =
          registry.read(input_name).texture().atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();

        return
          [this, frameHndl, applyCtxHndl, antialiasedHndl] { applyFsr(frameHndl.get(), applyCtxHndl.ref(), antialiasedHndl.get()); };
      });
    }

    return true;
  }

  void applyFsr(Texture *in_color, const ApplyContext &apply_context, Texture *target)
  {
    TIME_D3D_PROFILE(applyFsr);

    G_ASSERT(in_color);
    G_ASSERT(apply_context.depthTexture);
    G_ASSERT(apply_context.motionTexture);

    G_ASSERT_RETURN(target, );

    amd::FSR::UpscalingArgs args;
    args.colorTexture = in_color;
    args.depthTexture = apply_context.depthTexture;
    args.motionVectors = apply_context.motionTexture;
    args.exposureTexture = apply_context.exposureTexture;
    args.outputTexture = target;
    args.reactiveTexture = nullptr;
    args.transparencyAndCompositionTexture = nullptr;
    args.jitter.x = apply_context.jitterPixelOffset.x;
    args.jitter.y = apply_context.jitterPixelOffset.y;
    args.motionVectorScale.x = app_glue()->getInputResolution().x;
    args.motionVectorScale.y = app_glue()->getInputResolution().y;
    args.reset = apply_context.resetHistory;
    args.debugView = amdfsr_debug_view;
    args.sharpness = 0;
    args.timeElapsed = apply_context.timeElapsed;
    args.preExposure = 1;
    args.inputResolution = app_glue()->getInputResolution();
    args.outputResolution = app_glue()->getOutputResolution();
    args.fovY = apply_context.persp.hk;
    args.nearPlane = apply_context.persp.zf; // near and far are swapped as for inverted depth,
    args.farPlane = apply_context.persp.zn;  // FSR require far to be closer than near
    args.frameIndex = dagor_get_global_frame_id();
    fsr->applyUpscaling(args);
  }

  bool tryInitXess(const IPoint2 &outputResolution, IPoint2 &inputResolution, IPoint2 &min_dynamic_resolution,
    IPoint2 &max_dynamic_resolution, const char *input_name)
  {
    if (method != AntialiasingMethod::XeSS)
      return false;

    bool ready = XessState(d3d::driver_command(Drv3dCommand::GET_XESS_STATE)) == XessState::READY;
    if (!ready)
      return false;

    d3d::driver_command(Drv3dCommand::GET_XESS_RESOLUTION, &inputResolution, &min_dynamic_resolution, &max_dynamic_resolution);

    velocityScale = Point2(0, 0); // force reset motion scale to each xess init, otherwise it will stuck with default data
    d3d::driver_command(Drv3dCommand::SET_XESS_VELOCITY_SCALE, &velocityScale.x, &velocityScale.y);

    initCommonUpscaling("Xess", int(convert_to_xess(quality)), inputResolution, outputResolution);

    if (input_name)
    {
      aaApplyNode = dafg::register_node("xess", DAFG_PP_NODE_SRC, [this, outputResolution, input_name](dafg::Registry registry) {
        textureFlg = TEXFMT_A16B16G16R16F | TEXCF_UNORDERED;
        auto antialiasedHndl = registry.createTexture2d("frame_after_aa", {textureFlg, outputResolution})
                                 .atStage(dafg::Stage::CS)
                                 .useAs(dafg::Usage::SHADER_RESOURCE)
                                 .handle();

        auto applyCtxHndl = registry.readBlob<ApplyContext>("aa_apply_context").handle();
        auto frameHndl =
          registry.read(input_name).texture().atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();

        return
          [this, frameHndl, applyCtxHndl, antialiasedHndl] { applyXess(frameHndl.get(), applyCtxHndl.ref(), antialiasedHndl.get()); };
      });
    }

    return true;
  }

  void applyXess(Texture *in_color, const ApplyContext &apply_context, Texture *target)
  {
    TIME_D3D_PROFILE(applyXess);

    G_ASSERT(in_color);
    G_ASSERT(apply_context.depthTexture);
    G_ASSERT(apply_context.motionTexture);

    G_ASSERT_RETURN(target, );

    const IPoint2 &renderingResolution = app_glue()->getInputResolution();
    XessParams xessParams = {};
    xessParams.inColor = in_color;
    xessParams.inDepth = apply_context.depthTexture;
    xessParams.inMotionVectors = apply_context.motionTexture;
    xessParams.inJitterOffsetX = apply_context.jitterPixelOffset.x;
    xessParams.inJitterOffsetY = apply_context.jitterPixelOffset.y;
    xessParams.outColor = target;
    xessParams.inInputWidth = renderingResolution.x;
    xessParams.inInputHeight = renderingResolution.y;
    xessParams.inColorDepthOffsetX = apply_context.inputOffset.x;
    xessParams.inColorDepthOffsetY = apply_context.inputOffset.y;
    xessParams.inReset = apply_context.resetHistory;

    d3d::driver_command(Drv3dCommand::EXECUTE_XESS, &xessParams);
  }

  bool tryInitTaa(const IPoint2 &outputResolution, IPoint2 &inputResolution, const char *input_name)
  {
    if (method != AntialiasingMethod::TAA)
      return false;

    float resolutionScale = 1;
    if (percentage > 0)
      resolutionScale = percentage;
    else
      switch (quality)
      {
        case UpscalingQuality::Native: resolutionScale = 1.0f; break;
        case UpscalingQuality::UltraQualityPlus: // this preset is not supported for TAA, fallback to UltraQuality
        case UpscalingQuality::UltraQuality: resolutionScale = 1.3f; break;
        case UpscalingQuality::Quality: resolutionScale = 1.5f; break;
        case UpscalingQuality::Balanced: resolutionScale = 1.7f; break;
        case UpscalingQuality::Performance: resolutionScale = 2.0f; break;
        default:
          resolutionScale = 1.0f;
          G_ASSERT(false);
          break;
      }

    inputResolution.x = round(outputResolution.x / (resolutionScale));
    inputResolution.y = round(outputResolution.y / (resolutionScale));

    if (inputResolution.x < 32 || inputResolution.y < 32)
    {
      logerr("specified temporalResolutionScale %f can't be used at resolution %ix%i", resolutionScale, outputResolution.x,
        outputResolution.y);
      return false;
    }

    const char *shaderName = "taa_lq"; // no more HQ TAA on WT
    taa = eastl::make_unique<TemporalAA>(shaderName, inputResolution, outputResolution, app_glue()->getTargetFormat(), true, true);
    taa->loadParamsFromBlk(::dgs_get_game_params()->getBlockByNameEx("taa"));

    mipBias = taa->getLodBias();

    if (input_name)
    {
      aaApplyNode = dafg::register_node("taa", DAFG_PP_NODE_SRC, [this, outputResolution, input_name](dafg::Registry registry) {
        textureFlg = TEXFMT_A16B16G16R16F | TEXCF_RTARGET;
        auto antialiasedHndl = registry.createTexture2d("frame_after_aa", {textureFlg, outputResolution})
                                 .atStage(dafg::Stage::PS_OR_CS)
                                 .useAs(dafg::Usage::COLOR_ATTACHMENT)
                                 .handle();

        auto frameHndl =
          registry.read(input_name).texture().atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
        registry.readBlob<ApplyContext>("aa_apply_context"); // For proper ordering

        return [this, frameHndl, antialiasedHndl] { applyTaa(frameHndl.get(), antialiasedHndl.get()); };
      });
    }

    return true;
  }

  void applyTaa(Texture *in_color, Texture *target)
  {
    TIME_D3D_PROFILE(applyTaa);

    G_ASSERT(in_color);
    G_ASSERT_RETURN(target, );

    SCOPE_RESET_SHADER_BLOCKS;

    static int global_frameBlockId = ShaderGlobal::getBlockId("global_frame");
    ShaderGlobal::setBlock(global_frameBlockId, ShaderGlobal::LAYER_FRAME);

    taa->apply(in_color, target);
  }

  bool tryInitMobileTaa(const IPoint2 &display_resolution, const IPoint2 &rendering_resolution)
  {
    if (method != AntialiasingMethod::MobileTAA && method != AntialiasingMethod::MobileTAALow)
      return false;

    textureFlg = hdrrender::is_hdr_enabled() ? TEXFMT_A16B16G16R16F : TEXFMT_DEFAULT;
    const char *shaderName = "taa_wtm"; // taa version without dynamic tex for wtm
    taa = eastl::make_unique<TemporalAA>(shaderName, rendering_resolution, display_resolution, textureFlg, true, false, true);

    taa->loadParamsFromBlk(::dgs_get_game_params()->getBlockByNameEx("taa"));

    mipBias = taa->getLodBias();

    return true;
  }

  void applyMobileTaa(Texture *source_tex, Texture *dest_tex)
  {
    TIME_D3D_PROFILE(applyMobileTaa);

    G_ASSERT(source_tex);

    SCOPE_RESET_SHADER_BLOCKS;

    static int global_frameBlockId = ShaderGlobal::getBlockId("global_frame");
    ShaderGlobal::setBlock(global_frameBlockId, ShaderGlobal::LAYER_FRAME);

    d3d::set_render_target({}, DepthAccess::RW, {{dest_tex, 0, 0}});
    taa->applyToCurrentTarget(source_tex);

    // We have change of attachments inside TAA, so we have to start subsequent pass from loading backbuffer.
    d3d::allow_render_pass_target_load();
  }

  bool tryInitTsr(const IPoint2 &outputResolution, IPoint2 &inputResolution, const char *input_name,
    const IPoint2 &displayResolution = {0, 0})
  {
    if (method != AntialiasingMethod::TSR)
      return false;

    float resolutionScale = 1;

    if (TemporalSuperResolution::parse_preset(app_glue()->isVrHmdEnabled()) == TemporalSuperResolution::Preset::High)
    {
      if (percentage > 0)
        resolutionScale = percentage;
      else
        switch (quality)
        {
          case UpscalingQuality::Native: resolutionScale = 1.0f; break;
          case UpscalingQuality::UltraQualityPlus: // this preset is not supported for TSR, fallback to UltraQuality
          case UpscalingQuality::UltraQuality: resolutionScale = 1.3f; break;
          case UpscalingQuality::Quality: resolutionScale = 1.5f; break;
          case UpscalingQuality::Balanced: resolutionScale = 1.7f; break;
          case UpscalingQuality::Performance: resolutionScale = 2.0f; break;
          default:
            resolutionScale = 1.0f;
            G_ASSERT(false);
            break;
        }
    }

    inputResolution.x = round(outputResolution.x / (resolutionScale));
    inputResolution.y = round(outputResolution.y / (resolutionScale));

    if (inputResolution.x < 32 || inputResolution.y < 32)
    {
      logerr("specified temporalResolutionScale %f can't be used at resolution %ix%i", resolutionScale, outputResolution.x,
        outputResolution.y);
      return false;
    }

    initCommonUpscaling("TSR", int(resolutionScale * 100.f), inputResolution, outputResolution);

    tsr = eastl::make_unique<TemporalSuperResolution>(inputResolution, outputResolution, app_glue()->isVrHmdEnabled());

    mipBias = tsr->getLodBias();

    if (input_name)
    {
      bool isSsaa = displayResolution.x > 0 && (outputResolution.x > displayResolution.x || outputResolution.y > displayResolution.y);
      tsr->setExtraTexFlags(isSsaa ? TEXCF_RTARGET : 0);
      aaApplyNode = tsr->createApplierNode(input_name);
    }

    return true;
  }

  void applyTsr(Texture *in_color, const ApplyContext &apply_context, Texture *out_color, Texture *history_color,
    Texture *out_confidence, Texture *history_confidence, Texture *reactive_tex)
  {
    TIME_D3D_PROFILE(applyTsr);

    tsr->apply(in_color, out_color, history_color, out_confidence, history_confidence, reactive_tex,
      tsr->getDebugRenderTarget().getTex2D(), Point4::ZERO, apply_context.resetHistory, apply_context.jitterPixelOffset,
      apply_context.vrVrsMask, apply_context.inputResolution);
  }

  bool tryInitMetalFx()
  {
    if (method != AntialiasingMethod::METALFX)
      return false;

    if (!is_metalfx_upscale_supported())
      return false;

    return true;
  }

  void applyMetalFx(Texture *source_tex, Texture *dest_tex)
  {
    TIME_D3D_PROFILE(applyMetalFx);

    G_ASSERT(source_tex);

    const Point2 &displayResolution = app_glue()->getDisplayResolution();
    textureFlg = (hdrrender::is_hdr_enabled() ? TEXFMT_R11G11B10F : TEXFMT_A8R8G8B8) | TEXCF_RTARGET;
    if (!rTargetMetalfxUpscale)
      rTargetMetalfxUpscale = RTargetPool::get(displayResolution.x, displayResolution.y, textureFlg, 1);

    auto rtMetalfxUpscale = rTargetMetalfxUpscale->acquire();

    MtlFxUpscaleParams params;
    params.colorMode = MtlfxColorMode::PERCEPTUAL;
    params.color = source_tex;
    params.output = rtMetalfxUpscale.get()->getBaseTex();
    d3d::driver_command(Drv3dCommand::EXECUTE_METALFX_UPSCALE, &params);
    d3d::stretch_rect(rtMetalfxUpscale.get()->getBaseTex(), dest_tex, nullptr, nullptr);
    d3d::set_render_target();

    rtMetalfxUpscale.reset();
  }

#if HAS_SGSR
  bool tryInitSgsr(const IPoint2 &inputResolution)
  {
    if (method != AntialiasingMethod::SGSR)
      return false;

    sgsr = eastl::make_unique<SnapdragonSuperResolution>();
    sgsr->SetViewport(inputResolution.x, inputResolution.y);

    return true;
  }

  void applySgsr(Texture *source_tex, Texture *dest_tex)
  {
    TIME_D3D_PROFILE(applySgsr);

    G_ASSERT(source_tex);

    static int global_frameBlockId = ShaderGlobal::getBlockId("global_frame");
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
    sgsr->render(source_tex, dest_tex);
    ShaderGlobal::setBlock(global_frameBlockId, ShaderGlobal::LAYER_FRAME);
  }

  bool tryInitSgsr2(const IPoint2 &outputResolution, IPoint2 &inputResolution)
  {
    if (method != AntialiasingMethod::SGSR2)
      return false;
    sgsr2.reset(app_glue()->createSGSR2(inputResolution, outputResolution));
    mipBias = sgsr2->getLodBias();
    return true;
  }

  void applySgsr2(Texture *source_tex, Texture *dest_tex, bool reset)
  {
    TIME_D3D_PROFILE(applySgsr2);

    G_ASSERT(source_tex);

    sgsr2->apply(source_tex, dest_tex, reset);
  }
#endif

  bool tryInitSmaa(const IPoint2 &resolution)
  {
    if (method != AntialiasingMethod::SMAA)
      return false;
    smaa = eastl::make_unique<SMAA>(resolution);
    return true;
  }

  void applySmaa(Texture *source_tex, Texture *dest_tex) { smaa->apply(source_tex, dest_tex); }

#if _TARGET_C2

























































































#endif

  void scheduleGeneratedFrames(const FrameGenContext &ctx)
  {
    auto blendGui = [&]() {
      static int ui_texVarId = get_shader_variable_id("ui_tex", true);

      SCOPE_RENDER_TARGET;
      ShaderGlobal::set_texture(ui_texVarId, ctx.uiTexture);
      d3d::set_render_target(ctx.finalImageTexture ? ctx.finalImageTexture : d3d::get_backbuffer_tex(), 0);
      uiBlend.render();
      ShaderGlobal::set_texture(ui_texVarId, nullptr);
    };

    if (method == AntialiasingMethod::DLSS)
    {
      nv::Streamline *streamline = nullptr;
      d3d::driver_command(Drv3dCommand::GET_STREAMLINE, &streamline);
      nv::DLSSFrameGeneration *dlssg = streamline ? streamline->getDlssGFeature(0) : nullptr;
      if (!dlssg)
        return;

      enableFrameGeneration(true);

      nv::DlssGParams dlssGParams = {};
      dlssGParams.inHUDless = ctx.finalImageTexture ? ctx.finalImageTexture : d3d::get_backbuffer_tex();
      dlssGParams.inUI = ctx.uiTexture;
      dlssGParams.inMotionVectors = ctx.motionVectorTexture;
      dlssGParams.inDepth = ctx.depthTexture;
      dlssGParams.inJitterOffsetX = ctx.jitterPixelOffset.x;
      dlssGParams.inJitterOffsetY = ctx.jitterPixelOffset.y;
      dlssGParams.inMVScaleX = 1.f;
      dlssGParams.inMVScaleY = 1.f;
      dlssGParams.camera.projection = ctx.noJitterProjTm;
      dlssGParams.camera.projectionInverse = inverse44(ctx.noJitterProjTm);
      dlssGParams.camera.reprojection = inverse44(ctx.noJitterGlobTm) * ctx.prevNoJitterGlobTm;
      dlssGParams.camera.reprojectionInverse = inverse44(ctx.prevNoJitterGlobTm) * ctx.noJitterGlobTm;
      dlssGParams.camera.right = ctx.viewItm.getcol(0);
      dlssGParams.camera.up = ctx.viewItm.getcol(1);
      dlssGParams.camera.forward = ctx.viewItm.getcol(2);
      dlssGParams.camera.position = ctx.viewItm.getcol(3);
      dlssGParams.camera.nearZ = ctx.noJitterPersp.zn;
      dlssGParams.camera.farZ = ctx.noJitterPersp.zf;
      dlssGParams.camera.fov = 2 * atan(1.f / ctx.noJitterPersp.wk);
      dlssGParams.camera.aspect = ctx.noJitterPersp.hk / ctx.noJitterPersp.wk;
      dlssGParams.frameId = dagor_get_global_frame_id();
      dlssGParams.inReset = ctx.resetHistory;

      d3d::driver_command(Drv3dCommand::EXECUTE_DLSS_G, &dlssGParams);
    }
    else if (method == AntialiasingMethod::FSR)
    {
      enableFrameGeneration(true);

      amd::FSR::FrameGenArgs args;
      args.colorTexture = ctx.finalImageTexture ? ctx.finalImageTexture : d3d::get_backbuffer_tex();
      args.depthTexture = ctx.depthTexture;
      args.motionVectors = ctx.motionVectorTexture;
      args.uiTexture = ctx.uiTexture;
      args.jitter.x = ctx.jitterPixelOffset.x;
      args.jitter.y = ctx.jitterPixelOffset.y;
      args.motionVectorScale = Point2::xy(app_glue()->getInputResolution());
      args.reset = ctx.resetHistory;
      args.debugView = amdfsr_debug_view;
      args.timeElapsed = ctx.timeElapsed;
      args.inputResolution = app_glue()->getInputResolution();
      args.outputResolution = app_glue()->getOutputResolution();
      args.fovY = 2 * atan(1.f / ctx.noJitterPersp.wk);
      args.nearPlane = ctx.noJitterPersp.zf; // near and far are swapped as for inverted depth,
      args.farPlane = ctx.noJitterPersp.zn;  // FSR require far to be closer than near
      args.frameIndex = dagor_get_global_frame_id();
      fsr->scheduleGeneratedFrames(args);

      if (fsr->isFrameGenerationSuppressed())
        blendGui();
    }
    else if (method == AntialiasingMethod::XeSS)
    {
      enableFrameGeneration(true);

      TIME_D3D_PROFILE(applyXess);

      XessFgParams xessParams = {};
      xessParams.viewTm = orthonormalized_inverse(ctx.viewItm);
      xessParams.projTm = ctx.noJitterProjTm;
      xessParams.inColorHudless = ctx.finalImageTexture ? ctx.finalImageTexture : d3d::get_backbuffer_tex();
      xessParams.inUi = ctx.uiTexture;
      xessParams.inDepth = ctx.depthTexture;
      xessParams.inMotionVectors = ctx.motionVectorTexture;
      xessParams.inMotionVectorScaleX = app_glue()->getInputResolution().x;
      xessParams.inMotionVectorScaleY = app_glue()->getInputResolution().y;
      xessParams.inJitterOffsetX = ctx.jitterPixelOffset.x;
      xessParams.inJitterOffsetY = ctx.jitterPixelOffset.y;
      xessParams.inFrameIndex = dagor_get_global_frame_id();
      xessParams.inReset = ctx.resetHistory;

      d3d::driver_command(Drv3dCommand::XESS_SCHEDULE_GEN_FRAME, &xessParams);

      // Blend ui to the output. XeSS only does this for generated frames for some reason.
      blendGui();
    }
  }

  void suppressFrameGeneration(bool suppress)
  {
    if (method == AntialiasingMethod::DLSS)
    {
      nv::Streamline *streamline = nullptr;
      d3d::driver_command(Drv3dCommand::GET_STREAMLINE, &streamline);
      nv::DLSSFrameGeneration *dlssg = streamline ? streamline->getDlssGFeature(0) : nullptr;
      if (dlssg)
        dlssg->setSuppressed(suppress);
    }
    else if (method == AntialiasingMethod::FSR)
    {
      fsr->suppressFrameGeneration(suppress);
    }
    else if (method == AntialiasingMethod::XeSS)
    {
      d3d::driver_command(Drv3dCommand::XESS_SUPPRESS_FG, &suppress);
    }
  }

  void enableFrameGeneration(bool enable)
  {
    if (method == AntialiasingMethod::DLSS)
    {
      nv::Streamline *streamline = nullptr;
      d3d::driver_command(Drv3dCommand::GET_STREAMLINE, &streamline);
      nv::DLSSFrameGeneration *dlssg = streamline ? streamline->getDlssGFeature(0) : nullptr;
      if (dlssg)
        dlssg->setEnabled(enable ? getCfgFrameGenCount() : 0);
    }
    else if (method == AntialiasingMethod::FSR)
    {
      fsr->enableFrameGeneration(enable);
    }
    else if (method == AntialiasingMethod::XeSS)
    {
      d3d::driver_command(Drv3dCommand::XESS_ENABLE_FG, &enable);
    }
  }

  int getPresentedFrameCount()
  {
    if (method == AntialiasingMethod::DLSS)
    {
      if (dlss_without_streamline())
        return 1;
      else
      {
        nv::Streamline *streamline = nullptr;
        d3d::driver_command(Drv3dCommand::GET_STREAMLINE, &streamline);
        nv::DLSSFrameGeneration *dlssg = streamline ? streamline->getDlssGFeature(0) : nullptr;
        if (dlssg)
          return dlssg->getActualFramesPresented();
      }
    }
    else if (method == AntialiasingMethod::FSR)
    {
      return fsr ? fsr->getPresentedFrameCount() : 1;
    }
    else if (method == AntialiasingMethod::XeSS)
    {
      int frameCount = 1;
      d3d::driver_command(Drv3dCommand::GET_XESS_PRESENTED_FRAME_COUNT, &frameCount);
      return frameCount;
    }

    return 1;
  }

  int getSupportedGeneratedFrames(AntialiasingMethod for_method, bool exclusive_fullscreen)
  {
    if (for_method == AntialiasingMethod::DLSS)
    {
      if (dlss_without_streamline())
        return 0;
      else
      {
        nv::Streamline *streamline = nullptr;
        d3d::driver_command(Drv3dCommand::GET_STREAMLINE, &streamline);
        if (streamline)
          return streamline->getMaximumNumberOfGeneratedFrames();
      }
    }
    else if (for_method == AntialiasingMethod::FSR)
    {
      if (exclusive_fullscreen)
        return 0;

      return amd::FSR::getMaximumNumberOfGeneratedFrames();
    }
    else if (for_method == AntialiasingMethod::XeSS)
    {
      if (exclusive_fullscreen)
        return 0;

      int genFrames = 0;
      d3d::driver_command(Drv3dCommand::GET_XESS_SUPPORTED_GEN_FRAMES, &genFrames);
      return genFrames;
    }

    return 0;
  }

  const char *getFrameGenerationUnsupportedReason(const char *method_name, bool exclusive_fullscreen) const
  {
    static const char *kOsOutOfDate = "frame_generation/os_out_of_date";
    static const char *kDriverOutOfDate = "frame_generation/driver_out_of_date";
    static const char *kGpuNotSupported = "frame_generation/gpu_not_supported";
    static const char *kHwSchedulingDisabled = "frame_generation/hws_disabled";
    static const char *kInitFailed = "frame_generation/init_failed";
    static const char *kDisabled = "frame_generation/disabled";
    static const char *kExclusiveFullscreen = "frame_generation/exclusive_fullscreen";
    static const char *kNotSupported = "frame_generation/not_supported";
    static const char *kNoVideoExtensions = "frame_generation/no_video_extensions";

    bool isValid = true;
    AntialiasingMethod aaMethod = method_name ? convert_method(method_name, isValid) : method;
    if (!isValid)
      return kNotSupported;

    switch (aaMethod)
    {
      case AntialiasingMethod::DLSS:
      {
        if (dlss_without_streamline())
          return kNotSupported;

        nv::Streamline *streamline = nullptr;
        d3d::driver_command(Drv3dCommand::GET_STREAMLINE, &streamline);
        if (!streamline)
          return kNotSupported;

        switch (streamline->isDlssGSupported())
        {
          case nv::SupportState::OSOutOfDate: return kOsOutOfDate;
          case nv::SupportState::DriverOutOfDate: return kDriverOutOfDate;
          case nv::SupportState::AdapterNotSupported: return kGpuNotSupported;
          case nv::SupportState::DisabledHWS: return kHwSchedulingDisabled;
          case nv::SupportState::NotSupported: return kNotSupported;
          case nv::SupportState::NoVideoExtensions: return kNoVideoExtensions;
          case nv::SupportState::Supported: break;
        }

        if (streamline->getMaximumNumberOfGeneratedFrames() <= 0)
          return kNotSupported;

        break;
      }
      case AntialiasingMethod::FSR:
      {
        if (!fsr)
          return kNotSupported;
        if (!fsr->isFrameGenerationSupported())
          return kNotSupported;
        if (exclusive_fullscreen)
          return kExclusiveFullscreen;
        if (amd::FSR::getMaximumNumberOfGeneratedFrames() <= 0)
          return kNotSupported;

        break;
      }
      case AntialiasingMethod::XeSS:
      {
        auto xessState = XessState(d3d::driver_command(Drv3dCommand::GET_XESS_STATE));
        switch (xessState)
        {
          case XessState::UNSUPPORTED_DEVICE: return kGpuNotSupported;
          case XessState::UNSUPPORTED_DRIVER: return kDriverOutOfDate;
          case XessState::INIT_ERROR_UNKNOWN: return kInitFailed;
          case XessState::DISABLED: return kDisabled;
          case XessState::SUPPORTED:
          case XessState::READY: break;
        }

        int genFrames = 0;
        d3d::driver_command(Drv3dCommand::GET_XESS_SUPPORTED_GEN_FRAMES, &genFrames);
        if (genFrames <= 0)
          return kNotSupported;
        if (exclusive_fullscreen)
          return kExclusiveFullscreen;

        break;
      }
      default: return kNotSupported;
    }

    return nullptr;
  }

  bool isFrameGenerationEnabled(AntialiasingMethod config_method, bool exclusive_fullscreen)
  {
    return getCfgFrameGenCount() > 0 && getSupportedGeneratedFrames(config_method, exclusive_fullscreen) > 0;
  }

  bool isFrameGenerationEnabled(bool exclusive_fullscreen) { return isFrameGenerationEnabled(method, exclusive_fullscreen); }

  bool needGuiInTexture()
  {
    if (method == AntialiasingMethod::FSR)
      return fsr && fsr->isFrameGenerationActive();
    else if (method == AntialiasingMethod::XeSS)
    {
      bool enabled = false;
      d3d::driver_command(Drv3dCommand::GET_XESS_FG_ENABLED, &enabled);
      return enabled;
    }
    return false;
  }
};

InitOnDemand<Context> g_ctx;

void migrate(const char *config_name)
{
#if _TARGET_PC_WIN || _TARGET_PC_LINUX || _TARGET_PC_MACOSX
  auto video = dgs_get_settings()->getBlockByNameEx("video");

  if (!video->paramExists("antialiasing_mode"))
  {
    // This is the first time the game was started with the new module. Lets migrate the old settings.

    eastl::string methodName = "off";
    eastl::string qualityName = "native";

    auto fromXessQuality = [&](int quality) {
      switch (XessQuality(quality))
      {
        case XessQuality::BALANCED: return convert(UpscalingQuality::Balanced);
        case XessQuality::PERFORMANCE: return convert(UpscalingQuality::Performance);
        case XessQuality::QUALITY: return convert(UpscalingQuality::Quality);
        case XessQuality::ULTRA_QUALITY: return convert(UpscalingQuality::UltraQuality);
        case XessQuality::ULTRA_QUALITY_PLUS: return convert(UpscalingQuality::UltraQualityPlus);
        case XessQuality::ULTRA_PERFORMANCE: return convert(UpscalingQuality::UltraPerformance);
        case XessQuality::SETTING_AA:
        default: return convert(UpscalingQuality::Native);
      }
    };

    auto fromDlssQuality = [&](int dlss) {
      switch (nv::DLSS::Mode(dlss))
      {
        case nv::DLSS::Mode::Off:
        case nv::DLSS::Mode::DLAA: return convert(UpscalingQuality::Native);
        case nv::DLSS::Mode::UltraQuality: return convert(UpscalingQuality::UltraQuality);
        case nv::DLSS::Mode::MaxQuality: return convert(UpscalingQuality::Quality);
        case nv::DLSS::Mode::Balanced: return convert(UpscalingQuality::Balanced);
        case nv::DLSS::Mode::MaxPerformance: return convert(UpscalingQuality::Performance);
        case nv::DLSS::Mode::UltraPerformance: return convert(UpscalingQuality::UltraPerformance);
        default: return convert(UpscalingQuality::Native);
      }
    };

    auto getMethodName = [&](int oldMode) {
      switch (oldMode)
      {
        default:
        case 0: return convert(AntialiasingMethod::None);
        case 1: return convert(AntialiasingMethod::FXAAHigh);
        case 2: return convert(AntialiasingMethod::TAA);
        case 3: return convert(AntialiasingMethod::TSR);
        case 4: return convert(AntialiasingMethod::DLSS);
        case 5: return convert(AntialiasingMethod::MSAA);
        case 6: return convert(AntialiasingMethod::XeSS);
        case 7: return convert(AntialiasingMethod::FSR);
        case 8: return convert(AntialiasingMethod::SSAA);
#if _TARGET_C2

#endif
      }
    };

    if (auto oldMode = video->getInt("antiAliasingMode", -1); oldMode > -1)
    {
      methodName = getMethodName(oldMode);
      switch (oldMode)
      {
        case 3: // TSR
        {
          float scale = video->getReal("temporalUpsamplingRatio", 100.0f);
          qualityName = eastl::to_string(1.f / (scale / 100.0f));
          break;
        }
        case 4: // DLSS
        {
          if (auto dlss = video->getInt("dlssQuality", -1); dlss > -1)
            qualityName = fromDlssQuality(dlss);
          break;
        }
        case 6: // XeSS
        {
          if (auto xess = video->getInt("xessQuality", -1); xess > -1)
            qualityName = fromXessQuality(xess);
          break;
        }
      }
    }
    else
    {
      auto oldName = video->getStr("postfx_antialiasing", "none");
      if (auto xess = video->getInt("xessQuality", -1); xess > -1)
      {
        methodName = convert(AntialiasingMethod::XeSS);
        qualityName = fromXessQuality(xess);
      }
      else if (auto dlss = video->getInt("dlssQuality", -1); dlss > -1)
      {
        methodName = convert(AntialiasingMethod::DLSS);
        qualityName = fromDlssQuality(dlss);
      }
      else if (stricmp(oldName, "fxaa") == 0)
      {
        methodName = convert(AntialiasingMethod::FXAALow);
        qualityName = convert(UpscalingQuality::Native);
      }
      else if (stricmp(oldName, "high_fxaa") == 0)
      {
        methodName = convert(AntialiasingMethod::FXAAHigh);
        qualityName = convert(UpscalingQuality::Native);
      }
      else if (stricmp(oldName, "low_taa") == 0)
      {
        methodName = convert(AntialiasingMethod::TAA);
        float scale = video->getReal("temporalResolutionScale", 1.0f);
        if (scale >= 1.0f)
          qualityName = convert(UpscalingQuality::Native);
        else if (scale >= 0.77f)
          qualityName = convert(UpscalingQuality::UltraQuality);
        else if (scale >= 0.66f)
          qualityName = convert(UpscalingQuality::Quality);
        else if (scale >= 0.59f)
          qualityName = convert(UpscalingQuality::Balanced);
        else
          qualityName = convert(UpscalingQuality::Performance);
      }
    }

    DataBlock patch;
    auto patchVideo = patch.addBlock("video");
    patchVideo->addStr("antialiasing_mode", methodName.c_str());
    patchVideo->addStr("antialiasing_upscaling", qualityName.c_str());

    dgs_apply_config_blk(patch, true, false);

    DataBlock config_blk(framemem_ptr());
    dblk::load(config_blk, config_name, dblk::ReadFlag::ROBUST);
    dgs_apply_config_blk_ex(config_blk, patch, true, false);
    config_blk.saveToTextFile(config_name);
  }
#endif
}

void init(AppGlue *app_glue)
{
  app_glue_ptr = app_glue;

  if (g_ctx)
    return;

  g_ctx.demandInit();

  g_ctx->init();

  interlocked_release_store_ptr(dwc_get_frames_presented, get_presented_frame_count);
}

void close()
{
  if (!g_ctx)
    return;

  interlocked_release_store_ptr(dwc_get_frames_presented, (int (*)()) nullptr);
  g_ctx.demandDestroy();
}

void on_render_resolution_changed(const IPoint2 &rendering_resolution)
{
  static int taa_input_resolutionVarId = get_shader_variable_id("taa_input_resolution", true);

  ShaderGlobal::set_float4(taa_input_resolutionVarId, rendering_resolution.x, rendering_resolution.y, 0, 0);
}

void reinit()
{
  if (!g_ctx)
    return;

  d3d::driver_command(Drv3dCommand::D3D_FLUSH);

  g_ctx->reset();
  g_ctx->init();
}

void recreate(const IPoint2 &display_resolution, const IPoint2 &postfx_resolution, IPoint2 &rendering_resolution,
  IPoint2 &min_dynamic_resolution, IPoint2 &max_dynamic_resolution, const char *input_name)
{
  if (!g_ctx)
    return;

  reinit();

  rendering_resolution = {0, 0};
  min_dynamic_resolution = {0, 0};
  max_dynamic_resolution = {0, 0};

  auto fallbackToNone = [&]() {
    g_ctx->method = AntialiasingMethod::None;
    rendering_resolution = postfx_resolution;
  };

  switch (g_ctx->method)
  {
    case AntialiasingMethod::None:
    case AntialiasingMethod::MSAA:
    case AntialiasingMethod::FXAALow:
    case AntialiasingMethod::FXAAHigh:
    case AntialiasingMethod::SSAA:
    case AntialiasingMethod::MOBILE_MSAA: rendering_resolution = postfx_resolution; break;
#if _TARGET_C2




#endif
    case AntialiasingMethod::DLSS:
      if (!g_ctx->tryInitDlss(postfx_resolution, rendering_resolution, min_dynamic_resolution, max_dynamic_resolution, input_name))
        fallbackToNone();
      break;
    case AntialiasingMethod::FSR:
      if (!g_ctx->tryInitFsr(postfx_resolution, rendering_resolution, min_dynamic_resolution, max_dynamic_resolution, input_name))
        fallbackToNone();
      break;
    case AntialiasingMethod::XeSS:
      if (!g_ctx->tryInitXess(postfx_resolution, rendering_resolution, min_dynamic_resolution, max_dynamic_resolution, input_name))
        fallbackToNone();
      break;
    case AntialiasingMethod::TAA:
      if (!g_ctx->tryInitTaa(postfx_resolution, rendering_resolution, input_name))
        fallbackToNone();
      break;
    case AntialiasingMethod::TSR:
      if (!g_ctx->tryInitTsr(postfx_resolution, rendering_resolution, input_name, display_resolution))
        fallbackToNone();
      else
      {
        min_dynamic_resolution = get_min_dynamic_resolution(rendering_resolution);
        max_dynamic_resolution = rendering_resolution;
      }
      break;
    case AntialiasingMethod::MobileTAA:
      rendering_resolution = postfx_resolution;
      if (!g_ctx->tryInitMobileTaa(display_resolution, rendering_resolution))
        fallbackToNone();
      break;
    case AntialiasingMethod::MobileTAALow:
      rendering_resolution = postfx_resolution;
      if (!g_ctx->tryInitMobileTaa(rendering_resolution, rendering_resolution))
        fallbackToNone();
      break;
    case AntialiasingMethod::METALFX:
      rendering_resolution = postfx_resolution;
      if (!g_ctx->tryInitMetalFx())
        fallbackToNone();
      break;
#if HAS_SGSR
    case AntialiasingMethod::SGSR:
      rendering_resolution = postfx_resolution;
      if (!g_ctx->tryInitSgsr(postfx_resolution))
        fallbackToNone();
      break;
    case AntialiasingMethod::SGSR2:
      rendering_resolution = postfx_resolution;
      if (!g_ctx->tryInitSgsr2(display_resolution, rendering_resolution))
        fallbackToNone();
      break;
#endif
    case AntialiasingMethod::SMAA:
      rendering_resolution = postfx_resolution;
      if (!g_ctx->tryInitSmaa(rendering_resolution))
        fallbackToNone();
      break;
    default:
      logerr("unknown antialiasing method %s", (uint32_t)g_ctx->method);
      fallbackToNone();
      break;
  }

  if (min_dynamic_resolution.x == 0 || min_dynamic_resolution.y == 0 || max_dynamic_resolution.x == 0 || max_dynamic_resolution.y == 0)
  {
    min_dynamic_resolution = rendering_resolution;
    max_dynamic_resolution = rendering_resolution;
  }
}

AntialiasingMethod get_method() { return g_ctx ? g_ctx->method : AntialiasingMethod::None; }
void set_method(AntialiasingMethod method)
{
  if (g_ctx)
    g_ctx->method = method;
}

AntialiasingMethod get_method_from_settings()
{
  return (app_glue()->hasMinimumRenderFeatures() || app_glue()->useMobileCodepath()) ? convert_method(get_method_name_from_config())
                                                                                     : AntialiasingMethod::None;
}

AntialiasingMethod get_method_from_name(const char *name) { return convert_method(name); }

const char *get_method_name(AntialiasingMethod method) { return convert(method); }

const char *get_quality_name(UpscalingQuality quality) { return convert(quality, false); }

UpscalingQuality get_quality() { return g_ctx ? g_ctx->quality : UpscalingQuality::Native; }

void set_quality_override(const char *quality_name)
{
  if (g_ctx)
  {
    bool valid = true;
    g_ctx->qualityOverride = quality_name ? (int)convert_quality(quality_name, valid) : -1;
    G_ASSERT(valid);
  }
}

bool is_quality_override_active() { return g_ctx && g_ctx->qualityOverride >= 0; }

bool is_valid_method_name(const char *name)
{
  bool isValid;
  convert_method(name, isValid);
  return isValid;
}

bool is_valid_upscaling_name(const char *name)
{
  bool isValid;
  convert_quality(name, isValid);
  return isValid;
}

bool is_temporal()
{
  if (!g_ctx)
    return false;

  return g_ctx->method == AntialiasingMethod::TAA || g_ctx->method == AntialiasingMethod::TSR ||
         g_ctx->method == AntialiasingMethod::DLSS || g_ctx->method == AntialiasingMethod::XeSS ||
         g_ctx->method == AntialiasingMethod::FSR || g_ctx->method == AntialiasingMethod::MobileTAA ||
         g_ctx->method == AntialiasingMethod::SGSR2 || g_ctx->method == AntialiasingMethod::MobileTAALow
#if _TARGET_C2

#endif
    ;
}

bool need_motion_vectors() { return is_temporal(); }

bool support_dynamic_resolution()
{
  if (!g_ctx)
    return false;

  if (g_ctx->getCfgFrameGenCount())
    return false;

  return g_ctx->method == AntialiasingMethod::TSR || g_ctx->method == AntialiasingMethod::DLSS ||
         g_ctx->method == AntialiasingMethod::FSR || g_ctx->method == AntialiasingMethod::XeSS;
}

bool need_prev_depth()
{
  if (!g_ctx)
    return false;
#if _TARGET_C2

#else
  return false;
#endif
}

bool need_prev_motion_vectors()
{
  if (!g_ctx)
    return false;
#if _TARGET_C2

#else
  return false;
#endif
}

bool need_reactive_tex(bool vr_mode)
{
  if (!g_ctx)
    return false;
  return g_ctx->method == AntialiasingMethod::TSR && g_ctx->quality != UpscalingQuality::Native && !vr_mode;
}

bool need_reactive_tex_from_settings(bool vr_mode)
{
  return convert_method(get_method_name_from_config()) == AntialiasingMethod::TSR && !vr_mode;
}

int pull_settings_change()
{
  if (!g_ctx)
    return 0;

  return g_ctx->parseSettings(false);
}

TemporalAA *get_taa() { return g_ctx ? g_ctx->taa.get() : nullptr; }
TemporalSuperResolution *get_tsr() { return g_ctx ? g_ctx->tsr.get() : nullptr; }

float get_mip_bias() { return is_temporal() ? g_ctx->mipBias : 0; }

void adjust_mip_bias(const IPoint2 &dynamic_resolution, const IPoint2 &output_resolution)
{
  if (!g_ctx)
    return;

  g_ctx->mipBias = is_temporal() ? log2((float)dynamic_resolution.y / (float)output_resolution.y) : 0;
}

bool is_metalfx_upscale_supported()
{
  static bool metalfxSupported =
    (MtlfxUpscaleState(d3d::driver_command(Drv3dCommand::GET_METALFX_UPSCALE_STATE)) == MtlfxUpscaleState::READY);

  return flagMetalfxUpscale.get() && metalfxSupported;
}

void before_render_frame()
{
  if (!g_ctx || g_ctx->method != AntialiasingMethod::TAA)
    return;

  G_ASSERT(g_ctx->taa);

  g_ctx->taa->beforeRenderFrame();
}

void before_render_view(int view_index)
{
  if (!g_ctx)
    return;

  static int taa_hangar_passVarId = get_shader_variable_id("taa_hangar_pass", true);

  ShaderGlobal::set_int(taa_hangar_passVarId, app_glue()->getHangarPassValue());

  if (g_ctx->taa)
    g_ctx->taa->setCurrentView(view_index);

  if (g_ctx->method == AntialiasingMethod::XeSS)
  {
    const Point2 &renderingResolution = app_glue()->getInputResolution();
    if (g_ctx->velocityScale != renderingResolution)
    {
      g_ctx->velocityScale = renderingResolution;
      d3d::driver_command(Drv3dCommand::SET_XESS_VELOCITY_SCALE, &g_ctx->velocityScale.x, &g_ctx->velocityScale.y);
    }
  }
}

Point2 get_jitter_offset(const RenderView &view, bool vr_mode)
{
  if (!g_ctx)
    return Point2::ZERO;

  switch (g_ctx->method)
  {
    case AntialiasingMethod::FSR:
    case AntialiasingMethod::DLSS:
    case AntialiasingMethod::XeSS:
    case AntialiasingMethod::TSR:
#if _TARGET_C2

#endif
    {
      int phase = dagor_get_global_frame_id() % uint32_t(g_ctx->subsamples) + 1;
      return Point2(halton_sequence(phase, 2) - 0.5f, halton_sequence(phase, 3) - 0.5f);
    }
    case AntialiasingMethod::TAA:
    case AntialiasingMethod::MobileTAA:
    case AntialiasingMethod::MobileTAALow:
    {
      if (g_ctx->taa->beforeRenderView(app_glue()->getUvReprojectionToPrevFrameTmNoJitter()))
        return g_ctx->taa->getJitterOffset();
      break;
    }
#if HAS_SGSR
    case AntialiasingMethod::SGSR2:
    {
      g_ctx->sgsr2->beforeRenderView(view);
      return g_ctx->sgsr2->getJitterOffset();
      break;
    }
#endif
    default: break;
  }

  return Point2::ZERO;
}


void apply_fxaa(AntialiasingMethod method, Texture *src_color, Texture *src_depth, const Point4 &tc_scale_offset)
{
  if (!g_ctx)
    return;

  g_ctx->antialiasing->setType(method == AntialiasingMethod::FXAALow ? AA_TYPE_LOW_FXAA : AA_TYPE_HIGH_FXAA);
  g_ctx->antialiasing->apply(src_color, src_depth, tc_scale_offset);
}

void apply_mobile_aa(Texture *source_tex, Texture *dest_tex, bool temporal_reset)
{
  switch (g_ctx->method)
  {
    case AntialiasingMethod::METALFX: g_ctx->applyMetalFx(source_tex, dest_tex); break;
    case AntialiasingMethod::MobileTAA:
    case AntialiasingMethod::MobileTAALow: g_ctx->applyMobileTaa(source_tex, dest_tex); break;
#if HAS_SGSR
    case AntialiasingMethod::SGSR: g_ctx->applySgsr(source_tex, dest_tex); break;
    case AntialiasingMethod::SGSR2: g_ctx->applySgsr2(source_tex, dest_tex, temporal_reset); break;
#endif
    case AntialiasingMethod::SMAA: g_ctx->applySmaa(source_tex, dest_tex); break;
    default: break;
  }
}

const char *get_available_methods(bool is_vr, bool names_only)
{
  if (!g_ctx)
    return "off";

  static eastl::string aaOptions;
  aaOptions = "off";

  bool isCompat = !app_glue()->hasMinimumRenderFeatures();

  if (!isCompat)
  {
    aaOptions += ";low_fxaa;high_fxaa";

    if (app_glue()->isTiledRender())
      aaOptions += ";taa";

    nv::DLSS::State dlssState;
    dag::Expected<eastl::string, nv::SupportState> dlssVer;

    if (dlss_without_streamline())
    {
      nv::DLSS *dlss = nullptr;
      d3d::driver_command(Drv3dCommand::GET_DLSS, &dlss);
      dlssState = dlss ? dlss->getState() : nv::DLSS::State::NOT_IMPLEMENTED;
      dlssVer = dlss ? dlss->getVersion() : dag::Unexpected(nv::SupportState::NotSupported);
    }
    else
    {
      nv::Streamline *streamline = nullptr;
      d3d::driver_command(Drv3dCommand::GET_STREAMLINE, &streamline);
      dlssState = streamline ? streamline->getDlssState() : nv::DLSS::State::NOT_IMPLEMENTED;
      dlssVer = streamline ? streamline->getDlssVersion() : dag::Unexpected(nv::SupportState::NotSupported);
    }

    if (dlssState == nv::DLSS::State::SUPPORTED || dlssState == nv::DLSS::State::READY)
    {
      aaOptions += ";dlss";
      if (dlssVer.has_value() && !names_only)
      {
        carray<eastl::string, 3> versionSuffix = {"310", "DLSS", "DLSS 4"}; // move me to gamebase data when list will be more than 3
        eastl::string finalVersion = dlssVer.value();
        for (int i = 0; i < versionSuffix.size(); i += 2)
        {
          if (memcmp(versionSuffix[i].data(), finalVersion.data(), versionSuffix[i].size()) == 0)
          {
            finalVersion.append_sprintf("|%s|%s", versionSuffix[i + 1].c_str(), versionSuffix[i + 2].c_str());
            break;
          }
        }

        aaOptions += "|" + finalVersion;
      }
    }

    auto xessState = XessState(d3d::driver_command(Drv3dCommand::GET_XESS_STATE));
    if (!is_vr && (xessState == XessState::SUPPORTED || xessState == XessState::READY))
    {
      aaOptions += ";xess";
      char xessver[256];
      d3d::driver_command(Drv3dCommand::GET_XESS_VERSION, xessver, reinterpret_cast<void *>(eastl::size(xessver)));
      if (stricmp(xessver, "unknown") != 0 && !names_only)
      {
        aaOptions += "|";
        aaOptions += xessver;
      }
    }

    if (!is_vr && (g_ctx->fsr && g_ctx->fsr->isUpscalingSupported()))
    {
      aaOptions += ";fsr";
      auto version = g_ctx->fsr->getVersionString();
      if (!version.empty() && !names_only)
        aaOptions += "|" + version;
    }

    if (app_glue()->hasMinimumRenderFeatures())
      aaOptions += ";tsr";

#if _TARGET_C2


#endif

    if (app_glue()->pushSSAAoption())
      aaOptions += ";ssaa";
  }


  return aaOptions.c_str();
}

const char *get_available_upscaling_options(const char *method)
{
  if (!g_ctx)
    return "native";

  static eastl::string names;
  names = "native";

  auto res = app_glue()->getOutputResolution();

  if (method)
  {
    if (stricmp(method, "taa") == 0)
    {
      names.clear();

      names += convert(UpscalingQuality::Native, true);
      names += convert(UpscalingQuality::UltraQuality, true);
      names += convert(UpscalingQuality::Quality, true);
      names += convert(UpscalingQuality::Balanced, true);
      names += convert(UpscalingQuality::Performance, true);
    }
    else if (stricmp(method, "dlss") == 0)
    {
      using Mode = nv::DLSS::Mode;

      names.clear();

      if (is_dlss_quality_available_at_resolution(int(Mode::DLAA), res.x, res.y))
        names += convert(UpscalingQuality::Native, true);
      if (is_dlss_quality_available_at_resolution(int(Mode::UltraQuality), res.x, res.y))
        names += convert(UpscalingQuality::UltraQuality, true);
      if (is_dlss_quality_available_at_resolution(int(Mode::MaxQuality), res.x, res.y))
        names += convert(UpscalingQuality::Quality, true);
      if (is_dlss_quality_available_at_resolution(int(Mode::Balanced), res.x, res.y))
        names += convert(UpscalingQuality::Balanced, true);
      if (is_dlss_quality_available_at_resolution(int(Mode::MaxPerformance), res.x, res.y))
        names += convert(UpscalingQuality::Performance, true);
      if (is_dlss_quality_available_at_resolution(int(Mode::UltraPerformance), res.x, res.y))
        names += convert(UpscalingQuality::UltraPerformance, true);

      if (!names.empty() && names[0] == ';')
        names.erase(names.begin());
    }
    else if (stricmp(method, "xess") == 0)
    {
      using Mode = XessQuality;

      names.clear();

      if (is_xess_quality_available_at_resolution(int(Mode::SETTING_AA), res.x, res.y))
        names += convert(UpscalingQuality::Native, true);
      if (is_xess_quality_available_at_resolution(int(Mode::ULTRA_QUALITY_PLUS), res.x, res.y))
        names += convert(UpscalingQuality::UltraQualityPlus, true);
      if (is_xess_quality_available_at_resolution(int(Mode::ULTRA_QUALITY), res.x, res.y))
        names += convert(UpscalingQuality::UltraQuality, true);
      if (is_xess_quality_available_at_resolution(int(Mode::QUALITY), res.x, res.y))
        names += convert(UpscalingQuality::Quality, true);
      if (is_xess_quality_available_at_resolution(int(Mode::BALANCED), res.x, res.y))
        names += convert(UpscalingQuality::Balanced, true);
      if (is_xess_quality_available_at_resolution(int(Mode::PERFORMANCE), res.x, res.y))
        names += convert(UpscalingQuality::Performance, true);
      if (is_xess_quality_available_at_resolution(int(Mode::ULTRA_PERFORMANCE), res.x, res.y))
        names += convert(UpscalingQuality::UltraPerformance, true);
    }
    else if (stricmp(method, "fsr") == 0)
    {
      names.clear();

      names += convert(UpscalingQuality::Native, true);
      names += convert(UpscalingQuality::Quality, true);
      names += convert(UpscalingQuality::Balanced, true);
      names += convert(UpscalingQuality::Performance, true);
      names += convert(UpscalingQuality::UltraPerformance, true);
    }
    else if (stricmp(method, "tsr") == 0)
    {
      names.clear();

      names += convert(UpscalingQuality::Native, true);
      names += convert(UpscalingQuality::UltraQuality, true);
      names += convert(UpscalingQuality::Quality, true);
      names += convert(UpscalingQuality::Balanced, true);
      names += convert(UpscalingQuality::Performance, true);
    }
  }

  if (!names.empty() && names[0] == ';')
    names.erase(names.begin());

  auto result = names.c_str();
  return result;
}

#if DEBUG_INTERFACE_ENABLED
int process_console_cmd(const char *argv[], int argc)
{
  int found = 0;

  CONSOLE_CHECK_NAME("render", "taa_halton", 1, 2)
  {
    if (auto taa = get_taa(); taa && argc > 1)
      taa->params.useHalton = atoi(argv[1]);
  }
  CONSOLE_CHECK_NAME("render", "taa_subsample_scale", 1, 2)
  {
    if (auto taa = get_taa())
    {
      if (argc > 1)
        taa->params.subsampleScale = atof(argv[1]);
      console::print_d("subsample scale is %f", taa->params.subsampleScale);
    }
  }
  CONSOLE_CHECK_NAME("render", "taa_newframe_falloff", 1, 3)
  {
    if (auto taa = get_taa())
    {
      if (argc >= 2)
        taa->params.newframeFalloffStill = atof(argv[1]);
      if (argc == 3)
        taa->params.newframeFalloffMotion = atof(argv[2]);
      console::print_d("newframeFalloff %.2f - %.2f", taa->params.newframeFalloffStill, taa->params.newframeFalloffMotion);
    }
  }
  CONSOLE_CHECK_NAME("render", "taa_newframe_falloff_dynamic", 1, 3)
  {
    if (auto taa = get_taa())
    {
      if (argc >= 2)
        taa->params.newframeFalloffDynamicStill = atof(argv[1]);
      if (argc == 3)
        taa->params.newframeFalloffDynamicMotion = atof(argv[2]);
      console::print_d("newframeFalloffDynamic %.2f - %.2f", taa->params.newframeFalloffDynamicStill,
        taa->params.newframeFalloffDynamicMotion);
    }
  }
  CONSOLE_CHECK_NAME("render", "taa_motion_difference_max", 1, 2)
  {
    if (auto taa = get_taa())
    {
      if (argc > 1)
        taa->params.motionDifferenceMax = atof(argv[1]);
      console::print_d("motionDifferenceMax %f", taa->params.motionDifferenceMax);
    }
  }
  CONSOLE_CHECK_NAME("render", "taa_motion_difference_max_weight", 1, 2)
  {
    if (auto taa = get_taa())
    {
      if (argc > 1)
        taa->params.motionDifferenceMaxWeight = atof(argv[1]);
      console::print_d("motionDifferenceMaxWeight %f", taa->params.motionDifferenceMaxWeight);
    }
  }
  CONSOLE_CHECK_NAME("render", "taa_frame_weight_min", 1, 3)
  {
    if (auto taa = get_taa())
    {
      if (argc >= 2)
        taa->params.frameWeightMinStill = atof(argv[1]);
      if (argc == 3)
        taa->params.frameWeightMinMotion = atof(argv[2]);
      console::print_d("frameWeightMin %.2f - %.2f", taa->params.frameWeightMinStill, taa->params.frameWeightMinMotion);
    }
  }
  CONSOLE_CHECK_NAME("render", "taa_frame_weight_min_dynamic", 1, 3)
  {
    if (auto taa = get_taa())
    {
      if (argc >= 2)
        taa->params.frameWeightMinDynamicStill = atof(argv[1]);
      if (argc == 3)
        taa->params.frameWeightMinDynamicMotion = atof(argv[2]);
      console::print_d("frameWeightMinDynamic %.2f - %.2f", taa->params.frameWeightMinDynamicStill,
        taa->params.frameWeightMinDynamicMotion);
    }
  }
  CONSOLE_CHECK_NAME("render", "taa_sharpening", 1, 2)
  {
    if (auto taa = get_taa())
    {
      if (argc > 1)
        taa->params.sharpening = atof(argv[1]);
      console::print_d("sharpening %f", taa->params.sharpening);
    }
  }
  CONSOLE_CHECK_NAME("render", "taa_clamping_gamma", 1, 2)
  {
    if (auto taa = get_taa())
    {
      if (argc > 1)
        taa->params.clampingGamma = atof(argv[1]);
      console::print_d("clampingGamma %f", taa->params.clampingGamma);
    }
  }
  CONSOLE_CHECK_NAME("render", "taa_subsamples", 1, 2)
  {
    if (auto taa = get_taa())
    {
      if (argc > 1)
        taa->params.subsamples = atoi(argv[1]);
      console::print_d("subsamples %d", taa->params.subsamples);
    }
  }
  CONSOLE_CHECK_NAME("render", "taa_frame_weight_motion_lerp", 1, 3)
  {
    if (auto taa = get_taa())
    {
      if (argc == 3)
      {
        taa->params.frameWeightMotionThreshold = atof(argv[1]);
        taa->params.frameWeightMotionMax = atof(argv[2]);
      }
      else if (argc == 2)
        console::print("min and max required");
      else
        console::print_d("frameWeightMotionLerp max = %.2f, threshold %.2f", taa->params.frameWeightMotionThreshold,
          taa->params.frameWeightMotionMax);
    }
  }

  return found;
}
#else
int process_console_cmd(const char *[], int) { return 0; }
#endif

bool is_ssaa_compatible()
{
  auto method = get_method_from_settings();
  // TODO: XeSS can be supported too but the driver doesn't support XeSS output resolution change for now
  // SSAA is not allowed with RT, there is no HW powerfull enough now for it, but users ofter max everyhing out without
  // realisation of performance impact
  return !(method == AntialiasingMethod::DLSS || method == AntialiasingMethod::XeSS || app_glue()->isRayTracingEnabled());
}

bool need_gui_in_texture() { return g_ctx && g_ctx->needGuiInTexture(); }

bool allows_noise() { return g_ctx->prepareDlssRayReconstruction != nullptr; }

void suppress_frame_generation(bool suppress)
{
  if (g_ctx)
    g_ctx->suppressFrameGeneration(suppress);
}

void enable_frame_generation(bool enable)
{
  if (g_ctx)
    g_ctx->enableFrameGeneration(enable);
}

void schedule_generated_frames(const FrameGenContext &ctx)
{
  if (g_ctx)
    g_ctx->scheduleGeneratedFrames(ctx);
}

int get_supported_generated_frames(const char *method, bool exclusive_fullscreen)
{
  if (!g_ctx)
    return 0;
  return g_ctx->getSupportedGeneratedFrames(convert_method(method), exclusive_fullscreen);
}

const char *get_frame_generation_unsupported_reason(const char *method, bool exclusive_fullscreen)
{
  if (!g_ctx)
    return nullptr;

  return g_ctx->getFrameGenerationUnsupportedReason(method, exclusive_fullscreen);
}

int get_presented_frame_count()
{
  if (!is_main_thread() || !g_ctx)
    return 1;
  return g_ctx->getPresentedFrameCount();
}

bool is_frame_generation_enabled()
{
  if (!g_ctx)
    return false;

  return g_ctx->isFrameGenerationEnabled(dgs_get_window_mode() == WindowMode::FULLSCREEN_EXCLUSIVE);
}

bool is_frame_generation_enabled_in_config()
{
  if (!g_ctx)
    return false;

  return g_ctx->isFrameGenerationEnabled(get_method_from_settings(), dgs_get_window_mode() == WindowMode::FULLSCREEN_EXCLUSIVE);
}

unsigned int get_frame_after_aa_flags()
{
  if (!g_ctx)
    return 0;

  switch (g_ctx->method)
  {
    case AntialiasingMethod::None:
    case AntialiasingMethod::MSAA:
    case AntialiasingMethod::FXAALow:
    case AntialiasingMethod::FXAAHigh:
    case AntialiasingMethod::MOBILE_MSAA: return TEXFMT_R11G11B10F | TEXCF_RTARGET;
    default: return g_ctx->textureFlg;
  }
}

bool try_init_dlss(IPoint2 postfx_resolution, IPoint2 &rendering_resolution, const char *input_name)
{
  IPoint2 dummy;

  if (g_ctx)
    return g_ctx->tryInitDlss(postfx_resolution, rendering_resolution, dummy, dummy, input_name);
  return false;
}

void apply_dlss(Texture *in_color, const ApplyContext &apply_context, Texture *target)
{
  if (g_ctx)
    g_ctx->applyDlss(in_color, apply_context, target);
}

bool is_ray_reconstruction_enabled()
{
  if (g_ctx)
    return g_ctx->prepareDlssRayReconstruction != nullptr;
  return false;
}

bool try_init_tsr(IPoint2 postfx_resolution, IPoint2 &rendering_resolution, const char *input_name)
{
  if (g_ctx)
    return g_ctx->tryInitTsr(postfx_resolution, rendering_resolution, input_name);
  return false;
}

void apply_tsr(Texture *in_color, const ApplyContext &apply_context, Texture *out_color, Texture *history_color,
  Texture *out_confidence, Texture *history_confidence, Texture *reactive_tex)
{
  if (g_ctx)
    g_ctx->applyTsr(in_color, apply_context, out_color, history_color, out_confidence, history_confidence, reactive_tex);
}

bool try_init_fsr(IPoint2 postfx_resolution, IPoint2 &rendering_resolution, const char *input_name)
{
  IPoint2 dummy;

  if (g_ctx)
    return g_ctx->tryInitFsr(postfx_resolution, rendering_resolution, dummy, dummy, input_name);
  return false;
}

void apply_fsr(Texture *in_color, const ApplyContext &apply_context, Texture *target)
{
  if (g_ctx)
    g_ctx->applyFsr(in_color, apply_context, target);
}

bool try_init_xess(IPoint2 outputResolution, IPoint2 &inputResolution, const char *input_name)
{
  IPoint2 dummy;

  if (g_ctx)
    return g_ctx->tryInitXess(outputResolution, inputResolution, dummy, dummy, input_name);
  return false;
}

void apply_xess(Texture *in_color, const ApplyContext &apply_context, Texture *target)
{
  if (g_ctx)
    g_ctx->applyXess(in_color, apply_context, target);
}

#if _TARGET_C2












#endif


} // namespace render::antialiasing
