//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <ioSys/dag_dataBlock.h>
#include <math/dag_Point3.h>
#include <math/dag_Quat.h>
#include <math/dag_mathUtils.h>
#include <math/dag_TMatrix4D.h>
#include <math/integer/dag_IPoint2.h>
#include <drv/3d/dag_decl.h>
#include <shaders/dag_overrideStateId.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/array.h>
#include <EASTL/vector.h>
#include <3d/dag_resPtr.h>
#include <osApiWrappers/dag_threads.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <3d/dag_stereoIndex.h>
#include <drv/hid/dag_hiVrInput.h>

class TMatrix4D;
struct Driver3dPerspective;
class DataBlock;

class PostFxRenderer;

struct VRDevice
{
  enum class RenderingAPI
  {
    Default,
    D3D11,
    D3D12,
    Vulkan,
  };

  enum class State
  {
    Uninitialized,
    Idle,
    Ready,
    Synchronized,
    Visible,
    Focused,
    Stopping,
    Loosing,
    Exiting,
  };

  enum class SpaceType
  {
    Stage,
    Local,
  };

  enum class MirrorMode
  {
    Left,
    Right,
    Both,
  };

  enum class StereoMode
  {
    Multipass,
    SideBySideHorizontal,
    SideBySideVertical,
  };

  struct ApplicationData
  {
    const char *name;
    uint32_t version;
  };

  struct FrameData
  {
    struct ViewData
    {
      float fovLeft, fovRight, fovUp, fovDown;
      Quat orientation;
      DPoint3 position;

      Driver3dPerspective projection;
      TMatrix4D cameraTransform;
      TMatrix4D viewTransform;
    };

    ViewData views[2];
    ViewData boundingView;

    ManagedTexView frameTargets[2];
    ManagedTexView frameDepths[2];

    uint64_t frameTargetSwapchainHandles[2];
    uint64_t frameDepthSwapchainHandles[2];

    RectInt frameRects[2];
    IPoint2 frameOffsets[2];

    bool depthFilled;

    float nearZ;
    float farZ;

    float zoom;

    int64_t displayTime;

    bool frameStartedSuccessfully;

    uint64_t frameId;
    int backBufferIndex;
  };

  using AdapterID = int64_t;

  enum class Session
  {
    Start,
    End
  };
  using SessionCallback = eastl::function<bool(Session)>;

  static void create(RenderingAPI rendering_api, const ApplicationData &application_data);
  static VRDevice *getInstance();
  static VRDevice *getInstanceIfActive();
  static void deleteInstance();
  static bool hasActiveSession();

  VRDevice();
  virtual ~VRDevice();

  virtual RenderingAPI getRenderingAPI() const = 0;
  virtual AdapterID getAdapterId() const = 0;
  virtual const char *getRequiredDeviceExtensions() const = 0;
  virtual const char *getRequiredInstanceExtensions() const = 0;
  virtual eastl::pair<uint64_t, uint64_t> getRequiredGraphicsAPIRange() const = 0;

  virtual void getViewResolution(int &width, int &height) const = 0;
  virtual float getViewAspect() const = 0;

  virtual bool isHDR() const { return false; }

  virtual bool isActive() const = 0;

  virtual void setZoom(float zoom) = 0;

  bool setRenderingDevice();

  virtual void setUpSpace() = 0;

  virtual bool isReadyForRender() = 0;

  virtual void tick(const SessionCallback &cb) = 0;

  template <typename CB>
  void tick(const CB &cb)
  {
    SessionCallback cbfn(cb);
    tick(cbfn);
  }

  virtual bool prepareFrame(FrameData &frameData, float zNear, float zFar) = 0;
  virtual void beginFrame(FrameData &frameData) = 0;

  virtual bool hasQuitRequest() const = 0;

  virtual bool isConfigurationNeeded() const { return false; }

  virtual bool hasScreenMask() = 0;

  void prepareVrsMask(FrameData &frameData);
  void enableVrsMask(StereoIndex stereo_index, bool combined);
  void disableVrsMask();
  const ManagedTex *getVrsMask(StereoIndex stereo_index, bool combined) const;

  virtual void beforeSoftDeviceReset() = 0;
  virtual void afterSoftDeviceReset() = 0;

  virtual bool isInStateTransition() const = 0;

  virtual eastl::vector<float> getSupportedRefreshRates() const { return {}; }
  virtual bool setRefreshRate(float rate) const
  {
    G_UNUSED(rate);
    return false;
  }
  virtual float getRefreshRate() const { return 0; }

  virtual bool isEmulated() const { return false; }

  virtual bool isMirrorDisabled() const { return false; }

  virtual void enableAutoMirroring(bool enable) { G_UNUSED(enable); }

  bool renderScreenMask(const TMatrix4 &projection, int view_index, float scale = 1, int value = 0);
  void forceDisableScreenMask(bool disable);

  void setMirrorMode(MirrorMode mode);
  void renderMirror(FrameData &frame);

  StereoMode getStereoMode() const { return stereoMode; }

  using StateChangeListerer = void (*)(State);
  State setStateChangeListener(StateChangeListerer listener);
  State getCurrentState() const { return currentState; }

  virtual void setStereoIndex(StereoIndex index) { G_UNUSED(index); };

  static float calcBoundingView(FrameData &frameData);

  static void fovValuesToDriverPerspective(float fovLeft, float fovRight, float fovUp, float fovDown, float zNear, float zFar,
    float zoom, Driver3dPerspective &result);
  static void modifyCameraTransform(const Quat &orientationOffset, const Point3 &positionOffset, TMatrix4D &cameraTransform);

  static MirrorMode getMirrorModeFromSettings();
  static bool getForceDisableScreenMaskFromSettings();

  static void applyAllFromSettings();

  static void calcViewTransforms(VRDevice::FrameData::ViewData &view, float zNear, float zFar, float zoom);

  static bool shouldBeEnabled();
  static void setEnabled(bool enabled);

  static bool isPresenceSensorForcedToBeOn();

  using VrInput = HumanInput::VrInput;
  virtual VrInput *getInputHandler() = 0;
  static VrInput *getInput() { return getInstance() ? getInstance()->getInputHandler() : nullptr; }
  static VrInput *getInputIfUsable()
  {
    bool usable = getInstance() && getInstance()->getInputHandler() && getInstance()->getInputHandler()->isInUsableState();
    return usable ? getInstance()->getInputHandler() : nullptr;
  }
  static void setAndCallInputInitializationCallback(VrInput::InitializationCallback cb);

  static int getConfiguredRefreshRate();

  // Expecting normalized frequency in the range from 0.0 to 1.0
  virtual void setHmdRumble(float /* frequency */){/* VOID */};
  virtual void stopHmdRumble() { setHmdRumble(0.f); }

protected:
  virtual bool isInitialized() const = 0;

  virtual bool setRenderingDeviceImpl() = 0;

  virtual void beginRender(FrameData &frameData) = 0;
  virtual void endRender(FrameData &frameData) = 0;
  void tearDownScreenMaskResources();
  void prepareScreenMask(const TMatrix4 &projection, int view_index);
  virtual void retrieveScreenMaskTriangles(const TMatrix4 &projection, eastl::vector<Point4> &visibilityMaskVertices,
    eastl::vector<uint16_t> &visibilityMaskIndices, int view_index) = 0;

  eastl::unique_ptr<PostFxRenderer> mirrorRenderer;
  shaders::UniqueOverrideStateId mirrorState;
  MirrorMode mirrorMode = MirrorMode::Left;

  DynamicShaderHelper screenMaskRenderer;

  VDECL screenMaskVertexDeclaration = BAD_VDECL;
  int screenMaskVertexStride = -1;
  UniqueBuf screenMaskVertexBuffer[2];
  UniqueBuf screenMaskIndexBuffer[2];
  int screenMaskTriangleCount[2] = {0, 0};

  bool disableScreenMask = false;
  bool screenMaskAvailable = true;

  bool callSessionStartedCallbackIfAvailable = false;
  bool callSessionEndedCallbackIfAvailable = false;

  bool inHmdStateTransition = false;

  StereoMode stereoMode = StereoMode::Multipass;

  UniqueTex vrs_mask_textures[3];
  float current_vrs_inner_factor = -1;
  float current_vrs_outer_factor = -1;

  void changeState(State newState);

private:
  StateChangeListerer stateChangeListener = nullptr;
  State currentState = State::Uninitialized;
};
