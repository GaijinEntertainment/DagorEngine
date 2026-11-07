// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/componentTypes.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <EASTL/utility.h>
#include <ecs/anim/anim.h>
#include <ecs/phys/collRes.h>
#include <main/level.h>
#include <math/dag_mathUtils.h>

#include <render/cameraParams.h>
#include <render/daFrameGraph/daFG.h>
#include <render/renderEvent.h>
#include <render/world/frameGraphHelpers.h>

#include <3d/dag_render.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_viewScissor.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <shaders/dag_dynSceneRes.h>
#include <shaders/dag_overrideStates.h>
#include <shaders/dag_shaders.h>


template <typename Callable>
inline void get_collimator_tube_rivalry_lens_ecs_query(ecs::EntityManager &manager, Callable c);
template <typename Callable>
inline void get_collimator_tube_rivalry_lens_animchar_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable c);
template <typename Callable>
inline void get_collimator_tube_rivalry_aim_ecs_query(ecs::EntityManager &manager, Callable c);

static constexpr float RIVALRY_MIN_AIM_POSITION = 0.1f;
static constexpr float RIVALRY_HIP_DARKENING = 1.0f;

static const ShaderVariableInfo collimator_tube_rivalry_darkeningVarId("collimator_tube_rivalry_darkening");
static const ShaderVariableInfo collimator_tube_rivalry_visible_angle_radVarId("collimator_tube_rivalry_visible_angle_rad");
static const ShaderVariableInfo collimator_tube_rivalry_lengthVarId("collimator_tube_rivalry_length");
static const ShaderVariableInfo collimator_tube_rivalry_lens_forwardVarId("collimator_tube_rivalry_lens_forward");

static float get_scope_length(const CollisionResource &collres)
{
  const Point3 w = collres.boundingBox.width();
  return max(w.x, max(w.y, w.z));
}

static eastl::pair<float, bool> get_hero_aim_position()
{
  float aimPosition = 0.f;
  get_collimator_tube_rivalry_aim_ecs_query(*g_entity_mgr,
    [&](float aim_data__aimPosition ECS_REQUIRE(eastl::true_type camera__active)) { aimPosition = aim_data__aimPosition; });
  return {aimPosition, aimPosition > RIVALRY_MIN_AIM_POSITION};
}

static bool render_lens(const ShaderElement &elem, float aim_position)
{
  bool rendered = false;
  get_collimator_tube_rivalry_lens_ecs_query(*g_entity_mgr,
    [&](const ecs::EntityId &collimator_moa_render__gun_mod_eid, int collimator_moa_render__rigid_id,
      int collimator_moa_render__relem_id) {
      if (collimator_moa_render__rigid_id < 0 || collimator_moa_render__relem_id < 0)
        return;
      get_collimator_tube_rivalry_lens_animchar_ecs_query(*g_entity_mgr, collimator_moa_render__gun_mod_eid,
        [&](const AnimV20::AnimcharRendComponent &animchar_render, const Point3 &gunmod__collimator_moa_lens_axis,
          const CollisionResource &collres, float gunmod__collimator_moa_tube_rivalry_darkening,
          float gunmod__collimator_moa_tube_rivalry_visible_angle) {
          const DynamicRenderableSceneInstance *scene = animchar_render.getSceneInstance();
          if (!scene || !scene->getLodsResource())
            return;
          const DynamicRenderableSceneResource *sceneRes = scene->getLodsResource()->lods[0].scene;
          if (!sceneRes)
            return;

          dag::ConstSpan<DynamicRenderableSceneResource::RigidObject> rigids = sceneRes->getRigidsConst();
          if (collimator_moa_render__rigid_id >= rigids.size())
            return;
          const DynamicRenderableSceneResource::RigidObject &rigid = rigids[collimator_moa_render__rigid_id];

          dag::ConstSpan<ShaderMesh::RElem> relems = rigid.mesh->getMesh()->getAllElems();
          if (collimator_moa_render__relem_id >= relems.size())
            return;
          const ShaderMesh::RElem &relem = relems[collimator_moa_render__relem_id];
          if (relem.vertexData->isEmpty())
            return;

          const TMatrix &lensWtm = scene->getNodeWtm(rigid.nodeId);
          ShaderGlobal::set_float(collimator_tube_rivalry_darkeningVarId,
            cvt(aim_position, RIVALRY_MIN_AIM_POSITION, 1.f, RIVALRY_HIP_DARKENING, gunmod__collimator_moa_tube_rivalry_darkening));
          ShaderGlobal::set_float(collimator_tube_rivalry_visible_angle_radVarId,
            max(DegToRad(gunmod__collimator_moa_tube_rivalry_visible_angle), 1e-4f));
          ShaderGlobal::set_float(collimator_tube_rivalry_lengthVarId, get_scope_length(collres));
          ShaderGlobal::set_float4(collimator_tube_rivalry_lens_forwardVarId, gunmod__collimator_moa_lens_axis);

          d3d::settm(TM_WORLD, lensWtm);
          relem.vertexData->setToDriver();
          relem.renderWithElem(elem);
          d3d::settm(TM_WORLD, TMatrix::IDENT);
          rendered = true;
        });
    });
  return rendered;
}

static dafg::NodeHandle make_collimator_tube_rivalry_mask_node()
{
  auto ns = dafg::root() / "opaque" / "closeups";
  return ns.registerNode("collimator_tube_rivalry_mask_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.modify("dynmodel_global_state_access_optional_token").blob<OrderingToken>().optional();

    auto maskHndl = registry.create("collimator_tube_rivalry_mask")
                      .texture({TEXFMT_R8 | TEXCF_RTARGET, registry.getResolution<2>("main_view")})
                      .atStage(dafg::Stage::PS)
                      .useAs(dafg::Usage::COLOR_ATTACHMENT)
                      .handle();
    auto depthHndl = registry.modifyTexture("gbuf_depth_done").atStage(dafg::Stage::PS).useAs(dafg::Usage::DEPTH_ATTACHMENT).handle();

    registry.requestState().setFrameBlock("global_frame");
    registry.readBlob<CameraParams>("current_cockpit_camera")
      .bindAsView<&CameraParams::viewRotTm>()
      .bindAsProj<&CameraParams::jitterProjTm>();

    return [shader = DynamicShaderHelper("collimator_tube_rivalry_mask"), maskHndl, depthHndl]() {
      const auto [aimPosition, rivalryVisible] = get_hero_aim_position();
      if (!rivalryVisible)
        return;

      d3d::clear_rt({maskHndl.get()}, make_clear_value(0.f, 0.f, 0.f, 0.f));
      d3d::set_render_target({depthHndl.get(), 0, 0}, DepthAccess::RW, {{maskHndl.get(), 0, 0}});
      render_lens(*shader.shader, aimPosition);
    };
  });
}

static dafg::NodeHandle make_collimator_tube_rivalry_cut_depth_node()
{
  auto ns = dafg::root() / "opaque" / "closeups";
  return ns.registerNode("collimator_tube_rivalry_cut_depth_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.modify("dynmodel_global_state_access_optional_token").blob<OrderingToken>().optional();

    shaders::OverrideState stateOverride;
    stateOverride.set(shaders::OverrideState::Z_FUNC);
    stateOverride.zFunc = CMPF_ALWAYS;
    registry.requestState().setFrameBlock("global_frame").enableOverride(stateOverride);

    auto gbuf2Hndl =
      registry.modifyTexture("gbuf_2_done").atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT).optional().handle();
    auto depthHndl = registry.modifyTexture("gbuf_depth_done").atStage(dafg::Stage::PS).useAs(dafg::Usage::DEPTH_ATTACHMENT).handle();
    registry.readTexture("collimator_tube_rivalry_mask").atStage(dafg::Stage::PS).bindToShaderVar("collimator_tube_rivalry_mask");

    auto earlyPassOnlyHndl = registry.modify("hero_cockpit_early_pass_only").blob<bool>().optional().handle();
    registry.readBlob<CameraParams>("current_cockpit_camera")
      .bindAsView<&CameraParams::viewRotTm>()
      .bindAsProj<&CameraParams::jitterProjTm>();

    return [shader = DynamicShaderHelper("collimator_tube_rivalry_cut_depth"), gbuf2Hndl, depthHndl, earlyPassOnlyHndl]() {
      const auto [aimPosition, rivalryVisible] = get_hero_aim_position();
      if (!rivalryVisible || ::grs_draw_wire)
        return;

      d3d::set_render_target({depthHndl.get(), 0, 0}, DepthAccess::RW, {{gbuf2Hndl.get(), 0, 0}});
      int x, y, w, h;
      float minz, maxz;
      d3d::getview(x, y, w, h, minz, maxz);
      d3d::setview(x, y, w, h, 0, 0);
      const bool rendered = render_lens(*shader.shader, aimPosition);
      d3d::setview(x, y, w, h, minz, maxz);

      bool *earlyPassOnly = earlyPassOnlyHndl.get();
      if (rendered && earlyPassOnly)
        *earlyPassOnly = true;
    };
  });
}

static dafg::NodeHandle make_collimator_tube_rivalry_node()
{
  return dafg::register_node("collimator_tube_rivalry_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.modify("after_late_trans_token").blob<OrderingToken>();
    registry.requestState().setFrameBlock("global_frame");
    auto targetHndl =
      registry.modifyTexture("target_for_transparency").atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT).handle();
    auto reactiveMaskHndl =
      registry.modifyTexture("reactive_mask").atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT).optional().handle();
    auto depthHndl =
      registry.readTexture("depth_for_transparency").atStage(dafg::Stage::PS).useAs(dafg::Usage::DEPTH_ATTACHMENT).handle();
    registry.readBlob<CameraParams>("current_cockpit_camera")
      .bindAsView<&CameraParams::viewRotTm>()
      .bindAsProj<&CameraParams::jitterProjTm>();

    return [shader = DynamicShaderHelper("collimator_tube_rivalry"), targetHndl, reactiveMaskHndl, depthHndl]() {
      const auto [aimPosition, rivalryVisible] = get_hero_aim_position();
      if (!rivalryVisible)
        return;

      d3d::set_render_target({depthHndl.view().getBaseTex(), 0, 0}, DepthAccess::RW,
        {{targetHndl.get(), 0, 0}, {reactiveMaskHndl.get(), 0, 0}});
      render_lens(*shader.shader, aimPosition);
    };
  });
}

ECS_TAG(render)
static void create_collimator_tube_rivalry_nodes_es(const EventLevelLoaded &, ecs::EntityManager &manager)
{
  manager.getOrCreateSingletonEntity(ECS_HASH("collimator_tube_rivalry_nodes"));
}

ECS_TAG(render)
ECS_ON_EVENT(OnCameraMainViewNodeConstruction)
ECS_REQUIRE(ecs::Tag collimator_tube_rivalry_registrator)
static void collimator_tube_rivalry_view_nodes_es(const OnCameraMainViewNodeConstruction &evt)
{
  evt.nodes->push_back(make_collimator_tube_rivalry_mask_node());
  evt.nodes->push_back(make_collimator_tube_rivalry_cut_depth_node());
  evt.nodes->push_back(make_collimator_tube_rivalry_node());
}
