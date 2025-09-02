// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "cameraInCamera.h"

#include <ecs/anim/anim.h>
#include <ecs/core/entityManager.h>
#include <ecs/phys/collRes.h>
#include <ecs/render/updateStageRender.h>

#include <render/daFrameGraph/multiplexing.h>
#include <render/viewVecs.h>
#include <shaders/dag_dynSceneRes.h>

#include <render/rendererFeatures.h>
#include <render/world/aimRender.h>
#define INSIDE_RENDERER 1
#include <render/world/private_worldRenderer.h>


template <typename Callable>
static inline void update_camcam_transforms_ecs_query(Callable);

template <typename Callable>
static inline void update_camcam_state_ecs_query(Callable);

template <typename Callable>
static inline void get_camcam_uv_remapping_ecs_query(Callable);

template <typename Callable>
static inline void get_camcam_render_state_ecs_query(Callable);

template <typename Callable>
static inline void get_camcam_lens_only_zoom_state_ecs_query(Callable);

template <typename Callable>
static inline void get_scope_animchar_and_colres_ecs_query(ecs::EntityId, Callable);

template <typename Callable>
static inline void vaidate_scope_ecs_query(ecs::EntityId, Callable);

template <typename Callable>
static inline void check_if_scope_disables_camcam_ecs_query(ecs::EntityId, Callable);

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
static ShaderVariableInfo view_vecLT("view_vecLT", true);
static ShaderVariableInfo view_vecRT("view_vecRT", true);
static ShaderVariableInfo view_vecLB("view_vecLB", true);
static ShaderVariableInfo view_vecRB("view_vecRB", true);
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

  ShaderGlobal::set_real(var::camera_in_camera_active, 0.0f);
}

bool activate_view()
{
  bool isActiveVal = false;

  update_camcam_state_ecs_query([&isActiveVal](bool &camcam__lens_render_active, bool &camcam__lens_only_zoom_enabled,
                                  bool *camcam__has_pending_validation = nullptr) {
    const bool hasCamCam = renderer_has_feature(CAMERA_IN_CAMERA);
    const AimRenderingData aimData = get_aim_rendering_data();

    bool scopeDisablesCamCam = false;
    check_if_scope_disables_camcam_ecs_query(aimData.entityWithScopeLensEid,
      [&scopeDisablesCamCam](const bool gunmod__lensOnlyZoomDisabled) { scopeDisablesCamCam = gunmod__lensOnlyZoomDisabled; });

    const bool featureEnabledForScope = hasCamCam && !scopeDisablesCamCam;

    camcam__lens_render_active = featureEnabledForScope && aimData.lensRenderEnabled;
    camcam__lens_only_zoom_enabled = featureEnabledForScope && aimData.entityWithScopeLensEid;

    if (camcam__has_pending_validation)
      *camcam__has_pending_validation = camcam__lens_only_zoom_enabled;

    isActiveVal = camcam__lens_render_active;
  });

  ShaderGlobal::set_real(var::camera_in_camera_active, isActiveVal ? 1.0f : 0.0f);

  return isActiveVal;
}

bool is_lens_render_active()
{
  bool active = false;
  get_camcam_render_state_ecs_query([&](bool camcam__lens_render_active) { active = camcam__lens_render_active; });
  return active;
}

bool is_lens_only_zoom_enabled()
{
  bool enabled = false;
  get_camcam_lens_only_zoom_state_ecs_query([&](bool camcam__lens_only_zoom_enabled) { enabled = camcam__lens_only_zoom_enabled; });
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
  AimRenderingData aimData = get_aim_rendering_data();
  if (!aimData.entityWithScopeLensEid)
    return;

  const AnimV20::AnimcharRendComponent *lensAnimcharRender = nullptr;
  const CollisionResource *collision = nullptr;

  get_scope_animchar_and_colres_ecs_query(aimData.entityWithScopeLensEid,
    [&](const AnimV20::AnimcharRendComponent &animchar_render, const CollisionResource *collres = nullptr) {
      lensAnimcharRender = &animchar_render;
      collision = collres;
    });

  if (!lensAnimcharRender)
    return;

  const DynamicRenderableSceneInstance *scene = lensAnimcharRender->getSceneInstance();
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

  ShaderGlobal::set_color4(var::camera_in_camera_prev_vp_ellipse_center, prevFrameEllipse.center);
  ShaderGlobal::set_color4(var::camera_in_camera_prev_vp_ellipse_xy_axes, prevFrameAxes);
  ShaderGlobal::set_color4(var::camera_in_camera_vp_ellipse_center, curFrameEllipse.center);
  ShaderGlobal::set_color4(var::camera_in_camera_vp_ellipse_xy_axes, curFrameAxes);
  ShaderGlobal::set_real(var::camera_in_camera_vp_y_scale, viewportScale);
  ShaderGlobal::set_color4(var::camera_in_camera_vp_uv_remapping, uvRemapping);

  update_camcam_transforms_ecs_query([&uvRemapping](Point4 &camcam__uv_remapping) { camcam__uv_remapping = uvRemapping; });
}

static void set_uv_remapping_shvar(const dafg::multiplexing::Index &multiplexing_index)
{
  const bool isMainView = multiplexing_index.subCamera == 0;

  Point4 defVal = Point4(1.0f, 0.0f, 1.0f, 0.0f);
  if (!isMainView)
  {
    get_camcam_uv_remapping_ecs_query([&defVal](const Point4 &camcam__uv_remapping) { defVal = camcam__uv_remapping; });
  }

  ShaderGlobal::set_color4(var::camera_in_camera_vp_uv_remapping, defVal);
}

static shaders::OverrideState get_stencil_test_override_state()
{
  shaders::OverrideState st;
  st.set(shaders::OverrideState::STENCIL);
  st.stencil.set(CMPF_EQUAL, STNCLOP_KEEP, STNCLOP_KEEP, STNCLOP_KEEP, 0xFF, 0xFF);
  return st;
}

RenderMainViewOnly::RenderMainViewOnly(const dafg::multiplexing::Index &index)
{
  isMainView = index.subCamera == 0;
  isCamCamRenderActive = is_lens_render_active();


  if (isCamCamRenderActive && isMainView)
  {
    shaders::set_stencil_ref(0);
    shaders::overrides::set_master_state(get_stencil_test_override_state());
  }
}

RenderMainViewOnly::~RenderMainViewOnly()
{
  if (isCamCamRenderActive && isMainView)
    shaders::overrides::reset_master_state();
}

Color4 replace_color4(const ShaderVariableInfo &shvar, const auto &new_val)
{
  const Color4 oldVal = ShaderGlobal::get_color4(shvar);
  ShaderGlobal::set_color4(shvar, new_val);
  return oldVal;
};


ApplyMasterState::ApplyMasterState(const dafg::multiplexing::Index &index)
{
  set_uv_remapping_shvar(index);
  hasStencilTest = is_lens_render_active();

  if (hasStencilTest)
  {
    shaders::set_stencil_ref(index.subCamera);
    shaders::overrides::set_master_state(get_stencil_test_override_state());
  }
}

ApplyMasterState::ApplyMasterState(const dafg::multiplexing::Index &index, const CameraParams &view) : ApplyMasterState(index)
{
  if (!hasStencilTest)
    return;

  const ViewVecs viewVecs = calc_view_vecs(view.viewTm, view.jitterProjTm);
  savedViewVecLT = replace_color4(var::view_vecLT, viewVecs.viewVecLT);
  savedViewVecRT = replace_color4(var::view_vecRT, viewVecs.viewVecRT);
  savedViewVecLB = replace_color4(var::view_vecLB, viewVecs.viewVecLB);
  savedViewVecRB = replace_color4(var::view_vecRB, viewVecs.viewVecRB);

  hasSavedState = true;
}

ApplyMasterState::~ApplyMasterState()
{
  if (hasStencilTest)
    shaders::overrides::reset_master_state();

  if (hasSavedState)
  {
    ShaderGlobal::set_color4(var::view_vecLT, savedViewVecLT);
    ShaderGlobal::set_color4(var::view_vecRT, savedViewVecRT);
    ShaderGlobal::set_color4(var::view_vecLB, savedViewVecLB);
    ShaderGlobal::set_color4(var::view_vecRB, savedViewVecRB);
  }
}

ApplyPostfxState::ApplyPostfxState(
  const dafg::multiplexing::Index &multiplexing_index, const CameraParams &view, const bool use_stencil)
{
  set_uv_remapping_shvar(multiplexing_index);

  if (!is_lens_render_active())
    return;

  lensRenderActive = true;

  isMainView = multiplexing_index.subCamera == 0;
  ShaderGlobal::set_real(var::camera_in_camera_postfx_lens_view, isMainView ? 0.0f : 1.0f);

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

  const ViewVecs viewVecs = calc_view_vecs(view.viewTm, view.jitterProjTm);
  savedViewVecLT = replace_color4(var::view_vecLT, viewVecs.viewVecLT);
  savedViewVecRT = replace_color4(var::view_vecRT, viewVecs.viewVecRT);
  savedViewVecLB = replace_color4(var::view_vecLB, viewVecs.viewVecLB);
  savedViewVecRB = replace_color4(var::view_vecRB, viewVecs.viewVecRB);
}

ApplyPostfxState::~ApplyPostfxState()
{
  ShaderGlobal::set_real(var::camera_in_camera_postfx_lens_view, 0.0f);

  if (hasStencilTest)
    shaders::overrides::reset_master_state();

  if (isMainView)
    return;

  ShaderGlobal::set_color4(var::projtm_psf_0, savedProjtmPsf0);
  ShaderGlobal::set_color4(var::projtm_psf_1, savedProjtmPsf1);
  ShaderGlobal::set_color4(var::projtm_psf_2, savedProjtmPsf2);
  ShaderGlobal::set_color4(var::projtm_psf_3, savedProjtmPsf3);

  ShaderGlobal::set_color4(var::globtm_psf_0, savedGlobtmPsf0);
  ShaderGlobal::set_color4(var::globtm_psf_1, savedGlobtmPsf1);
  ShaderGlobal::set_color4(var::globtm_psf_2, savedGlobtmPsf2);
  ShaderGlobal::set_color4(var::globtm_psf_3, savedGlobtmPsf3);

  ShaderGlobal::set_color4(var::globtm_no_ofs_psf_0, savedGlobtmNoOfsPsf0);
  ShaderGlobal::set_color4(var::globtm_no_ofs_psf_1, savedGlobtmNoOfsPsf1);
  ShaderGlobal::set_color4(var::globtm_no_ofs_psf_2, savedGlobtmNoOfsPsf2);
  ShaderGlobal::set_color4(var::globtm_no_ofs_psf_3, savedGlobtmNoOfsPsf3);

  ShaderGlobal::set_color4(var::view_vecLT, savedViewVecLT);
  ShaderGlobal::set_color4(var::view_vecRT, savedViewVecRT);
  ShaderGlobal::set_color4(var::view_vecLB, savedViewVecLB);
  ShaderGlobal::set_color4(var::view_vecRB, savedViewVecRB);

  if (hasPrevState)
  {
    ShaderGlobal::set_color4(var::prev_globtm_psf_0, savedPrevGlobtmPsf0);
    ShaderGlobal::set_color4(var::prev_globtm_psf_1, savedPrevGlobtmPsf1);
    ShaderGlobal::set_color4(var::prev_globtm_psf_2, savedPrevGlobtmPsf2);
    ShaderGlobal::set_color4(var::prev_globtm_psf_3, savedPrevGlobtmPsf3);

    ShaderGlobal::set_color4(var::prev_globtm_no_ofs_psf_0, savedPrevGlobtmNoOfsPsf0);
    ShaderGlobal::set_color4(var::prev_globtm_no_ofs_psf_1, savedPrevGlobtmNoOfsPsf1);
    ShaderGlobal::set_color4(var::prev_globtm_no_ofs_psf_2, savedPrevGlobtmNoOfsPsf2);
    ShaderGlobal::set_color4(var::prev_globtm_no_ofs_psf_3, savedPrevGlobtmNoOfsPsf3);
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

ECS_TAG(dev, render)
ECS_TRACK(camcam__has_pending_validation)
static void camcam_validate_scope_es_event_handler(const ecs::Event &, bool camcam__has_pending_validation)
{
  if (!camcam__has_pending_validation)
    return;

  const AimRenderingData aimData = get_aim_rendering_data();

  const CollisionResource *collision = nullptr;
  const char *collresRes = nullptr;
  const char *animcharRes = nullptr;
  const char *itemName = nullptr;
  const char *lensNode = nullptr;
  bool entityValid = false;

  vaidate_scope_ecs_query(aimData.entityWithScopeLensEid,
    [&](const AnimV20::AnimcharRendComponent &animchar_render, const ecs::string *gunmod__lensNode = nullptr,
      const ecs::string *item__name = nullptr, const ecs::string *animchar__res = nullptr, const CollisionResource *collres = nullptr,
      const ecs::string *collres__res = nullptr) {
      G_UNUSED(animchar_render);
      collision = collres;
      collresRes = collres__res ? collres__res->c_str() : nullptr;
      animcharRes = animchar__res ? animchar__res->c_str() : nullptr;
      itemName = item__name ? item__name->c_str() : "<no item__name>";
      lensNode = gunmod__lensNode ? gunmod__lensNode->c_str() : nullptr;
      entityValid = true;
    });

  if (!entityValid)
    return;

  eastl::string error;
  if (!lensNode)
  {
    error.append("  [>>>Contant Gameplay-Team<<<]: template misses 'gunmod__lensNode:t'");
  }
  else if (aimData.lensCollisionNodeId < 0)
  {
    if (!collision)
      error.append("  [>>>Contant Gameplay-Team<<<]: entity template misses 'collres{}'\n");
    else
    {
      if (collresRes == nullptr)
        error.append("  [>>>Contant Gameplay-Team<<<]: entity template misses 'collres__res:t'\n");
      else
        error.append_sprintf(
          "  [>>>Contact Artists-Team<<<]: collres with name '%s' doesn't have node '%s' with 'collision:t=sphere'\n", collresRes,
          lensNode);
    }
  }
  else
  {
    G_ASSERT_RETURN(collision, );
    const CollisionNode *cn = collision->getNode(aimData.lensCollisionNodeId);
    const bool isSphereType = cn->type == COLLISION_NODE_TYPE_SPHERE;
    const bool hasBehavaiorFlags = cn->behaviorFlags != 0;

    if (hasBehavaiorFlags)
      error.append_sprintf(
        "  [>>>Contact Artists-Team<<<]: collres with name '%s' has incorrect object properties in node '%s'. It must NOT "
        "enable any collision behavior "
        "(trace - no, phys - no etc)",
        collresRes, lensNode);

    if (!isSphereType)
    {
      error.append_sprintf(
        "  [>>>Contact Artists-Team<<<]: collres with name '%s' has incorrect collision in node '%s', it must declare "
        "collision:t=sphere (trace - no, phys - no etc)\n",
        collresRes, lensNode);
    }
  }

  if (!error.empty())
    logerr("Sniper Scope [item__name:%d] validation failure: \n%s", itemName, error.c_str());
}