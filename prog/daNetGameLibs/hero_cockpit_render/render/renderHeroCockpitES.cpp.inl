// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_texStreamingContext.h>

#include <ecs/anim/anim.h>
#include <ecs/anim/animchar_visbits.h>
#include <ecs/render/shaderVar.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>

#include <render/daFrameGraph/ecs/frameGraphNode.h>
#include <render/renderEvent.h>
#include <render/world/dynModelRenderPass.h>
#include <render/world/dynModelRenderer.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/global_vars.h>

#include <shaders/dag_shaderBlock.h>
#include <ioSys/dag_dataBlock.h>
#include <ecs/render/updateStageRender.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <shaders/dag_dynSceneRes.h>
#include <3d/dag_texMgr.h>
#include <3d/dag_texIdSet.h>

template <typename Callable>
inline void get_animchar_draw_info_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable c);
template <typename Callable>
inline void set_visbits_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable c);
template <typename Callable>
inline void get_hero_cockpit_entities_const_ecs_query(ecs::EntityManager &manager, Callable c);
template <typename Callable>
inline void render_depth_prepass_hero_cockpit_ecs_query(ecs::EntityManager &manager, Callable c);
template <typename Callable>
inline void fill_hero_cockpit_textures_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable c);

extern ShaderBlockIdHolder dynamicDepthSceneBlockId;
extern ShaderBlockIdHolder dynamicSceneBlockId;

namespace var
{
static ShaderVariableInfo hero_cockpit_bbox_min = ShaderVariableInfo("hero_cockpit_bbox_min", true);
static ShaderVariableInfo hero_cockpit_bbox_max = ShaderVariableInfo("hero_cockpit_bbox_max", true);
static ShaderVariableInfo hero_cockpit_camera_enabled = ShaderVariableInfo("hero_cockpit_camera_enabled", true);
} // namespace var

static void process_animchar(dynmodel_renderer::DynModelRenderingState &state,
  DynamicRenderableSceneInstance *scene_instance,
  dynmodel_renderer::NeedPreviousMatrices need_previous_matrices,
  const ecs::Point4List *additional_data,
  TexStreamingContext texCtx)
{
  state.process_animchar(ShaderMesh::STG_opaque, ShaderMesh::STG_atest, scene_instance,
    animchar_additional_data::get_optional_data(additional_data), need_previous_matrices, {},
    dynmodel_renderer::PathFilterView::NULL_FILTER, 0, dynmodel_renderer::RenderPriority::DEFAULT, nullptr, texCtx);
}

static void process_animchar_eid(ecs::EntityManager &manager,
  dynmodel_renderer::DynModelRenderingState &state,
  ecs::EntityId animchar_eid,
  dynmodel_renderer::NeedPreviousMatrices need_previous_matrices,
  TexStreamingContext texCtx)
{
  get_animchar_draw_info_ecs_query(manager, animchar_eid,
    [&state, texCtx, need_previous_matrices](AnimV20::AnimcharRendComponent &animchar_render, animchar_visbits_t &animchar_visbits,
      const ecs::Point4List *additional_data) {
      if (!(animchar_visbits & VISFLG_COCKPIT_VISIBLE))
        return;
      animchar_visbits |= VISFLG_MAIN_VISIBLE;
      process_animchar(state, animchar_render.getSceneInstance(), need_previous_matrices, additional_data, texCtx);
    });
}

static const dynmodel_renderer::DynamicBufferHolder *get_buffer_for_state(ecs::EntityManager &manager,
  dynmodel_renderer::DynModelRenderingState &state,
  const ecs::EidList &cockpit_entities,
  dynmodel_renderer::NeedPreviousMatrices need_previous_matrices,
  TexStreamingContext tex_ctx = TexStreamingContext(0))
{
  for (ecs::EntityId eid : cockpit_entities)
    process_animchar_eid(manager, state, eid, need_previous_matrices, tex_ctx);
  state.prepareForRender();
  return state.requestBuffer(dynmodel_renderer::BufferType::MAIN);
}

static void set_cockpit_visibility()
{
  get_hero_cockpit_entities_const_ecs_query(*g_entity_mgr, [&](const ecs::EidList &hero_cockpit_entities) {
    for (ecs::EntityId eid : hero_cockpit_entities)
      set_visbits_ecs_query(*g_entity_mgr, eid, [](animchar_visbits_t &animchar_visbits) { mark_cockpit_visible(animchar_visbits); });
  });
}

static void render_hero_cockpit_prepass(ecs::EntityManager &manager, const ecs::EidList &cockpit_entities)
{
  STATE_GUARD_0(ShaderGlobal::set_int(is_hero_cockpitVarId, VALUE), 1);
  ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Depth));

  dynmodel_renderer::DynModelRenderingState &state = dynmodel_renderer::get_immediate_state();
  const dynmodel_renderer::DynamicBufferHolder *buffer =
    get_buffer_for_state(manager, state, cockpit_entities, dynmodel_renderer::NeedPreviousMatrices::No);
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

static void render_hero_cockpit_colorpass(
  ecs::EntityManager &manager, const ecs::EidList &cockpit_entities, TexStreamingContext tex_ctx)
{
  STATE_GUARD_0(ShaderGlobal::set_int(is_hero_cockpitVarId, VALUE), 1);
  ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Color));

  dynmodel_renderer::DynModelRenderingState &state = dynmodel_renderer::get_immediate_state();
  const dynmodel_renderer::DynamicBufferHolder *buffer =
    get_buffer_for_state(manager, state, cockpit_entities, dynmodel_renderer::NeedPreviousMatrices::Yes, tex_ctx);
  if (!buffer)
    return;

  state.setVars(buffer->buffer.getBufId());
  SCENE_LAYER_GUARD(dynamicSceneBlockId);
  state.render(buffer->curOffset);
}

static void render_depth_prepass_hero_cockpit(ecs::EntityManager &manager, const ecs::EidList &cockpit_entities)
{
  if (cockpit_entities.empty())
    return;
  STATE_GUARD_0(ShaderGlobal::set_int(is_hero_cockpitVarId, VALUE), 1);

  ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Depth));

  dynmodel_renderer::DynModelRenderingState &state = dynmodel_renderer::get_immediate_state();
  const dynmodel_renderer::DynamicBufferHolder *buffer =
    get_buffer_for_state(manager, state, cockpit_entities, dynmodel_renderer::NeedPreviousMatrices::No);
  if (!buffer)
    return;

  state.setVars(buffer->buffer.getBufId());
  SCENE_LAYER_GUARD(dynamicDepthSceneBlockId);
  state.render(buffer->curOffset);
}

static dafg::NodeHandle makePrepareHeroCockpitNode()
{
  return dafg::register_node("prepare_hero_cockpit", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    // `prepare_hero_cockpit` can modify animchar visbits to hide cockpit during main dynamics rendering
    registry.create("prepare_hero_cockpit_token").blob<OrderingToken>();
    return []() { set_cockpit_visibility(); };
  });
}

static dafg::NodeHandle makeHeroCockpitEarlyPrepassNode()
{
  auto ns = dafg::root() / "opaque" / "closeups";
  return ns.registerNode("hero_cockpit_early_prepass_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) -> eastl::function<void()> {
    registry.read("shadow_visibility_token").blob<OrderingToken>();
    registry.read("hidden_animchar_nodes_token").blob<OrderingToken>().optional();

    registry.requestState().allowWireframe().setFrameBlock("global_frame");

    registry.read("current_camera").blob<CameraParams>().bindAsProj<&CameraParams::jitterProjTm>();
    registry.read("viewtm_no_offset").blob<TMatrix>().bindAsView();

    if (need_hero_cockpit_flag_in_prepass)
    {
      render_to_gbuffer(registry);

      auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();
      return [strmCtxHndl] {
        render_depth_prepass_hero_cockpit_ecs_query(*g_entity_mgr,
          [&](ecs::EidList &hero_cockpit_entities, bool render_hero_cockpit_into_early_prepass) {
            if (!render_hero_cockpit_into_early_prepass || hero_cockpit_entities.empty())
              return;
            render_hero_cockpit_colorpass(*g_entity_mgr, hero_cockpit_entities, strmCtxHndl.ref());
          });
      };
    }
    else
    {
      render_to_gbuffer_prepass(registry);

      return [] {
        render_depth_prepass_hero_cockpit_ecs_query(*g_entity_mgr,
          [&](ecs::EidList &hero_cockpit_entities, bool render_hero_cockpit_into_early_prepass) {
            if (!render_hero_cockpit_into_early_prepass || hero_cockpit_entities.empty())
              return;
            render_depth_prepass_hero_cockpit(*g_entity_mgr, hero_cockpit_entities);
          });
      };
    }
  });
}


static dafg::NodeHandle makeHeroCockpitLatePrepassNode()
{
  auto decoNs = dafg::root() / "opaque" / "decorations";
  return decoNs.registerNode("hero_cockpit_late_prepass_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.read("shadow_visibility_token").blob<OrderingToken>();
    registry.read("hidden_animchar_nodes_token").blob<OrderingToken>().optional();

    shaders::OverrideState overrideState;
    overrideState.set(shaders::OverrideState::Z_FUNC);
    overrideState.zFunc = CMPF_ALWAYS;
    registry.requestState().setFrameBlock("global_frame").enableOverride(overrideState);

    render_to_gbuffer_prepass(registry);

    registry.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewRotTm>().bindAsProj<&CameraParams::jitterProjTm>();
    return [] {
      get_hero_cockpit_entities_const_ecs_query(*g_entity_mgr, [&](const ecs::EidList &hero_cockpit_entities) {
        if (hero_cockpit_entities.empty())
          return;
        render_hero_cockpit_prepass(*g_entity_mgr, hero_cockpit_entities);
      });
    };
  });
}


static dafg::NodeHandle makeHeroCockpitColorpassNode()
{
  auto decoNs = dafg::root() / "opaque" / "decorations";
  return decoNs.registerNode("hero_cockpit_colorpass_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.requestState().setFrameBlock("global_frame").allowWireframe();
    render_to_gbuffer(registry);

    registry.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewRotTm>().bindAsProj<&CameraParams::jitterProjTm>();
    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();

    return [strmCtxHndl] {
      get_hero_cockpit_entities_const_ecs_query(*g_entity_mgr, [&](const ecs::EidList &hero_cockpit_entities) {
        if (hero_cockpit_entities.empty())
          return;

        render_hero_cockpit_colorpass(*g_entity_mgr, hero_cockpit_entities, strmCtxHndl.ref());
      });
    };
  });
}

ECS_TAG(render)
ECS_AFTER(animchar_before_render_es)
static void filter_hero_cockpit_es(const UpdateStageInfoBeforeRender &, ecs::EidList &hero_cockpit_entities)
{
  // Filter invisible parts.
  auto newEnd = eastl::remove_if(hero_cockpit_entities.begin(), hero_cockpit_entities.end(), [](ecs::EntityId eid) {
    animchar_visbits_t animchar_visbits = ECS_GET_OR(eid, animchar_visbits, animchar_visbits_t(VISFLG_ALL_BITS));
    return !(animchar_visbits & (VISFLG_MAIN_AND_SHADOW_VISIBLE | VISFLG_MAIN_VISIBLE | VISFLG_COCKPIT_VISIBLE));
  });
  hero_cockpit_entities.resize(newEnd - hero_cockpit_entities.begin());
}

ECS_TAG(render)
ECS_AFTER(filter_hero_cockpit_es)
static void mark_hero_cockpit_textures_important_es(
  const UpdateStageInfoBeforeRender &, ecs::EntityManager &manager, const ecs::EidList &hero_cockpit_entities)
{
  TextureIdSet cockpitTextures;
  for (ecs::EntityId animchar_eid : hero_cockpit_entities)
  {
    fill_hero_cockpit_textures_ecs_query(manager, animchar_eid, [&cockpitTextures](AnimV20::AnimcharRendComponent &animchar_render) {
      animchar_render.getVisualResource()->gatherUsedTex(cockpitTextures);
    });
  }
  mark_managed_textures_important(cockpitTextures);
}

ECS_TAG(render)
ECS_AFTER(animchar_before_render_es) // require for execute animchar_before_render_es as early as possible
static void set_hero_cockpit_params_es(const UpdateStageInfoBeforeRender &,
  const ShaderVar &hero_cockpit_vec,
  const Point4 &hero_cockpit_vec_value,
  const ShaderVar &hero_cockpit_camera_to_point,
  const Point4 &hero_cockpit_camera_to_point_value,
  const Point4 &hero_cockpit_bbox_min_value,
  const Point4 &hero_cockpit_bbox_max_value,
  int hero_cockpit_camera_enabled_value)
{
  ShaderGlobal::set_float4(hero_cockpit_vec, hero_cockpit_vec_value);
  ShaderGlobal::set_float4(hero_cockpit_camera_to_point, hero_cockpit_camera_to_point_value);
  ShaderGlobal::set_float4(var::hero_cockpit_bbox_min, hero_cockpit_bbox_min_value);
  ShaderGlobal::set_float4(var::hero_cockpit_bbox_max, hero_cockpit_bbox_max_value);
  ShaderGlobal::set_int(var::hero_cockpit_camera_enabled, hero_cockpit_camera_enabled_value);
}

ECS_TAG(render)
ECS_ON_EVENT(BeforeLoadLevel)
static void init_hero_cockpit_nodes_es_event_handler(const ecs::Event &,
  dafg::NodeHandle &hero_cockpit_early_prepass_node,
  dafg::NodeHandle &hero_cockpit_late_prepass_node,
  dafg::NodeHandle &hero_cockpit_colorpass_node,
  dafg::NodeHandle &prepare_hero_cockpit_node)
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
