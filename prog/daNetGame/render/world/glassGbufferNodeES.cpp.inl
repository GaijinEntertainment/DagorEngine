// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <ecs/anim/animchar_visbits.h>
#include <ecs/anim/anim.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>

#include <ecs/render/shaderVar.h>
#include <ecs/render/resPtr.h>
#include <ecs/render/updateStageRender.h>

#include <rendInst/rendInstGenRender.h>
#include <rendInst/rendInstExtraRender.h>

#include <render/daFrameGraph/ecs/frameGraphNode.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/global_vars.h>
#include <render/world/dynModelRenderPass.h>
#include <render/world/dynModelRenderer.h>
#include <render/world/renderPrecise.h>
#include <render/world/defaultVrsSettings.h>
#include <render/world/cameraViewVisibilityManager.h>
#include <render/renderEvent.h>
#include <render/renderSettings.h>

#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_shaderVarsUtils.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <render/world/bvh.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <render/world/animCharRenderUtil.h>
#include <shaders/dag_dynSceneRes.h>

template <typename Callable>
static void send_on_rttr_changed_event_ecs_query(Callable c);

template <typename Callable>
static void animchar_render_trans_ecs_query(Callable c);

ECS_UNICAST_EVENT_TYPE(OnRTTRChanged);
ECS_REGISTER_EVENT(OnRTTRChanged);

extern ShaderBlockIdHolder dynamicSceneTransBlockId;

extern void render_mainhero_trans(const TMatrix &view_tm);

static void render_state(
  dynmodel_renderer::DynModelRenderingState &state, dynmodel_renderer::BufferType buf_type, const TMatrix &vtm, int block)
{
  state.prepareForRender();

  const dynmodel_renderer::DynamicBufferHolder *buffer = state.requestBuffer(buf_type);
  if (!buffer)
    return;

  d3d::settm(TM_VIEW, vtm);

  state.setVars(buffer->buffer.getBufId());
  SCENE_LAYER_GUARD(block);
  state.render(buffer->curOffset);
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear, ChangeRenderFeatures, OnRTTRChanged)
static void glass_gbuffer_renderer_es(const ecs::Event &evt, dafg::NodeHandle &glass_gbuffer_renderer_node)
{
  if (auto *changedFeatures = evt.cast<ChangeRenderFeatures>())
    if (!(changedFeatures->isFeatureChanged(CAMERA_IN_CAMERA)))
      return;

  bool only_depth = is_rttr_enabled() ? false : true;
  const char *nodeName = only_depth ? "glass_depth_node" : "glass_depth_normal_node";

  auto nodeNs = dafg::root() / "transparent" / "close";
  glass_gbuffer_renderer_node = nodeNs.registerNode(nodeName, DAFG_PP_NODE_SRC, [only_depth](dafg::Registry registry) {
    shaders::OverrideState overrideState;
    overrideState.set(shaders::OverrideState::Z_WRITE_ENABLE);
    overrideState.zFunc = CMPF_GREATEREQUAL;
    registry.orderMeAfter("begin"); // need for correct ordering inside transparent "group"
    registry.read("ri_update_token").blob<OrderingToken>();
    auto depth = registry
                   .createTexture2d("glass_depth", dafg::History::No,
                     {TEXFMT_DEPTH32 | TEXCF_RTARGET, registry.getResolution<2>("main_view", 0.5)})
                   .clear(make_clear_value(0.f, 0));
    if (only_depth)
    {
      registry.allowAsyncPipelines().requestRenderPass().depthRw(depth);
    }
    else
    {
      auto target = registry
                      .createTexture2d("glass_target", dafg::History::No,
                        {TEXFMT_A16B16G16R16F | TEXCF_RTARGET | TEXCF_UNORDERED, registry.getResolution<2>("main_view", 0.5)})
                      .clear(make_clear_value(0.f, 0.f, 0.f, 0.f));

      registry.allowAsyncPipelines().requestRenderPass().color({target}).depthRw(depth);
    }

    registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
    registry.setPriority(TRANSPARENCY_NODE_PRIORITY_RENDINST);

    registry.requestState().allowWireframe().enableOverride(overrideState).setFrameBlock("global_frame");
    int rendinstTransSceneBlockid = ShaderGlobal::getBlockId("rendinst_trans_scene");

    auto cameraHndl = use_camera_in_camera(registry);
    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();
    return [cameraHndl, strmCtxHndl, rendinstTransSceneBlockid, glass_normal_passVarId = get_shader_variable_id("glass_normal_pass")] {
      ShaderGlobal::set_int(glass_normal_passVarId, 1);
      RenderPrecise renderPrecise(cameraHndl.ref().viewTm, cameraHndl.ref().cameraWorldPos);
      SCENE_LAYER_GUARD(rendinstTransSceneBlockid);
      CameraViewVisibilityMgr *camJobsMgr = cameraHndl.ref().jobsMgr;
      rendinst::render::renderRIGen(rendinst::RenderPass::Normal, camJobsMgr->getRiMainVisibility(), cameraHndl.ref().viewItm,
        rendinst::LayerFlag::Transparent, rendinst::OptimizeDepthPass::No, 1, rendinst::AtestStage::All, nullptr, strmCtxHndl.ref());

      const auto &camera = cameraHndl.ref();
      const auto &texCtx = strmCtxHndl.ref();
      dynmodel_renderer::DynModelRenderingState &state = dynmodel_renderer::get_immediate_state();

      // we need to exlude tank cockpit glass from processs because this cocpit is complete fake  a gives a incorrect reflcetion result
      animchar_render_trans_ecs_query([&state, &texCtx](ECS_REQUIRE(ecs::Tag requires_trans_render)
                                                          ECS_REQUIRE_NOT(ecs::Tag late_transparent_render, ecs::Tag cockpitEntity)
                                                            AnimV20::AnimcharRendComponent &animchar_render,
                                        animchar_visbits_t animchar_visbits,
                                        const ecs::UInt8List *animchar_render__nodeVisibleStgFilters) {
        if (animchar_visbits & VISFLG_MAIN_VISIBLE) //< reuse visibility check from render
        {
          const DynamicRenderableSceneInstance *scene = animchar_render.getSceneInstance();
          const uint8_t *filter = animchar_render__nodeVisibleStgFilters ? animchar_render__nodeVisibleStgFilters->begin() : nullptr;
          const uint32_t pathFilterSize = animchar_render__nodeVisibleStgFilters ? animchar_render__nodeVisibleStgFilters->size() : 0;
          G_ASSERT(!animchar_render__nodeVisibleStgFilters || pathFilterSize == scene->getNodeCount());
          state.process_animchar(ShaderMesh::STG_trans, ShaderMesh::STG_trans, scene, animchar_additional_data::get_null_data(), false,
            nullptr, filter, pathFilterSize, UpdateStageInfoRender::RENDER_MAIN, false, dynmodel_renderer::RenderPriority::HIGH,
            nullptr, texCtx);
        }
      });

      TMatrix vtm = camera.viewTm;
      vtm.setcol(3, 0, 0, 0);
      render_state(state, dynmodel_renderer::BufferType::TRANSPARENT_MAIN, vtm, dynamicSceneTransBlockId);
      d3d::settm(TM_VIEW, camera.viewTm);

      render_mainhero_trans(cameraHndl.ref().viewTm);
      ShaderGlobal::set_int(glass_normal_passVarId, 0);
    };
  });
}


ECS_TRACK(render_settings__enableRTTR, render_settings__bare_minimum)
ECS_REQUIRE(bool render_settings__enableRTTR, bool render_settings__bare_minimum)
ECS_AFTER(bvh_render_settings_changed_es)
static void glass_rttr_recreate_es(const ecs::Event &)
{
  send_on_rttr_changed_event_ecs_query([](ecs::EntityId eid ECS_REQUIRE(dafg::NodeHandle & glass_gbuffer_renderer_node)) {
    g_entity_mgr->sendEventImmediate(eid, OnRTTRChanged{});
  });
}