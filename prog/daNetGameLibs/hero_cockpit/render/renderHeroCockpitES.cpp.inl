// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_texStreamingContext.h>

#include <ecs/anim/anim.h>
#include <ecs/anim/animchar_visbits.h>
#include <ecs/anim/slotAttach.h>
#include <daECS/core/entitySystem.h>

#include <render/daBfg/ecs/frameGraphNode.h>
#include <render/renderEvent.h>
#include <render/world/aimRender.h>
#include <render/world/dynModelRenderPass.h>
#include <render/world/dynModelRenderer.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/global_vars.h>

#include <shaders/dag_shaderBlock.h>
#include <ioSys/dag_dataBlock.h>
#include <ecs/render/updateStageRender.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_matricesAndPerspective.h>

template <typename Callable>
inline void gather_hero_cockpit_entities_ecs_query(Callable c);
template <typename Callable>
inline ecs::QueryCbResult cockpit_camera_ecs_query(Callable c);
template <typename Callable>
inline void get_animchar_draw_info_ecs_query(ecs::EntityId eid, Callable c);
template <typename Callable>
inline void set_visbits_ecs_query(ecs::EntityId eid, Callable c);
template <typename Callable>
inline void in_vehicle_cockpit_ecs_query(Callable c);
template <typename Callable>
inline void is_hero_draw_above_geometry_ecs_query(Callable c);
template <typename Callable>
inline void fill_hero_cockpit_entities_ecs_query(Callable c);
template <typename Callable>
inline void get_hero_cockpit_entities_const_ecs_query(Callable c);
template <typename Callable>
inline void check_gun_type_for_prepass_ecs_query(ecs::EntityId, Callable c);
template <typename Callable>
inline void is_gun_intersect_static_ecs_query(Callable c);
template <typename Callable>
inline void render_depth_prepass_hero_cockpit_ecs_query(Callable c);

extern int dynamicDepthSceneBlockId;
extern int dynamicSceneBlockId;

static void process_animchar(dynmodel_renderer::DynModelRenderingState &state,
  DynamicRenderableSceneInstance *scene_instance,
  bool need_previous_matrices,
  const ecs::Point4List *additional_data,
  TexStreamingContext texCtx)
{
  Point4 zero(0, 0, 0, 0);
  bool hasData = additional_data && !additional_data->empty();
  const Point4 *dataPtr = hasData ? additional_data->data() : &zero;
  const int dataSize = hasData ? additional_data->size() : 1;

  state.process_animchar(ShaderMesh::STG_opaque, ShaderMesh::STG_atest, scene_instance, dataPtr, dataSize, need_previous_matrices,
    nullptr, nullptr, 0, 0, false, dynmodel_renderer::RenderPriority::DEFAULT, nullptr, texCtx);
}

static void process_animchar_eid(dynmodel_renderer::DynModelRenderingState &state,
  ecs::EntityId animchar_eid,
  bool need_previous_matrices,
  TexStreamingContext texCtx)
{
  get_animchar_draw_info_ecs_query(animchar_eid,
    [&state, texCtx, need_previous_matrices](AnimV20::AnimcharRendComponent &animchar_render, uint8_t &animchar_visbits,
      const ecs::Point4List *additional_data) {
      if (!(animchar_visbits & VISFLG_COCKPIT_VISIBLE))
        return;
      animchar_visbits |= VISFLG_MAIN_VISIBLE;
      process_animchar(state, animchar_render.getSceneInstance(), need_previous_matrices, additional_data, texCtx);
    });
}

static const dynmodel_renderer::DynamicBufferHolder *get_buffer_for_state(dynmodel_renderer::DynModelRenderingState &state,
  const ecs::EidList &cockpit_entities,
  bool need_previous_matrices,
  TexStreamingContext tex_ctx = TexStreamingContext(0))
{
  for (ecs::EntityId eid : cockpit_entities)
    process_animchar_eid(state, eid, need_previous_matrices, tex_ctx);
  state.prepareForRender();
  return state.requestBuffer(dynmodel_renderer::BufferType::MAIN);
}

static void set_cockpit_visibility()
{
  get_hero_cockpit_entities_const_ecs_query([&](const ecs::EidList &hero_cockpit_entities) {
    for (ecs::EntityId eid : hero_cockpit_entities)
      set_visbits_ecs_query(eid, [](uint8_t &animchar_visbits) { animchar_visbits = VISFLG_COCKPIT_VISIBLE; });
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

static inline bool is_hero_draw_above_geometry()
{
  bool res = true;
  is_hero_draw_above_geometry_ecs_query([&](bool hero__isDrawAboveGeometry) {
    res = hero__isDrawAboveGeometry;
    return ecs::QueryCbResult::Stop;
  });
  return res;
}

static inline bool is_gun_intersect_static()
{
  bool res = false;
  is_gun_intersect_static_ecs_query([&](const bool hero_cockpit__intersected) {
    res = hero_cockpit__intersected;
    return ecs::QueryCbResult::Stop;
  });
  return res;
}

static inline bool shouldRenderCockpit()
{
  if (cockpit_camera_ecs_query([](bool isHeroCockpitCam ECS_REQUIRE(eastl::true_type camera__active)) {
        return isHeroCockpitCam ? ecs::QueryCbResult::Stop : ecs::QueryCbResult::Continue;
      }) == ecs::QueryCbResult::Continue)
    return false;
  return !in_vehicle_cockpit() && is_hero_draw_above_geometry();
}

static void gather_hero_cockpit_entities(ecs::EntityId gunEid, bool earlyExitCockpit)
{
  fill_hero_cockpit_entities_ecs_query([&](ecs::EidList &hero_cockpit_entities, bool &render_hero_cockpit_into_early_prepass) {
    hero_cockpit_entities.clear();
    render_hero_cockpit_into_early_prepass = false;
    if (earlyExitCockpit)
      return;
    bool heroInVehicle = false;
    gather_hero_cockpit_entities_ecs_query(
      [&](ecs::EntityId eid ECS_REQUIRE(ecs::Tag cockpitEntity) ECS_REQUIRE(ecs::Tag hero), bool isInVehicle = false,
        const ecs::EidList &attaches_list = ecs::EidList{}, const ecs::EidList &human_weap__currentGunModEids = ecs::EidList{}) {
        heroInVehicle = isInVehicle;
        if (heroInVehicle)
          return;

        hero_cockpit_entities.push_back(eid);
        hero_cockpit_entities.insert(hero_cockpit_entities.end(), attaches_list.begin(), attaches_list.end());
        hero_cockpit_entities.insert(hero_cockpit_entities.end(), human_weap__currentGunModEids.begin(),
          human_weap__currentGunModEids.end());
      });
    check_gun_type_for_prepass_ecs_query(gunEid,
      [&](bool gun__reloadable = false, bool notRenderIntoPrepass = false, const ecs::EidList &attaches_list = ecs::EidList{}) {
        render_hero_cockpit_into_early_prepass = gun__reloadable && !notRenderIntoPrepass && !is_gun_intersect_static();
        if (!heroInVehicle)
          hero_cockpit_entities.insert(hero_cockpit_entities.end(), attaches_list.begin(), attaches_list.end());
      });
  });
}

static void render_hero_cockpit_prepass(const ecs::EidList &cockpit_entities)
{
  STATE_GUARD_0(ShaderGlobal::set_int(is_hero_cockpitVarId, VALUE), 1);
  ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Depth));

  dynmodel_renderer::DynModelRenderingState &state = dynmodel_renderer::get_immediate_state();
  const dynmodel_renderer::DynamicBufferHolder *buffer = get_buffer_for_state(state, cockpit_entities, false);
  if (!buffer)
    return;

  int x, y, w, h;
  float minz, maxz;
  d3d::getview(x, y, w, h, minz, maxz);

  // Slap the cockpit on top of everything in the depth buffer
  d3d::setview(x, y, w, h, 0, 0);
  state.setVars(buffer->buffer.getBufId());
  SCENE_LAYER_GUARD(dynamicDepthSceneBlockId);
  state.render(buffer->curOffset);
  d3d::setview(x, y, w, h, minz, maxz);
}

static void render_hero_cockpit_colorpass(const ecs::EidList &cockpit_entities, TexStreamingContext tex_ctx)
{
  STATE_GUARD_0(ShaderGlobal::set_int(is_hero_cockpitVarId, VALUE), 1);
  ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Color));

  dynmodel_renderer::DynModelRenderingState &state = dynmodel_renderer::get_immediate_state();
  const dynmodel_renderer::DynamicBufferHolder *buffer = get_buffer_for_state(state, cockpit_entities, true, tex_ctx);
  if (!buffer)
    return;

  state.setVars(buffer->buffer.getBufId());
  SCENE_LAYER_GUARD(dynamicSceneBlockId);
  state.render(buffer->curOffset);
}

static void render_depth_prepass_hero_cockpit(const ecs::EidList &cockpit_entities)
{
  if (cockpit_entities.empty())
    return;
  STATE_GUARD_0(ShaderGlobal::set_int(is_hero_cockpitVarId, VALUE), 1);

  ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Depth));

  dynmodel_renderer::DynModelRenderingState &state = dynmodel_renderer::get_immediate_state();
  const dynmodel_renderer::DynamicBufferHolder *buffer = get_buffer_for_state(state, cockpit_entities, false);
  if (!buffer)
    return;

  state.setVars(buffer->buffer.getBufId());
  SCENE_LAYER_GUARD(dynamicDepthSceneBlockId);
  state.render(buffer->curOffset);
}

static dabfg::NodeHandle makePrepareHeroCockpitNode()
{
  return dabfg::register_node("prepare_hero_cockpit", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    // `prepare_hero_cockpit` can modify animchar visbits to hide cockpit during main dynamics rendering
    registry.orderMeBefore("async_animchar_rendering_start_node");
    return []() { set_cockpit_visibility(); };
  });
}

static dabfg::NodeHandle makeHeroCockpitEarlyPrepassNode()
{
  auto ns = dabfg::root() / "opaque" / "closeups";
  return ns.registerNode("hero_cockpit_early_prepass_node", DABFG_PP_NODE_SRC,
    [](dabfg::Registry registry) -> eastl::function<void()> {
      registry.read("shadow_visibility_token").blob<OrderingToken>();
      registry.requestState().allowWireframe().setFrameBlock("global_frame");

      registry.read("current_camera").blob<CameraParams>().bindAsProj<&CameraParams::jitterProjTm>();
      registry.read("viewtm_no_offset").blob<TMatrix>().bindAsView();

      if (need_hero_cockpit_flag_in_prepass)
      {
        render_to_gbuffer(registry);

        auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();
        return [strmCtxHndl] {
          render_depth_prepass_hero_cockpit_ecs_query(
            [&](ecs::EidList &hero_cockpit_entities, bool render_hero_cockpit_into_early_prepass) {
              if (!render_hero_cockpit_into_early_prepass || hero_cockpit_entities.empty())
                return;
              render_hero_cockpit_colorpass(hero_cockpit_entities, strmCtxHndl.ref());
            });
        };
      }
      else
      {
        render_to_gbuffer_prepass(registry);

        return [] {
          render_depth_prepass_hero_cockpit_ecs_query(
            [&](ecs::EidList &hero_cockpit_entities, bool render_hero_cockpit_into_early_prepass) {
              if (!render_hero_cockpit_into_early_prepass || hero_cockpit_entities.empty())
                return;
              render_depth_prepass_hero_cockpit(hero_cockpit_entities);
            });
        };
      }
    });
}


static dabfg::NodeHandle makeHeroCockpitLatePrepassNode()
{
  auto decoNs = dabfg::root() / "opaque" / "decorations";
  return decoNs.registerNode("hero_cockpit_late_prepass_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.read("shadow_visibility_token").blob<OrderingToken>();

    shaders::OverrideState overrideState;
    overrideState.set(shaders::OverrideState::Z_FUNC);
    overrideState.zFunc = CMPF_ALWAYS;
    registry.requestState().setFrameBlock("global_frame").enableOverride(overrideState);

    render_to_gbuffer_prepass(registry);

    registry.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewRotTm>().bindAsProj<&CameraParams::jitterProjTm>();
    return [] {
      get_hero_cockpit_entities_const_ecs_query([&](const ecs::EidList &hero_cockpit_entities) {
        if (hero_cockpit_entities.empty())
          return;
        render_hero_cockpit_prepass(hero_cockpit_entities);
      });
    };
  });
}


static dabfg::NodeHandle makeHeroCockpitColorpassNode()
{
  auto decoNs = dabfg::root() / "opaque" / "decorations";
  return decoNs.registerNode("hero_cockpit_colorpass_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.requestState().setFrameBlock("global_frame");
    render_to_gbuffer(registry);

    registry.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewRotTm>().bindAsProj<&CameraParams::jitterProjTm>();
    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();

    return [strmCtxHndl] {
      get_hero_cockpit_entities_const_ecs_query([&](const ecs::EidList &hero_cockpit_entities) {
        if (hero_cockpit_entities.empty())
          return;

        render_hero_cockpit_colorpass(hero_cockpit_entities, strmCtxHndl.ref());
      });
    };
  });
}

ECS_TAG(render)
ECS_REQUIRE(eastl::true_type camera__active)
ECS_AFTER(reset_occlusion_data_es)
ECS_BEFORE(prepare_aim_occlusion_es)
static void prepare_hero_cockpit_es(const UpdateStageInfoBeforeRender &,
  const ecs::EntityId aim_data__gunEid = ecs::INVALID_ENTITY_ID,
  const int aim_data__lensNodeId = -1,
  const bool aim_data__lensRenderEnabled = false)
{
  TIME_PROFILE_DEV(hero_cockpit_prepare);
  bool earlyExit = (aim_data__lensRenderEnabled && aim_data__lensNodeId >= 0) || !shouldRenderCockpit();
  gather_hero_cockpit_entities(aim_data__gunEid, earlyExit);
}

ECS_TAG(render)
ECS_AFTER(animchar_before_render_es)
static void filter_hero_cockpit_es(const UpdateStageInfoBeforeRender &, ecs::EidList &hero_cockpit_entities)
{
  // Filter invisible parts.
  auto newEnd = eastl::remove_if(hero_cockpit_entities.begin(), hero_cockpit_entities.end(), [](ecs::EntityId eid) {
    uint8_t animchar_visbits = ECS_GET_OR(eid, animchar_visbits, uint8_t(0xFF));
    return !(animchar_visbits & (VISFLG_MAIN_AND_SHADOW_VISIBLE | VISFLG_MAIN_VISIBLE | VISFLG_COCKPIT_VISIBLE));
  });
  hero_cockpit_entities.resize(newEnd - hero_cockpit_entities.begin());
}

ECS_TAG(render)
ECS_ON_EVENT(BeforeLoadLevel)
static void init_hero_cockpit_nodes_es_event_handler(const ecs::Event &,
  dabfg::NodeHandle &hero_cockpit_early_prepass_node,
  dabfg::NodeHandle &hero_cockpit_late_prepass_node,
  dabfg::NodeHandle &hero_cockpit_colorpass_node,
  dabfg::NodeHandle &prepare_hero_cockpit_node)
{
  const DataBlock *graphicsBlk = ::dgs_get_settings()->getBlockByNameEx("graphics");
  bool tiledRenderArch = graphicsBlk->getBool("tiledRenderArch", d3d::get_driver_desc().caps.hasTileBasedArchitecture);
  bool shouldRenderHeroCockpit = graphicsBlk->getBool("shouldRenderHeroCockpit", !tiledRenderArch);

  if (!shouldRenderHeroCockpit)
    return;
  prepare_hero_cockpit_node = makePrepareHeroCockpitNode();
  hero_cockpit_early_prepass_node = makeHeroCockpitEarlyPrepassNode();
  hero_cockpit_late_prepass_node = makeHeroCockpitLatePrepassNode();
  hero_cockpit_colorpass_node = makeHeroCockpitColorpassNode();
}
