// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "scopeAimRender.h"

#include <3d/dag_quadIndexBuffer.h>
#include <camera/sceneCam.h>
#include <animChar/dag_animCharVis.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/dataComponent.h>
#include <daECS/core/entitySystem.h>
#include <ecs/anim/anim.h>
#include <ecs/anim/animchar_visbits.h>
#include <ecs/camera/getActiveCameraSetup.h>
#include <shaders/dag_dynSceneRes.h>
#include <ecs/render/resPtr.h>
#include <ecs/render/shaders.h>
#include <ecs/render/shaderVar.h>
#include <ecs/render/updateStageRender.h>
#include <game/player.h>

#include <render/deferredRenderer.h>
#include <render/dof/dof_ps.h>
#include <render/rendererFeatures.h>
#include <render/renderEvent.h>
#include <render/world/dynModelRenderPass.h>
#include <render/world/dynModelRenderer.h>
#include <render/world/global_vars.h>
#include <render/world/postfxRender.h>
#include <scope/shaders/scope_mask_common.hlsli>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_shaderBlock.h>
#include <util/dag_convar.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <math/dag_TMatrix4D.h>

#define SCOPE_AIM_RENDER_VARS \
  VAR(lens_zoom_factor)       \
  VAR(scope_mask)             \
  VAR(scope_lens_local_x)     \
  VAR(scope_lens_local_y)     \
  VAR(scope_lens_local_z)     \
  VAR(scope_reticle_tex)      \
  VAR(scope_radius)           \
  VAR(scope_length)           \
  VAR(night_vision_tex)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
SCOPE_AIM_RENDER_VARS
#undef VAR

using namespace dynmodel_renderer;

template <typename Callable>
inline void set_scope_lens_phys_params_ecs_query(ecs::EntityId eid, Callable c);

template <typename Callable>
inline void render_scope_reticle_ecs_query(ecs::EntityId eid, Callable c);

template <typename Callable>
inline void process_animchar_ecs_query(ecs::EntityId eid, Callable c);

template <typename Callable>
inline void reprojection_cockpit_data_ecs_query(ecs::EntityId eid, Callable c);

template <typename Callable>
inline void put_into_occlusion_ecs_query(Callable c);

template <typename Callable>
inline bool get_rearsight_dist_ecs_query(ecs::EntityId eid, Callable c);

template <typename Callable>
inline bool get_aim_dof_weap_ecs_query(ecs::EntityId eid, Callable c);

template <typename Callable>
inline bool get_aim_dof_scope_ecs_query(ecs::EntityId eid, Callable c);

template <typename Callable>
inline ecs::QueryCbResult cockpit_camera_ecs_query(Callable c);

template <typename Callable>
inline void enable_scope_lod_change_ecs_query(ecs::EntityId eid, Callable c);

template <typename Callable>
inline void enable_scope_ri_lod_change_ecs_query(ecs::EntityId eid, Callable c);

template <typename Callable>
inline static void get_dof_entity_ecs_query(Callable c);

template <typename Callable>
inline static void set_scope_lens_reticle_ecs_query(ecs::EntityId eid, Callable c);

template <typename Callable>
inline static void nightvision_lens_ecs_query(ecs::EntityId eid, Callable c);

template <typename Callable>
inline static void get_hero_cockpit_entities_with_prepass_flag_ecs_query(Callable c);

template <typename Callable>
inline static void prepare_scope_aim_rendering_data_ecs_query(Callable c);

template <typename Callable>
inline void in_vehicle_cockpit_ecs_query(Callable c);

extern int dynamicSceneBlockId;
extern int dynamicDepthSceneBlockId;
extern int dynamicSceneTransBlockId;

extern ConVarT<bool, false> vrs_dof;

ECS_AUTO_REGISTER_COMPONENT(mat44f, "weapon_rearsight_node__nodeTm", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(mat44f, "close_geometry_prev_to_curr_frame_transform", nullptr, 0);


enum
{
  LENS_DETAIL_FULL_NIGHT_VISON = -1,
  LENS_DETAIL_FULL = 0,
  LENS_DETAIL_REFLECTIONS
};
enum
{
  LENS_RENDER_OPTICS = 0,
  LENS_RENDER_FRAME,
  LENS_RENDER_DEPTH,
  LENS_RENDER_MASK,
  LENS_RENDER_DEPTH_BY_MASK
};

static void set_scope_nightvision_tex(ecs::EntityId entity_with_scope_lens)
{
  nightvision_lens_ecs_query(entity_with_scope_lens,
    [&](const SharedTex &gunmod__nightvisionTex) { ShaderGlobal::set_texture(night_vision_texVarId, gunmod__nightvisionTex); });
}

static void reset_scope_nightvision_tex() { ShaderGlobal::set_texture(night_vision_texVarId, BAD_TEXTUREID); }

static void set_scope_lens_phys_params(
  ecs::EntityId entity_with_scope_lens_eid, int lens_node_id, float scope_radius, float scope_length)
{
  set_scope_lens_phys_params_ecs_query(entity_with_scope_lens_eid, [&](AnimV20::AnimcharRendComponent &animchar_render) {
    auto scene = animchar_render.getSceneInstance();
    auto sceneResource = scene->getCurSceneResource();
    if (!sceneResource) // It's might be nullptr until first call to `setLod/chooseLod[ByDistSq]`
      return;
    for (auto &o : sceneResource->getRigidsConst())
    {
      if (o.nodeId != lens_node_id)
        continue;

      for (auto &elem : o.mesh->getMesh()->getElems(ShaderMesh::STG_trans))
      {
        float currentScopeRadius = 0.0f, currentScopeLength = 0.0f;
        if (elem.mat->getRealVariable(scope_radiusVarId.get_var_id(), currentScopeRadius) && currentScopeRadius != scope_radius)
          elem.mat->set_real_param(scope_radiusVarId.get_var_id(), scope_radius);
        if (elem.mat->getRealVariable(scope_lengthVarId.get_var_id(), currentScopeLength) && currentScopeLength != scope_length)
          elem.mat->set_real_param(scope_lengthVarId.get_var_id(), scope_length);
      }
    }
  });
}

static inline bool in_vehicle_cockpit()
{
  bool res = false;
  in_vehicle_cockpit_ecs_query([&](bool cockpit__isHeroInCockpit ECS_REQUIRE(ecs::Tag vehicleWithWatched)) {
    res = cockpit__isHeroInCockpit;
    return ecs::QueryCbResult::Stop;
  });
  return res;
}

ECS_TAG(render)
ECS_REQUIRE(eastl::true_type camera__active)
ECS_BEFORE(start_occlusion_and_sw_raster_es)
static void prepare_aim_occlusion_es(
  const UpdateStageInfoBeforeRender &info, const ecs::EntityId aim_data__gunEid, const bool aim_data__isAiming)
{
  bool needCockpitReprojection = false;
  get_hero_cockpit_entities_with_prepass_flag_ecs_query(
    [&](ecs::EidList &hero_cockpit_entities, bool &render_hero_cockpit_into_early_prepass) {
      needCockpitReprojection = (!hero_cockpit_entities.empty() && render_hero_cockpit_into_early_prepass);
    });

  if (!(needCockpitReprojection || aim_data__isAiming) || in_vehicle_cockpit())
    return;
  mat44f gunPrevToCurrFrameTM;
  float cockpitBoundingSphereRadius;
  bool occlusionIsReady = false;
  reprojection_cockpit_data_ecs_query(aim_data__gunEid, [&](mat44f weapon_rearsight_node__nodeTm, int weapon_rearsight_node__nodeIdx) {
    if (weapon_rearsight_node__nodeIdx < 0)
      return;

    // Take the rearsight position to correctly adjust cockpit distance in reprojection.
    Point3_vec4 rearsightNodePos;
    v_st(&rearsightNodePos.x, weapon_rearsight_node__nodeTm.col3);

    static mat44f oldGunTm = weapon_rearsight_node__nodeTm;
    mat44f invOldGunTm;
    v_mat44_inverse43(invOldGunTm, oldGunTm);
    v_mat44_mul43(gunPrevToCurrFrameTM, weapon_rearsight_node__nodeTm, invOldGunTm);
    oldGunTm = weapon_rearsight_node__nodeTm;

    // Bias for cockpit to account for rest of the gun on screen behind scope
    const float REAR_SIGHT_TO_BARREL_END_DISTANCE_BIAS = 0.5f;
    cockpitBoundingSphereRadius = length(rearsightNodePos - info.camPos) + REAR_SIGHT_TO_BARREL_END_DISTANCE_BIAS;
    occlusionIsReady = true;
  });

  put_into_occlusion_ecs_query(
    [&](float &close_geometry_bounding_radius, mat44f &close_geometry_prev_to_curr_frame_transform, bool &occlusion_available) {
      G_ASSERT(!occlusion_available);
      close_geometry_bounding_radius = cockpitBoundingSphereRadius;
      close_geometry_prev_to_curr_frame_transform = (mat44f_cref)gunPrevToCurrFrameTM;
      occlusion_available = occlusionIsReady;
    });
}

static void process_animchars_and_render(dag::ConstSpan<ecs::EntityId> eids,
  uint32_t startStage,
  uint32_t endStage,
  int blockId,
  uint8_t visMask,
  bool hide,
  bool need_previous_matrices,
  const TexStreamingContext &texCtx,
  int nodeId = -1)
{
  DynModelRenderingState &state = dynmodel_renderer::get_immediate_state();
  for (ecs::EntityId eid : eids)
  {
    if (eid) // Investigate: does this check really needed?
      process_animchar_ecs_query(eid,
        [&state, startStage, endStage, nodeId, visMask, hide, need_previous_matrices, texCtx](
          AnimV20::AnimcharRendComponent &animchar_render, uint8_t &animchar_visbits,
          const ecs::Point4List *additional_data ECS_REQUIRE(eastl::true_type animchar_render__enabled = true)) {
          // VISFLG_COCKPIT_VISIBLE not connected with cockpit render here.
          // Just if there is only VISFLG_COCKPIT_VISIBLE the animchar will
          // be invisible for main animchar render (what we need). Also cockpit is disabled when
          // scope render is enabled. So we can freely use the flag as a filter for visible objects
          // when render occurs several time per frame (for screenshoots now).
          if ((animchar_visbits & visMask) == visMask || animchar_visbits == VISFLG_COCKPIT_VISIBLE)
          {
            Point4 zero(0, 0, 0, 0);
            const bool noAdditionalData = additional_data == nullptr || additional_data->empty();
            const Point4 *additionalData = noAdditionalData ? &zero : additional_data->data();
            const uint32_t additionalDataSize = noAdditionalData ? 1 : additional_data->size();
            bool needImmediateConstPS = startStage == ShaderMesh::STG_opaque; // for customization decals on planes
            auto scene = animchar_render.getSceneInstance();
            if (DAGOR_LIKELY(nodeId < 0))
            {
              state.process_animchar(startStage, endStage, scene, additionalData, additionalDataSize, need_previous_matrices, nullptr,
                nullptr, 0, 0, needImmediateConstPS, RenderPriority::DEFAULT, nullptr, texCtx);
            }
            else
            {
              eastl::bitset<256> nodeVisibility; // nodes count for scopes is small
              for (int i = 0, n = scene->getNodeCount(); i < n; i++)
              {
                nodeVisibility.set(i, !scene->getNodeHidden(i));
                scene->showNode(i, i == nodeId);
              }
              state.process_animchar(startStage, endStage, scene, additionalData, additionalDataSize, need_previous_matrices, nullptr,
                nullptr, 0, 0, needImmediateConstPS, RenderPriority::DEFAULT, nullptr, texCtx);
              for (int i = 0, n = scene->getNodeCount(); i < n; i++)
                scene->showNode(i, nodeVisibility[i]);
            }
            if (hide)
              animchar_visbits = VISFLG_COCKPIT_VISIBLE;
          }
        });
  }

  state.prepareForRender();
  const DynamicBufferHolder *buffer = state.requestBuffer(BufferType::MAIN);
  if (!buffer)
    return;

  state.setVars(buffer->buffer.getBufId());
  SCENE_LAYER_GUARD(blockId);
  state.render(buffer->curOffset);
}

static __forceinline void render_scope_trans(ecs::EntityId eid, int nodeId, const TexStreamingContext &texCtx)
{
  G_ASSERT(nodeId >= 0);
  process_animchars_and_render(make_span_const(&eid, 1), ShaderMesh::STG_trans, ShaderMesh::STG_trans, dynamicSceneTransBlockId, 0,
    false, false, texCtx, nodeId);
}

static void render_scope_reticle_quad(ecs::EntityId eid, int lens_node_id)
{
  render_scope_reticle_ecs_query(eid,
    [&](AnimV20::AnimcharRendComponent &animchar_render, Point2 gunmod__scopeRadLen, const SharedTex &gunmod__reticleTex,
      const ShaderVar &scope_reticle_world_tm_0, const ShaderVar &scope_reticle_world_tm_1, const ShaderVar &scope_reticle_world_tm_2,
      const ShaderVar &scope_reticle_radius,
      const ShadersECS &scope_reticle_shader ECS_REQUIRE(eastl::true_type animchar_render__enabled = true)) {
      if (!scope_reticle_shader)
        return;

      auto scene = animchar_render.getSceneInstance();
      TMatrix worldTm = scene->getNodeWtm(lens_node_id);
      Point4 row0(worldTm[0][0], worldTm[1][0], worldTm[2][0], worldTm[3][0]);
      Point4 row1(worldTm[0][1], worldTm[1][1], worldTm[2][1], worldTm[3][1]);
      Point4 row2(worldTm[0][2], worldTm[1][2], worldTm[2][2], worldTm[3][2]);

      ShaderGlobal::set_color4(scope_reticle_world_tm_0, row0);
      ShaderGlobal::set_color4(scope_reticle_world_tm_1, row1);
      ShaderGlobal::set_color4(scope_reticle_world_tm_2, row2);

      ShaderGlobal::set_real(scope_reticle_radius, gunmod__scopeRadLen.x);
      ShaderGlobal::set_texture(scope_reticle_texVarId, gunmod__reticleTex);

      index_buffer::use_quads_16bit();
      scope_reticle_shader.shElem->setStates();
      d3d::setvsrc(0, 0, 0);
      d3d::drawind(PRIM_TRILIST, 0, 2, 0);
    });
}

static void render_scope_crosshair(const ScopeAimRenderingData &scopeAimData, const TexStreamingContext &texCtx)
{
  if (scopeAimData.crosshairNodeId >= 0)
    render_scope_trans(scopeAimData.entityWithScopeLensEid, scopeAimData.crosshairNodeId, texCtx);
  else
    render_scope_reticle_quad(scopeAimData.entityWithScopeLensEid, scopeAimData.lensNodeId);
}

void render_scope_prepass(const ScopeAimRenderingData &scopeAimData, const TexStreamingContext &texCtx)
{
  TIME_D3D_PROFILE(render_scope_prepass);
  STATE_GUARD_0(ShaderGlobal::set_int(is_hero_cockpitVarId, VALUE), 1);
  ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Depth));
  process_animchars_and_render(make_span_const(scopeAimData.scopeLensCockpitEntities), ShaderMesh::STG_opaque, ShaderMesh::STG_atest,
    dynamicDepthSceneBlockId, VISFLG_MAIN_AND_SHADOW_VISIBLE, true, true, texCtx);
}

void render_scope(const ScopeAimRenderingData &scopeAimData, const TexStreamingContext &texCtx)
{
  TIME_D3D_PROFILE(render_scope);
  STATE_GUARD_0(ShaderGlobal::set_int(is_hero_cockpitVarId, VALUE), 1);
  ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Color));
  process_animchars_and_render(make_span_const(scopeAimData.scopeLensCockpitEntities), ShaderMesh::STG_opaque, ShaderMesh::STG_atest,
    dynamicSceneBlockId, VISFLG_MAIN_AND_SHADOW_VISIBLE, true, true, texCtx);
}

void render_scope_lens_mask(const ScopeAimRenderingData &scopeAimData, const TexStreamingContext &texCtx)
{
  TIME_D3D_PROFILE(render_scope_lens);
  ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Depth));
  ShaderGlobal::set_int(lens_render_modeVarId, LENS_RENDER_MASK);
  ShaderGlobal::set_int(lens_detail_levelVarId, LENS_DETAIL_REFLECTIONS);
  render_scope_trans(scopeAimData.entityWithScopeLensEid, scopeAimData.lensNodeId, texCtx);
}

void render_scope_lens_hole(const ScopeAimRenderingData &scopeAimData, const bool by_mask, const TexStreamingContext &texCtx)
{
  TIME_D3D_PROFILE(cut_lens_depth_in_scope);
  ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Depth));
  ShaderGlobal::set_int(lens_render_modeVarId, by_mask ? LENS_RENDER_DEPTH_BY_MASK : LENS_RENDER_DEPTH);
  render_scope_trans(scopeAimData.entityWithScopeLensEid, scopeAimData.lensNodeId, texCtx);

  ShaderGlobal::set_int(lens_render_modeVarId, LENS_RENDER_OPTICS);
}

void prepare_scope_vrs_mask(ComputeShaderElement *scope_vrs_mask_cs, Texture *scope_vrs_mask_tex, TEXTUREID depth)
{
  TIME_D3D_PROFILE(prepare_scope_vrs_mask);
  TextureInfo info;
  scope_vrs_mask_tex->getinfo(info);
  static int scope_vrs_mask_tex_uav_no = ShaderGlobal::get_slot_by_name("scope_vrs_mask_tex_uav_no");
  STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, scope_vrs_mask_tex_uav_no, VALUE, 0, 0), scope_vrs_mask_tex);
  ShaderGlobal::set_texture(scope_maskVarId, depth);
  scope_vrs_mask_cs->dispatch((info.w - 1) / SCOPE_MASK_NUMTHREADS + 1, (info.h - 1) / SCOPE_MASK_NUMTHREADS + 1, 1);
}

void render_scope_trans_forward(const ScopeAimRenderingData &scopeAimData, const TexStreamingContext &texCtx)
{
  TIME_D3D_PROFILE(lens_render);

  ShaderGlobal::set_int(lens_detail_levelVarId, scopeAimData.nightVision ? LENS_DETAIL_FULL_NIGHT_VISON : LENS_DETAIL_FULL);
  ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Color));

  render_scope_crosshair(scopeAimData, texCtx);

  ShaderGlobal::set_int(lens_detail_levelVarId, LENS_DETAIL_REFLECTIONS);
  ShaderGlobal::set_int(lens_render_modeVarId, LENS_RENDER_OPTICS);
}


void render_lens_frame(
  const ScopeAimRenderingData &scopeAimData, TEXTUREID frameTexId, TEXTUREID lensSourceTexId, const TexStreamingContext &texCtx)
{
  if (scopeAimData.nightVision)
    set_scope_nightvision_tex(scopeAimData.entityWithScopeLensEid);

  ShaderGlobal::set_int(lens_detail_levelVarId, scopeAimData.nightVision ? LENS_DETAIL_FULL_NIGHT_VISON : LENS_DETAIL_FULL);
  ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Color));

  // What is seen through scope might differ from what is actually behind it (e.g., in case of thermal vision).
  // In such a case, the scope should be filled with what has already been rendered as a source image for it.
  // TODO: rewrite this using ECS and dabfg::NamedSlot()
  if (lensSourceTexId == BAD_TEXTUREID)
    ShaderGlobal::set_texture(lens_frame_texVarId, frameTexId);
  else
    ShaderGlobal::set_texture(lens_frame_texVarId, lensSourceTexId);
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    ShaderGlobal::set_sampler(lens_frame_tex_samplerstateVarId, d3d::request_sampler(smpInfo));
  }

  ShaderGlobal::set_int(lens_render_modeVarId, LENS_RENDER_FRAME);
  // Just copy frame in the area of lens.
  render_scope_trans(scopeAimData.entityWithScopeLensEid, scopeAimData.lensNodeId, texCtx);
}

void render_lens_optics(
  const ScopeAimRenderingData &scopeAimData, const TexStreamingContext &texCtx, const PostFxRenderer &fadingRenderer)
{
  // fade area outside of scope with black color if we can't provide blur
  if (!renderer_has_feature(FeatureRenderFlags::BLOOM)) // todo: fix dof render implementation dependencies on bloom
    fadingRenderer.render();

  ShaderGlobal::set_int(lens_detail_levelVarId, LENS_DETAIL_FULL);
  ShaderGlobal::set_int(lens_render_modeVarId, LENS_RENDER_OPTICS);
  float lensBrightness = ECS_GET_OR(scopeAimData.entityWithScopeLensEid, gunmod__lensBrightness, 1.0f);
  ShaderGlobal::set_real(lens_brighthnessVarId, lensBrightness);
  Point3 params = ECS_GET_OR(scopeAimData.entityWithScopeLensEid, gunmod__distortionParams, Point3(0.8f, 0.7f, 0.07f));
  ShaderGlobal::set_color4(lens_distortion_paramsVarId, Color4(params.x, params.y, params.z, 0.0f));
  Point3 lensLocalX = ECS_GET_OR(scopeAimData.entityWithScopeLensEid, gunmod__lensLocalX, Point3(1, 0, 0));
  Point3 lensLocalY = ECS_GET_OR(scopeAimData.entityWithScopeLensEid, gunmod__lensLocalY, Point3(0, 1, 0));
  Point3 lensLocalZ = ECS_GET_OR(scopeAimData.entityWithScopeLensEid, gunmod__lensLocalZ, Point3(0, 0, 1));
  ShaderGlobal::set_color4(scope_lens_local_xVarId, Color4(lensLocalX.x, lensLocalX.y, lensLocalX.z, 0.0f));
  ShaderGlobal::set_color4(scope_lens_local_yVarId, Color4(lensLocalY.x, lensLocalY.y, lensLocalY.z, 0.0f));
  ShaderGlobal::set_color4(scope_lens_local_zVarId, Color4(lensLocalZ.x, lensLocalZ.y, lensLocalZ.z, 0.0f));
  const Point2 *scopeRadLen = ECS_GET_NULLABLE(Point2, scopeAimData.entityWithScopeLensEid, gunmod__scopeRadLen);
  if (scopeRadLen)
    set_scope_lens_phys_params(scopeAimData.entityWithScopeLensEid, scopeAimData.lensNodeId, scopeRadLen->x, scopeRadLen->y);

  // Render lens back with optic effects.
  render_scope_trans(scopeAimData.entityWithScopeLensEid, scopeAimData.lensNodeId, texCtx);
  ShaderGlobal::set_int(lens_detail_levelVarId, LENS_DETAIL_REFLECTIONS);
  ShaderGlobal::set_int(lens_render_modeVarId, LENS_RENDER_OPTICS);
  reset_scope_nightvision_tex();
}

void restore_scope_aim_dof(const AimDofSettings &savedDofSettings)
{
  if (!savedDofSettings.changed)
    return;
  get_dof_entity_ecs_query([&](DepthOfFieldPS &dof) {
    dof.setDoFParams(savedDofSettings.focus);
    dof.setOn(savedDofSettings.on);
    dof.setMinCheckDistance(savedDofSettings.minCheckDistance);
  });
}

void prepare_aim_dof(const ScopeAimRenderingData &scopeAimData,
  const AimRenderingData &aim_data,
  AimDofSettings &dof_settings,
  Texture *aim_dof_depth,
  const TexStreamingContext &tex_ctx)
{
  get_dof_entity_ecs_query([&scopeAimData, &aim_data, &dof_settings, &tex_ctx, aim_dof_depth](DepthOfFieldPS &dof) {
    DOFProperties focus = dof.getDoFParams();
    dof_settings.focus = focus;
    dof_settings.on = dof.isOn();
    dof_settings.minCheckDistance = dof.getMinCheckDistance();

    float focusPlaneDist = 0.5f;
    float nearBlurAmount = 0.0f;
    float farBlurAmount = 0.0f;
    if (aim_data.lensRenderEnabled)
    {
      get_aim_dof_scope_ecs_query(scopeAimData.entityWithScopeLensEid,
        [&](float gunmod__focusPlaneShift, float gunmod__dofNearAmountPercent, float gunmod__dofFarAmountPercent) {
          focusPlaneDist = gunmod__focusPlaneShift;
          nearBlurAmount = scopeAimData.nearDofEnabled ? gunmod__dofNearAmountPercent * 0.01 : 0.0f;
          farBlurAmount = aim_data.farDofEnabled ? gunmod__dofFarAmountPercent * 0.01 : 0.0f;
        });

      TIME_D3D_PROFILE(scope_aim_dof_prepare);
      ShaderGlobal::set_real(lens_dof_depthVarId, focusPlaneDist);
      ShaderGlobal::set_int(lens_render_modeVarId, LENS_RENDER_DEPTH);
      ShaderGlobal::set_int(lens_detail_levelVarId, LENS_DETAIL_REFLECTIONS);
      ScopeRenderTarget scopeRt;
      d3d::set_render_target((Texture *)NULL, 0);
      d3d::set_depth(aim_dof_depth, DepthAccess::RW);
      render_scope_trans(scopeAimData.entityWithScopeLensEid, scopeAimData.lensNodeId, tex_ctx);
      ShaderGlobal::set_int(lens_render_modeVarId, LENS_RENDER_OPTICS);
      d3d::resource_barrier({aim_dof_depth, RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    }
    else if (scopeAimData.nearDofEnabled) // Far dof disabled anyway without lens.
    {
      ecs::EntityId dofWeapEid = scopeAimData.isAimingThroughScope ? scopeAimData.entityWithScopeLensEid : scopeAimData.gunEid;
      float dist = 0.0f;
      // In case of scopes may be not queried.
      get_rearsight_dist_ecs_query(dofWeapEid, [&dist](mat44f weapon_rearsight_node__nodeTm) {
        ecs::EntityId camEid = get_cur_cam_entity();
        TMatrix camTm = g_entity_mgr->getOr(camEid, ECS_HASH("transform"), TMatrix::IDENT);
        Point3 camPos = camTm.getcol(3);
        Point3_vec4 rearsightPos;
        v_st(&rearsightPos.x, weapon_rearsight_node__nodeTm.col3);
        dist = length(rearsightPos - camPos);
      });
      get_aim_dof_weap_ecs_query(dofWeapEid, [&dist, &nearBlurAmount](float weap__focusPlaneShift, float weap__dofNearAmountPercent) {
        dist += weap__focusPlaneShift;
        nearBlurAmount = weap__dofNearAmountPercent * 0.01f;
      });
      focusPlaneDist = dist;
    }

    focus.setFarDof(focusPlaneDist, focusPlaneDist + 1, farBlurAmount);
    const float NEAR_DOF_MIN_DIST = 0.01f;
    if (focusPlaneDist - 1e-5f > NEAR_DOF_MIN_DIST)
      focus.setNearDof(NEAR_DOF_MIN_DIST, focusPlaneDist, nearBlurAmount);
    else
      focus.setNoNearDoF();

    dof.setOn(true);
    dof.setDoFParams(focus);
    dof.setMinCheckDistance(0.0f);
    dof.setSimplifiedRendering(scopeAimData.simplifiedAimDof);
    dof_settings.changed = true;
  });
}

static void enable_scope_lod_change(bool enable)
{
  ecs::EntityId heroEid = game::get_controlled_hero();
  ecs::EntityId camEid = ECS_GET_OR(heroEid, bindedCamera, ecs::INVALID_ENTITY_ID);
  enable_scope_lod_change_ecs_query(camEid,
    [enable](bool &shooter_cam__isScopeLodChangeEnabled) { shooter_cam__isScopeLodChangeEnabled = enable; });
}

static void enable_scope_ri_change(bool enable)
{
  ecs::EntityId heroEid = game::get_controlled_hero();
  ecs::EntityId camEid = ECS_GET_OR(heroEid, bindedCamera, ecs::INVALID_ENTITY_ID);
  enable_scope_ri_lod_change_ecs_query(camEid,
    [enable](bool &shooter_cam__isScopeRiLodChangeEnabled) { shooter_cam__isScopeRiLodChangeEnabled = enable; });
}

ECS_TAG(render)
ECS_ON_EVENT(BeforeLoadLevel, ChangeRenderFeatures)
static void scope_quality_render_features_changed_es(const ecs::Event &evt)
{
  int scopeImageQuality = ::dgs_get_settings()->getBlockByNameEx("graphics")->getInt("scopeImageQuality", 0);
  bool aimNearDofEnabled = scopeImageQuality >= 1;
  bool simplifiedAimDof = scopeImageQuality < 2;
  g_entity_mgr->broadcastEventImmediate(UpdateAimDofSettings(aimNearDofEnabled, simplifiedAimDof));

  if (evt.is<ChangeRenderFeatures>())
  {
    enable_scope_lod_change(scopeImageQuality >= 2);
    enable_scope_ri_change(scopeImageQuality == 3);
  }
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_REQUIRE(ShadersECS scope_reticle_shader)
static void init_scope_reticle_quad_rendering_es_event_handler(const ecs::Event &) { index_buffer::init_quads_16bit(); }

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(ShadersECS scope_reticle_shader)
static void release_scope_reticle_quad_rendering_es_event_handler(const ecs::Event &) { index_buffer::release_quads_16bit(); }

static ScopeAimRenderingData prepare_scope_aim_rendering_data(const CameraParams &camera, const IPoint2 &main_view_res)
{
  ScopeAimRenderingData scopeAimData;
  prepare_scope_aim_rendering_data_ecs_query(
    [&scopeAimData, &camera, &main_view_res](ECS_REQUIRE(eastl::true_type camera__active) int aim_data__lensNodeId,
      int aim_data__crosshairNodeId, ecs::EntityId aim_data__entityWithScopeLensEid, ecs::EntityId aim_data__gunEid,
      const ecs::EidList &aim_data__scopeLensCockpitEntities, bool aim_data__isAiming, bool aim_data__isAimingThroughScope,
      bool aim_data__nightVision, bool aim_data__nearDofEnabled, bool aim_data__simplifiedAimDof, const float aim_data__scopeWeaponFov,
      const int aim_data__scopeWeaponFovType, const float aim_data__scopeWeaponLensZoomFactor) {
      scopeAimData.lensNodeId = aim_data__lensNodeId;
      scopeAimData.crosshairNodeId = aim_data__crosshairNodeId;
      scopeAimData.entityWithScopeLensEid = aim_data__entityWithScopeLensEid;
      scopeAimData.gunEid = aim_data__gunEid;
      scopeAimData.scopeLensCockpitEntities.assign(aim_data__scopeLensCockpitEntities.begin(),
        aim_data__scopeLensCockpitEntities.end());
      scopeAimData.isAiming = aim_data__isAiming;
      scopeAimData.isAimingThroughScope = aim_data__isAimingThroughScope;
      scopeAimData.nightVision = aim_data__nightVision;
      scopeAimData.nearDofEnabled = aim_data__nearDofEnabled;
      scopeAimData.simplifiedAimDof = aim_data__simplifiedAimDof;

      ShaderGlobal::set_real(lens_zoom_factorVarId, aim_data__scopeWeaponLensZoomFactor);

      if (aim_data__scopeWeaponFov != 0.0f)
      {
        const auto [horFov, verFov] =
          calc_hor_ver_fov(aim_data__scopeWeaponFov, (FovMode)aim_data__scopeWeaponFovType, main_view_res.x, main_view_res.y);
        TMatrix4D weapJitterProjTm = dmatrix_perspective_reverse(horFov, verFov, camera.jitterPersp.zn, camera.jitterPersp.zf,
          camera.jitterPersp.ox, camera.jitterPersp.oy);

        scopeAimData.jitterProjTm = (TMatrix4)weapJitterProjTm;

        scopeAimData.noJitterProjTm = scopeAimData.jitterProjTm;
        scopeAimData.noJitterProjTm(2, 0) -= camera.jitterPersp.ox;
        scopeAimData.noJitterProjTm(2, 1) -= camera.jitterPersp.oy;
      }
      else
      {
        scopeAimData.jitterProjTm = camera.jitterProjTm;
        scopeAimData.noJitterProjTm = camera.noJitterProjTm;
      }
    });
  return scopeAimData;
}

dabfg::NodeHandle makeSetupScopeAimRenderingDataNode()
{
  return dabfg::register_node("setup_scope_aim_rendering_data", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    auto scopeAimRenderDataHandle = registry.createBlob<ScopeAimRenderingData>("scope_aim_render_data", dabfg::History::No).handle();
    auto cameraParams = registry.read("current_camera").blob<CameraParams>().handle();
    auto mainViewResolution = registry.getResolution<2>("main_view");
    return [scopeAimRenderDataHandle, cameraParams, mainViewResolution]() {
      auto &scopeAimData = scopeAimRenderDataHandle.ref();
      scopeAimData = prepare_scope_aim_rendering_data(cameraParams.ref(), mainViewResolution.get());
    };
  });
}
