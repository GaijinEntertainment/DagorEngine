// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "streamline_adapter.h"

#include "drv_log_defs.h"

#include <debug/dag_log.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <3d/dag_lowLatency.h>
#include <EASTL/fixed_vector.h>
#include <osApiWrappers/dag_miscApi.h>

#include <sl.h>
#include <sl_helpers.h>
#include <sl_consts.h>
#include <sl_hooks.h>
#include <sl_dlss.h>
#include <sl_dlss_g.h>
#include <sl_reflex.h>

#include <dxgi.h>
#include <d3d12video.h>

namespace
{
namespace sl_funcs
{
SL_FUN_DECL(slInit);
SL_FUN_DECL(slShutdown);
SL_FUN_DECL(slSetD3DDevice);
SL_FUN_DECL(slUpgradeInterface);
SL_FUN_DECL(slGetNativeInterface);
SL_FUN_DECL(slIsFeatureSupported);
SL_FUN_DECL(slGetFeatureFunction);
SL_FUN_DECL(slGetNewFrameToken);
SL_FUN_DECL(slSetConstants);
SL_FUN_DECL(slSetTag);
SL_FUN_DECL(slEvaluateFeature);
SL_FUN_DECL(slAllocateResources);
SL_FUN_DECL(slFreeResources);
SL_FUN_DECL(slGetFeatureVersion);

SL_FUN_DECL(slDLSSGetOptimalSettings);
SL_FUN_DECL(slDLSSGetState);
SL_FUN_DECL(slDLSSSetOptions);

SL_FUN_DECL(slPCLSetMarker);
SL_FUN_DECL(slReflexSetOptions);
SL_FUN_DECL(slReflexGetState);
SL_FUN_DECL(slReflexSleep);

SL_FUN_DECL(slDLSSGSetOptions);
SL_FUN_DECL(slDLSSGGetState);

void load_interposer(void *module)
{
#define LOAD_FUNC(x)                                                         \
  sl_funcs::x = reinterpret_cast<PFun_##x *>(os_dll_get_symbol(module, #x)); \
  G_ASSERTF(sl_funcs::x, "sl: Failed to load function " #x);

  LOAD_FUNC(slInit);
  LOAD_FUNC(slShutdown);
  LOAD_FUNC(slSetD3DDevice);
  LOAD_FUNC(slUpgradeInterface);
  LOAD_FUNC(slGetNativeInterface);
  LOAD_FUNC(slIsFeatureSupported);
  LOAD_FUNC(slGetFeatureFunction);
  LOAD_FUNC(slGetNewFrameToken);
  LOAD_FUNC(slSetConstants);
  LOAD_FUNC(slSetTag);
  LOAD_FUNC(slEvaluateFeature);
  LOAD_FUNC(slAllocateResources);
  LOAD_FUNC(slFreeResources);
  LOAD_FUNC(slGetFeatureVersion);
#undef LOAD_FUNC
}

bool unload_interposer(void *module)
{
  slInit = nullptr;
  slShutdown = nullptr;
  slSetD3DDevice = nullptr;
  slUpgradeInterface = nullptr;
  slGetNativeInterface = nullptr;
  slIsFeatureSupported = nullptr;
  slGetFeatureFunction = nullptr;
  slGetNewFrameToken = nullptr;
  slSetConstants = nullptr;
  slSetTag = nullptr;
  slEvaluateFeature = nullptr;
  slAllocateResources = nullptr;
  slFreeResources = nullptr;
  slGetFeatureVersion = nullptr;
  return os_dll_close(module);
}

void load_dlss()
{
  G_VERIFY(sl_funcs::slGetFeatureFunction(sl::kFeatureDLSS, "slDLSSGetOptimalSettings", (void *&)slDLSSGetOptimalSettings) ==
           sl::Result::eOk);
  G_VERIFY(sl_funcs::slGetFeatureFunction(sl::kFeatureDLSS, "slDLSSSetOptions", (void *&)slDLSSSetOptions) == sl::Result::eOk);
}

void load_reflex()
{
  G_VERIFY(sl_funcs::slGetFeatureFunction(sl::kFeaturePCL, "slPCLSetMarker", (void *&)slPCLSetMarker) == sl::Result::eOk);
  G_VERIFY(sl_funcs::slGetFeatureFunction(sl::kFeatureReflex, "slReflexSetOptions", (void *&)slReflexSetOptions) == sl::Result::eOk);
  G_VERIFY(sl_funcs::slGetFeatureFunction(sl::kFeatureReflex, "slReflexGetState", (void *&)slReflexGetState) == sl::Result::eOk);
  G_VERIFY(sl_funcs::slGetFeatureFunction(sl::kFeatureReflex, "slReflexSleep", (void *&)slReflexSleep) == sl::Result::eOk);
}

void load_dlss_g()
{
  G_VERIFY(sl_funcs::slGetFeatureFunction(sl::kFeatureDLSS_G, "slDLSSGSetOptions", (void *&)slDLSSGSetOptions) == sl::Result::eOk);
  G_VERIFY(sl_funcs::slGetFeatureFunction(sl::kFeatureDLSS_G, "slDLSSGGetState", (void *&)slDLSSGGetState) == sl::Result::eOk);
}
} // namespace sl_funcs

void logMessageCallback(sl::LogType type, const char *msg)
{
  auto toDagorLogLevel = [](auto type) {
    switch (type)
    {
      case sl::LogType::eError:
      case sl::LogType::eWarn: return LOGLEVEL_WARN;
      case sl::LogType::eInfo: return LOGLEVEL_DEBUG;
      default: G_ASSERT(false);
    }
    return LOGLEVEL_FATAL;
  };

  if (auto length = strlen(msg); length > 2)
    logmessage(toDagorLogLevel(type), "sl: %.*s", length - 1, msg); // extract the extra \n character
}
} // namespace

StreamlineAdapter::InterposerHandleType StreamlineAdapter::loadInterposer()
{
  if (WindowsVersion winVersion = get_windows_version(); winVersion.MajorVersion < 10)
    return {nullptr, nullptr};

  auto settings = ::dgs_get_settings();
  if (settings->getBlockByNameEx("video")->getBool("disableStreamline", false))
    return {nullptr, nullptr};

  if (int appId = settings->getInt("nvidia_app_id", 0); appId == 0)
  {
    logdbg("sl: No app ID is set for project.");
    return {nullptr, nullptr};
  }

  StreamlineAdapter::InterposerHandleType interposer(os_dll_load("sl.interposer.dll"), &sl_funcs::unload_interposer);

  if (!interposer)
  {
    D3D_ERROR("sl: Could not load Streamline interposer DLL");
    return {nullptr, nullptr};
  }

  sl_funcs::load_interposer(interposer.get());
  return interposer;
}

void *StreamlineAdapter::getInterposerSymbol(const InterposerHandleType &interposer, const char *name)
{
  return os_dll_get_symbol(interposer.get(), name);
}

struct StreamlineAdapter::InitArgs
{
  sl::Preferences preferences;
  eastl::fixed_vector<sl::Feature, 4> features;
};

bool StreamlineAdapter::init(eastl::optional<StreamlineAdapter> &adapter, RenderAPI api, SupportOverrideMap support_override)
{
  // if the sl is disabled or isn't available for any reason, the inteposer load will not load the slInit
  if (!sl_funcs::slInit)
  {
    logdbg("sl: The interposer isn't loaded");
    return false;
  }

  auto settings = ::dgs_get_settings();
  auto initArgs = eastl::make_unique<InitArgs>();
  initArgs->preferences = {};
  initArgs->preferences.applicationId = settings->getInt("nvidia_app_id", 0);
  initArgs->preferences.logLevel = DAGOR_DBGLEVEL > 0 ? sl::LogLevel::eVerbose : sl::LogLevel::eDefault;
  initArgs->preferences.logMessageCallback = &logMessageCallback;
  initArgs->preferences.flags = sl::PreferenceFlags::eDisableCLStateTracking | sl::PreferenceFlags::eUseManualHooking;

  initArgs->features = {sl::kFeatureDLSS, sl::kFeatureReflex, sl::kFeaturePCL};
  if (api == RenderAPI::DX12)
  {
    initArgs->features.push_back(sl::kFeatureDLSS_G);
  }

  initArgs->preferences.featuresToLoad = initArgs->features.data();
  initArgs->preferences.numFeaturesToLoad = initArgs->features.size();
  initArgs->preferences.engine = sl::EngineType::eCustom;
  initArgs->preferences.engineVersion = "6.0.0.0";
  initArgs->preferences.projectId = settings->getStr("nvidia_project_id", "6352d1d6-3e63-4dba-8da1-c96bdc320a02");

  auto toSlRenderApi = [](auto api) {
    switch (api)
    {
      case RenderAPI::DX11: return sl::RenderAPI::eD3D11;
      case RenderAPI::DX12: return sl::RenderAPI::eD3D12;
    }
    return sl::RenderAPI{};
  };

  initArgs->preferences.renderAPI = toSlRenderApi(api);
  if (SL_FAILED(result, sl_funcs::slInit(initArgs->preferences, sl::kSDKVersion)))
  {
    logwarn("sl: Failed to initialize Streamline: %s. DLSS and Reflex will be disabled.", getResultAsStr(result));
    // slShutdown will set PluginManager::s_status to ePluginsUnloaded to make sure we can try again later
    G_VERIFY(sl_funcs::slShutdown() == sl::Result::eOk);
    return false;
  }

  adapter.emplace(eastl::move(initArgs), eastl::move(support_override));
  return true;
}

StreamlineAdapter::StreamlineAdapter(eastl::unique_ptr<InitArgs> &&initArgs, SupportOverrideMap support_override) :
  initArgs{eastl::move(initArgs)}, //
  supportOverride(eastl::move(support_override)),
  currentFrameId(lowlatency::get_current_frame())
{
  // start frame automatically because streamline might be reinitialized mid-frame
  auto result = sl_funcs::slGetNewFrameToken(frameTokens[currentFrameId % MAX_CONCURRENT_FRAMES], &currentFrameId);
  G_ASSERT(result == sl::Result::eOk);
}

StreamlineAdapter::~StreamlineAdapter()
{
  G_ASSERT(sl_funcs::slShutdown);
  sl::Result result = sl_funcs::slShutdown();
  G_ASSERT(result == sl::Result::eOk);
}

void *StreamlineAdapter::hook(IUnknown *object)
{
  if (!sl_funcs::slUpgradeInterface)
  {
    logdbg("sl: Hook isn't available");
    return object;
  }

  void *proxy = object;
  if (SL_FAILED(result, sl_funcs::slUpgradeInterface(&proxy)))
  {
    D3D_ERROR("sl: Failed to hook device: %s", getResultAsStr(result));
    return object;
  }

  return proxy;
}

void StreamlineAdapter::setAdapterAndDevice(IDXGIAdapter1 *adapter, ID3D11Device *device)
{
  this->adapter = adapter;
  sl::Result result = sl_funcs::slSetD3DDevice(device);
  G_ASSERT(result == sl::Result::eOk);
}

void StreamlineAdapter::setAdapterAndDevice(IDXGIAdapter1 *adapter, ID3D12Device5 *device)
{
  this->adapter = adapter;
  sl::Result result = sl_funcs::slSetD3DDevice(device);
  G_ASSERT(result == sl::Result::eOk);
}

void StreamlineAdapter::preRecover()
{
  adapter.Reset();
  G_ASSERT(sl_funcs::slShutdown);
  sl::Result result = sl_funcs::slShutdown();
  G_ASSERT(result == sl::Result::eOk);
  dlssState = State::DISABLED;
}

void StreamlineAdapter::recover()
{
  if (SL_FAILED(result, sl_funcs::slInit(initArgs->preferences, sl::kSDKVersion)))
  {
    logwarn("sl: Failed to initialize Streamline: %s. DLSS and Reflex will be disabled.", getResultAsStr(result));
    // slShutdown will set PluginManager::s_status to ePluginsUnloaded to make sure we can try again later
    G_VERIFY(sl_funcs::slShutdown() == sl::Result::eOk);
  }
  startFrame(lowlatency::get_current_frame());
}

static nv::SupportState toSupportState(sl::Result result)
{
  switch (result)
  {
    case sl::Result::eErrorOSOutOfDate: return nv::SupportState::OSOutOfDate;
    case sl::Result::eErrorDriverOutOfDate: return nv::SupportState::DriverOutOfDate;
    case sl::Result::eErrorNoSupportedAdapterFound:
    case sl::Result::eErrorAdapterNotSupported: return nv::SupportState::AdapterNotSupported;
    case sl::Result::eErrorOSDisabledHWS: return nv::SupportState::DisabledHWS;
    default: return nv::SupportState::NotSupported;
  }
}

static nv::SupportState isFeatureSupported(sl::Feature feature, IDXGIAdapter1 *adapter,
  const StreamlineAdapter::SupportOverrideMap &support_override)
{
  if (auto it = support_override.find(feature); it != support_override.end())
  {
    return it->second;
  }

  DXGI_ADAPTER_DESC desc{};
  sl::AdapterInfo adapterInfo{};
  if (adapter && SUCCEEDED(adapter->GetDesc(&desc)))
  {
    adapterInfo.deviceLUID = (uint8_t *)&desc.AdapterLuid;
    adapterInfo.deviceLUIDSizeInBytes = sizeof(LUID);
  }

  if (SL_FAILED(result, sl_funcs::slIsFeatureSupported(feature, adapterInfo)))
  {
    return toSupportState(result);
  }
  else
  {
    return nv::SupportState::Supported;
  }
}

nv::SupportState StreamlineAdapter::isDlssSupported() const
{
  return isFeatureSupported(sl::kFeatureDLSS, adapter.Get(), supportOverride);
}

nv::SupportState StreamlineAdapter::isDlssGSupported() const
{
  return isFeatureSupported(sl::kFeatureDLSS_G, adapter.Get(), supportOverride);
}

nv::SupportState StreamlineAdapter::isReflexSupported() const
{
  return isFeatureSupported(sl::kFeatureReflex, adapter.Get(), supportOverride);
}

void StreamlineAdapter::startFrame(uint32_t frame_id)
{
  auto result = sl_funcs::slGetNewFrameToken(frameTokens[frame_id % MAX_CONCURRENT_FRAMES], &frame_id);
  G_ASSERT(result == sl::Result::eOk);
  currentFrameId = frame_id;
}

void StreamlineAdapter::initializeReflexState() { sl_funcs::load_reflex(); }

static sl::PCLMarker toPCLMarker(lowlatency::LatencyMarkerType marker)
{
  switch (marker)
  {
    case lowlatency::LatencyMarkerType::SIMULATION_START: return sl::PCLMarker::eSimulationStart;
    case lowlatency::LatencyMarkerType::SIMULATION_END: return sl::PCLMarker::eSimulationEnd;
    case lowlatency::LatencyMarkerType::RENDERSUBMIT_START: return sl::PCLMarker::eRenderSubmitStart;
    case lowlatency::LatencyMarkerType::RENDERSUBMIT_END: return sl::PCLMarker::eRenderSubmitEnd;
    case lowlatency::LatencyMarkerType::PRESENT_START: return sl::PCLMarker::ePresentStart;
    case lowlatency::LatencyMarkerType::PRESENT_END: return sl::PCLMarker::ePresentEnd;
    case lowlatency::LatencyMarkerType::TRIGGER_FLASH: return sl::PCLMarker::eTriggerFlash;
  }
  return {};
}

bool StreamlineAdapter::setMarker(uint32_t frame_id, lowlatency::LatencyMarkerType marker_type)
{
  auto token = frameTokens[frame_id % MAX_CONCURRENT_FRAMES];
  if (token == nullptr)
  {
    logwarn("Called setMarker(%d) for a frame that wasn't started. Dropping marker silently.", eastl::to_underlying(marker_type));
    return true;
  }
  return sl_funcs::slPCLSetMarker(toPCLMarker(marker_type), *token) == sl::Result::eOk;
}

static sl::ReflexMode toReflexMode(nv::Reflex::ReflexMode mode)
{
  switch (mode)
  {
    case nv::Reflex::ReflexMode::Off: return sl::ReflexMode::eOff;
    case nv::Reflex::ReflexMode::On: return sl::ReflexMode::eLowLatency;
    case nv::Reflex::ReflexMode::OnPlusBoost: return sl::ReflexMode::eLowLatencyWithBoost;
  }
  return {};
}

static nv::Reflex::ReflexStats toReflexStats(const sl::ReflexReport &report)
{
  return nv::Reflex::ReflexStats{report.frameID, report.inputSampleTime, report.simStartTime, report.simEndTime,
    report.renderSubmitStartTime, report.renderSubmitEndTime, report.presentStartTime, report.presentEndTime, report.driverStartTime,
    report.driverEndTime, report.osRenderQueueStartTime, report.osRenderQueueEndTime, report.gpuRenderStartTime,
    report.gpuRenderEndTime, report.gpuActiveRenderTimeUs, report.gpuFrameTimeUs};
}

bool StreamlineAdapter::setReflexOptions(nv::Reflex::ReflexMode mode, unsigned frameLimitUs)
{
  sl::ReflexOptions options;
  options.frameLimitUs = frameLimitUs;
  options.mode = toReflexMode(mode);
  currentReflexMode = mode;
  bool ok = sl_funcs::slReflexSetOptions(options) == sl::Result::eOk;
  if (ok)
    currentReflexMode = mode;
  return ok;
}

nv::Reflex::ReflexMode StreamlineAdapter::getReflexMode() const { return currentReflexMode; }

uint32_t StreamlineAdapter::getFrameId() const { return currentFrameId; }

eastl::optional<nv::Reflex::ReflexState> StreamlineAdapter::getReflexState() const
{
  sl::ReflexState state;
  bool ok = sl_funcs::slReflexGetState(state) == sl::Result::eOk;
  if (!ok)
    return eastl::nullopt;

  nv::Reflex::ReflexState result{state.lowLatencyAvailable, state.latencyReportAvailable, state.flashIndicatorDriverControlled};
  static_assert(eastl::size(state.frameReport) == eastl::size(result.stats));
  eastl::transform(state.frameReport, state.frameReport + eastl::size(state.frameReport), result.stats, toReflexStats);
  return result;
}

bool StreamlineAdapter::sleep()
{
  auto result = sl_funcs::slReflexSleep(*frameTokens[currentFrameId % MAX_CONCURRENT_FRAMES]);
  G_ASSERT(result == sl::Result::eOk);
  return result == sl::Result::eOk;
}

static sl::DLSSMode toDLSSMode(nv::DLSS::Mode mode)
{
  switch (mode)
  {
    case nv::DLSS::Mode::Off: return sl::DLSSMode::eOff;
    case nv::DLSS::Mode::MaxPerformance: return sl::DLSSMode::eMaxPerformance;
    case nv::DLSS::Mode::Balanced: return sl::DLSSMode::eBalanced;
    case nv::DLSS::Mode::MaxQuality: return sl::DLSSMode::eMaxQuality;
    case nv::DLSS::Mode::UltraPerformance: return sl::DLSSMode::eUltraPerformance;
    case nv::DLSS::Mode::UltraQuality: return sl::DLSSMode::eUltraQuality;
    case nv::DLSS::Mode::DLAA: return sl::DLSSMode::eDLAA;
  }
  return {};
}

eastl::optional<nv::DLSS::OptimalSettings> StreamlineAdapter::getOptimalSettings(nv::DLSS::Mode mode, IPoint2 output_resolution) const
{
  if (dlssState != State::READY && dlssState != State::SUPPORTED)
    return eastl::nullopt;

  sl::DLSSOptimalSettings optimalSettings{};
  sl::DLSSOptions options{};

  options.mode = toDLSSMode(mode);
  options.outputWidth = output_resolution.x;
  options.outputHeight = output_resolution.y;
  if (SL_FAILED(result, sl_funcs::slDLSSGetOptimalSettings(options, optimalSettings)))
  {
    return eastl::nullopt;
  }
  else
  {
    return OptimalSettings{optimalSettings.optimalRenderWidth, optimalSettings.optimalRenderHeight, optimalSettings.optimalSharpness};
  }
}

bool StreamlineAdapter::setOptions(int viewportId, nv::DLSS::Mode mode, IPoint2 output_resolution, float sharpness)
{
  if (dlssState != State::READY && dlssState != State::SUPPORTED)
    return false;

  sl::DLSSOptions options{};
  options.mode = toDLSSMode(mode);
  options.outputWidth = output_resolution.x;
  options.outputHeight = output_resolution.y;
  options.sharpness = sharpness;

  return sl_funcs::slDLSSSetOptions(viewportId, options) == sl::Result::eOk;
}

static sl::float4 toFloat4(const Point4 &p) { return sl::float4(p.x, p.y, p.z, p.w); }
static sl::float3 toFloat3(const Point3 &p) { return sl::float3(p.x, p.y, p.z); }

static sl::float4x4 toFloat4x4(const TMatrix4 &t)
{
  return sl::float4x4{toFloat4(t.row[0]), toFloat4(t.row[1]), toFloat4(t.row[2]), toFloat4(t.row[3])};
}

bool StreamlineAdapter::evaluateDlss(uint32_t frame_id, int viewportId, const nv::DlssParams<void> &params, void *commandBuffer)
{
  if (dlssState != State::READY)
    return false;

  sl::Constants constants{};
  // If camera motion is marked as not included then DLSS will calculate reprojection for pixels marked as invalid.
  constants.mvecScale = {params.inMVScaleX, params.inMVScaleY};
  constants.jitterOffset = {params.inJitterOffsetX, params.inJitterOffsetY};
  constants.cameraPinholeOffset = {0.f, 0.f};
  constants.cameraViewToClip = toFloat4x4(params.camera.projection);
  constants.clipToCameraView = toFloat4x4(params.camera.projectionInverse);
  constants.clipToPrevClip = toFloat4x4(params.camera.reprojection);
  constants.prevClipToClip = toFloat4x4(params.camera.reprojectionInverse);
  constants.cameraPos = toFloat3(params.camera.position);
  constants.cameraUp = toFloat3(params.camera.up);
  constants.cameraRight = toFloat3(params.camera.right);
  constants.cameraFwd = toFloat3(params.camera.forward);
  constants.cameraNear = params.camera.nearZ;
  constants.cameraFar = params.camera.farZ;
  constants.cameraFOV = params.camera.fov;
  constants.cameraAspectRatio = params.camera.aspect;
  constants.depthInverted = sl::Boolean::eTrue;
  constants.cameraMotionIncluded = sl::Boolean::eTrue;
  constants.motionVectors3D = sl::Boolean::eFalse;
  constants.reset = sl::Boolean::eFalse;

  auto token = frameTokens[frame_id % MAX_CONCURRENT_FRAMES];
  G_ASSERT_RETURN(token != nullptr, false);

  auto result = sl_funcs::slSetConstants(constants, *token, viewportId);
  G_ASSERT(result == sl::Result::eOk);

  sl::Resource inColor{sl::ResourceType::eTex2d, params.inColor, params.inColorState};
  sl::Resource inDepth{sl::ResourceType::eTex2d, params.inDepth, params.inDepthState};
  sl::Resource inMotionVectors{sl::ResourceType::eTex2d, params.inMotionVectors, params.inMotionVectorsState};
  sl::Resource inExposure{sl::ResourceType::eTex2d, params.inExposure, params.inExposureState};
  sl::Resource outColor{sl::ResourceType::eTex2d, params.outColor, params.outColorState};

  eastl::fixed_vector<sl::ResourceTag, 5> tags = {
    {&inColor, sl::kBufferTypeScalingInputColor, sl::ResourceLifecycle::eValidUntilEvaluate},
    {&inDepth, sl::kBufferTypeDepth, sl::ResourceLifecycle::eValidUntilEvaluate},
    {&inMotionVectors, sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eValidUntilEvaluate},
    {&inExposure, sl::kBufferTypeExposure, sl::ResourceLifecycle::eValidUntilEvaluate},
    {&outColor, sl::kBufferTypeScalingOutputColor, sl::ResourceLifecycle::eValidUntilEvaluate}};

  result = sl_funcs::slSetTag(viewportId, tags.data(), tags.size(), commandBuffer);
  G_ASSERT(result == sl::Result::eOk);

  const sl::ViewportHandle viewport(viewportId);
  const sl::BaseStructure *inputs[] = {&viewport};
  if (SL_FAILED(result, sl_funcs::slEvaluateFeature(sl::kFeatureDLSS, *token, inputs, eastl::size(inputs), commandBuffer)))
  {
    if (result == sl::Result::eWarnOutOfVRAM)
    {
      logwarn("sl: DLSS ran out of VRAM. We can still continue but expects severe performance degradation.");
    }
    else
    {
      D3D_ERROR("sl: Failed to evaluate DLSS. Result: %s", getResultAsStr(result));
      return false;
    }
  }

  return true;
}

void StreamlineAdapter::initializeDlssState()
{
  if (dlssState == State::DISABLED)
  {
    switch (isDlssSupported())
    {
      case nv::SupportState::DisabledHWS: // not reported by dlss
      case nv::SupportState::NotSupported: dlssState = State::NGX_INIT_ERROR_UNKNOWN; break;
      case nv::SupportState::AdapterNotSupported: dlssState = State::NOT_SUPPORTED_INCOMPATIBLE_HARDWARE; break;
      case nv::SupportState::DriverOutOfDate: dlssState = State::NOT_SUPPORTED_OUTDATED_VGA_DRIVER; break;
      case nv::SupportState::OSOutOfDate: dlssState = State::NOT_SUPPORTED_32BIT; break;
      case nv::SupportState::Supported:
        dlssState = State::SUPPORTED;
        sl_funcs::load_dlss();
        if (isDlssGSupported() == nv::SupportState::Supported)
          sl_funcs::load_dlss_g();
        break;
    }
  }
}

bool StreamlineAdapter::createDlssFeature(int viewportId, IPoint2 output_resolution, void *commandBuffer)
{
  if (dlssState != State::SUPPORTED)
    return false;

  const DataBlock &blk_video = *dgs_get_settings()->getBlockByNameEx("video");
  float sharpness = blk_video.getInt("dlssSharpness", -100.f) / 100.f;
  nv::DLSS::Mode mode = nv::DLSS::Mode::Off;
  if (blk_video.getNameId("antialiasing_mode") > -1 && blk_video.getNameId("antialiasing_upscaling") > -1)
  {
    if (stricmp(blk_video.getStr("antialiasing_mode", "off"), "dlss") == 0)
    {
      auto quality = blk_video.getStr("antialiasing_upscaling", "native");
      if (stricmp(quality, "native") == 0)
        mode = nv::DLSS::Mode::DLAA;
      else if (stricmp(quality, "ultra_quality") == 0)
        mode = nv::DLSS::Mode::UltraQuality;
      else if (stricmp(quality, "quality") == 0)
        mode = nv::DLSS::Mode::MaxQuality;
      else if (stricmp(quality, "balanced") == 0)
        mode = nv::DLSS::Mode::Balanced;
      else if (stricmp(quality, "performance") == 0)
        mode = nv::DLSS::Mode::MaxPerformance;
      else if (stricmp(quality, "ultra_performance") == 0)
        mode = nv::DLSS::Mode::UltraPerformance;
    }
  }
  else
    mode = nv::DLSS::Mode(blk_video.getInt("dlssQuality", -1));

  if (mode == nv::DLSS::Mode::Off)
    return false;

  int renderWidth = output_resolution.x;
  int renderHeight = output_resolution.y;
  if (auto optimalSettings = getOptimalSettings(mode, output_resolution))
  {
    if (sharpness < 0.f)
      sharpness = optimalSettings->sharpness;
    renderWidth = optimalSettings->renderWidth;
    renderHeight = optimalSettings->renderHeight;
  }

  if (renderHeight <= 0 || renderWidth <= 0)
    return false;

  // these values are used for initializing early, can be changed later
  setOptions(viewportId, mode, output_resolution, sharpness);

  // we are going to create a dummy frame to trigger initialization
  sl::FrameToken *frameToken;
  uint32_t frameIndex = 0;
  auto result = sl_funcs::slGetNewFrameToken(frameToken, &frameIndex);
  G_ASSERT_RETURN(result == sl::Result::eOk, false);

  // dummy constants, the below flags set up DLSS correctly, even though SL complains
  // about missing jitter and matrices. Those are not needed for initialization.
  sl::Constants constants{};
  constants.depthInverted = sl::Boolean::eTrue;
  constants.cameraMotionIncluded = sl::Boolean::eTrue;
  constants.motionVectors3D = sl::Boolean::eFalse;
  constants.reset = sl::Boolean::eTrue;

  result = sl_funcs::slSetConstants(constants, *frameToken, viewportId);
  G_ASSERT_RETURN(result == sl::Result::eOk || result == sl::Result::eErrorDuplicatedConstants, false);

  // Dummy resources passed to SL, only the extent is used, the resources are never touched.
  struct DummyResource : public IUnknown
  {
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) { return 0; }
    virtual ULONG STDMETHODCALLTYPE AddRef(void) { return ++cntr; }
    virtual ULONG STDMETHODCALLTYPE Release(void)
    {
      if (cntr == 1)
      {
        delete this;
        return 0;
      }
      else
        return --cntr;
    }
    ULONG cntr = 0;
  } *dummyResource = new DummyResource;

  sl::Resource inColor{sl::ResourceType::eTex2d, dummyResource, 0};
  sl::Resource inDepth{sl::ResourceType::eTex2d, dummyResource, 0};
  sl::Resource inMotionVectors{sl::ResourceType::eTex2d, dummyResource, 0};
  sl::Resource inExposure{sl::ResourceType::eTex2d, dummyResource, 0};
  sl::Resource outColor{sl::ResourceType::eTex2d, dummyResource, 0};

  sl::Extent outputExtent = {0, 0, output_resolution.x, output_resolution.y};
  sl::Extent renderExtent = {0, 0, renderWidth, renderHeight};
  sl::Extent exposureExtent = {0, 0, 1, 1};

  eastl::fixed_vector<sl::ResourceTag, 5> tags = {
    {&inColor, sl::kBufferTypeScalingInputColor, sl::ResourceLifecycle::eValidUntilPresent, &renderExtent},
    {&inDepth, sl::kBufferTypeDepth, sl::ResourceLifecycle::eValidUntilPresent, &renderExtent},
    {&inMotionVectors, sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eValidUntilPresent, &renderExtent},
    {&inExposure, sl::kBufferTypeExposure, sl::ResourceLifecycle::eValidUntilPresent, &exposureExtent},
    {&outColor, sl::kBufferTypeScalingOutputColor, sl::ResourceLifecycle::eValidUntilPresent, &outputExtent}};

  result = sl_funcs::slSetTag(viewportId, tags.data(), tags.size(), commandBuffer);
  G_ASSERT_RETURN(result == sl::Result::eOk, false);

  result = sl_funcs::slAllocateResources(commandBuffer, sl::kFeatureDLSS, viewportId);
  if (result == sl::Result::eOk)
    dlssState = State::READY;
  else
    D3D_ERROR("Failed to initialize DLSS. Error: %s", sl::getResultAsStr(result));

  return result == sl::Result::eOk;
}

bool StreamlineAdapter::releaseDlssFeature(int viewportId)
{
  if (dlssState != State::READY)
    return false;

  bool ok = sl_funcs::slFreeResources(sl::kFeatureDLSS, viewportId) == sl::Result::eOk;
  if (ok)
    dlssState = State::SUPPORTED;

  return ok;
}

static eastl::string versionToString(const sl::Version &version)
{
  return eastl::string(eastl::string::CtorSprintf(), "%d.%d.%d", version.major, version.minor, version.build);
}

dag::Expected<eastl::string, nv::SupportState> StreamlineAdapter::getDlssVersion() const
{
  if (!sl_funcs::slGetFeatureVersion)
    return dag::Unexpected(nv::SupportState::NotSupported);

  sl::FeatureVersion version;
  if (SL_FAILED(result, sl_funcs::slGetFeatureVersion(sl::kFeatureDLSS, version)))
    return dag::Unexpected(toSupportState(result));

  return eastl::string{versionToString(version.versionNGX)};
}

bool StreamlineAdapter::createDlssGFeature(int viewportId, void *commandBuffer)
{
  return sl_funcs::slAllocateResources(commandBuffer, sl::kFeatureDLSS_G, viewportId) == sl::Result::eOk;
}

bool StreamlineAdapter::releaseDlssGFeature(int viewportId)
{
  return sl_funcs::slFreeResources(sl::kFeatureDLSS_G, viewportId) == sl::Result::eOk;
}

bool StreamlineAdapter::isFrameGenerationSupported() const
{
  return dlssState == State::READY && isDlssGSupported() == nv::SupportState::Supported;
}

static void api_error_callback(const sl::APIError &lastError) { logwarn("sl: API error: %d", lastError.hres); }

bool StreamlineAdapter::enableDlssG(int viewportId)
{
  sl::DLSSGOptions options{};
  options.mode = sl::DLSSGMode::eOn;
  options.onErrorCallback = &api_error_callback;
  bool ok = sl_funcs::slDLSSGSetOptions(viewportId, options) == sl::Result::eOk;
  if (ok)
    frameGenerationEnabled = true;

  sl::DLSSGState state;
  ok = sl_funcs::slDLSSGGetState(viewportId, state, &options) == sl::Result::eOk;
  G_ASSERT_RETURN(ok, false);
  if (state.status != sl::DLSSGStatus::eOk)
  {
    D3D_ERROR("sl: Failed to enable DLSS-G. Error %d", eastl::to_underlying(state.status));
    disableDlssG(viewportId);
  }

  return ok;
}

bool StreamlineAdapter::disableDlssG(int viewportId)
{
  sl::DLSSGOptions options{};
  options.mode = sl::DLSSGMode::eOff;
  if (sl_funcs::slDLSSGSetOptions(viewportId, options) != sl::Result::eOk)
    return false;

  frameGenerationEnabled = false;
  return true;
}

void StreamlineAdapter::setDlssGSuppressed(bool suppressed)
{
  if (!frameGenerationEnabled)
    return;

  isSuppressed = suppressed;
}

bool StreamlineAdapter::evaluateDlssG(int viewportId, const nv::DlssGParams<void> &params, void *commandBuffer)
{
  sl::DLSSGOptions options{};
  options.onErrorCallback = &api_error_callback;
  options.mode = isSuppressed ? sl::DLSSGMode::eOff : sl::DLSSGMode::eOn;
  options.flags = sl::DLSSGFlags::eRetainResourcesWhenOff;
  sl_funcs::slDLSSGSetOptions(viewportId, options);

  if (isSuppressed)
    return true;

  sl::Resource inHUDless{sl::ResourceType::eTex2d, params.inHUDless, params.inHUDlessState};
  sl::Resource inUI{sl::ResourceType::eTex2d, params.inUI, params.inUIState};
  sl::Resource inDepth{sl::ResourceType::eTex2d, params.inDepth, params.inDepthState};
  sl::Resource inMotionVectors{sl::ResourceType::eTex2d, params.inMotionVectors, params.inMotionVectorsState};

  sl::ResourceTag tags[] = {
    {&inHUDless, sl::kBufferTypeHUDLessColor, sl::ResourceLifecycle::eOnlyValidNow},
    {&inUI, sl::kBufferTypeUIColorAndAlpha, sl::ResourceLifecycle::eValidUntilPresent},
    {&inDepth, sl::kBufferTypeDepth, sl::ResourceLifecycle::eValidUntilPresent},
    {&inMotionVectors, sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eValidUntilPresent},
  };

  bool success = sl_funcs::slSetTag(viewportId, tags, eastl::size(tags), commandBuffer) == sl::Result::eOk;
  G_ASSERT(success);

  return success;
}

unsigned StreamlineAdapter::getActualFramesPresented() const
{
  if (!frameGenerationEnabled)
    return 1;

  sl::DLSSGState state;
  auto result = sl_funcs::slDLSSGGetState(0, state, nullptr);
  G_ASSERT_RETURN(result == sl::Result::eOk && state.status == sl::DLSSGStatus::eOk, 1);
  return state.numFramesActuallyPresented;
}