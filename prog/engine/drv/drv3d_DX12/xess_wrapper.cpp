// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "xess_wrapper.h"
#include "device.h"

#include <osApiWrappers/dag_dynLib.h>
#include <osApiWrappers/dag_direct.h>
#include <supp/dag_comPtr.h>

#include <xess_sdk/inc/xess/xess_d3d12.h>
#include <xess_sdk/inc/xess/xess_debug.h>

#include <xess_sdk/inc/xell/xell_d3d12.h>

#include <xess_sdk/inc/xess_fg/xefg_swapchain_d3d12.h>
#include <xess_sdk/inc/xess_fg/xefg_swapchain_debug.h>

#include <type_traits>

#include <3d/gpuLatency.h>
#include <util/dag_finally.h>

static constexpr auto xefg_init_flags = XEFG_SWAPCHAIN_INIT_FLAG_INVERTED_DEPTH;

using namespace drv3d_dx12;

void get_fg_initializers(DXGIFactory *&factory, DXGISwapChain *&swapchain, ID3D12CommandQueue *&graphicsQueue);
void shutdown_internal_swapchain();
bool adopt_external_swapchain(DXGISwapChain *swapchain);
void create_default_swapchain();


static const char *get_xess_result_as_string(xess_result_t result)
{
  switch (result)
  {
#define CASE(x) \
  case xess_result_t::x: return #x
    CASE(XESS_RESULT_SUCCESS);
    CASE(XESS_RESULT_ERROR_UNSUPPORTED_DEVICE);
    CASE(XESS_RESULT_ERROR_UNSUPPORTED_DRIVER);
    CASE(XESS_RESULT_ERROR_UNINITIALIZED);
    CASE(XESS_RESULT_ERROR_INVALID_ARGUMENT);
    CASE(XESS_RESULT_ERROR_DEVICE_OUT_OF_MEMORY);
    CASE(XESS_RESULT_ERROR_DEVICE);
    CASE(XESS_RESULT_ERROR_NOT_IMPLEMENTED);
    CASE(XESS_RESULT_ERROR_INVALID_CONTEXT);
    CASE(XESS_RESULT_ERROR_OPERATION_IN_PROGRESS);
    CASE(XESS_RESULT_ERROR_UNSUPPORTED);
    CASE(XESS_RESULT_ERROR_CANT_LOAD_LIBRARY);
    default: CASE(XESS_RESULT_ERROR_UNKNOWN);
#undef CASE
  }
}

static XessWrapper::ErrorKind toErrorKind(xess_result_t result)
{
  switch (result)
  {
    case XESS_RESULT_SUCCESS: return XessWrapper::ErrorKind::Unknown; // not an error
    case XESS_RESULT_ERROR_UNSUPPORTED_DEVICE: return XessWrapper::ErrorKind::UnsupportedDevice;
    case XESS_RESULT_ERROR_UNSUPPORTED_DRIVER: return XessWrapper::ErrorKind::UnsupportedDriver;
    case XESS_RESULT_ERROR_UNINITIALIZED: return XessWrapper::ErrorKind::Uninitialized;
    case XESS_RESULT_ERROR_INVALID_ARGUMENT: return XessWrapper::ErrorKind::InvalidArgument;
    case XESS_RESULT_ERROR_DEVICE_OUT_OF_MEMORY: return XessWrapper::ErrorKind::DeviceOutOfMemory;
    case XESS_RESULT_ERROR_DEVICE: return XessWrapper::ErrorKind::Device;
    case XESS_RESULT_ERROR_NOT_IMPLEMENTED: return XessWrapper::ErrorKind::NotImplemented;
    case XESS_RESULT_ERROR_INVALID_CONTEXT: return XessWrapper::ErrorKind::InvalidContext;
    case XESS_RESULT_ERROR_OPERATION_IN_PROGRESS: return XessWrapper::ErrorKind::OperationInProgress;
    case XESS_RESULT_ERROR_UNSUPPORTED: return XessWrapper::ErrorKind::Unsupported;
    case XESS_RESULT_ERROR_CANT_LOAD_LIBRARY: return XessWrapper::ErrorKind::CantLoadLibrary;
    default: return XessWrapper::ErrorKind::Unknown;
  }
}

eastl::string XessWrapper::errorKindToString(ErrorKind kind)
{
  switch (kind)
  {
    case ErrorKind::UnsupportedDevice: return "Unsupported Device";
    case ErrorKind::UnsupportedDriver: return "Unsupported Driver";
    case ErrorKind::Uninitialized: return "Uninitialized";
    case ErrorKind::InvalidArgument: return "Invalid Argument";
    case ErrorKind::DeviceOutOfMemory: return "Device Out Of Memory";
    case ErrorKind::Device: return "Device error";
    case ErrorKind::NotImplemented: return "Not Implemented";
    case ErrorKind::InvalidContext: return "Invalid Context";
    case ErrorKind::OperationInProgress: return "Operation In Progress";
    case ErrorKind::Unsupported: return "Unsupported";
    case ErrorKind::CantLoadLibrary: return "Can't Load Library";
    default: return "Unknown";
  }
}

namespace drv3d_dx12
{

static xess_quality_settings_t toXeSSQuality(int quality)
{
  static constexpr xess_quality_settings_t xessQualities[] = {XESS_QUALITY_SETTING_PERFORMANCE, XESS_QUALITY_SETTING_BALANCED,
    XESS_QUALITY_SETTING_QUALITY, XESS_QUALITY_SETTING_ULTRA_QUALITY, XESS_QUALITY_SETTING_ULTRA_QUALITY_PLUS, XESS_QUALITY_SETTING_AA,
    XESS_QUALITY_SETTING_ULTRA_PERFORMANCE};

  G_ASSERT(quality < eastl::extent<decltype(xessQualities)>::value);
  return xessQualities[quality];
}

class XessWrapperImpl
{
public:
  bool loadLibrary()
  {
    libxess.reset(os_dll_load("libxess.dll"));
    if (!libxess)
    {
      logdbg("DX12: XeSS: Failed to load libxess.dll, xess will be disabled");
      return false;
    }

#define LOAD_FUNC(x)                                                    \
  do                                                                    \
  {                                                                     \
    x = static_cast<decltype(x)>(os_dll_get_symbol(libxess.get(), #x)); \
    if (x == nullptr)                                                   \
      return false;                                                     \
  } while (0)

    LOAD_FUNC(xessD3D12CreateContext); //-V516
    LOAD_FUNC(xessGetProperties);      //-V516
    LOAD_FUNC(xessD3D12Init);          //-V516
    LOAD_FUNC(xessGetInputResolution); //-V516
    LOAD_FUNC(xessDestroyContext);     //-V516
    LOAD_FUNC(xessD3D12Execute);       //-V516
    LOAD_FUNC(xessSetVelocityScale);   //-V516
    LOAD_FUNC(xessStartDump);          //-V516
    LOAD_FUNC(xessGetVersion);         //-V516
#undef LOAD_FUNC

    libxess_fg.reset(os_dll_load("libxess_fg.dll"));
    if (!libxess_fg)
    {
      logdbg("DX12: XeSS_FG: Failed to load libxess_fg.dll, xess_fg will be disabled");
    }
    else
    {

#define LOAD_FUNC(x)                                                       \
  do                                                                       \
  {                                                                        \
    x = static_cast<decltype(x)>(os_dll_get_symbol(libxess_fg.get(), #x)); \
    if (x == nullptr)                                                      \
      return false;                                                        \
  } while (0)

      LOAD_FUNC(xefgSwapChainD3D12CreateContext);         //-V516
      LOAD_FUNC(xefgSwapChainD3D12InitFromSwapChainDesc); //-V516
      LOAD_FUNC(xefgSwapChainD3D12GetSwapChainPtr);       //-V516
      LOAD_FUNC(xefgSwapChainD3D12TagFrameResource);      //-V516
      LOAD_FUNC(xefgSwapChainTagFrameConstants);          //-V516
      LOAD_FUNC(xefgSwapChainSetEnabled);                 //-V516
      LOAD_FUNC(xefgSwapChainSetPresentId);               //-V516
      LOAD_FUNC(xefgSwapChainGetLastPresentStatus);       //-V516
      LOAD_FUNC(xefgSwapChainSetLoggingCallback);         //-V516
      LOAD_FUNC(xefgSwapChainDestroy);                    //-V516
      LOAD_FUNC(xefgSwapChainSetLatencyReduction);        //-V516
      LOAD_FUNC(xefgSwapChainSetSceneChangeThreshold);    //-V516
      LOAD_FUNC(xefgSwapChainEnableDebugFeature);         //-V516
      LOAD_FUNC(xefgSwapChainD3D12BuildPipelines);        //-V516
#undef LOAD_FUNC
    }

    return true;
  }

  void makeFgContext()
  {
    auto result = xefgSwapChainD3D12CreateContext(static_cast<ID3D12Device *>(d3d::get_device()), &m_xefgContext);
    if (result < XEFG_SWAPCHAIN_RESULT_SUCCESS)
      logdbg("[XeFG] xefgSwapChainD3D12CreateContext failed with %d", result);
    else if (result > XEFG_SWAPCHAIN_RESULT_SUCCESS)
      logwarn("[XeFG] xefgSwapChainD3D12CreateContext succeeded with warning %d", result);

    if (result >= XEFG_SWAPCHAIN_RESULT_SUCCESS)
    {
      G_VERIFY(xefgSwapChainD3D12BuildPipelines(m_xefgContext, nullptr, false, xefg_init_flags) == XEFG_SWAPCHAIN_RESULT_SUCCESS);

#if DAGOR_DBGLEVEL < 1
      auto logLevel = XEFG_SWAPCHAIN_LOGGING_LEVEL_ERROR;
#else
      auto logLevel = XEFG_SWAPCHAIN_LOGGING_LEVEL_WARNING;
#endif

      xefgSwapChainSetLoggingCallback(
        m_xefgContext, logLevel,
        [](const char *message, xefg_swapchain_logging_level_t level, void *) {
          switch (level)
          {
            case XEFG_SWAPCHAIN_LOGGING_LEVEL_DEBUG: logdbg("[XEFG] %s", message); break;
            case XEFG_SWAPCHAIN_LOGGING_LEVEL_INFO: logdbg("[XEFG] %s", message); break;
            case XEFG_SWAPCHAIN_LOGGING_LEVEL_WARNING: logwarn("[XEFG] %s", message); break;
            case XEFG_SWAPCHAIN_LOGGING_LEVEL_ERROR: logerr("[XEFG] %s", message); break;
          }
        },
        nullptr);
    }

    lastPresentStatus = xefg_swapchain_present_status_t{};
  }

  bool init(void *device)
  {
    if (!loadLibrary() || !checkResult(xessD3D12CreateContext(static_cast<ID3D12Device *>(device), &m_xessContext)))
      return false;

    m_state = XessState::SUPPORTED;

    if (libxess_fg)
      makeFgContext();

    return true;
  }

  bool createFeature(int quality, uint32_t target_width, uint32_t target_height)
  {
    xess_properties_t props;
    memset(&props, 0, sizeof(props));
    m_desiredOutputResolution.x = target_width;
    m_desiredOutputResolution.y = target_height;
    if (!checkResult(xessGetProperties(m_xessContext, &m_desiredOutputResolution, &props)))
      return false;

    xess_d3d12_init_params_t initParms;
    memset(&initParms, 0, sizeof(initParms));
    initParms.outputResolution = m_desiredOutputResolution;
    initParms.qualitySetting = toXeSSQuality(quality);
    initParms.initFlags = xess_init_flags_t::XESS_INIT_FLAG_INVERTED_DEPTH;

    if (!checkResult(xessD3D12Init(m_xessContext, &initParms)))
      return false;

    // Get optimal input resolution
    if (!checkResult(xessGetInputResolution(m_xessContext, &m_desiredOutputResolution, initParms.qualitySetting, &m_renderResolution)))
      return false;

    m_state = XessState::READY;
    return true;
  }

  bool shutdownFg()
  {
    if (m_xefgContext)
    {
      if (fgActive)
      {
        d3d::driver_command(Drv3dCommand::ACQUIRE_OWNERSHIP);
        FINALLY([] { d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP); });

        // Reset the swapchain from the backend. Don't set a default one just yet, as the XeFG
        // swapchain still exist and there can be only one with flip model.
        shutdown_internal_swapchain();
        create_default_swapchain();
      }

      auto result = xefgSwapChainDestroy(m_xefgContext);
      G_ASSERT_RETURN(result == XEFG_SWAPCHAIN_RESULT_SUCCESS, false);
      m_xefgContext = nullptr;

      if (fgActive)
      {
        lastPresentStatus = xefg_swapchain_present_status_t{};

        // Reset XeLL to the original state.
        auto xell = GpuLatency::getInstance();
        G_ASSERT(xell);
        xell->setOptions(xellBackupState ? GpuLatency::Mode::On : GpuLatency::Mode::Off, 0);
      }

      fgActive = false;
    }

    return true;
  }

  bool shutdown()
  {
    return shutdownFg() && (m_state == XessState::READY || m_state == XessState::SUPPORTED) &&
           xessDestroyContext(m_xessContext) == XESS_RESULT_SUCCESS;
  }

  xess_result_t evaluateXess(void *context, const void *params)
  {
    const XessParamsDx12 &xessParams = *static_cast<const XessParamsDx12 *>(params);

    G_ASSERT_RETURN(xessParams.inColor != nullptr, XESS_RESULT_ERROR_INVALID_ARGUMENT);
    G_ASSERT_RETURN(xessParams.inDepth != nullptr, XESS_RESULT_ERROR_INVALID_ARGUMENT);
    G_ASSERT_RETURN(xessParams.inMotionVectors != nullptr, XESS_RESULT_ERROR_INVALID_ARGUMENT);
    G_ASSERT_RETURN(xessParams.outColor != nullptr, XESS_RESULT_ERROR_INVALID_ARGUMENT);
    G_ASSERTF_RETURN(abs(xessParams.inJitterOffsetX) <= 0.5f && abs(xessParams.inJitterOffsetY) <= 0.5f,
      XESS_RESULT_ERROR_INVALID_ARGUMENT,
      "Invalid parameters values in xessParams. Jitter values should be in range "
      "[-0.5, 0.5], but they are: x:%f, y:%f",
      xessParams.inJitterOffsetX, xessParams.inJitterOffsetY);

    ID3D12GraphicsCommandList *commandList = static_cast<ID3D12GraphicsCommandList *>(context);
    xess_d3d12_execute_params_t exec_params{};
    exec_params.inputWidth = xessParams.inInputWidth;
    exec_params.inputHeight = xessParams.inInputHeight;
    exec_params.jitterOffsetX = xessParams.inJitterOffsetX;
    exec_params.jitterOffsetY = xessParams.inJitterOffsetY;
    exec_params.exposureScale = 1.0f;

    exec_params.pColorTexture = xessParams.inColor->getHandle();
    exec_params.pVelocityTexture = xessParams.inMotionVectors->getHandle();
    exec_params.pOutputTexture = xessParams.outColor->getHandle();
    exec_params.pDepthTexture = xessParams.inDepth->getHandle();

    exec_params.inputColorBase.x = xessParams.inColorDepthOffsetX;
    exec_params.inputColorBase.y = xessParams.inColorDepthOffsetY;
    exec_params.inputDepthBase.x = xessParams.inColorDepthOffsetX;
    exec_params.inputDepthBase.y = xessParams.inColorDepthOffsetY;

    return xessD3D12Execute(m_xessContext, commandList, &exec_params);
  }

  bool evaluate(void *context, const void *params)
  {
    G_ASSERT_RETURN(m_state == XessState::READY, false);

    xess_result_t result = evaluateXess(context, params);
    if (result != xess_result_t::XESS_RESULT_SUCCESS)
    {
      D3D_ERROR("DX12: XeSS: Failed to evaluate XeSS: %s", get_xess_result_as_string(result));
      return false;
    }

    return true;
  }

  XessState getXessState() const { return m_state; }

  void getXeSSRenderResolution(int &w, int &h) const
  {
    w = m_renderResolution.x;
    h = m_renderResolution.y;
  }

  void setVelocityScale(float x, float y) { xessSetVelocityScale(m_xessContext, x, y); }

  bool isXessQualityAvailableAtResolution(uint32_t target_width, uint32_t target_height, int xess_quality) const
  {
    xess_2d_t targetResolution = {target_width, target_height};
    xess_2d_t inputResolution{};
    xess_result_t result = xessGetInputResolution(m_xessContext, &targetResolution, toXeSSQuality(xess_quality), &inputResolution);

    return result == XESS_RESULT_SUCCESS && inputResolution.x > 0 && inputResolution.y > 0;
  }

  void startDump(const char *path, int numberOfFrames)
  {
    if (dd_mkpath(path))
    {
      xess_dump_parameters_t dumpParameters = {};
      dumpParameters.path = path;
      dumpParameters.frame_count = numberOfFrames;
      xess_result_t result = xessStartDump(m_xessContext, &dumpParameters);
      if (result != XESS_RESULT_SUCCESS)
      {
        D3D_ERROR("DX12: Failed to create XeSS dump: %s", get_xess_result_as_string(result));
      }
    }
  }

  dag::Expected<eastl::string, XessWrapper::ErrorKind> getVersion()
  {
    if (xessGetVersion)
    {
      xess_version_t version;
      if (auto result = xessGetVersion(&version); result == XESS_RESULT_SUCCESS)
        return eastl::string(eastl::string::CtorSprintf(), "%d.%d.%d", version.major, version.minor, version.patch);
      else
        return dag::Unexpected(toErrorKind(result));
    }
    else
    {
      return dag::Unexpected(XessWrapper::ErrorKind::CantLoadLibrary);
    }
  }

  bool isFrameGenerationSupported() const { return m_xefgContext; }

  bool isFrameGenerationEnabled() const { return m_xefgContext && fgActive; }

  void enableFrameGeneration(bool enable)
  {
    if (enable)
    {
      if (fgActive)
        return;

      d3d::driver_command(Drv3dCommand::ACQUIRE_OWNERSHIP);
      FINALLY([] { d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP); });

      // Make the framegen swapchain and replace the driver one with it
      ComPtr<DXGIFactory> dxgiFactory;
      ComPtr<DXGISwapChain> dxgiSwapchain;
      ComPtr<ID3D12CommandQueue> d3dGraphicsQueue;
      get_fg_initializers(*dxgiFactory.GetAddressOf(), *dxgiSwapchain.GetAddressOf(), *d3dGraphicsQueue.GetAddressOf());

      DXGI_SWAP_CHAIN_DESC swapchainDesc = {};
      dxgiSwapchain->GetDesc(&swapchainDesc);
      HWND hwnd;
      dxgiSwapchain->GetHwnd(&hwnd);
      DXGI_SWAP_CHAIN_DESC1 desc1;
      dxgiSwapchain->GetDesc1(&desc1);
      DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc;
      dxgiSwapchain->GetFullscreenDesc(&fullscreenDesc);
      dxgiSwapchain.Reset();

      G_ASSERT_RETURN(fullscreenDesc.Windowed, );

      // XeLL need to be enabled
      xellBackupState = false;

      auto xell = GpuLatency::getInstance();
      if (!xell)
        xell = GpuLatency::create(GpuVendor::INTEL);

      G_ASSERT_RETURN(xell, );

      if (xell->isEnabled())
        xellBackupState = true;
      else
        xell->setOptions(GpuLatency::Mode::On, 0);

      xefgSwapChainSetLatencyReduction(m_xefgContext, xell->getHandle());

      // Remove the old swapchain first, because only one swapchain can exist for the window with flip model
      shutdown_internal_swapchain();

      xefg_swapchain_d3d12_init_params_t params = {
        .initFlags = xefg_init_flags,
        .maxInterpolatedFrames = 1,
        .uiMode = XEFG_SWAPCHAIN_UI_MODE_HUDLESS_UITEXTURE,
      };

      [[maybe_unused]] auto result = xefgSwapChainD3D12InitFromSwapChainDesc(m_xefgContext, hwnd, &desc1, &fullscreenDesc,
        d3dGraphicsQueue.Get(), dxgiFactory.Get(), &params);
      G_ASSERT_RETURN(result == XEFG_SWAPCHAIN_RESULT_SUCCESS, );

      ComPtr<IDXGISwapChain3> newDxgiSwapchain;
      result = xefgSwapChainD3D12GetSwapChainPtr(m_xefgContext, IID_PPV_ARGS(newDxgiSwapchain.GetAddressOf()));
      G_ASSERT(result == XEFG_SWAPCHAIN_RESULT_SUCCESS);

      if (!adopt_external_swapchain(newDxgiSwapchain.Get()))
      {
        // try to restore the default DXGI swapchain if XESS FG swapchain creation is failed
        create_default_swapchain();
        return;
      }

      result = xefgSwapChainSetEnabled(m_xefgContext, true);
      G_ASSERT(result == XEFG_SWAPCHAIN_RESULT_SUCCESS);

      fgActive = true;

#if 0
      xefgSwapChainEnableDebugFeature(m_xefgContext, XEFG_SWAPCHAIN_DEBUG_FEATURE_SHOW_ONLY_INTERPOLATION, true, nullptr);
      xefgSwapChainEnableDebugFeature(m_xefgContext, XEFG_SWAPCHAIN_DEBUG_FEATURE_PRESENT_FAILED_INTERPOLATION, true, nullptr);
#endif
    }
    else
    {
      if (m_xefgContext && fgActive)
      {
        shutdownFg();
        makeFgContext();
      }
    }
  }

  void suppressFrameGeneration(bool suppress)
  {
    if (m_xefgContext && fgActive)
    {
      auto result = xefgSwapChainSetEnabled(m_xefgContext, !suppress);
      G_UNUSED(result);
      G_ASSERT(result == XEFG_SWAPCHAIN_RESULT_SUCCESS);
    }
  }

  void doScheduleGeneratedFrames(const void *args, void *command_list)
  {
    xefgSwapChainGetLastPresentStatus(m_xefgContext, &lastPresentStatus);

    if (m_xefgContext && fgActive)
    {
      auto &fgArgs = *static_cast<const XessFgParamsDx12 *>(args);
      auto d3dCommandList = static_cast<ID3D12CommandList *>(command_list);

      xefg_swapchain_frame_constant_data_t constData = {};
      memcpy(constData.viewMatrix, fgArgs.viewTm, sizeof(fgArgs.viewTm));
      memcpy(constData.projectionMatrix, fgArgs.projTm, sizeof(fgArgs.projTm));
      constData.jitterOffsetX = fgArgs.inJitterOffsetX;
      constData.jitterOffsetY = fgArgs.inJitterOffsetY;
      constData.motionVectorScaleX = fgArgs.inMotionVectorScaleX;
      constData.motionVectorScaleY = fgArgs.inMotionVectorScaleY;
      constData.resetHistory = fgArgs.inReset;

      auto result = xefgSwapChainTagFrameConstants(m_xefgContext, fgArgs.inFrameIndex, &constData);
      G_UNUSED(result);
      G_ASSERT(result == XEFG_SWAPCHAIN_RESULT_SUCCESS);

      auto tagResource = [&](Image *img, xefg_swapchain_resource_type_t type) {
        auto desc = img->getHandle()->GetDesc();

        xefg_swapchain_d3d12_resource_data_t resData = {};
        resData.type = type;
        resData.validity = XEFG_SWAPCHAIN_RV_ONLY_NOW;
        resData.resourceSize = {(uint32_t)desc.Width, (uint32_t)desc.Height};
        resData.pResource = img->getHandle();
        resData.incomingState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        if (type == XEFG_SWAPCHAIN_RES_DEPTH)
          resData.incomingState |= D3D12_RESOURCE_STATE_DEPTH_READ;
        auto result = xefgSwapChainD3D12TagFrameResource(m_xefgContext, d3dCommandList, fgArgs.inFrameIndex, &resData);
        G_UNUSED(result);
        G_ASSERT(result == XEFG_SWAPCHAIN_RESULT_SUCCESS);
      };

      tagResource(fgArgs.inColorHudless, XEFG_SWAPCHAIN_RES_HUDLESS_COLOR);
      tagResource(fgArgs.inMotionVectors, XEFG_SWAPCHAIN_RES_MOTION_VECTOR);
      tagResource(fgArgs.inDepth, XEFG_SWAPCHAIN_RES_DEPTH);
      tagResource(fgArgs.inUi, XEFG_SWAPCHAIN_RES_UI);

      result = xefgSwapChainSetPresentId(m_xefgContext, fgArgs.inFrameIndex);
      G_ASSERT(result == XEFG_SWAPCHAIN_RESULT_SUCCESS);
    }
  }

  int getPresentedFrameCount() { return lastPresentStatus.isFrameGenEnabled ? lastPresentStatus.framesPresented : 1; }

private:
  bool checkResult(xess_result_t result)
  {
    if (result != XESS_RESULT_SUCCESS)
    {
      logwarn("DX12: XeSS could not initialize: %s", get_xess_result_as_string(result));
    }

    switch (result)
    {
      case XESS_RESULT_ERROR_UNSUPPORTED_DEVICE: m_state = XessState::UNSUPPORTED_DEVICE; break;
      case XESS_RESULT_ERROR_UNSUPPORTED_DRIVER: m_state = XessState::UNSUPPORTED_DRIVER; break;
      case XESS_RESULT_SUCCESS: return true;
      default: m_state = XessState::INIT_ERROR_UNKNOWN;
    }
    return false;
  }

  xess_context_handle_t m_xessContext = nullptr;
  xess_2d_t m_desiredOutputResolution = {0, 0};
  xess_2d_t m_renderResolution = {0, 0};
  XessState m_state = XessState::DISABLED;

  xefg_swapchain_handle_t m_xefgContext = nullptr;
  xefg_swapchain_present_status_t lastPresentStatus = {};
  bool fgActive = false;

  bool xellBackupState = false;

  DagorDynLibHolder libxess;
  DagorDynLibHolder libxess_fg;
  decltype(::xessD3D12CreateContext) *xessD3D12CreateContext = nullptr;
  decltype(::xessGetProperties) *xessGetProperties = nullptr;
  decltype(::xessD3D12Init) *xessD3D12Init = nullptr;
  decltype(::xessGetInputResolution) *xessGetInputResolution = nullptr;
  decltype(::xessDestroyContext) *xessDestroyContext = nullptr;
  decltype(::xessD3D12Execute) *xessD3D12Execute = nullptr;
  decltype(::xessSetVelocityScale) *xessSetVelocityScale = nullptr;
  decltype(::xessStartDump) *xessStartDump = nullptr;
  decltype(::xessGetVersion) *xessGetVersion = nullptr;

  decltype(::xefgSwapChainD3D12CreateContext) *xefgSwapChainD3D12CreateContext = nullptr;
  decltype(::xefgSwapChainD3D12InitFromSwapChainDesc) *xefgSwapChainD3D12InitFromSwapChainDesc = nullptr;
  decltype(::xefgSwapChainD3D12GetSwapChainPtr) *xefgSwapChainD3D12GetSwapChainPtr = nullptr;
  decltype(::xefgSwapChainD3D12TagFrameResource) *xefgSwapChainD3D12TagFrameResource = nullptr;
  decltype(::xefgSwapChainTagFrameConstants) *xefgSwapChainTagFrameConstants = nullptr;
  decltype(::xefgSwapChainSetEnabled) *xefgSwapChainSetEnabled = nullptr;
  decltype(::xefgSwapChainSetPresentId) *xefgSwapChainSetPresentId = nullptr;
  decltype(::xefgSwapChainGetLastPresentStatus) *xefgSwapChainGetLastPresentStatus = nullptr;
  decltype(::xefgSwapChainSetLoggingCallback) *xefgSwapChainSetLoggingCallback = nullptr;
  decltype(::xefgSwapChainDestroy) *xefgSwapChainDestroy = nullptr;
  decltype(::xefgSwapChainSetLatencyReduction) *xefgSwapChainSetLatencyReduction = nullptr;
  decltype(::xefgSwapChainSetSceneChangeThreshold) *xefgSwapChainSetSceneChangeThreshold = nullptr;
  decltype(::xefgSwapChainEnableDebugFeature) *xefgSwapChainEnableDebugFeature = nullptr;
  decltype(::xefgSwapChainD3D12BuildPipelines) *xefgSwapChainD3D12BuildPipelines = nullptr;
};

} // namespace drv3d_dx12

#if DAGOR_DBGLEVEL > 0
XessWrapper *debugActiveWrapper = nullptr;
#endif

XessWrapper::XessWrapper() : pImpl(new XessWrapperImpl)
{
#if DAGOR_DBGLEVEL > 0
  debugActiveWrapper = this;
#endif
}

XessWrapper::~XessWrapper() = default;

bool XessWrapper::xessInit(void *device) { return pImpl->init(device); }

bool XessWrapper::xessCreateFeature(int quality, uint32_t target_width, uint32_t target_height)
{
  return pImpl->createFeature(quality, target_width, target_height);
}

bool XessWrapper::xessShutdown() { return pImpl->shutdown(); }

bool XessWrapper::evaluateXess(void *context, const void *params) { return pImpl->evaluate(context, params); }

void XessWrapper::setVelocityScale(float x, float y) { return pImpl->setVelocityScale(x, y); }

bool XessWrapper::isXessQualityAvailableAtResolution(uint32_t target_width, uint32_t target_height, int xess_quality) const
{
  return pImpl->isXessQualityAvailableAtResolution(target_width, target_height, xess_quality);
}

bool XessWrapper::isFrameGenerationSupported() const { return pImpl->isFrameGenerationSupported(); }

bool XessWrapper::isFrameGenerationEnabled() const { return pImpl->isFrameGenerationEnabled(); }

void XessWrapper::enableFrameGeneration(bool enable) { pImpl->enableFrameGeneration(enable); }

void XessWrapper::suppressFrameGeneration(bool suppress) { pImpl->suppressFrameGeneration(suppress); }

void XessWrapper::doScheduleGeneratedFrames(const void *args, void *command_list)
{
  pImpl->doScheduleGeneratedFrames(args, command_list);
}

int XessWrapper::getPresentedFrameCount() { return pImpl->getPresentedFrameCount(); }

XessState XessWrapper::getXessState() const { return pImpl->getXessState(); }

void XessWrapper::getXeSSRenderResolution(int &w, int &h) const { pImpl->getXeSSRenderResolution(w, h); }

void XessWrapper::startDump(const char *path, int numberOfFrames) { pImpl->startDump(path, numberOfFrames); }

dag::Expected<eastl::string, XessWrapper::ErrorKind> XessWrapper::getVersion() const { return pImpl->getVersion(); }

#if DAGOR_DBGLEVEL > 0

#include <gui/dag_imgui.h>
#include <imgui/imgui.h>

static void xess_debug_imgui()
{
  static char dumpPath[DAGOR_MAX_PATH] = "xessDump";
  ImGui::InputText("Dump path", dumpPath, IM_ARRAYSIZE(dumpPath));

  static int framesToDump = 1;
  ImGui::SliderInt("Number of frames to dump", &framesToDump, 1, 10);

  if (ImGui::Button("Start") && debugActiveWrapper)
  {
    debugActiveWrapper->startDump(dumpPath, framesToDump);
  }
}

REGISTER_IMGUI_WINDOW("Render", "XeSS debug", xess_debug_imgui);

#endif

class IntelGpuLatency : public GpuLatency
{
public:
  IntelGpuLatency()
  {
    G_ASSERT(d3d::get_driver_code().is(d3d::dx12));

    if (!loadLibrary())
      return;

    auto result = xellD3D12CreateContext((ID3D12Device *)d3d::get_device(), &handle);
    if (result == XELL_RESULT_SUCCESS)
    {
      modes = {Mode::Off, Mode::On};

#if DAGOR_DBGLEVEL < 1
      auto logLevel = XELL_LOGGING_LEVEL_ERROR;
#else
      auto logLevel = XELL_LOGGING_LEVEL_WARNING;
#endif

      xellSetLoggingCallback(handle, logLevel, [](const char *message, xell_logging_level_t level) {
        switch (level)
        {
          case XELL_LOGGING_LEVEL_DEBUG: logdbg("[XELL] %s", message); break;
          case XELL_LOGGING_LEVEL_INFO: logdbg("[XELL] %s", message); break;
          case XELL_LOGGING_LEVEL_WARNING: logwarn("[XELL] %s", message); break;
          case XELL_LOGGING_LEVEL_ERROR: logerr("[XELL] %s", message); break;
        }
      });
    }
    else
    {
      switch (result)
      {
        case XELL_RESULT_ERROR_UNSUPPORTED_DEVICE:
          logwarn("XeLL initialization failed with XELL_RESULT_ERROR_UNSUPPORTED_DEVICE");
          break;
        case XELL_RESULT_ERROR_UNSUPPORTED_DRIVER:
          logwarn("XeLL initialization failed with XELL_RESULT_ERROR_UNSUPPORTED_DRIVER");
          break;
        default: logerr("XeLL initialization failed with %d", result); break;
      }
      modes = {Mode::Off};
    }
  }

  ~IntelGpuLatency()
  {
    if (handle)
      xellDestroyContext(handle);
  }

  dag::ConstSpan<Mode> getAvailableModes() const override { return make_span_const(modes); }

  void startFrame(uint32_t) override {}

  void setMarker(uint32_t frame_id, lowlatency::LatencyMarkerType marker_type) override
  {
    if (!handle)
      return;

    switch (marker_type)
    {
      case lowlatency::LatencyMarkerType::SIMULATION_START: xellAddMarkerData(handle, frame_id, XELL_SIMULATION_START); break;
      case lowlatency::LatencyMarkerType::SIMULATION_END: xellAddMarkerData(handle, frame_id, XELL_SIMULATION_END); break;
      case lowlatency::LatencyMarkerType::RENDERSUBMIT_START: xellAddMarkerData(handle, frame_id, XELL_RENDERSUBMIT_START); break;
      case lowlatency::LatencyMarkerType::RENDERSUBMIT_END: xellAddMarkerData(handle, frame_id, XELL_RENDERSUBMIT_END); break;
      case lowlatency::LatencyMarkerType::PRESENT_START: xellAddMarkerData(handle, frame_id, XELL_PRESENT_START); break;
      case lowlatency::LatencyMarkerType::PRESENT_END: xellAddMarkerData(handle, frame_id, XELL_PRESENT_END); break;
      default: break;
    }
  }

  void setOptions(Mode mode, uint32_t frame_limit_us) override
  {
    if (!handle)
      return;

    xell_sleep_params_t params = {};
    params.minimumIntervalUs = frame_limit_us;
    switch (mode)
    {
      case Mode::Off: params.bLowLatencyMode = 0; break;
      case Mode::On:
      case Mode::OnPlusBoost: params.bLowLatencyMode = 1; break;
    }

    // params.bLowLatencyBoost can be set once it becomes available.

    xellSetSleepMode(handle, &params);
  }

  void sleep(uint32_t frame_id) override
  {
    if (!handle)
      return;

    xellSleep(handle, frame_id);
  }

  lowlatency::LatencyData getStatisticsSince(uint32_t frame_id, uint32_t max_count = 0) override
  {
    // Anti Lag 2 has no support for this
    G_UNUSED(frame_id);
    G_UNUSED(max_count);
    lowlatency::LatencyData ret;
    return ret;
  }

  void *getHandle() const override { return handle; }

  bool isEnabled() const override
  {
    if (!handle)
      return false;

    xell_sleep_params_t params = {};
    if (xellGetSleepMode(handle, &params) != XELL_RESULT_SUCCESS)
      return false;

    return params.bLowLatencyMode == 1;
  }

  GpuVendor getVendor() const override { return GpuVendor::INTEL; }

  bool isVsyncAllowed() const override { return true; }

private:
  bool loadLibrary()
  {
    xellD3D12CreateContext = nullptr;
    xellDestroyContext = nullptr;
    xellAddMarkerData = nullptr;
    xellSetSleepMode = nullptr;
    xellSleep = nullptr;
    xellGetSleepMode = nullptr;
    xellSetLoggingCallback = nullptr;

    libxell.reset(os_dll_load("libxell.dll"));
    if (!libxell)
    {
      logdbg("DX12: XeLL: Failed to load libxell.dll, xell will be disabled");
      return false;
    }

#define LOAD_FUNC(x)                                                    \
  do                                                                    \
  {                                                                     \
    x = static_cast<decltype(x)>(os_dll_get_symbol(libxell.get(), #x)); \
    if (x == nullptr)                                                   \
      return false;                                                     \
  } while (0)

    LOAD_FUNC(xellD3D12CreateContext); //-V516
    LOAD_FUNC(xellDestroyContext);     //-V516
    LOAD_FUNC(xellAddMarkerData);      //-V516
    LOAD_FUNC(xellSetSleepMode);       //-V516
    LOAD_FUNC(xellSleep);              //-V516
    LOAD_FUNC(xellGetSleepMode);       //-V516
    LOAD_FUNC(xellSetLoggingCallback); //-V516
#undef LOAD_FUNC

    return true;
  }

  dag::Vector<Mode> modes = {Mode::Off};
  xell_context_handle_t handle = nullptr;

  DagorDynLibHolder libxell;
  decltype(::xellD3D12CreateContext) *xellD3D12CreateContext = nullptr;
  decltype(::xellDestroyContext) *xellDestroyContext = nullptr;
  decltype(::xellAddMarkerData) *xellAddMarkerData = nullptr;
  decltype(::xellSetSleepMode) *xellSetSleepMode = nullptr;
  decltype(::xellSleep) *xellSleep = nullptr;
  decltype(::xellGetSleepMode) *xellGetSleepMode = nullptr;
  decltype(::xellSetLoggingCallback) *xellSetLoggingCallback = nullptr;
};

GpuLatency *create_gpu_latency_intel() { return new IntelGpuLatency(); }
