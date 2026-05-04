// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "cameraInCamera.h"

#include <ecs/anim/anim.h>
#include <ecs/camera/getActiveCameraSetup.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <ecs/phys/collRes.h>
#include <ecs/render/updateStageRender.h>

#include <render/daFrameGraph/multiplexing.h>
#include <render/viewVecs.h>
#include <shaders/dag_dynSceneRes.h>

#include <render/rendererFeatures.h>
#include <render/world/aimRender.h>
#define INSIDE_RENDERER 1
#include <render/world/private_worldRenderer.h>


CONSOLE_BOOL_VAL("camcam", use_delayed_deactivation, true);

template <typename Callable>
static inline void update_camcam_transforms_ecs_query(ecs::EntityManager &manager, Callable);

template <typename Callable>
static inline void update_camcam_state_ecs_query(ecs::EntityManager &manager, Callable);

template <typename Callable>
static inline void get_camcam_uv_remapping_ecs_query(ecs::EntityManager &manager, Callable);

template <typename Callable>
static inline void get_camcam_render_state_ecs_query(ecs::EntityManager &manager, Callable);

template <typename Callable>
static inline void get_camcam_lens_only_zoom_state_ecs_query(ecs::EntityManager &manager, Callable);

template <typename Callable>
static inline void check_if_scope_disables_camcam_ecs_query(ecs::EntityManager &manager, ecs::EntityId, Callable);

template <typename Callable>
static inline void check_if_frame_after_deactivation_ecs_query(ecs::EntityManager &manager, Callable);

template <typename Callable>
static inline void get_camcam_frame_number_ecs_query(ecs::EntityManager &manager, Callable);

namespace var
{
static ShaderVariableInfo camera_in_camera_prev_vp_ellipse_center("camera_in_camera_prev_vp_ellipse_center", true);
static ShaderVariableInfo camera_in_camera_prev_vp_ellipse_xy_axes("camera_in_camera_prev_vp_ellipse_xy_axes", true);
static ShaderVariableInfo camera_in_camera_vp_ellipse_center("camera_in_camera_vp_ellipse_center", true);
static ShaderVariableInfo camera_in_camera_vp_ellipse_xy_axes("camera_in_camera_vp_ellipse_xy_axes", true);
static ShaderVariableInfo camera_in_camera_vp_y_scale("camera_in_camera_vp_y_scale", true);
static ShaderVariableInfo camera_in_camera_active("camera_in_camera_active", true);
static ShaderVariableInfo camera_in_camera_vp_uv_remapping("camera_in_camera_vp_uv_remapping", true);
static ShaderVariableInfo camera_in_camera_postfx_lens_view("camera_in_camera_postfx_lens_view", true);

static ShaderVariableInfo projtm_psf_0("projtm_psf_0", true);
static ShaderVariableInfo projtm_psf_1("projtm_psf_1", true);
static ShaderVariableInfo projtm_psf_2("projtm_psf_2", true);
static ShaderVariableInfo projtm_psf_3("projtm_psf_3", true);
static ShaderVariableInfo prev_globtm_psf_0("prev_globtm_psf_0", true);
static ShaderVariableInfo prev_globtm_psf_1("prev_globtm_psf_1", true);
static ShaderVariableInfo prev_globtm_psf_2("prev_globtm_psf_2", true);
static ShaderVariableInfo prev_globtm_psf_3("prev_globtm_psf_3", true);
static ShaderVariableInfo prev_globtm_no_ofs_psf_0("prev_globtm_no_ofs_psf_0", false);
static ShaderVariableInfo prev_globtm_no_ofs_psf_1("prev_globtm_no_ofs_psf_1", false);
static ShaderVariableInfo prev_globtm_no_ofs_psf_2("prev_globtm_no_ofs_psf_2", false);
static ShaderVariableInfo prev_globtm_no_ofs_psf_3("prev_globtm_no_ofs_psf_3", false);
static ShaderVariableInfo globtm_psf_0("globtm_psf_0", true);
static ShaderVariableInfo globtm_psf_1("globtm_psf_1", true);
static ShaderVariableInfo globtm_psf_2("globtm_psf_2", true);
static ShaderVariableInfo globtm_psf_3("globtm_psf_3", true);
static ShaderVariableInfo globtm_no_ofs_psf_0("globtm_no_ofs_psf_0", true);
static ShaderVariableInfo globtm_no_ofs_psf_1("globtm_no_ofs_psf_1", true);
static ShaderVariableInfo globtm_no_ofs_psf_2("globtm_no_ofs_psf_2", true);
static ShaderVariableInfo globtm_no_ofs_psf_3("globtm_no_ofs_psf_3", true);
} // namespace var

namespace camera_in_camera
{
void setup(const bool feature_enabled)
{
  const ecs::EntityId eid = g_entity_mgr->getSingletonEntity(ECS_HASH("camera_in_camera"));

  if (!eid && feature_enabled)
    g_entity_mgr->createEntitySync("camera_in_camera");
  else if (eid && !feature_enabled)
    g_entity_mgr->destroyEntity(eid);

  ShaderGlobal::set_float(var::camera_in_camera_active, 0.0f);
}

bool is_frame_after_deactivation()
{
  bool v = false;
  check_if_frame_after_deactivation_ecs_query(*g_entity_mgr,
    [&v](const bool camcam__frame_after_deactivation) { v = camcam__frame_after_deactivation; });
  return v;
}

int get_frame_number()
{
  int i = -1;
  get_camcam_frame_number_ecs_query(*g_entity_mgr, [&i](const int camcam__iFrame) { i = camcam__iFrame; });
  return i;
}

static void process_lens_activation_state(const bool feature_enabled_for_scope,
  bool &camcam__lens_render_active,
  int &camcam__iFrame,
  bool &camcam__frame_after_deactivation,
  const AimRenderingData &aim_data)
{
  if (!feature_enabled_for_scope)
  {
    camcam__lens_render_active = false;
    camcam__frame_after_deactivation = false;
    camcam__iFrame = -1;
    return;
  }

  const bool wasLookingThroughTheLens = camcam__lens_render_active;
  const bool lookingThroughTheLens = aim_data.lensRenderEnabled;

  camcam__frame_after_deactivation = wasLookingThroughTheLens && !lookingThroughTheLens;
  camcam__lens_render_active = lookingThroughTheLens;
  camcam__iFrame = camcam__lens_render_active ? camcam__iFrame + 1 : -1;
}

static inline bool check_if_scope_disables_camcam(const ecs::EntityId entity_with_scope)
{
  const bool hasWeaponWithScopeInHands = static_cast<bool>(entity_with_scope);
  if (!hasWeaponWithScopeInHands)
    return true;

  bool scopeDisablesCamCam = false;
  check_if_scope_disables_camcam_ecs_query(*g_entity_mgr, entity_with_scope,
    [&scopeDisablesCamCam](const bool gunmod__lensOnlyZoomDisabled) {
      if (gunmod__lensOnlyZoomDisabled)
        scopeDisablesCamCam = gunmod__lensOnlyZoomDisabled;
    });

  return scopeDisablesCamCam;
}

bool activate_view()
{
  ShaderGlobal::set_float(var::camera_in_camera_active, 0.0f);

  bool isActiveVal = false;

  update_camcam_state_ecs_query(*g_entity_mgr, [&isActiveVal](bool &camcam__lens_render_active, int &camcam__iFrame,
                                                 const bool camcam__lens_only_zoom_enabled, bool &camcam__frame_after_deactivation) {
    const AimRenderingData aimData = get_aim_rendering_data();
    process_lens_activation_state(camcam__lens_only_zoom_enabled, camcam__lens_render_active, camcam__iFrame,
      camcam__frame_after_deactivation, aimData);

    isActiveVal = camcam__lens_render_active;
  });

  return isActiveVal;
}

bool is_lens_render_active()
{
  bool active = false;
  get_camcam_render_state_ecs_query(*g_entity_mgr, [&](bool camcam__lens_render_active) { active = camcam__lens_render_active; });
  return active;
}

bool is_lens_only_zoom_enabled()
{
  bool enabled = false;
  get_camcam_lens_only_zoom_state_ecs_query(*g_entity_mgr,
    [&](bool camcam__lens_only_zoom_enabled) { enabled = camcam__lens_only_zoom_enabled; });
  return enabled;
}

bool is_main_view(const dafg::multiplexing::Index &index) { return index.subCamera == 0; }

static Point4 calc_uv_remapping(const CameraParams &main_view, const CameraParams &lens_view)
{
  const float horScale = main_view.jitterPersp.wk / lens_view.jitterPersp.wk;
  const float verScale = main_view.jitterPersp.hk / lens_view.jitterPersp.hk;

  // 1. UV_lens, [0,1] = > [-1:1]
  //  x * 2 - 1
  // 2. [-1:1] => [-scale:scale]
  //  x * 2 * scale - scale
  // 3. [-scale:scale] => UV_main.
  //  X * 0.5 + 0.5
  //  (x * 2 * scale - scale) * 0.5 + 0.5
  //  x*scale - 0.5*scale + 0.5

  const Point2 xRemap{horScale, 0.5f - 0.5f * horScale};
  const Point2 yRemap{verScale, 0.5f - 0.5f * verScale};

  return Point4{xRemap.x, xRemap.y, yRemap.x, yRemap.y};
}

namespace
{
struct ViewportEllipse
{
  Point2 xAxis;
  Point2 yAxis;
  Point2 center;
};

ViewportEllipse calc_viewport_ellipse(
  const float lens_bounding_sphere_radius, const TMatrix lens_wtm, const TMatrix4 view_rot_jitter_proj_tm, float viewport_scale)
{
  const Point3 lensRight = -lens_wtm.col[0] * lens_bounding_sphere_radius;
  const Point3 lensUp = lens_wtm.col[2] * lens_bounding_sphere_radius;

  const Point3 centerWpos = lens_wtm.col[3];
  const Point3 rightWpos = centerWpos + lensRight;
  const Point3 upWpos = centerWpos + lensUp;

  const auto worldToViewport = [&view_rot_jitter_proj_tm, viewport_scale](const Point3 &wpos) {
    const Point4 posCs = Point4::xyz1(wpos) * view_rot_jitter_proj_tm;
    Point2 posVp = Point2::xy(posCs);
    posVp = safediv(posVp, posCs.w);
    posVp = posVp * 0.5f + Point2(0.5f, 0.5f);
    posVp.y = 1.0 - posVp.y;
    posVp.y *= viewport_scale;

    return posVp;
  };

  const auto calcScaledEllipseAxis = [](const Point2 &center, const Point2 &one) {
    const Point2 axis = one - center;
    const float axisLenSq = lengthSq(axis);
    const Point2 axisDirPerLen = safediv(axis, axisLenSq);
    return axisDirPerLen;
  };

  const Point2 centerVp = worldToViewport(centerWpos);
  const Point2 rightVp = worldToViewport(rightWpos);
  const Point2 upVp = worldToViewport(upWpos);

  ViewportEllipse ve;
  ve.center = centerVp;
  ve.xAxis = calcScaledEllipseAxis(centerVp, rightVp);
  ve.yAxis = calcScaledEllipseAxis(centerVp, upVp);

  return ve;
}
} // namespace

void update_transforms(const CameraParams &main_view, const CameraParams &prev_main_view, const CameraParams &lens_view)
{
  const AimRenderingData aimData = get_aim_rendering_data();
  const DynamicRenderableSceneInstance *scene = get_scope_lens(aimData);
  if (!scene)
    return;

  const TMatrix &lensWtm = scene->getNodeWtm(aimData.lensNodeId);
  const TMatrix &lensPrevWtm = scene->getPrevNodeWtm(aimData.lensNodeId);
  const float viewportScale = main_view.noJitterPersp.wk / main_view.noJitterPersp.hk;

  const float lensBoundingSphereRadius = aimData.lensBoundingSphereRadius;
  const ViewportEllipse curFrameEllipse =
    calc_viewport_ellipse(lensBoundingSphereRadius, lensWtm, main_view.viewRotJitterProjTm, viewportScale);
  const ViewportEllipse prevFrameEllipse =
    calc_viewport_ellipse(lensBoundingSphereRadius, lensPrevWtm, prev_main_view.viewRotJitterProjTm, viewportScale);

  const Point4 curFrameAxes{curFrameEllipse.xAxis.x, curFrameEllipse.xAxis.y, curFrameEllipse.yAxis.x, curFrameEllipse.yAxis.y};
  const Point4 prevFrameAxes{prevFrameEllipse.xAxis.x, prevFrameEllipse.xAxis.y, prevFrameEllipse.yAxis.x, prevFrameEllipse.yAxis.y};

  const Point4 uvRemapping = calc_uv_remapping(main_view, lens_view);

  ShaderGlobal::set_float4(var::camera_in_camera_prev_vp_ellipse_center, prevFrameEllipse.center);
  ShaderGlobal::set_float4(var::camera_in_camera_prev_vp_ellipse_xy_axes, prevFrameAxes);
  ShaderGlobal::set_float4(var::camera_in_camera_vp_ellipse_center, curFrameEllipse.center);
  ShaderGlobal::set_float4(var::camera_in_camera_vp_ellipse_xy_axes, curFrameAxes);
  ShaderGlobal::set_float(var::camera_in_camera_vp_y_scale, viewportScale);
  ShaderGlobal::set_float4(var::camera_in_camera_vp_uv_remapping, uvRemapping);

  update_camcam_transforms_ecs_query(*g_entity_mgr,
    [&uvRemapping](Point4 &camcam__uv_remapping) { camcam__uv_remapping = uvRemapping; });
}

static void set_uv_remapping_shvar(const dafg::multiplexing::Index &multiplexing_index)
{
  const bool isMainView = multiplexing_index.subCamera == 0;

  Point4 defVal = Point4(1.0f, 0.0f, 1.0f, 0.0f);
  if (!isMainView)
  {
    get_camcam_uv_remapping_ecs_query(*g_entity_mgr, [&defVal](const Point4 &camcam__uv_remapping) { defVal = camcam__uv_remapping; });
  }

  ShaderGlobal::set_float4(var::camera_in_camera_vp_uv_remapping, defVal);
}

static shaders::OverrideState get_stencil_test_override_state()
{
  shaders::OverrideState st;
  st.set(shaders::OverrideState::STENCIL);
  st.stencil.set(CMPF_EQUAL, STNCLOP_KEEP, STNCLOP_KEEP, STNCLOP_KEEP, 0xFF, 0xFF);
  return st;
}

ActivateOnly::ActivateOnly() { ShaderGlobal::set_float(var::camera_in_camera_active, is_lens_render_active() ? 1.0f : 0.0f); }
ActivateOnly::~ActivateOnly() { ShaderGlobal::set_float(var::camera_in_camera_active, 0.0f); }

RenderMainViewOnly::RenderMainViewOnly(const dafg::multiplexing::Index &index)
{
  isMainView = index.subCamera == 0;
  isCamCamRenderActive = is_lens_render_active();

  ShaderGlobal::set_float(var::camera_in_camera_active, isCamCamRenderActive ? 1.0f : 0.0f);

  if (isCamCamRenderActive && isMainView)
  {
    shaders::set_stencil_ref(0);
    shaders::overrides::set_master_state(get_stencil_test_override_state());
  }
}

RenderMainViewOnly::~RenderMainViewOnly()
{
  ShaderGlobal::set_float(var::camera_in_camera_active, 0.0f);

  if (isCamCamRenderActive && isMainView)
    shaders::overrides::reset_master_state();
}

Color4 replace_color4(const ShaderVariableInfo &shvar, const auto &new_val)
{
  const Color4 oldVal = ShaderGlobal::get_float4(shvar);
  ShaderGlobal::set_float4(shvar, new_val);
  return oldVal;
};


ApplyMasterState::ApplyMasterState(const dafg::multiplexing::Index &index, const OpaqueFlags flags)
{
  set_uv_remapping_shvar(index);
  const bool isCamCamRenderActive = is_lens_render_active();
  hasStencilTest = isCamCamRenderActive && (flags != OpaqueFlags::NoStencil);

  ShaderGlobal::set_float(var::camera_in_camera_active, isCamCamRenderActive ? 1.0f : 0.0f);


  if (hasStencilTest)
  {
    shaders::set_stencil_ref(index.subCamera);
    shaders::overrides::set_master_state(get_stencil_test_override_state());
  }
}

ApplyMasterState::~ApplyMasterState()
{
  ShaderGlobal::set_float(var::camera_in_camera_active, 0.0f);

  if (hasStencilTest)
    shaders::overrides::reset_master_state();
}

ApplyPostfxState::ApplyPostfxState(
  const dafg::multiplexing::Index &multiplexing_index, const CameraParams &view, const bool use_stencil)
{
  set_uv_remapping_shvar(multiplexing_index);

  if (!is_lens_render_active())
    return;

  lensRenderActive = true;
  ShaderGlobal::set_float(var::camera_in_camera_active, 1.0f);

  isMainView = multiplexing_index.subCamera == 0;
  ShaderGlobal::set_float(var::camera_in_camera_postfx_lens_view, isMainView ? 0.0f : 1.0f);

  if (use_stencil)
  {
    hasStencilTest = true;
    shaders::set_stencil_ref(isMainView ? 0 : 1);

    shaders::OverrideState st;
    st.set(shaders::OverrideState::Z_WRITE_DISABLE);
    st.set(shaders::OverrideState::STENCIL);
    st.stencil.set(CMPF_EQUAL, STNCLOP_KEEP, STNCLOP_KEEP, STNCLOP_KEEP, 0xFF, 0xFF);
    shaders::overrides::set_master_state(st);
  }

  if (isMainView)
    return;

  savedProjtmPsf0 = replace_color4(var::projtm_psf_0, Color4(view.jitterProjTm[0]));
  savedProjtmPsf1 = replace_color4(var::projtm_psf_1, Color4(view.jitterProjTm[1]));
  savedProjtmPsf2 = replace_color4(var::projtm_psf_2, Color4(view.jitterProjTm[2]));
  savedProjtmPsf3 = replace_color4(var::projtm_psf_3, Color4(view.jitterProjTm[3]));

  const TMatrix4 globtm = view.jitterGlobtm.transpose();
  savedGlobtmPsf0 = replace_color4(var::globtm_psf_0, Color4(globtm[0]));
  savedGlobtmPsf1 = replace_color4(var::globtm_psf_1, Color4(globtm[1]));
  savedGlobtmPsf2 = replace_color4(var::globtm_psf_2, Color4(globtm[2]));
  savedGlobtmPsf3 = replace_color4(var::globtm_psf_3, Color4(globtm[3]));

  const TMatrix4 globtmNoOfs = view.viewRotJitterProjTm.transpose();
  savedGlobtmNoOfsPsf0 = replace_color4(var::globtm_no_ofs_psf_0, Color4(globtmNoOfs[0]));
  savedGlobtmNoOfsPsf1 = replace_color4(var::globtm_no_ofs_psf_1, Color4(globtmNoOfs[1]));
  savedGlobtmNoOfsPsf2 = replace_color4(var::globtm_no_ofs_psf_2, Color4(globtmNoOfs[2]));
  savedGlobtmNoOfsPsf3 = replace_color4(var::globtm_no_ofs_psf_3, Color4(globtmNoOfs[3]));
}

ApplyPostfxState::~ApplyPostfxState()
{
  ShaderGlobal::set_float(var::camera_in_camera_active, 0.0f);
  ShaderGlobal::set_float(var::camera_in_camera_postfx_lens_view, 0.0f);

  if (hasStencilTest)
    shaders::overrides::reset_master_state();

  if (isMainView)
    return;

  ShaderGlobal::set_float4(var::projtm_psf_0, savedProjtmPsf0);
  ShaderGlobal::set_float4(var::projtm_psf_1, savedProjtmPsf1);
  ShaderGlobal::set_float4(var::projtm_psf_2, savedProjtmPsf2);
  ShaderGlobal::set_float4(var::projtm_psf_3, savedProjtmPsf3);

  ShaderGlobal::set_float4(var::globtm_psf_0, savedGlobtmPsf0);
  ShaderGlobal::set_float4(var::globtm_psf_1, savedGlobtmPsf1);
  ShaderGlobal::set_float4(var::globtm_psf_2, savedGlobtmPsf2);
  ShaderGlobal::set_float4(var::globtm_psf_3, savedGlobtmPsf3);

  ShaderGlobal::set_float4(var::globtm_no_ofs_psf_0, savedGlobtmNoOfsPsf0);
  ShaderGlobal::set_float4(var::globtm_no_ofs_psf_1, savedGlobtmNoOfsPsf1);
  ShaderGlobal::set_float4(var::globtm_no_ofs_psf_2, savedGlobtmNoOfsPsf2);
  ShaderGlobal::set_float4(var::globtm_no_ofs_psf_3, savedGlobtmNoOfsPsf3);

  if (hasPrevState)
  {
    ShaderGlobal::set_float4(var::prev_globtm_psf_0, savedPrevGlobtmPsf0);
    ShaderGlobal::set_float4(var::prev_globtm_psf_1, savedPrevGlobtmPsf1);
    ShaderGlobal::set_float4(var::prev_globtm_psf_2, savedPrevGlobtmPsf2);
    ShaderGlobal::set_float4(var::prev_globtm_psf_3, savedPrevGlobtmPsf3);

    ShaderGlobal::set_float4(var::prev_globtm_no_ofs_psf_0, savedPrevGlobtmNoOfsPsf0);
    ShaderGlobal::set_float4(var::prev_globtm_no_ofs_psf_1, savedPrevGlobtmNoOfsPsf1);
    ShaderGlobal::set_float4(var::prev_globtm_no_ofs_psf_2, savedPrevGlobtmNoOfsPsf2);
    ShaderGlobal::set_float4(var::prev_globtm_no_ofs_psf_3, savedPrevGlobtmNoOfsPsf3);
  }
}

ApplyPostfxState::ApplyPostfxState(
  const dafg::multiplexing::Index &multiplexing_index, const CameraParams &cur_view, const CameraParams &prev_view, bool use_stencil) :
  ApplyPostfxState(multiplexing_index, cur_view, use_stencil)
{
  if (!lensRenderActive || isMainView)
    return;

  const TMatrix4 prevGlobtm = prev_view.jitterGlobtm.transpose();
  savedPrevGlobtmPsf0 = replace_color4(var::prev_globtm_psf_0, Color4(prevGlobtm[0]));
  savedPrevGlobtmPsf1 = replace_color4(var::prev_globtm_psf_1, Color4(prevGlobtm[1]));
  savedPrevGlobtmPsf2 = replace_color4(var::prev_globtm_psf_2, Color4(prevGlobtm[2]));
  savedPrevGlobtmPsf3 = replace_color4(var::prev_globtm_psf_3, Color4(prevGlobtm[3]));

  const TMatrix4 prevGlobtmNoOfs = prev_view.viewRotJitterProjTm.transpose();
  savedPrevGlobtmNoOfsPsf0 = replace_color4(var::prev_globtm_no_ofs_psf_0, Color4(prevGlobtmNoOfs[0]));
  savedPrevGlobtmNoOfsPsf1 = replace_color4(var::prev_globtm_no_ofs_psf_1, Color4(prevGlobtmNoOfs[1]));
  savedPrevGlobtmNoOfsPsf2 = replace_color4(var::prev_globtm_no_ofs_psf_2, Color4(prevGlobtmNoOfs[2]));
  savedPrevGlobtmNoOfsPsf3 = replace_color4(var::prev_globtm_no_ofs_psf_3, Color4(prevGlobtmNoOfs[3]));

  hasPrevState = true;
}
} // namespace camera_in_camera

ECS_TAG(render)
ECS_BEFORE(camera_set_sync)
ECS_BEFORE(camcam_activate_view_es)
static void camcam_preprocess_prev_frame_weapon_es(const ecs::UpdateStageInfoAct &, bool &camcam__lens_only_zoom_enabled)
{
  const bool hasCamCam = renderer_has_feature(CAMERA_IN_CAMERA);
  const AimRenderingData aimData = get_aim_rendering_data();

  const bool scopeDisablesCamCam = camera_in_camera::check_if_scope_disables_camcam(aimData.entityWithScopeLensEid);
  const bool featureEnabledForScope = hasCamCam && !scopeDisablesCamCam;

  camcam__lens_only_zoom_enabled = featureEnabledForScope;
}

ECS_TAG(render)
ECS_BEFORE(camera_update_lods_scaling_es)
ECS_AFTER(update_shooter_camera_aim_parameters_es)
static void camcam_activate_view_es(const ecs::UpdateStageInfoAct &) { camera_in_camera::activate_view(); }