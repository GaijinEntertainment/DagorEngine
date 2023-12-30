#include "xess_wrapper.h"
#include "device.h"

#include <osApiWrappers/dag_dynLib.h>

#include <xess_sdk/inc/xess/xess_d3d12.h>

#include <type_traits>

using namespace drv3d_dx12;

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

namespace drv3d_dx12
{

static xess_quality_settings_t toXeSSQuality(int quality)
{
  static constexpr xess_quality_settings_t xessQualities[] = {
    XESS_QUALITY_SETTING_PERFORMANCE, XESS_QUALITY_SETTING_BALANCED, XESS_QUALITY_SETTING_QUALITY, XESS_QUALITY_SETTING_ULTRA_QUALITY};

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
#undef LOAD_FUNC

    return true;
  }

  bool init(void *device)
  {
    if (!loadLibrary() || !checkResult(xessD3D12CreateContext(static_cast<ID3D12Device *>(device), &m_xessContext)))
      return false;

    m_state = XessState::SUPPORTED;
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

  bool shutdown()
  {
    return (m_state == XessState::READY || m_state == XessState::SUPPORTED) &&
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
      logerr("DX12: XeSS: Failed to evaluate XeSS: %s", get_xess_result_as_string(result));
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

  eastl::unique_ptr<void, DagorDllCloser> libxess;
  decltype(::xessD3D12CreateContext) *xessD3D12CreateContext;
  decltype(::xessGetProperties) *xessGetProperties;
  decltype(::xessD3D12Init) *xessD3D12Init;
  decltype(::xessGetInputResolution) *xessGetInputResolution;
  decltype(::xessDestroyContext) *xessDestroyContext;
  decltype(::xessD3D12Execute) *xessD3D12Execute;
  decltype(::xessSetVelocityScale) *xessSetVelocityScale;
};

} // namespace drv3d_dx12

XessWrapper::XessWrapper() : pImpl(new XessWrapperImpl) {}

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

XessState XessWrapper::getXessState() const { return pImpl->getXessState(); }

void XessWrapper::getXeSSRenderResolution(int &w, int &h) const { pImpl->getXeSSRenderResolution(w, h); }
