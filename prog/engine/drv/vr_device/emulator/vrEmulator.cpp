// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "vrEmulator.h"

#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_commands.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_overrideStates.h>
#include <debug/dag_log.h>
#include <util/dag_string.h>
#include <EASTL/string.h>
#include <drv/3d/dag_decl.h>
#include <drv/3d/dag_resetDevice.h>
#include <EASTL/algorithm.h>
#include <EASTL/shared_ptr.h>
#include <util/dag_convar.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_Quat.h>
#include <perfMon/dag_perfTimer.h>
#include <perfMon/dag_statDrv.h>
#include <atomic>

#if _TARGET_PC_WIN
#include <Windows.h>
#endif // _TARGET_PC_WIN

#if _TARGET_C2













#else
// The default profile is the Oculus Quest 2
static int hmd_resolution_h = 2064;
static int hmd_resolution_v = 2096;
static float hmd_separation = 0.0681346512074297f;
static float hmd_fovl_left = -0.907571197f;
static float hmd_fovl_right = 0.785398185f;
static float hmd_fovl_up = 0.837758064f;
static float hmd_fovl_down = -0.872664630f;
static float hmd_fovr_left = -0.785398185f;
static float hmd_fovr_right = 0.907571197f;
static float hmd_fovr_up = 0.837758064f;
static float hmd_fovr_down = -0.872664630f;
static Quat hmd_vergence = Quat(0.f, 0.f, 0.f, 1.f);
#endif

CONSOLE_BOOL_VAL("xr", emulate_screen_mask, true);


VRDevice *create_emulator()
{
  const char *profile = ::dgs_get_settings()->getBlockByNameEx("xr")->getStr("emulatorProfile", "none");
  if (stricmp(profile, "none") != 0)
    return new VREmulatorDevice(profile);
  return nullptr;
}


VREmulatorDevice::VREmulatorDevice(const char *profile)
{
  auto xrBlock = ::dgs_get_settings()->getBlockByNameEx("xr");
  isHmdOn = xrBlock->getBool("emulatorHmdOnAtStart", true);
  fpsLimit = xrBlock->getInt("emulatorFpsLimit", 140);
  isHmdWorn = isHmdOn;
  callFirstHmdOn = isHmdOn;

  if (profile)
  {
    int resolution_h, resolution_v;
    float fovl_left, fovl_right, fovl_up, fovl_down, fovr_left, fovr_right, fovr_up, fovr_down, separation;
    Quat vergence(0.f, 0.f, 0.f, 1.f);

    int elements = sscanf(profile, "%d,%d,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f", &resolution_h, &resolution_v, &separation,
      &fovl_left, &fovl_right, &fovl_up, &fovl_down, &fovr_left, &fovr_right, &fovr_up, &fovr_down, &vergence.x, &vergence.y,
      &vergence.z, &vergence.w);

    if (elements >= 11)
    {
      hmd_resolution_h = resolution_h;
      hmd_resolution_v = resolution_v;
      hmd_separation = separation;
      hmd_fovl_left = fovl_left;
      hmd_fovl_right = fovl_right;
      hmd_fovl_up = fovl_up;
      hmd_fovl_down = fovl_down;
      hmd_fovr_left = fovr_left;
      hmd_fovr_right = fovr_right;
      hmd_fovr_up = fovr_up;
      hmd_fovr_down = fovr_down;
    }
    if (elements >= 15)
    {
      hmd_vergence = vergence;
    }
  }
  debug("[XR] emulator enabled, resolution %dx%d", hmd_resolution_h, hmd_resolution_v);
}

VREmulatorDevice::~VREmulatorDevice() {}

VRDevice::RenderingAPI VREmulatorDevice::getRenderingAPI() const { return VRDevice::RenderingAPI::Default; }

VRDevice::AdapterID VREmulatorDevice::getAdapterId() const { return 0; }

const char *VREmulatorDevice::getRequiredDeviceExtensions() const { return ""; }

const char *VREmulatorDevice::getRequiredInstanceExtensions() const { return ""; }

eastl::pair<uint64_t, uint64_t> VREmulatorDevice::getRequiredGraphicsAPIRange() const { return {0, 0}; }

void VREmulatorDevice::getViewResolution(int &width, int &height) const
{
  width = hmd_resolution_h;
  height = hmd_resolution_v;
}

float VREmulatorDevice::getViewAspect() const
{
  int width, height;
  getViewResolution(width, height);
  return width / (float)height;
}

bool VREmulatorDevice::isHDR() const
{
#if _TARGET_C2

#else
  static bool isHDR = ::dgs_get_settings()->getBlockByNameEx("xr")->getBool("emulatorHDR", false);
#endif
  return isHDR;
}

bool VREmulatorDevice::isActive() const { return isHmdOn; }

void VREmulatorDevice::setZoom(float zoom) { this->zoom = fmax(zoom, 0.01f); }

bool VREmulatorDevice::setRenderingDeviceImpl() { return true; }

void VREmulatorDevice::setUpSpace()
{
  position = Point3(0, 0, 0);
  orientation.identity();
}

bool VREmulatorDevice::isReadyForRender() { return hasActiveSession(); }

void VREmulatorDevice::tick(const SessionCallback &cb)
{
  bool isReset = false;
  bool isRotateLeft = false;
  bool isRotateRight = false;
  bool isRotateUp = false;
  bool isRotateDown = false;
  bool isMoveUp = false;
  bool isMoveDown = false;
  bool isMoveForward = false;
  bool isMoveBackwards = false;
  bool isMoveLeft = false;
  bool isMoveRight = false;
  bool isSwapPressed = false;
  bool isWearingStatusTogglePressed = false;
  bool isTiltLeft = false;
  bool isTiltRight = false;
#if _TARGET_PC_WIN
  bool isCtrl = GetAsyncKeyState(VK_CONTROL) & 0x8000;
  isReset = GetAsyncKeyState(VK_NUMPAD5) & 0x8000;
  isRotateLeft = GetAsyncKeyState(VK_NUMPAD1) & 0x8000;
  isRotateRight = GetAsyncKeyState(VK_NUMPAD3) & 0x8000;
  isRotateUp = GetAsyncKeyState(VK_NUMPAD7) & 0x8000;
  isRotateDown = GetAsyncKeyState(VK_NUMPAD9) & 0x8000;
  isMoveUp = GetAsyncKeyState(VK_ADD) & 0x8000;
  isMoveDown = GetAsyncKeyState(VK_SUBTRACT) & 0x8000;
  isMoveForward = (GetAsyncKeyState(VK_NUMPAD8) & 0x8000) && !isCtrl;
  isMoveBackwards = (GetAsyncKeyState(VK_NUMPAD2) & 0x8000) && !isCtrl;
  isMoveLeft = (GetAsyncKeyState(VK_NUMPAD4) & 0x8000) && !isCtrl;
  isMoveRight = (GetAsyncKeyState(VK_NUMPAD6) & 0x8000) && !isCtrl;
  isSwapPressed = GetAsyncKeyState(VK_MULTIPLY) & 0x8000;
  isWearingStatusTogglePressed = GetAsyncKeyState(VK_DIVIDE) & 0x8000;
  isTiltLeft = (GetAsyncKeyState(VK_DECIMAL) & 0x8000) && isCtrl;
  isTiltRight = (GetAsyncKeyState(VK_DECIMAL) & 0x8000) && !isCtrl;
  LARGE_INTEGER qpf, qpc;
  QueryPerformanceFrequency(&qpf);
  QueryPerformanceCounter(&qpc);

  static double lastTick = double(qpc.QuadPart) / qpf.QuadPart;

  double currentTick = double(qpc.QuadPart) / qpf.QuadPart;
  double dt = currentTick - lastTick;

  lastTick = currentTick;
#else
  double dt = 0;
#endif // _TARGET_PC_WIN
  // We expect that all effects of a state transition to happen within one frame
  inHmdStateTransition = false;

  if (isReset)
  {
    position = Point3(0, 0, 0);
    orientation.identity();
  }

  TMatrix control;
  control.identity();
  Point3 side = orientation.getcol(0);

  dt *= 2; // Make the input a bit faster

  if (isRotateLeft)
  {
    TMatrix rot;
    rot.makeTM(orientation.getcol(1), -dt);
    control = rot * control;
  }
  else if (isRotateRight)
  {
    TMatrix rot;
    rot.makeTM(orientation.getcol(1), dt);
    control = rot * control;
  }
  if (isRotateUp)
  {
    TMatrix rot;
    rot.makeTM(side, dt);
    control = rot * control;
  }
  else if (isRotateDown)
  {
    TMatrix rot;
    rot.makeTM(side, -dt);
    control = rot * control;
  }
  if (isTiltLeft)
  {
    TMatrix rot;
    rot.makeTM(orientation.getcol(2), dt);
    control = rot * control;
  }
  else if (isTiltRight)
  {
    TMatrix rot;
    rot.makeTM(orientation.getcol(2), -dt);
    control = rot * control;
  }

  if (isMoveUp)
    position += Point3(0, dt, 0);
  if (isMoveDown)
    position -= Point3(0, dt, 0);
  if (isMoveLeft)
    position -= orientation.getcol(0) * dt;
  if (isMoveRight)
    position += orientation.getcol(0) * dt;
  if (isMoveForward)
    position += orientation.getcol(2) * dt;
  if (isMoveBackwards)
    position -= orientation.getcol(2) * dt;

  orientation = control * orientation;

  if (isHmdOn && callFirstHmdOn && cb)
    callSessionStartedCallbackIfAvailable = true;

  static bool zeroLocked = false;
  if (isSwapPressed)
  {
    if (!zeroLocked)
    {
      isHmdOn = !isHmdOn;
      zeroLocked = true;

      inHmdStateTransition = true;

      if (isHmdOn && cb)
        callSessionStartedCallbackIfAvailable = true;
      if (!isHmdOn && cb)
        callSessionEndedCallbackIfAvailable = true;
    }
  }
  else
    zeroLocked = false;

  static bool wearingStatusLocked = false;
  if (isWearingStatusTogglePressed)
  {
    if (!wearingStatusLocked)
    {
      isHmdWorn = !isHmdWorn;
      wearingStatusLocked = true;

      if (isHmdOn)
        changeState(isHmdWorn ? State::Focused : State::Idle);
    }
  }
  else
    wearingStatusLocked = false;

  if (callSessionStartedCallbackIfAvailable && callSessionEndedCallbackIfAvailable)
    callSessionStartedCallbackIfAvailable = callSessionEndedCallbackIfAvailable = false;

  if (callSessionStartedCallbackIfAvailable && cb)
  {
    inHmdStateTransition = true;
    if (cb(Session::Start))
    {
      changeState(State::Stopping);
      callSessionStartedCallbackIfAvailable = false;
      callFirstHmdOn = false;
    }
  }
  if (callSessionEndedCallbackIfAvailable && cb)
  {
    inHmdStateTransition = true;
    if (cb(Session::End))
    {
      changeState(isHmdWorn ? State::Focused : State::Idle);
      callSessionEndedCallbackIfAvailable = false;
    }
  }
}

bool VREmulatorDevice::prepareFrame(FrameData &frameData, float zNear, float zFar)
{
  if (!isHmdOn)
    return false;

  if (fpsLimit > 0)
  {
    static int64_t startRefTicks = profile_ref_ticks();

    TIME_PROFILE(wait_for_fps_limit);
    const unsigned targetFrameTimeUs = 1e6 / fpsLimit;
    const unsigned frameTimeUs = (unsigned)profile_time_usec(startRefTicks);
    int sleepUs = targetFrameTimeUs - frameTimeUs;
    if (sleepUs > 0 && sleepUs < 1e5)
      sleep_precise_usec(sleepUs, preciseSleepContext);

    startRefTicks = profile_ref_ticks();
  }

  int w, h;
  getViewResolution(w, h);

  if (!renderer)
  {
    TextureInfo info;
    Texture *bb = d3d::get_backbuffer_tex();
    if (bb)
      bb->getinfo(info);
    // info.cflg is inited to TEXFMT_DEFAULT no need to init it when bb is nullptr

    if (isHDR())
    {
      info.cflg &= ~TEXFMT_MASK;
      info.cflg |= TEXFMT_R11G11B10F;
    }

    switch (stereoMode)
    {
      case StereoMode::Multipass:
        renderTargets[0] = dag::create_tex(nullptr, w, h, info.cflg | TEXCF_RTARGET, 1, "VREmu0");
        renderTargets[1] = dag::create_tex(nullptr, w, h, info.cflg | TEXCF_RTARGET, 1, "VREmu1");
        break;
      case StereoMode::SideBySideHorizontal:
        renderTargets[0] = dag::create_tex(nullptr, w * 2, h, info.cflg | TEXCF_RTARGET, 1, "VREmuSBSH");
        break;
      case StereoMode::SideBySideVertical:
        renderTargets[0] = dag::create_tex(nullptr, w, h * 2, info.cflg | TEXCF_RTARGET, 1, "VREmuSBSV");
        break;
    }

    renderer = eastl::make_unique<PostFxRenderer>("debug_tex_overlay");

    shaders::OverrideState state;
    state.set(shaders::OverrideState::SCISSOR_ENABLED);
    state.set(shaders::OverrideState::BLEND_SRC_DEST);
    state.sblend = BLEND_ONE;
    state.dblend = BLEND_ZERO;
    state.set(shaders::OverrideState::BLEND_SRC_DEST_A);
    state.sblenda = BLEND_ZERO;
    state.dblenda = BLEND_ZERO;
    scissor = shaders::overrides::create(state);

    setUpSpace();
  }

  Quat orientationQuat = matrix_to_quat(orientation);
  Point3 side = orientation.getcol(0);
  Point3 separation = side * hmd_separation * 0.5f;

  frameData.views[0].fovLeft = hmd_fovl_left;
  frameData.views[0].fovRight = hmd_fovl_right;
  frameData.views[0].fovUp = hmd_fovl_up;
  frameData.views[0].fovDown = hmd_fovl_down;
  frameData.views[0].position = position - separation;
  frameData.views[0].orientation = orientationQuat * inverse(hmd_vergence);

  frameData.views[1].fovLeft = hmd_fovr_left;
  frameData.views[1].fovRight = hmd_fovr_right;
  frameData.views[1].fovUp = hmd_fovr_up;
  frameData.views[1].fovDown = hmd_fovr_down;
  frameData.views[1].position = position + separation;
  frameData.views[1].orientation = orientationQuat * hmd_vergence;

#if _TARGET_PC_WIN
  if (GetAsyncKeyState(VK_NUMPAD0) & 0x8000)
    frameData.views[1] = frameData.views[0];
#endif // _TARGET_PC_WIN

  switch (stereoMode)
  {
    case StereoMode::Multipass:
      frameData.frameRects[0] = RectInt{0, 0, w, h};
      frameData.frameRects[1] = RectInt{0, 0, w, h};
      frameData.frameOffsets[0] = IPoint2(0, 0);
      frameData.frameOffsets[1] = IPoint2(0, 0);
      frameData.frameTargets[0] = renderTargets[0];
      frameData.frameTargets[1] = renderTargets[1];
      break;
    case StereoMode::SideBySideHorizontal:
      frameData.frameRects[0] = RectInt{0, 0, w, h};
      frameData.frameRects[1] = RectInt{w, 0, w * 2, h};
      frameData.frameOffsets[0] = IPoint2(0, 0);
      frameData.frameOffsets[1] = IPoint2(1, 0);
      frameData.frameTargets[0] = renderTargets[0];
      frameData.frameTargets[1] = renderTargets[0];
      break;
    case StereoMode::SideBySideVertical:
      frameData.frameRects[0] = RectInt{0, 0, w, h};
      frameData.frameRects[1] = RectInt{0, h, w, h * 2};
      frameData.frameOffsets[0] = IPoint2(0, 0);
      frameData.frameOffsets[1] = IPoint2(0, 1);
      frameData.frameTargets[0] = renderTargets[0];
      frameData.frameTargets[1] = renderTargets[0];
      break;
  }

  calcViewTransforms(frameData.views[0], zNear, zFar, zoom);
  calcViewTransforms(frameData.views[1], zNear, zFar, zoom);

  const float extraZFar = calcBoundingView(frameData);
  calcViewTransforms(frameData.boundingView, zNear, zFar + extraZFar, zoom);

  frameData.zoom = zoom;

  frameData.depthFilled = false;

  frameData.nearZ = zNear;
  frameData.farZ = zFar;
  frameData.boundingExtraZFar = extraZFar;

  static uint64_t frameId = 0;
  frameData.frameId = frameId++;
  frameData.frameStartedSuccessfully = false;

  return true;
}

void VREmulatorDevice::beginFrame(FrameData &frameData)
{
  struct FrameEventCallbacks : public FrameEvents
  {
    FrameEventCallbacks(VREmulatorDevice *thiz, FrameData &frameData) : thiz(thiz), frameData(frameData) {}
    virtual void beginFrameEvent() { thiz->beginRender(frameData); }
    virtual void endFrameEvent()
    {
      thiz->endRender(frameData);
      delete this;
    }

    VREmulatorDevice *thiz;
    FrameData frameData;
  };

  const bool useFront = true;

  d3d::driver_command(Drv3dCommand::REGISTER_ONE_TIME_FRAME_EXECUTION_EVENT_CALLBACKS, new FrameEventCallbacks(this, frameData),
    reinterpret_cast<void *>(static_cast<intptr_t>(useFront)));
}

void VREmulatorDevice::beginRender(FrameData &) {}

void VREmulatorDevice::endRender(FrameData &frameData) {}

bool VREmulatorDevice::hasQuitRequest() const { return false; }
bool VREmulatorDevice::hasScreenMask() { return !disableScreenMask && emulate_screen_mask; }
void VREmulatorDevice::retrieveScreenMaskTriangles(const TMatrix4 &projection, eastl::vector<Point4> &visibilityMaskVertices,
  eastl::vector<uint16_t> &visibilityMaskIndices, int view_index)
{
  static constexpr int num_vertices = 102;
  static const float leftEyeVerts[4 * num_vertices] = {-0.650624931, 0.968010128, 1.00000000, 1.00000000, 0.330892593, 1.00001037,
    1.00000000, 1.00000000, -0.650624931, 1.00001037, 1.00000000, 1.00000000, -0.650624931, 0.968010128, 1.00000000, 1.00000000,
    0.330892593, 0.968010128, 1.00000000, 1.00000000, 0.330892593, 1.00001037, 1.00000000, 1.00000000, 1.00330222, 1.00001037,
    1.00000000, 1.00000000, 0.330892593, 1.00001037, 1.00000000, 1.00000000, 0.330892593, 0.968010128, 1.00000000, 1.00000000,
    1.00330222, 1.00001037, 1.00000000, 1.00000000, 0.330892593, 0.968010128, 1.00000000, 1.00000000, 0.650036216, 0.892009199,
    1.00000000, 1.00000000, 1.00330222, 1.00001037, 1.00000000, 1.00000000, 0.650036216, 0.892009199, 1.00000000, 1.00000000,
    0.854769886, 0.686007142, 1.00000000, 1.00000000, 1.00330222, 1.00001037, 1.00000000, 1.00000000, 0.854769886, 0.686007142,
    1.00000000, 1.00000000, 0.925021708, 0.316003293, 1.00000000, 1.00000000, 1.00330222, 1.00001037, 1.00000000, 1.00000000,
    0.925021708, 0.316003293, 1.00000000, 1.00000000, 1.00330222, 0.316003293, 1.00000000, 1.00000000, 0.925021708, 0.0480005145,
    1.00000000, 1.00000000, 1.00330222, 0.316003293, 1.00000000, 1.00000000, 0.925021708, 0.316003293, 1.00000000, 1.00000000,
    0.925021708, 0.0480005145, 1.00000000, 1.00000000, 1.00330222, -0.184001863, 1.00000000, 1.00000000, 1.00330222, 0.316003293,
    1.00000000, 1.00000000, 0.925021708, 0.0480005145, 1.00000000, 1.00000000, 0.904949725, -0.184001863, 1.00000000, 1.00000000,
    1.00330222, -0.184001863, 1.00000000, 1.00000000, 1.00330222, -1.00001025, 1.00000000, 1.00000000, 1.00330222, -0.184001863,
    1.00000000, 1.00000000, 0.904949725, -0.184001863, 1.00000000, 1.00000000, 1.00330222, -1.00001025, 1.00000000, 1.00000000,
    0.904949725, -0.184001863, 1.00000000, 1.00000000, 0.788532555, -0.456004620, 1.00000000, 1.00000000, 1.00330222, -1.00001025,
    1.00000000, 1.00000000, 0.788532555, -0.456004620, 1.00000000, 1.00000000, 0.575770140, -0.762007713, 1.00000000, 1.00000000,
    1.00330222, -1.00001025, 1.00000000, 1.00000000, 0.575770140, -0.762007713, 1.00000000, 1.00000000, 0.282719880, -0.954009771,
    1.00000000, 1.00000000, 1.00330222, -1.00001025, 1.00000000, 1.00000000, 0.282719880, -0.954009771, 1.00000000, 1.00000000,
    0.282719880, -1.00001025, 1.00000000, 1.00000000, 0.282719880, -1.00001025, 1.00000000, 1.00000000, 0.282719880, -0.954009771,
    1.00000000, 1.00000000, -0.648617744, -1.00001025, 1.00000000, 1.00000000, -0.648617744, -1.00001025, 1.00000000, 1.00000000,
    0.282719880, -0.954009771, 1.00000000, 1.00000000, -0.648617744, -0.954009771, 1.00000000, 1.00000000, -1.00389087, -1.00001025,
    1.00000000, 1.00000000, -0.648617744, -1.00001025, 1.00000000, 1.00000000, -0.648617744, -0.954009771, 1.00000000, 1.00000000,
    -1.00389087, -1.00001025, 1.00000000, 1.00000000, -0.648617744, -0.954009771, 1.00000000, 1.00000000, -0.761020482, -0.936009526,
    1.00000000, 1.00000000, -1.00389087, -1.00001025, 1.00000000, 1.00000000, -0.761020482, -0.936009526, 1.00000000, 1.00000000,
    -0.913567126, -0.818008363, 1.00000000, 1.00000000, -1.00389087, -1.00001025, 1.00000000, 1.00000000, -0.913567126, -0.818008363,
    1.00000000, 1.00000000, -0.979804635, -0.606006145, 1.00000000, 1.00000000, -1.00389087, -1.00001025, 1.00000000, 1.00000000,
    -0.979804635, -0.606006145, 1.00000000, 1.00000000, -1.00389087, -0.606006145, 1.00000000, 1.00000000, -1.00389087, -0.606006145,
    1.00000000, 1.00000000, -0.979804635, -0.606006145, 1.00000000, 1.00000000, -1.00389087, -0.278002828, 1.00000000, 1.00000000,
    -1.00389087, -0.278002828, 1.00000000, 1.00000000, -0.979804635, -0.606006145, 1.00000000, 1.00000000, -1.00389087, -0.280002803,
    1.00000000, 1.00000000, -1.00389087, -0.278002828, 1.00000000, 1.00000000, -1.00389087, -0.280002803, 1.00000000, 1.00000000,
    -1.00389087, 0.0480005145, 1.00000000, 1.00000000, -1.00389087, -0.278002828, 1.00000000, 1.00000000, -1.00389087, 0.0480005145,
    1.00000000, 1.00000000, -1.00389087, 0.358003765, 1.00000000, 1.00000000, -1.00389087, 0.358003765, 1.00000000, 1.00000000,
    -1.00389087, 0.0480005145, 1.00000000, 1.00000000, -1.00389087, 0.358003765, 1.00000000, 1.00000000, -1.00389087, 0.358003765,
    1.00000000, 1.00000000, -1.00389087, 0.358003765, 1.00000000, 1.00000000, -0.979804635, 0.632006526, 1.00000000, 1.00000000,
    -1.00389087, 0.358003765, 1.00000000, 1.00000000, -0.979804635, 0.632006526, 1.00000000, 1.00000000, -1.00389087, 0.632006526,
    1.00000000, 1.00000000, -1.00389087, 1.00001037, 1.00000000, 1.00000000, -1.00389087, 0.632006526, 1.00000000, 1.00000000,
    -0.979804635, 0.632006526, 1.00000000, 1.00000000, -1.00389087, 1.00001037, 1.00000000, 1.00000000, -0.979804635, 0.632006526,
    1.00000000, 1.00000000, -0.913567126, 0.832008660, 1.00000000, 1.00000000, -1.00389087, 1.00001037, 1.00000000, 1.00000000,
    -0.913567126, 0.832008660, 1.00000000, 1.00000000, -0.761020482, 0.950009882, 1.00000000, 1.00000000, -1.00389087, 1.00001037,
    1.00000000, 1.00000000, -0.761020482, 0.950009882, 1.00000000, 1.00000000, -0.650624931, 0.968010128, 1.00000000, 1.00000000,
    -1.00389087, 1.00001037, 1.00000000, 1.00000000, -0.650624931, 0.968010128, 1.00000000, 1.00000000, -0.650624931, 1.00001037,
    1.00000000, 1.00000000};
  static const float rightEyeVerts[4 * num_vertices] = {0.650414407, 0.968002796, 1.00000000, 1.00000000, 0.650414407, 1.00000262,
    1.00000000, 1.00000000, -0.330835342, 1.00000262, 1.00000000, 1.00000000, 0.650414407, 0.968002796, 1.00000000, 1.00000000,
    -0.330835342, 1.00000262, 1.00000000, 1.00000000, -0.330835342, 0.968002796, 1.00000000, 1.00000000, -1.00306165, 1.00000262,
    1.00000000, 1.00000000, -0.330835342, 0.968002796, 1.00000000, 1.00000000, -0.330835342, 1.00000262, 1.00000000, 1.00000000,
    -1.00306165, 1.00000262, 1.00000000, 1.00000000, -0.649891973, 0.892002404, 1.00000000, 1.00000000, -0.330835342, 0.968002796,
    1.00000000, 1.00000000, -1.00306165, 1.00000262, 1.00000000, 1.00000000, -0.854569852, 0.686001837, 1.00000000, 1.00000000,
    -0.649891973, 0.892002404, 1.00000000, 1.00000000, -1.00306165, 1.00000262, 1.00000000, 1.00000000, -0.924802423, 0.316000849,
    1.00000000, 1.00000000, -0.854569852, 0.686001837, 1.00000000, 1.00000000, -1.00306165, 1.00000262, 1.00000000, 1.00000000,
    -1.00306165, 0.316000849, 1.00000000, 1.00000000, -0.924802423, 0.316000849, 1.00000000, 1.00000000, -0.924802423, 0.0480001122,
    1.00000000, 1.00000000, -0.924802423, 0.316000849, 1.00000000, 1.00000000, -1.00306165, 0.316000849, 1.00000000, 1.00000000,
    -0.924802423, 0.0480001122, 1.00000000, 1.00000000, -1.00306165, 0.316000849, 1.00000000, 1.00000000, -1.00306165, -0.184000507,
    1.00000000, 1.00000000, -0.924802423, 0.0480001122, 1.00000000, 1.00000000, -1.00306165, -0.184000507, 1.00000000, 1.00000000,
    -0.904735923, -0.184000507, 1.00000000, 1.00000000, -1.00306165, -1.00000286, 1.00000000, 1.00000000, -0.904735923, -0.184000507,
    1.00000000, 1.00000000, -1.00306165, -0.184000507, 1.00000000, 1.00000000, -1.00306165, -1.00000286, 1.00000000, 1.00000000,
    -0.788350523, -0.456001222, 1.00000000, 1.00000000, -0.904735923, -0.184000507, 1.00000000, 1.00000000, -1.00306165, -1.00000286,
    1.00000000, 1.00000000, -0.575646043, -0.762002051, 1.00000000, 1.00000000, -0.788350523, -0.456001222, 1.00000000, 1.00000000,
    -1.00306165, -1.00000286, 1.00000000, 1.00000000, -0.282675773, -0.954002678, 1.00000000, 1.00000000, -0.575646043, -0.762002051,
    1.00000000, 1.00000000, -1.00306165, -1.00000286, 1.00000000, 1.00000000, -0.282675773, -1.00000286, 1.00000000, 1.00000000,
    -0.282675773, -0.954002678, 1.00000000, 1.00000000, -0.282675773, -1.00000286, 1.00000000, 1.00000000, 0.648407817, -1.00000286,
    1.00000000, 1.00000000, -0.282675773, -0.954002678, 1.00000000, 1.00000000, 0.648407817, -1.00000286, 1.00000000, 1.00000000,
    0.648407817, -0.954002678, 1.00000000, 1.00000000, -0.282675773, -0.954002678, 1.00000000, 1.00000000, 1.00358403, -1.00000286,
    1.00000000, 1.00000000, 0.648407817, -0.954002678, 1.00000000, 1.00000000, 0.648407817, -1.00000286, 1.00000000, 1.00000000,
    1.00358403, -1.00000286, 1.00000000, 1.00000000, 0.760779977, -0.936002493, 1.00000000, 1.00000000, 0.648407817, -0.954002678,
    1.00000000, 1.00000000, 1.00358403, -1.00000286, 1.00000000, 1.00000000, 0.913285017, -0.818002224, 1.00000000, 1.00000000,
    0.760779977, -0.936002493, 1.00000000, 1.00000000, 1.00358403, -1.00000286, 1.00000000, 1.00000000, 0.979504228, -0.606001616,
    1.00000000, 1.00000000, 0.913285017, -0.818002224, 1.00000000, 1.00000000, 1.00358403, -1.00000286, 1.00000000, 1.00000000,
    1.00358403, -0.606001616, 1.00000000, 1.00000000, 0.979504228, -0.606001616, 1.00000000, 1.00000000, 1.00358403, -0.606001616,
    1.00000000, 1.00000000, 1.00358403, -0.278000772, 1.00000000, 1.00000000, 0.979504228, -0.606001616, 1.00000000, 1.00000000,
    1.00358403, -0.278000772, 1.00000000, 1.00000000, 1.00358403, -0.280000716, 1.00000000, 1.00000000, 0.979504228, -0.606001616,
    1.00000000, 1.00000000, 1.00358403, -0.278000772, 1.00000000, 1.00000000, 1.00358403, 0.0480001122, 1.00000000, 1.00000000,
    1.00358403, -0.280000716, 1.00000000, 1.00000000, 1.00358403, -0.278000772, 1.00000000, 1.00000000, 1.00358403, 0.358001024,
    1.00000000, 1.00000000, 1.00358403, 0.0480001122, 1.00000000, 1.00000000, 1.00358403, 0.358001024, 1.00000000, 1.00000000,
    1.00358403, 0.358001024, 1.00000000, 1.00000000, 1.00358403, 0.0480001122, 1.00000000, 1.00000000, 1.00358403, 0.358001024,
    1.00000000, 1.00000000, 0.979504228, 0.632001698, 1.00000000, 1.00000000, 1.00358403, 0.358001024, 1.00000000, 1.00000000,
    1.00358403, 0.358001024, 1.00000000, 1.00000000, 1.00358403, 0.632001698, 1.00000000, 1.00000000, 0.979504228, 0.632001698,
    1.00000000, 1.00000000, 1.00358403, 1.00000262, 1.00000000, 1.00000000, 0.979504228, 0.632001698, 1.00000000, 1.00000000,
    1.00358403, 0.632001698, 1.00000000, 1.00000000, 1.00358403, 1.00000262, 1.00000000, 1.00000000, 0.913285017, 0.832002282,
    1.00000000, 1.00000000, 0.979504228, 0.632001698, 1.00000000, 1.00000000, 1.00358403, 1.00000262, 1.00000000, 1.00000000,
    0.760779977, 0.950002670, 1.00000000, 1.00000000, 0.913285017, 0.832002282, 1.00000000, 1.00000000, 1.00358403, 1.00000262,
    1.00000000, 1.00000000, 0.650414407, 0.968002796, 1.00000000, 1.00000000, 0.760779977, 0.950002670, 1.00000000, 1.00000000,
    1.00358403, 1.00000262, 1.00000000, 1.00000000, 0.650414407, 1.00000262, 1.00000000, 1.00000000, 0.650414407, 0.968002796,
    1.00000000, 1.00000000};

  visibilityMaskVertices.reserve(num_vertices);
  visibilityMaskIndices.reserve(num_vertices);
  if (view_index == 0)
  {
    for (int i = 0; i < num_vertices; i++)
    {
      visibilityMaskVertices.push_back(
        Point4(leftEyeVerts[4 * i], leftEyeVerts[4 * i + 1], leftEyeVerts[4 * i + 2], leftEyeVerts[4 * i + 3]));
      visibilityMaskIndices.push_back(i);
    }
  }
  else
  {
    for (int i = 0; i < num_vertices; i++)
    {
      visibilityMaskVertices.push_back(
        Point4(rightEyeVerts[4 * i], rightEyeVerts[4 * i + 1], rightEyeVerts[4 * i + 2], rightEyeVerts[4 * i + 3]));
      visibilityMaskIndices.push_back(i);
    }
  }
}

void VREmulatorDevice::beforeSoftDeviceReset() {}

void VREmulatorDevice::afterSoftDeviceReset() {}

bool VREmulatorDevice::isInStateTransition() const { return inHmdStateTransition; }

bool VREmulatorDevice::isInitialized() const { return true; }
