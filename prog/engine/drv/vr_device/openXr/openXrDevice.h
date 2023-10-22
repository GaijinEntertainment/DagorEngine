#pragma once

#include "drivers/dag_vr.h"
#include "3d/dag_drv3dCmd.h"
#include "3d/dag_texMgr.h"
#include "osApiWrappers/dag_events.h"
#include "osApiWrappers/dag_threads.h"
#include "shaders/dag_DynamicShaderHelper.h"
#include "EASTL/vector.h"
#include "EASTL/list.h"
#include "EASTL/string.h"
#include <atomic>
#include <mutex>

#include "openXr.h"
#include "openXrInputHandler.h"

#if _TARGET_PC_WIN
#include "Windows.h"
#include "d3d11.h"
#include "d3d12.h"

#include "vulkan/vulkan.h"

#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11
#define XR_USE_GRAPHICS_API_D3D12
#define XR_USE_GRAPHICS_API_VULKAN
#endif // _TARGET_PC_WIN

#if _TARGET_ANDROID
#include <3d/dag_drv3dConsts.h>
#include <supp/dag_android_native_app_glue.h>

#include "vulkan/vulkan.h"

#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_VULKAN

extern void *win32_get_instance();
#endif // _TARGET_ANDROID
#include <openxr/openxr_platform.h>

static void vr_before_reset(bool);
static void vr_after_reset(bool);

class OpenXRDevice : public VRDevice, public DeviceResetEventHandler
{
  friend struct VRDevice;
  friend void vr_before_reset(bool);
  friend void vr_after_reset(bool);

public:
  OpenXRDevice(RenderingAPI rendering_api, const ApplicationData &application_data);
  ~OpenXRDevice() override;

  virtual RenderingAPI getRenderingAPI() const override;
  virtual AdapterID getAdapterId() const override;
  virtual const char *getRequiredDeviceExtensions() const override;
  virtual const char *getRequiredInstanceExtensions() const override;
  virtual eastl::pair<uint64_t, uint64_t> getRequiredGraphicsAPIRange() const override;

  virtual void getViewResolution(int &width, int &height) const override;
  virtual float getViewAspect() const override;

  virtual bool isActive() const override;

  virtual void setZoom(float zoom) override;

  virtual bool setRenderingDeviceImpl() override;

  virtual void setUpSpace() override;

  virtual bool isReadyForRender() override;

  virtual void tick(const SessionCallback &cb) override;

  virtual bool prepareFrame(FrameData &frameData, float zNear, float zFar) override;
  virtual void beginFrame(FrameData &frameData) override;

  virtual bool hasQuitRequest() const override;

  virtual VrInput *getInputHandler() override { return &inputHandler; }

  virtual void preRecovery() override;
  virtual void recovery() override;

  virtual bool hasScreenMask() override;
  virtual void retrieveScreenMaskTriangles(const TMatrix4 &projection, eastl::vector<Point4> &visibilityMaskVertices,
    eastl::vector<uint16_t> &visibilityMaskIndices, int view_index) override;

  virtual void beforeSoftDeviceReset() override;
  virtual void afterSoftDeviceReset() override;

  virtual bool isInStateTransition() const override;

  virtual eastl::vector<float> getSupportedRefreshRates() const override;
  virtual bool setRefreshRate(float rate) const override;
  virtual float getRefreshRate() const override;

  virtual bool isMirrorDisabled() const override;

protected:
  virtual bool isInitialized() const override;

  virtual void beginRender(FrameData &frameData) override;
  virtual void endRender(FrameData &frameData) override;

private:
  struct SwapchainData
  {
    XrSwapchain handle = 0;
    int32_t width = 0;
    int32_t height = 0;
    eastl::vector<UniqueTex> imageViews;

    eastl::unique_ptr<SwapchainData> depth;
  };

  bool setUpInstance();
  void tearDownInstance();

  bool setUpSession();
  void tearDownSession();

  bool setUpSwapchains();
  bool setUpSwapchain(SwapchainData &swapchain, int width, int height);
  void tearDownSwapchains();
  void tearDownSwapchain(SwapchainData &swapchain);

  bool setUpSwapchainImage(SwapchainData &swapchain, int swap_chain_index, bool depth);
  bool setUpSwapchainImages();
  void tearDownSwapchainImages();
  void tearDownSwapchainImages(SwapchainData &swapchain);

  bool isExtensionSupported(const char *name) const;

  void setUpReferenceSpace();
  void setUpApplicationSpace();

  bool recreateOpenXr();

  bool initialized = false;

  RenderingAPI renderingAPI;

  State currentState = State::Uninitialized;

  eastl::vector<XrExtensionProperties> supportedExtensions;
  eastl::vector<XrEnvironmentBlendMode> supportedBlendModes;

  XrViewConfigurationView configViews[2] = {{XR_TYPE_VIEW_CONFIGURATION_VIEW}, {XR_TYPE_VIEW_CONFIGURATION_VIEW}};

  eastl::vector<const char *> requiredExtensions;

  XrInstance oxrInstance = 0;
  XrSystemId oxrSystemId = 0;
  XrSession oxrSession = 0;
  XrSpace oxrRefSpace = 0;
  XrSpace oxrAppSpace = 0;

  XrSessionState oxrSessionState;

  SwapchainData swapchains[2];

  eastl::string oxrRequredDeviceExtensions;
  eastl::string oxrRequredInstanceExtensions;

  bool sessionRunning = false;
  bool apiRequestedQuit = false;

  uint64_t frameId = 0;

  bool shouldResetAppSpace = true;

  float zoom = 1;

  eastl::string applicationName;
  uint32_t applicationVersion;

  eastl::vector<int64_t> swapchainFormats;

  bool enableDepthLayerExtension = true;
  bool enableHandTracking = false;

  OpenXrInputHandler inputHandler;

  XrDebugUtilsMessengerEXT oxrDebug = 0;

  bool supportsChangingRefreshRate = false;

  mutable bool gfxApiRangeCalled = false;
};
