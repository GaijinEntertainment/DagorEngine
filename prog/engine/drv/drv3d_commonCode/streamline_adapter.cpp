// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "streamline_adapter.h"

#include "3d/dag_latencyTypes.h"
#include "3d/dag_nvFeatures.h"
#include "drv/3d/dag_info.h"
#include "debug/dag_assert.h"
#include "drv_log_defs.h"
#include "sl_core_types.h"

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
#include <sl_dlss_d.h>
#include <sl_dlss_g.h>
#include <sl_reflex.h>

#include <dxgi.h>
#include <d3d12video.h>

#if _TARGET_PC_WIN && _TARGET_64BIT
#include <vulkan/vulkan.h>
#endif

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
SL_FUN_DECL(slIsFeatureLoaded);
SL_FUN_DECL(slGetFeatureFunction);
SL_FUN_DECL(slGetNewFrameToken);
SL_FUN_DECL(slSetConstants);
SL_FUN_DECL(slSetTag);
SL_FUN_DECL(slEvaluateFeature);
SL_FUN_DECL(slAllocateResources);
SL_FUN_DECL(slFreeResources);
SL_FUN_DECL(slGetFeatureVersion);

SL_FUN_DECL(slDLSSGetOptimalSettings);
SL_FUN_DECL(slDLSSSetOptions);

SL_FUN_DECL(slPCLSetMarker);
SL_FUN_DECL(slReflexSetOptions);
SL_FUN_DECL(slReflexGetState);
SL_FUN_DECL(slReflexSleep);

SL_FUN_DECL(slDLSSGSetOptions);
SL_FUN_DECL(slDLSSGGetState);

SL_FUN_DECL(slDLSSDGetOptimalSettings);
SL_FUN_DECL(slDLSSDSetOptions);

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
  LOAD_FUNC(slIsFeatureLoaded);
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
  slIsFeatureLoaded = nullptr;
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

void unload_dlss()
{
  slDLSSGetOptimalSettings = nullptr;
  slDLSSSetOptions = nullptr;
}

void load_reflex()
{
  G_VERIFY(sl_funcs::slGetFeatureFunction(sl::kFeaturePCL, "slPCLSetMarker", (void *&)slPCLSetMarker) == sl::Result::eOk);
  G_VERIFY(sl_funcs::slGetFeatureFunction(sl::kFeatureReflex, "slReflexSetOptions", (void *&)slReflexSetOptions) == sl::Result::eOk);
  G_VERIFY(sl_funcs::slGetFeatureFunction(sl::kFeatureReflex, "slReflexGetState", (void *&)slReflexGetState) == sl::Result::eOk);
  G_VERIFY(sl_funcs::slGetFeatureFunction(sl::kFeatureReflex, "slReflexSleep", (void *&)slReflexSleep) == sl::Result::eOk);
}

void unload_reflex()
{
  slPCLSetMarker = nullptr;
  slReflexSetOptions = nullptr;
  slReflexGetState = nullptr;
  slReflexSleep = nullptr;
}

void load_dlss_g()
{
  G_VERIFY(sl_funcs::slGetFeatureFunction(sl::kFeatureDLSS_G, "slDLSSGSetOptions", (void *&)slDLSSGSetOptions) == sl::Result::eOk);
  G_VERIFY(sl_funcs::slGetFeatureFunction(sl::kFeatureDLSS_G, "slDLSSGGetState", (void *&)slDLSSGGetState) == sl::Result::eOk);
}

void unload_dlss_g()
{
  slDLSSGSetOptions = nullptr;
  slDLSSGGetState = nullptr;
}

void load_dlss_d()
{
  G_VERIFY(sl_funcs::slGetFeatureFunction(sl::kFeatureDLSS_RR, "slDLSSDGetOptimalSettings", (void *&)slDLSSDGetOptimalSettings) ==
           sl::Result::eOk);
  G_VERIFY(sl_funcs::slGetFeatureFunction(sl::kFeatureDLSS_RR, "slDLSSDSetOptions", (void *&)slDLSSDSetOptions) == sl::Result::eOk);
}

void unload_dlss_d()
{
  slDLSSDGetOptimalSettings = nullptr;
  slDLSSDSetOptions = nullptr;
}

bool is_feature_available(sl::Feature feature)
{
  bool isAvailable = false;
  return slIsFeatureLoaded(feature, isAvailable) == sl::Result::eOk && isAvailable;
}

void load_all_available_features()
{
  if (is_feature_available(sl::kFeatureDLSS))
    load_dlss();
  if (is_feature_available(sl::kFeatureDLSS_RR))
    load_dlss_d();
  if (is_feature_available(sl::kFeatureDLSS_G))
    load_dlss_g();
  if (is_feature_available(sl::kFeatureReflex) && is_feature_available(sl::kFeaturePCL))
    load_reflex();
}

void unload_all_available_features()
{
  unload_dlss();
  unload_dlss_d();
  unload_dlss_g();
  unload_reflex();
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

sl::FrameToken &FrameTracker::getFrameToken(uint32_t frame_id)
{
  sl::FrameToken *&token = frameTokens[frame_id % MAX_CONCURRENT_FRAMES];
  if (!token)
    sl_funcs::slGetNewFrameToken(token, &frame_id);
  return *token;
}

void FrameTracker::startFrame(uint32_t frame_id)
{
  frameTokens[frame_id % MAX_CONCURRENT_FRAMES] = nullptr;
  constantsInitialized[frame_id % MAX_CONCURRENT_FRAMES] = {};
}

// COMMON STUFF

static sl::float4 toFloat4(const Point4 &p) { return sl::float4(p.x, p.y, p.z, p.w); }
static sl::float3 toFloat3(const Point3 &p) { return sl::float3(p.x, p.y, p.z); }

static sl::float4x4 toFloat4x4(const TMatrix4 &t)
{
  return sl::float4x4{toFloat4(t.row[0]), toFloat4(t.row[1]), toFloat4(t.row[2]), toFloat4(t.row[3])};
}

static void set_camera_constants(sl::Constants &constants, const nv::Camera &camera)
{
  constants.cameraPinholeOffset = {0.f, 0.f};
  constants.cameraViewToClip = toFloat4x4(camera.projection);
  constants.clipToCameraView = toFloat4x4(camera.projectionInverse);
  constants.clipToPrevClip = toFloat4x4(camera.reprojection);
  constants.prevClipToClip = toFloat4x4(camera.reprojectionInverse);
  constants.cameraPos = toFloat3(camera.position);
  constants.cameraUp = toFloat3(camera.up);
  constants.cameraRight = toFloat3(camera.right);
  constants.cameraFwd = toFloat3(camera.forward);
  constants.cameraNear = camera.nearZ;
  constants.cameraFar = camera.farZ;
  constants.cameraFOV = camera.fov;
  constants.cameraAspectRatio = camera.aspect;
}

static void set_common_constants(sl::Constants &constants)
{
  constants.depthInverted = sl::Boolean::eTrue;
  constants.cameraMotionIncluded = sl::Boolean::eTrue;
  constants.motionVectors3D = sl::Boolean::eFalse;
  constants.reset = sl::Boolean::eFalse;
}

static void set_motion_vector_constants(sl::Constants &constants, Point2 mv_scale, Point2 jitter_offset)
{
  constants.mvecScale = {mv_scale.x, mv_scale.y};
  constants.jitterOffset = {jitter_offset.x, jitter_offset.y};
}

template <typename Params>
void FrameTracker::initConstants(const Params &params, int viewport_id)
{
  const uint32_t frameId = params.frameId;
  if (constantsInitialized[frameId % MAX_CONCURRENT_FRAMES][viewport_id])
    return;

  sl::Constants constants{};
  set_motion_vector_constants(constants, {params.inMVScaleX, params.inMVScaleY}, {params.inJitterOffsetX, params.inJitterOffsetY});
  set_common_constants(constants);
  set_camera_constants(constants, params.camera);

  sl::Result result = sl_funcs::slSetConstants(constants, getFrameToken(frameId), viewport_id);
  switch (result)
  {
    case sl::Result::eOk: break;
    case sl::Result::eErrorDuplicatedConstants: logwarn("SL: Duplicated constants detected."); break;
    default: D3D_ERROR("DLSS(-RR/-G): Failed to set constants. Result: %s", sl::getResultAsStr(result));
  }

  constantsInitialized[frameId % MAX_CONCURRENT_FRAMES][viewport_id] = true;
}

StreamlineAdapter::InterposerHandleType StreamlineAdapter::loadInterposer()
{
#if !_TARGET_64BIT
  return {nullptr, nullptr};
#else

#if _TARGET_PC_WIN
  if (GetModuleHandleA("renderdoc.dll"))
  {
    logdbg("sl: RenderDoc detected. Streamline will be disabled for this session.");
    return {nullptr, nullptr};
  }
#endif

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
#endif
}

void *StreamlineAdapter::getInterposerSymbol(const InterposerHandleType &interposer, const char *name)
{
  return os_dll_get_symbol(interposer.get(), name);
}

struct StreamlineAdapter::InitArgs
{
  void setProjectId(const char *project_id)
  {
    projectId = project_id;
    preferences.projectId = projectId.c_str();
  }

  sl::Preferences preferences;
  eastl::fixed_vector<sl::Feature, 5> features;
  eastl::string projectId;
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

  initArgs->preferences.flags = sl::PreferenceFlags::eDisableCLStateTracking;
  if (api == RenderAPI::DX12 || api == RenderAPI::Vulkan)
    initArgs->preferences.flags |= sl::PreferenceFlags::eUseManualHooking;

  initArgs->features = {sl::kFeatureDLSS, sl::kFeatureReflex, sl::kFeaturePCL};
  if (api == RenderAPI::DX12 || api == RenderAPI::Vulkan)
  {
    initArgs->features.push_back(sl::kFeatureDLSS_G);
    initArgs->features.push_back(sl::kFeatureDLSS_RR);
  }

  initArgs->preferences.featuresToLoad = initArgs->features.data();
  initArgs->preferences.numFeaturesToLoad = initArgs->features.size();
  initArgs->preferences.engine = sl::EngineType::eCustom;
  initArgs->preferences.engineVersion = "6.0.0.0";
  initArgs->setProjectId(settings->getStr("nvidia_project_id", "6352d1d6-3e63-4dba-8da1-c96bdc320a02"));

  auto toSlRenderApi = [](auto api) {
    switch (api)
    {
      case RenderAPI::DX11: return sl::RenderAPI::eD3D11;
      case RenderAPI::DX12: return sl::RenderAPI::eD3D12;
      case RenderAPI::Vulkan: return sl::RenderAPI::eVulkan;
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
  supportOverride(eastl::move(support_override))
{}

StreamlineAdapter::~StreamlineAdapter()
{
  G_ASSERT(sl_funcs::slShutdown);
  for (size_t i = 0; i < MAX_VIEWPORTS; i++)
  {
    dlssFeatures[i].reset();
    dlssGFeatures[i].reset();
  }
  reflexFeature.reset();
  sl_funcs::unload_all_available_features();
  sl::Result result = sl_funcs::slShutdown();
  G_ASSERT(result == sl::Result::eOk);
}

void *StreamlineAdapter::hook(IUnknown *object)
{
  if (object == nullptr)
    return nullptr;

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

void *StreamlineAdapter::unhook(IUnknown *object)
{
  // slGetNativeInterface calls AddRef on the result on success, but from this function
  // the caller can't know if that happened, so call AddRef in every case.

  if (object == nullptr)
    return nullptr;

  if (!sl_funcs::slGetNativeInterface)
  {
    logdbg("sl: Unhook isn't available");
    object->AddRef();
    return object;
  }

  void *base = object;
  if (SL_FAILED(result, sl_funcs::slGetNativeInterface(object, &base)))
  {
    D3D_ERROR("sl: Failed to unhook device: %s", getResultAsStr(result));
    object->AddRef();
    return object;
  }

  return base;
}

void StreamlineAdapter::setAdapterAndDevice(IDXGIAdapter1 *adapter, ID3D11Device *device)
{
  this->adapter = adapter;
  sl::Result result = sl_funcs::slSetD3DDevice(device);
  G_ASSERT(result == sl::Result::eOk);

  sl_funcs::load_all_available_features();
}

void StreamlineAdapter::setAdapterAndDevice(IDXGIAdapter1 *adapter, ID3D12Device5 *device)
{
  this->adapter = adapter;
  sl::Result result = sl_funcs::slSetD3DDevice(device);
  G_ASSERT(result == sl::Result::eOk);

  sl_funcs::load_all_available_features();
}

void StreamlineAdapter::setVulkan() { sl_funcs::load_all_available_features(); }

void StreamlineAdapter::preRecover()
{
  adapter.Reset();
  G_ASSERT(sl_funcs::slShutdown);
  sl_funcs::unload_all_available_features();
  sl::Result result = sl_funcs::slShutdown();
  G_ASSERT(result == sl::Result::eOk);
}

void StreamlineAdapter::recover()
{
  if (SL_FAILED(result, sl_funcs::slInit(initArgs->preferences, sl::kSDKVersion)))
  {
    logwarn("sl: Failed to initialize Streamline: %s. DLSS and Reflex will be disabled.", getResultAsStr(result));
    // slShutdown will set PluginManager::s_status to ePluginsUnloaded to make sure we can try again later
    G_VERIFY(sl_funcs::slShutdown() == sl::Result::eOk);
  }
  frameTracker = {};
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

nv::SupportState StreamlineAdapter::isDlssRRSupported() const
{
  return isFeatureSupported(sl::kFeatureDLSS_RR, adapter.Get(), supportOverride);
}

nv::SupportState StreamlineAdapter::isReflexSupported() const
{
  return isFeatureSupported(sl::kFeatureReflex, adapter.Get(), supportOverride);
}

// DLSS

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

static nv::DLSS::Mode parse_mode_from_settings()
{
  const DataBlock &blk_video = *dgs_get_settings()->getBlockByNameEx("video");
  if (blk_video.getNameId("antialiasing_mode") > -1 && blk_video.getNameId("antialiasing_upscaling") > -1)
  {
    if (stricmp(blk_video.getStr("antialiasing_mode", "off"), "dlss") == 0)
    {
      auto quality = blk_video.getStr("antialiasing_upscaling", "native");
      if (stricmp(quality, "native") == 0)
        return nv::DLSS::Mode::DLAA;
      else if (stricmp(quality, "ultra_quality") == 0)
        return nv::DLSS::Mode::UltraQuality;
      else if (stricmp(quality, "quality") == 0)
        return nv::DLSS::Mode::MaxQuality;
      else if (stricmp(quality, "balanced") == 0)
        return nv::DLSS::Mode::Balanced;
      else if (stricmp(quality, "performance") == 0)
        return nv::DLSS::Mode::MaxPerformance;
      else if (stricmp(quality, "ultra_performance") == 0)
        return nv::DLSS::Mode::UltraPerformance;
    }
  }

  return nv::DLSS::Mode(blk_video.getInt("dlssQuality", -1));
}

static bool parse_ray_reconstruction_mode_from_settings()
{
  const DataBlock &blk_video = *dgs_get_settings()->getBlockByNameEx("video");
  return blk_video.getBool("rayReconstruction", false);
}

static void setup_dummy_frame(int viewport_id, IPoint2 output_resolution, nv::DLSS::Mode mode, void *command_buffer)
{
  sl::DLSSOptimalSettings optimalSettings{};
  sl::DLSSOptions options{};

  options.mode = toDLSSMode(mode);
  options.outputWidth = output_resolution.x;
  options.outputHeight = output_resolution.y;
  sl_funcs::slDLSSGetOptimalSettings(options, optimalSettings);

  int renderWidth = optimalSettings.optimalRenderWidth;
  int renderHeight = optimalSettings.optimalRenderHeight;

  if (renderHeight <= 0 || renderWidth <= 0)
    return;

  sl_funcs::slDLSSSetOptions(viewport_id, options);

  // we are going to create a dummy frame to trigger initialization
  sl::FrameToken *frameToken;
  uint32_t frameIndex = 0;
  auto result = sl_funcs::slGetNewFrameToken(frameToken, &frameIndex);
  G_ASSERT(result == sl::Result::eOk);

  // dummy constants, the below flags set up DLSS correctly, even though SL complains
  // about missing jitter and matrices. Those are not needed for initialization.
  sl::Constants constants{};
  constants.depthInverted = sl::Boolean::eTrue;
  constants.cameraMotionIncluded = sl::Boolean::eTrue;
  constants.motionVectors3D = sl::Boolean::eFalse;
  constants.reset = sl::Boolean::eTrue;

  result = sl_funcs::slSetConstants(constants, *frameToken, viewport_id);
  G_ASSERT(result == sl::Result::eOk || result == sl::Result::eErrorDuplicatedConstants);

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

  result = sl_funcs::slSetTag(viewport_id, tags.data(), tags.size(), command_buffer);
  G_ASSERT(result == sl::Result::eOk);
}

DLSSSuperResolution::DLSSSuperResolution(int viewport_id, void *command_buffer, FrameTracker &frame_tracker) :
  viewportId(viewport_id), frameTracker(frame_tracker)
{
  sl::Result result = sl_funcs::slAllocateResources(command_buffer, sl::kFeatureDLSS, viewportId);
  if (result != sl::Result::eOk)
    D3D_ERROR("DLSS: Failed to initialize. Error: %s", sl::getResultAsStr(result));
}

DLSSSuperResolution::~DLSSSuperResolution() { G_VERIFY(sl_funcs::slFreeResources(sl::kFeatureDLSS, viewportId) == sl::Result::eOk); }

eastl::optional<nv::DLSS::OptimalSettings> DLSSSuperResolution::getOptimalSettings(nv::DLSS::Mode mode,
  IPoint2 output_resolution) const
{
  sl::DLSSOptimalSettings optimalSettings{};
  sl::DLSSOptions options{};

  options.mode = toDLSSMode(mode);
  options.outputWidth = output_resolution.x;
  options.outputHeight = output_resolution.y;

  options.dlaaPreset = sl::DLSSPreset::ePresetJ;
  options.qualityPreset = sl::DLSSPreset::ePresetJ;
  options.balancedPreset = sl::DLSSPreset::ePresetJ;
  options.performancePreset = sl::DLSSPreset::ePresetJ;

  return sl_funcs::slDLSSGetOptimalSettings(options, optimalSettings) == sl::Result::eOk
           ? eastl::make_optional<OptimalSettings>(optimalSettings.optimalRenderWidth, optimalSettings.optimalRenderHeight, false)
           : eastl::nullopt;
}

bool DLSSSuperResolution::setOptions(nv::DLSS::Mode mode, IPoint2 output_resolution)
{
  sl::DLSSOptions options{};
  options.mode = toDLSSMode(mode);
  options.outputWidth = output_resolution.x;
  options.outputHeight = output_resolution.y;

  return sl_funcs::slDLSSSetOptions(viewportId, options) == sl::Result::eOk;
}

bool DLSSSuperResolution::isModeAvailableAtResolution(nv::DLSS::Mode mode, const IPoint2 &resolution)
{
  sl::DLSSOptimalSettings optimalSettings{};
  sl::DLSSOptions options{};

  options.mode = toDLSSMode(mode);
  options.outputWidth = resolution.x;
  options.outputHeight = resolution.y;
  return sl_funcs::slDLSSGetOptimalSettings(options, optimalSettings) == sl::Result::eOk && optimalSettings.optimalRenderHeight > 0 &&
         optimalSettings.optimalRenderWidth > 0;
}

bool DLSSSuperResolution::evaluate(const nv::DlssParams<void> &params, void *command_buffer)
{
  frameTracker.initConstants(params, viewportId);

  sl::Resource inColor{sl::ResourceType::eTex2d, params.inColor, params.inColorState};
  sl::Resource inDepth{sl::ResourceType::eTex2d, params.inDepth, params.inDepthState};
  sl::Resource inMotionVectors{sl::ResourceType::eTex2d, params.inMotionVectors, params.inMotionVectorsState};
  sl::Resource inExposure{sl::ResourceType::eTex2d, params.inExposure, params.inExposureState};
  sl::Resource outColor{sl::ResourceType::eTex2d, params.outColor, params.outColorState};

#if _TARGET_PC_WIN && _TARGET_64BIT
  if (d3d::get_driver_code().is(d3d::vulkan))
  {
    auto proc = [](sl::Resource &res, bool out = false) {
      auto &[image, info, view] = *(eastl::tuple<VkImage, VkImageCreateInfo, VkImageView> *)(res.native);
      res.native = image;
      res.view = view;
      res.width = info.extent.width;
      res.height = info.extent.height;
      res.nativeFormat = info.format;
      res.mipLevels = 1;
      res.arrayLayers = 1;
      res.usage = out ? VK_IMAGE_USAGE_STORAGE_BIT : VK_IMAGE_USAGE_SAMPLED_BIT;
    };

    proc(inColor);
    proc(inDepth);
    proc(inMotionVectors);
    proc(inExposure);
    proc(outColor, true);
  }
#endif

  eastl::fixed_vector<sl::ResourceTag, 5> tags = {
    {&inColor, sl::kBufferTypeScalingInputColor, sl::ResourceLifecycle::eValidUntilEvaluate},
    {&inDepth, sl::kBufferTypeDepth, sl::ResourceLifecycle::eValidUntilEvaluate},
    {&inMotionVectors, sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eValidUntilEvaluate},
    {&inExposure, sl::kBufferTypeExposure, sl::ResourceLifecycle::eValidUntilEvaluate},
    {&outColor, sl::kBufferTypeScalingOutputColor, sl::ResourceLifecycle::eValidUntilEvaluate}};

  sl::Result result = sl_funcs::slSetTag(viewportId, tags.data(), tags.size(), command_buffer);
  G_ASSERT(result == sl::Result::eOk);

  const sl::ViewportHandle viewport(viewportId);
  const sl::BaseStructure *inputs[] = {&viewport};
  if (SL_FAILED(result, sl_funcs::slEvaluateFeature(sl::kFeatureDLSS, frameTracker.getFrameToken(params.frameId), inputs,
                          eastl::size(inputs), command_buffer)))
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

nv::DLSS *StreamlineAdapter::createDlssFeature(int viewport_id, IPoint2 output_resolution, void *command_buffer)
{
  nv::DLSS::Mode modeFromSettings = parse_mode_from_settings();
  if (modeFromSettings == nv::DLSS::Mode::Off)
    return nullptr;

  // On VK it is not possible to make dummy resources. Also not needed.
  if (initArgs->preferences.renderAPI != sl::RenderAPI::eVulkan)
    setup_dummy_frame(viewport_id, output_resolution, modeFromSettings, command_buffer);

  bool wantRayReconstruction =
    this->initArgs->preferences.renderAPI == sl::RenderAPI::eD3D12 && parse_ray_reconstruction_mode_from_settings();
  bool hasRayReconstruction = isFeatureSupported(sl::kFeatureDLSS_RR, adapter.Get(), supportOverride) == nv::SupportState::Supported;
  if (wantRayReconstruction && !hasRayReconstruction)
  {
    wantRayReconstruction = false;
    logwarn("sl: Ray reconstruction requested but feature is not supported. Likely a .dll is missing. Falling back to regular DLSS.");
  }

  dlssFeatures[viewport_id] =
    wantRayReconstruction &&
        eastl::find(initArgs->features.begin(), initArgs->features.end(), sl::kFeatureDLSS_RR) != initArgs->features.end()
      ? static_cast<eastl::unique_ptr<nv::DLSS>>(eastl::make_unique<DLSSRayReconstruction>(viewport_id, command_buffer, frameTracker))
      : static_cast<eastl::unique_ptr<nv::DLSS>>(eastl::make_unique<DLSSSuperResolution>(viewport_id, command_buffer, frameTracker));
  return dlssFeatures[viewport_id].get();
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

// DLSSG

static void api_error_callback(const sl::APIError &lastError) { logwarn("DLSSG: API error: %d", lastError.hres); }

DLSSFrameGeneration::DLSSFrameGeneration(int viewport_id, void *command_buffer, FrameTracker &frame_tracker) :
  viewportId(viewport_id), frameTracker(frame_tracker)
{}

DLSSFrameGeneration::~DLSSFrameGeneration() { G_VERIFY(sl_funcs::slFreeResources(sl::kFeatureDLSS_G, viewportId) == sl::Result::eOk); }

static bool change_dlssg_mode(int viewport_id, bool retain_resources, int frames_to_generate)
{
  sl::DLSSGOptions options{};
  options.mode = frames_to_generate > 0 ? sl::DLSSGMode::eOn : sl::DLSSGMode::eOff;
  options.flags = retain_resources ? sl::DLSSGFlags::eRetainResourcesWhenOff : sl::DLSSGFlags(0);
  options.onErrorCallback = &api_error_callback;
  options.numFramesToGenerate = eastl::max(1, frames_to_generate);
  G_VERIFY(sl_funcs::slDLSSGSetOptions(viewport_id, options) == sl::Result::eOk);

  sl::DLSSGState state;
  G_VERIFY(sl_funcs::slDLSSGGetState(viewport_id, state, &options) == sl::Result::eOk);
  if (state.status != sl::DLSSGStatus::eOk)
    logerr("DLSSG: failed to set to %s state", frames_to_generate > 0 ? "enabled" : "disabled");
  return state.status == sl::DLSSGStatus::eOk ? frames_to_generate : 0;
}

void DLSSFrameGeneration::setEnabled(int frames_to_generate)
{
  if (frames_to_generate > 0)
  {
    this->framesToGenerate = frames_to_generate;
  }
  else
    this->framesToGenerate = change_dlssg_mode(viewportId, false, frames_to_generate);
}

void DLSSFrameGeneration::setSuppressed(bool suppressed) { this->suppressed = suppressed; }

bool DLSSFrameGeneration::evaluate(const nv::DlssGParams<void> &params, void *command_buffer)
{
  if (framesToGenerate <= 0)
    return true;

  if (suppressed)
  {
    // this is a workaround to a bug causing DLSS-G to only retain resources when options are passed in each frame
    change_dlssg_mode(viewportId, true, 0);
    return true;
  }
  else
  {
    change_dlssg_mode(viewportId, true, framesToGenerate);
  }

  frameTracker.initConstants(params, viewportId);

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

  bool success = sl_funcs::slSetTag(viewportId, tags, eastl::size(tags), command_buffer) == sl::Result::eOk;
  G_ASSERT(success);

  return success;
}

unsigned DLSSFrameGeneration::getActualFramesPresented() const
{
  if (framesToGenerate <= 0)
    return 1;

  sl::DLSSGState state;
  auto result = sl_funcs::slDLSSGGetState(0, state, nullptr);
  G_ASSERT_RETURN(result == sl::Result::eOk && state.status == sl::DLSSGStatus::eOk, 1);
  return state.numFramesActuallyPresented;
}

int DLSSFrameGeneration::getMaximumNumberOfGeneratedFrames()
{
  sl::DLSSGState state;
  auto result = sl_funcs::slDLSSGGetState(0, state, nullptr);
  G_ASSERT_RETURN(result == sl::Result::eOk && state.status == sl::DLSSGStatus::eOk, 0);
  return state.numFramesToGenerateMax;
}

static bool is_frame_generation_enabled_in_settings()
{
  const DataBlock &blk_video = *dgs_get_settings()->getBlockByNameEx("video");
  return blk_video.getInt("dlssFrameGenerationCount", 0) > 0 ||
         (stricmp(blk_video.getStr("antialiasing_mode", "off"), "dlss") == 0 && blk_video.getInt("antialiasing_fgc", 0) > 0);
}

DLSSFrameGeneration *StreamlineAdapter::createDlssGFeature(int viewport_id, void *command_buffer)
{
  if (!is_frame_generation_enabled_in_settings())
    return nullptr;

  dlssGFeatures[viewport_id].emplace(viewport_id, command_buffer, frameTracker);
  return &dlssGFeatures[viewport_id].value();
}

// REFLEX

Reflex::Reflex(FrameTracker &frame_tracker) : frameTracker(frame_tracker) {}

void Reflex::startFrame(uint32_t frame_id) { frameTracker.startFrame(frame_id); }

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

bool Reflex::setMarker(uint32_t frame_id, lowlatency::LatencyMarkerType marker_type)
{
  return sl_funcs::slPCLSetMarker(toPCLMarker(marker_type), frameTracker.getFrameToken(frame_id)) == sl::Result::eOk;
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

bool Reflex::setOptions(nv::Reflex::ReflexMode mode, unsigned frame_limit_us)
{
  sl::ReflexOptions options;
  options.frameLimitUs = frame_limit_us;
  options.mode = toReflexMode(mode);
  currentReflexMode = mode;
  bool ok = sl_funcs::slReflexSetOptions(options) == sl::Result::eOk;
  if (ok)
    currentReflexMode = mode;
  return ok;
}

eastl::optional<nv::Reflex::ReflexState> Reflex::getState() const
{
  sl::ReflexState state;
  bool ok = sl_funcs::slReflexGetState(state) == sl::Result::eOk;
  if (!ok)
    return eastl::nullopt;

  nv::Reflex::ReflexState result{state.lowLatencyAvailable, state.latencyReportAvailable, state.flashIndicatorDriverControlled};
  static_assert(eastl::size(state.frameReport) == eastl::size(result.stats));
  eastl::transform(eastl::begin(state.frameReport), eastl::end(state.frameReport), result.stats, toReflexStats);
  return result;
}

bool Reflex::sleep(uint32_t frame_id)
{
  auto result = sl_funcs::slReflexSleep(frameTracker.getFrameToken(frame_id));
  G_ASSERT(result == sl::Result::eOk);
  return result == sl::Result::eOk;
}

Reflex::ReflexMode Reflex::getCurrentMode() const { return currentReflexMode; }

Reflex *StreamlineAdapter::createReflexFeature()
{
  reflexFeature.emplace(frameTracker);
  return &reflexFeature.value();
}

// DLSSD

DLSSRayReconstruction::DLSSRayReconstruction(int viewport_id, void *command_buffer, FrameTracker &frame_tracker) :
  viewportId(viewport_id), frameTracker(frame_tracker)
{
  sl::Result result = sl_funcs::slAllocateResources(command_buffer, sl::kFeatureDLSS_RR, viewportId);
  if (result != sl::Result::eOk)
    D3D_ERROR("DLSS: Failed to initialize. Error: %s", sl::getResultAsStr(result));
  isRR = true;
}

DLSSRayReconstruction::~DLSSRayReconstruction()
{
  G_VERIFY(sl_funcs::slFreeResources(sl::kFeatureDLSS_RR, viewportId) == sl::Result::eOk);
}

eastl::optional<nv::DLSS::OptimalSettings> DLSSRayReconstruction::getOptimalSettings(nv::DLSS::Mode mode,
  IPoint2 output_resolution) const
{
  sl::DLSSDOptimalSettings optimalSettings{};
  sl::DLSSDOptions options{};

  options.mode = toDLSSMode(mode);
  options.outputWidth = output_resolution.x;
  options.outputHeight = output_resolution.y;
  return sl_funcs::slDLSSDGetOptimalSettings(options, optimalSettings) == sl::Result::eOk
           ? eastl::make_optional<OptimalSettings>(optimalSettings.optimalRenderWidth, optimalSettings.optimalRenderHeight, true)
           : eastl::nullopt;
}

static bool dlssd_set_options(int viewport_id, nv::DLSS::Mode mode, IPoint2 output_resolution, const TMatrix4 &worldToView,
  const TMatrix4 &viewToWorld)
{
  sl::DLSSDOptions options{};
  options.mode = toDLSSMode(mode);
  options.outputWidth = output_resolution.x;
  options.outputHeight = output_resolution.y;
  options.normalRoughnessMode = sl::DLSSDNormalRoughnessMode::ePacked;
  options.worldToCameraView = toFloat4x4(worldToView);
  options.cameraViewToWorld = toFloat4x4(viewToWorld);

  return sl_funcs::slDLSSDSetOptions(viewport_id, options) == sl::Result::eOk;
}

bool DLSSRayReconstruction::setOptions(nv::DLSS::Mode mode, IPoint2 output_resolution)
{
  bool ok = dlssd_set_options(viewportId, mode, output_resolution, TMatrix4::IDENT, TMatrix4::IDENT);
  if (ok)
  {
    this->currentMode = mode;
    this->currentOutputResolution = output_resolution;
  }
  return ok;
}

bool DLSSRayReconstruction::evaluate(const nv::DlssParams<void> &params, void *command_buffer)
{
  frameTracker.initConstants(params, viewportId);

  dlssd_set_options(viewportId, currentMode, currentOutputResolution, params.camera.worldToView, params.camera.viewToWorld);

  sl::Resource inColor{sl::ResourceType::eTex2d, params.inColor, params.inColorState};
  sl::Resource inDepth{sl::ResourceType::eTex2d, params.inDepth, params.inDepthState};
  sl::Resource inMotionVectors{sl::ResourceType::eTex2d, params.inMotionVectors, params.inMotionVectorsState};
  sl::Resource inExposure{sl::ResourceType::eTex2d, params.inExposure, params.inExposureState};
  sl::Resource inAlbedo{sl::ResourceType::eTex2d, params.inAlbedo, params.inAlbedoState};
  sl::Resource inSpecularAlbedo{sl::ResourceType::eTex2d, params.inSpecularAlbedo, params.inSpecularAlbedoState};
  sl::Resource inNormalRoughness{sl::ResourceType::eTex2d, params.inNormalRoughness, params.inNormalRoughnessState};
  sl::Resource inHitDist{sl::ResourceType::eTex2d, params.inHitDist, params.inHitDistState};
  sl::Resource outColor{sl::ResourceType::eTex2d, params.outColor, params.outColorState};

  eastl::fixed_vector<sl::ResourceTag, 9> tags = {
    {&inColor, sl::kBufferTypeScalingInputColor, sl::ResourceLifecycle::eValidUntilEvaluate},
    {&inDepth, sl::kBufferTypeDepth, sl::ResourceLifecycle::eValidUntilEvaluate},
    {&inMotionVectors, sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eValidUntilEvaluate},
    {&inExposure, sl::kBufferTypeExposure, sl::ResourceLifecycle::eValidUntilEvaluate},
    {&inAlbedo, sl::kBufferTypeAlbedo, sl::ResourceLifecycle::eValidUntilEvaluate},
    {&inSpecularAlbedo, sl::kBufferTypeSpecularAlbedo, sl::ResourceLifecycle::eValidUntilEvaluate},
    {&inNormalRoughness, sl::kBufferTypeNormalRoughness, sl::ResourceLifecycle::eValidUntilEvaluate},
    {&inHitDist, sl::kBufferTypeSpecularHitDistance, sl::ResourceLifecycle::eValidUntilEvaluate},
    {&outColor, sl::kBufferTypeScalingOutputColor, sl::ResourceLifecycle::eValidUntilEvaluate}};

  sl::Result result = sl_funcs::slSetTag(viewportId, tags.data(), tags.size(), command_buffer);
  G_ASSERT(result == sl::Result::eOk);

  const sl::ViewportHandle viewport(viewportId);
  const sl::BaseStructure *inputs[] = {&viewport};
  if (SL_FAILED(result, sl_funcs::slEvaluateFeature(sl::kFeatureDLSS_RR, frameTracker.getFrameToken(params.frameId), inputs,
                          eastl::size(inputs), command_buffer)))
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
