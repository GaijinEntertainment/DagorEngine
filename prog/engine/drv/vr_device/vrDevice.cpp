#include "drivers/dag_vr.h"
#include "3d/dag_drv3d.h"
#include "3d/dag_drv3dReset.h"
#include "3d/dag_tex3d.h"
#include "shaders/dag_postFxRenderer.h"
#include "shaders/dag_overrideStates.h"
#include "shaders/dag_shaders.h"
#include "shaders/dag_shaderBlock.h"
#include "3d/dag_texMgr.h"
#include "perfMon/dag_statDrv.h"
#include "math/integer/dag_IPoint2.h"

#if _TARGET_PC_WIN && DAGOR_DBGLEVEL > 0 // TODO: remove mask render toggle from here
#include "Windows.h"
#endif

VRDevice *create_vr_device(VRDevice::RenderingAPI, const VRDevice::ApplicationData &);
VRDevice *create_emulator();

static VRDevice *vr_instance = nullptr;

static HumanInput::VrInput::InitializationCallback vr_input_initialization_cb;
void VRDevice::setAndCallInputInitializationCallback(HumanInput::VrInput::InitializationCallback cb)
{
  if (auto vrin = VRDevice::getInput())
    vrin->setAndCallInitializationCallback(cb);
  else
  {
    debug("[XR][device] %scached input initialization callback", cb ? "" : "cleared ");
    vr_input_initialization_cb = cb; // Cache it until/if VR will be toggled later (e.g. via options)
  }
}


void VRDevice::create(RenderingAPI rendering_api, const ApplicationData &application_data)
{
  if (vr_instance)
    return;

  vr_instance = create_emulator();
  if (vr_instance == nullptr)
    vr_instance = create_vr_device(rendering_api, application_data);

  if (vr_instance && !vr_instance->isInitialized())
    del_it(vr_instance);

  if (vr_input_initialization_cb && VRDevice::getInput())
  {
    // Set but don't trigger: session setup has not finished yet - pending device reset
    VRDevice::getInput()->setInitializationCallback(vr_input_initialization_cb);
    vr_input_initialization_cb = nullptr;
  }
}

VRDevice *VRDevice::getInstance() { return vr_instance; }

VRDevice *VRDevice::getInstanceIfActive() { return hasActiveSession() ? vr_instance : nullptr; }

void VRDevice::deleteInstance() { del_it(vr_instance); }

bool VRDevice::hasActiveSession() { return vr_instance && vr_instance->isActive(); }

static bool has_vr_multiview_support()
{
  return d3d::get_driver_desc().caps.hasBasicViewInstancing || d3d::get_driver_desc().caps.hasStereoExpansion;
}

static const char *get_vr_multiview_support_string()
{
  if (d3d::get_driver_desc().caps.hasStereoExpansion)
    return "VR multiview is supported through stereo expansion.";
  if (d3d::get_driver_desc().caps.hasAcceleratedViewInstancing)
    return "VR multiview is supported through tier 3 view instancing.";
  if (d3d::get_driver_desc().caps.hasOptimizedViewInstancing)
    return "VR multiview is supported through tier 2 view instancing.";
  if (d3d::get_driver_desc().caps.hasBasicViewInstancing)
    return "VR multiview is supported through tier 1 view instancing.";
  return "VR multiview is not supported.";
}

VRDevice::VRDevice() = default;

VRDevice::~VRDevice() = default;

void VRDevice::setMirrorMode(MirrorMode mode) { mirrorMode = mode; }

static RectInt calc_viewport(float srcAspect, const RectInt &dst)
{
  IPoint2 dstSize = IPoint2(dst.right - dst.left, dst.bottom - dst.top);
  float dstAspect = float(dstSize.x) / dstSize.y;

  RectInt resultTXC; // (left, top, right, bottom) in texel space
#if _TARGET_C2









#else
  if (srcAspect >= dstAspect)
  {
    // If the VR image is wider than the screen area,
    // cut the left and right sides
    // If the screen area is wider than the VR image,
    // cut the top and bottom sides
    int m = (dst.left + dst.right) / 2;
    int av = dstSize.y * srcAspect;

    resultTXC.left = m - av / 2;
    resultTXC.top = dst.top;
    resultTXC.right = m + av / 2;
    resultTXC.bottom = dst.bottom;
  }
  else
  {
    // If the screen area is wider than the VR image,
    // cut the top and bottom sides
    int m = (dst.top + dst.bottom) / 2;
    int ah = dstSize.x / srcAspect;

    resultTXC.left = dst.left;
    resultTXC.top = m - ah / 2;
    resultTXC.right = dst.right;
    resultTXC.bottom = m + ah / 2;
  }
#endif

  return resultTXC;
};

void VRDevice::renderMirror(FrameData &frame)
{
  if (isMirrorDisabled())
    return;

  if (!mirrorRenderer)
  {
    mirrorRenderer = eastl::make_unique<PostFxRenderer>("vr_mirror");

    shaders::OverrideState state;
    state.set(shaders::OverrideState::SCISSOR_ENABLED);
    mirrorState = shaders::overrides::create(state);
  }

  static int vr_texture_sourceVarId = get_shader_variable_id("vr_texture_source");
  static int vr_texture_transformVarId = get_shader_variable_id("vr_texture_transform");

  shaders::overrides::set(mirrorState);

  int w, h;
  d3d::get_target_size(w, h);

  float srcAspect = getViewAspect();

  SCOPE_RESET_SHADER_BLOCKS;

  TextureInfo info;
  frame.frameTargets[0]->getinfo(info);

  Point4 uvTransformLeft, uvTransformRight;
  ManagedTexView leftTex, rightTex;

  switch (stereoMode)
  {
    case StereoMode::Multipass:
      leftTex = frame.frameTargets[0];
      rightTex = frame.frameTargets[1];
      uvTransformLeft = Point4(1, 1, 0, 0);
      uvTransformRight = Point4(1, 1, 0, 0);
      break;
    case StereoMode::SideBySideHorizontal:
      leftTex = frame.frameTargets[0];
      rightTex = frame.frameTargets[0];
      uvTransformLeft = Point4(0.5, 1, 0, 0);
      uvTransformRight = Point4(0.5, 1, 0.5, 0);
      break;
    case StereoMode::SideBySideVertical:
      leftTex = frame.frameTargets[0];
      rightTex = frame.frameTargets[0];
      uvTransformLeft = Point4(1, 0.5, 0, 0);
      uvTransformRight = Point4(1, 0.5, 0, 0.5);
      break;
  }

  switch (mirrorMode)
  {
    case MirrorMode::Left:
    {
      auto vp = calc_viewport(srcAspect, RectInt{0, 0, w, h});
      d3d::setview(vp.left, vp.top, vp.right - vp.left, vp.bottom - vp.top, 0, 1);
      d3d::setscissor(0, 0, w, h);
      ShaderGlobal::set_color4(vr_texture_transformVarId, uvTransformLeft);
      ShaderGlobal::set_texture(vr_texture_sourceVarId, leftTex);
      mirrorRenderer->render();
      break;
    }
    case MirrorMode::Right:
    {
      auto vp = calc_viewport(srcAspect, RectInt{0, 0, w, h});
      d3d::setview(vp.left, vp.top, vp.right - vp.left, vp.bottom - vp.top, 0, 1);
      d3d::setscissor(0, 0, w, h);
      ShaderGlobal::set_color4(vr_texture_transformVarId, uvTransformRight);
      ShaderGlobal::set_texture(vr_texture_sourceVarId, rightTex);
      mirrorRenderer->render();
      break;
    }
    case MirrorMode::Both:
    {
      auto vp = calc_viewport(srcAspect, RectInt{0, 0, w / 2, h});
      d3d::setview(vp.left, vp.top, vp.right - vp.left, vp.bottom - vp.top, 0, 1);
      d3d::setscissor(0, 0, w / 2, h);
      ShaderGlobal::set_color4(vr_texture_transformVarId, uvTransformLeft);
      ShaderGlobal::set_texture(vr_texture_sourceVarId, leftTex);
      mirrorRenderer->render();

      vp = calc_viewport(srcAspect, RectInt{w / 2, 0, w, h});
      d3d::setview(vp.left, vp.top, vp.right - vp.left, vp.bottom - vp.top, 0, 1);
      d3d::setscissor(w / 2, 0, w / 2, h);
      ShaderGlobal::set_color4(vr_texture_transformVarId, uvTransformRight);
      ShaderGlobal::set_texture(vr_texture_sourceVarId, rightTex);
      mirrorRenderer->render();
      break;
    }
  }

  shaders::overrides::reset();
}

static bool enableXr = false;

bool VRDevice::shouldBeEnabled()
{
  static bool once = true;
  if (once)
  {
    enableXr = dgs_get_settings()->getBlockByNameEx("gameplay")->getBool("enableVR", false);
    once = false;
  }

  return enableXr;
}

bool VRDevice::isPresenceSensorForcedToBeOn()
{
#if DAGOR_DBGLEVEL > 0
  static bool forced = dgs_get_settings()->getBlockByNameEx("xr")->getBool("forcePresence", false);
  return forced;
#else
  return false;
#endif
}

void VRDevice::setEnabled(bool enabled) { enableXr = enabled; }

static void vr_before_reset(bool full_reset)
{
  if (!full_reset)
    return;

  logdbg("[XR][device] vr_before_reset");
  if (vr_instance)
    vr_instance->beforeSoftDeviceReset();
}

static void vr_after_reset(bool full_reset)
{
  if (VRDevice::shouldBeEnabled())
  {
    bool initialVSyncValue = dgs_get_settings()->getBlockByNameEx("video")->getBool("vsync", false);
    d3d::enable_vsync(VRDevice::hasActiveSession() ? false : initialVSyncValue);
  }

  if (!full_reset)
    return;

  logdbg("[XR][device] vr_after_reset");
  if (vr_instance)
    vr_instance->afterSoftDeviceReset();
}

bool VRDevice::renderScreenMask(const TMatrix4 &projection, int view_index)
{
  if (!hasScreenMask())
    return false;

#if _TARGET_PC_WIN && DAGOR_DBGLEVEL > 0
  if (GetAsyncKeyState(VK_NUMPAD5) & 0x8000)
    return false;
#endif // _TARGET_PC_WIN && DAGOR_DBGLEVEL > 0

  TIME_D3D_PROFILE(OpenXRDevice_renderScreenMask);

  prepareScreenMask(projection, view_index);

  if (!screenMaskVertexBuffer[view_index].getBuf() || !screenMaskIndexBuffer[view_index].getBuf())
    return false;

  d3d::setvdecl(screenMaskVertexDeclaration);
  d3d::setvsrc(0, screenMaskVertexBuffer[view_index].getBuf(), screenMaskVertexStride);
  d3d::setind(screenMaskIndexBuffer[view_index].getBuf());

  screenMaskRenderer.shader->setStates();
  d3d::drawind(PRIM_TRILIST, 0, screenMaskTriangleCount[view_index], 0);

  return true;
}
void VRDevice::tearDownScreenMaskResources()
{
  screenMaskVertexDeclaration = BAD_VDECL;
  screenMaskRenderer.close();

  for (auto &buffer : screenMaskVertexBuffer)
    buffer.close();
  for (auto &buffer : screenMaskIndexBuffer)
    buffer.close();
}
void VRDevice::prepareScreenMask(const TMatrix4 &projection, int view_index)
{
  if (!screenMaskAvailable)
    return;

  if (screenMaskVertexBuffer[view_index].getBuf())
    return;

  eastl::vector<Point4> visibilityMaskVertices;
  eastl::vector<uint16_t> visibilityMaskIndices;

  retrieveScreenMaskTriangles(projection, visibilityMaskVertices, visibilityMaskIndices, view_index);

  if (visibilityMaskVertices.empty() || visibilityMaskIndices.empty())
  {
    return;
  }

  static CompiledShaderChannelId channels[] = {{SCTYPE_FLOAT4, SCUSAGE_POS, 0, 0}};

  if (!screenMaskRenderer.shader)
    screenMaskRenderer.init("openxr_screen_mask", channels, 1, "openxr_screen_mask");

  if (screenMaskVertexDeclaration == BAD_VDECL)
  {
    screenMaskVertexDeclaration = dynrender::addShaderVdecl(channels, 1);
    screenMaskVertexStride = dynrender::getStride(channels, 1);
    G_ASSERT(screenMaskVertexDeclaration != BAD_VDECL);
  }

  const char *vbName = view_index == 0 ? "OpenXRVisibilityVMask0" : "OpenXRVisibilityVMask1";
  const char *ibName = view_index == 0 ? "OpenXRVisibilityIMask0" : "OpenXRVisibilityIMask1";

  int bufFlags = SBCF_MAYBELOST | SBCF_CPU_ACCESS_WRITE;

  int vbSize = sizeof(Point4) * visibilityMaskVertices.size();
  int ibSize = sizeof(uint16_t) * visibilityMaskIndices.size();

  auto &vb = screenMaskVertexBuffer[view_index];
  auto &ib = screenMaskIndexBuffer[view_index];

  vb = dag::create_vb(vbSize, bufFlags, vbName);
  G_ASSERT(vb.getBuf());

  ib = dag::create_ib(ibSize, bufFlags, ibName);
  G_ASSERT(ib.getBuf());

  screenMaskTriangleCount[view_index] = visibilityMaskIndices.size() / 3;

  Point2 *vbData = nullptr;
  bool vbLockSuccess = vb.getBuf()->lock(0, vbSize, (void **)&vbData, VBLOCK_WRITEONLY);
  if (!vbLockSuccess || vbData == nullptr)
  {
    tearDownScreenMaskResources();
    return;
  }
  memcpy(vbData, visibilityMaskVertices.data(), vbSize);
  vb.getBuf()->unlock();

  uint16_t *ibData = nullptr;
  bool ibLockSuccess = ib.getBuf()->lock(0, ibSize, (void **)&ibData, VBLOCK_WRITEONLY);
  if (!ibLockSuccess || ibData == nullptr)
  {
    tearDownScreenMaskResources();
    return;
  }
  memcpy(ibData, visibilityMaskIndices.data(), ibSize);
  ib.getBuf()->unlock();
}

void VRDevice::forceDisableScreenMask(bool disable) { disableScreenMask = disable; }

float VRDevice::calcBoundingView(FrameData &frameData)
{
  const auto &left = frameData.views[0];
  const auto &right = frameData.views[1];
  auto &bounding = frameData.boundingView;

  const float maxFov = tan(
    eastl::max({abs(left.fovLeft), abs(left.fovRight), abs(right.fovLeft), abs(right.fovRight), abs(left.fovUp), abs(left.fovDown)}));

  // Here we are moving the viewpoint backwards to the point
  // where the FOV is covering both eyes
  Point4 k4 = left.cameraTransform.getcol(2) + right.cameraTransform.getcol(2);
  DPoint3 k = DPoint3::xyz(k4);
  k.normalize();
  DPoint3 v = right.position - left.position;
  DPoint3 m = (left.position + right.position) / 2;
  double d = v.length();
  double t = (0.5 * d) / maxFov;
  DPoint3 f = m - k * t;

  bounding.orientation = qinterp(left.orientation, right.orientation, 0.5f);
  bounding.fovLeft = -vectors_angle(bounding.orientation.getForward(), left.orientation.getForward()) + left.fovLeft;
  bounding.fovRight = vectors_angle(bounding.orientation.getForward(), right.orientation.getForward()) + right.fovRight;
  bounding.fovUp = eastl::max(abs(left.fovUp), abs(right.fovUp));
  bounding.fovDown = -eastl::max(abs(left.fovDown), abs(right.fovDown));
  bounding.position = f;

  return t;
}

void VRDevice::fovValuesToDriverPerspective(float fovLeft, float fovRight, float fovUp, float fovDown, float zNear, float zFar,
  float zoom, Driver3dPerspective &result)
{
  float left = tanf(fovLeft);
  float right = tanf(fovRight);
  float down = tanf(fovDown);
  float up = tanf(fovUp);

  float reciprocalWidth = 1.0f / (right - left);
  float reciprocalHeight = 1.0f / (up - down);

  result.zn = zNear;
  result.zf = zFar;
  if (is_equal_float(zoom, 1.0f))
  {
    result.wk = 2.0f * reciprocalWidth;
    result.hk = 2.0f * reciprocalHeight;
  }
  else
  {
    result.wk = 1.0f / (tanf((fovRight - fovLeft) / (2.f * zoom)));
    result.hk = 1.0f / (tanf((fovUp - fovDown) / (2.f * zoom)));
  }
  result.ox = -(left + right) * reciprocalWidth;
  result.oy = -(up + down) * reciprocalHeight;
}

void VRDevice::modifyCameraTransform(const Quat &orientationOffset, const Point3 &positionOffset, TMatrix4D &cameraTransform)
{
  TMatrix4D cameraOffset = quat_to_matrix(orientationOffset);
  cameraOffset.setrow(3, positionOffset.x, positionOffset.y, positionOffset.z, 1);
  cameraTransform = cameraOffset * cameraTransform;
}

VRDevice::MirrorMode VRDevice::getMirrorModeFromSettings()
{
  auto eyeName = ::dgs_get_settings()->getBlockByNameEx("video")->getStr("vreye", "left");
  MirrorMode mode = MirrorMode::Left;
  if (strcmp(eyeName, "right") == 0)
    mode = MirrorMode::Right;
  else if (strcmp(eyeName, "both") == 0)
    mode = MirrorMode::Both;

  return mode;
}

bool VRDevice::getForceDisableScreenMaskFromSettings()
{
  return ::dgs_get_settings()->getBlockByNameEx("video")->getBool("vrStreamerMode", false);
}

void VRDevice::applyAllFromSettings()
{
  if (auto vrDevice = VRDevice::getInstance())
  {
    vrDevice->setMirrorMode(getMirrorModeFromSettings());
    vrDevice->forceDisableScreenMask(getForceDisableScreenMaskFromSettings());
  }
}

int VRDevice::getConfiguredRefreshRate()
{
#if _TARGET_C2



#else
  return 0;
#endif
}

void VRDevice::calcViewTransforms(VRDevice::FrameData::ViewData &view, float zNear, float zFar, float zoom)
{
  VRDevice::fovValuesToDriverPerspective(view.fovLeft, view.fovRight, view.fovUp, view.fovDown, zNear, zFar, zoom, view.projection);

  TMatrix orientMatrix;
  orientMatrix.makeTM(view.orientation);
  orientMatrix.orthonormalize();
  view.cameraTransform = TMatrix4D(orientMatrix);
  view.cameraTransform.setrow(3, view.position.x, view.position.y, view.position.z, 1);
  view.viewTransform = orthonormalized_inverse(view.cameraTransform);
}

bool VRDevice::setRenderingDevice()
{
  const DataBlock *xrBlk = ::dgs_get_settings()->getBlockByNameEx("xr");

  bool useVrMultiview = xrBlk->getBool("useVrMultiview", false);
  if (useVrMultiview)
  {
    debug(get_vr_multiview_support_string());
    if (!has_vr_multiview_support())
    {
      useVrMultiview = false;
      logwarn("use_vr_multiview was requested, but there is no hardware support for it!");
    }

#if _TARGET_C2

#else
    auto stereoModeStr = xrBlk->getStr("vrMultiviewStereoMode", "vertical");
    stereoMode = stricmp(stereoModeStr, "vertical") == 0 ? StereoMode::SideBySideVertical : StereoMode::SideBySideHorizontal;
#endif
  }

  if (!useVrMultiview)
    stereoMode = StereoMode::Multipass;

  return setRenderingDeviceImpl();
}

REGISTER_D3D_BEFORE_RESET_FUNC(vr_before_reset);
REGISTER_D3D_AFTER_RESET_FUNC(vr_after_reset);
