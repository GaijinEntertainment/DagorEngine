// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/world/frameGraphHelpers.h>
#include <render/daFrameGraph/daFG.h>

#include <render/deferredRenderer.h>
#include <ecs/render/renderPasses.h>
#include <landMesh/virtualtexture.h>
#include <util/dag_convar.h>
#include <rendInst/rendInstExtraRender.h>
#include <rendInst/rendInstGenRender.h>
#include <render/renderEvent.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <streaming/dag_streamingBase.h>
#include <render/cables.h>
#include <render/grass/grassRender.h>
#include <render/renderEvent.h>
#include <render/world/dynModelRenderPass.h>
#include <render/world/renderPrecise.h>
#include <shaders/dag_renderScene.h>
#include <shaders/dag_shaderBlock.h>
#include <triangleSizeDebug/triangleSizeDebug.h>

#define INSIDE_RENDERER 1
#include "../private_worldRenderer.h"
#include "frameGraphNodes.h"
#include "../defaultVrsSettings.h"

extern ConVarT<bool, false> async_riex_opaque;
extern ConVarT<bool, false> resolveHZBBeforeDynamic;

eastl::fixed_vector<dafg::NodeHandle, 3> makeControlOpaqueCloseupsNodes()
{
  eastl::fixed_vector<dafg::NodeHandle, 3> result;

  auto closeupsNs = dafg::root() / "opaque" / "closeups";
  result.push_back(closeupsNs.registerNode("begin", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto ns = registry.root() / "init";
    start_gbuffer_rendering_region(registry, ns, true);

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();

    // closeups require a viewtm without translation
    auto closeupsViewTm = registry.create("viewtm_no_offset").blob<TMatrix>().handle();

    return [cameraHndl, closeupsViewTm]() {
      auto &viewTm = closeupsViewTm.ref();
      viewTm = cameraHndl.ref().viewTm;
      viewTm.setcol(3, 0, 0, 0);
    };
  }));

  result.push_back(closeupsNs.registerNode("complete_prepass", DAFG_PP_NODE_SRC,
    [](dafg::Registry registry) { complete_prepass_of_gbuffer_rendering_region(registry); }));

  result.push_back(
    closeupsNs.registerNode("end", DAFG_PP_NODE_SRC, [](dafg::Registry registry) { end_gbuffer_rendering_region(registry); }));

  return result;
}

eastl::fixed_vector<dafg::NodeHandle, 4> makeControlOpaqueStaticsNodes(const char *prev_region_ns)
{
  eastl::fixed_vector<dafg::NodeHandle, 4> result;

  auto ns = dafg::root() / "opaque" / "statics";

  result.push_back(ns.registerNode("after_async_animchar", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    // Ensure that the async animchar rendering (if it's enabled) is
    // started before static rendering and is done in parallel with it
    registry.read("async_animchar_rendering_started_token").blob<OrderingToken>().optional();
    registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
  }));

  result.push_back(ns.registerNode("begin", DAFG_PP_NODE_SRC, [prev_region_ns](dafg::Registry registry) {
    registry.orderMeAfter("after_async_animchar");

    registry.read("shadow_visibility_token").blob<OrderingToken>();
    registry.read("hidden_animchar_nodes_token").blob<OrderingToken>().optional();

    auto readFromNs = registry.root() / "opaque" / prev_region_ns;
    start_gbuffer_rendering_region(registry, readFromNs, true);

    return [](dafg::multiplexing::Index multiplexing_index) {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      bool isFirstIteration = multiplexing_index == dafg::multiplexing::Index{};
      // Ground, rendinsts and prefab (bin scene) use clipmap, so we
      // need feedback throughout statics rendering
      if (wr.clipmap && isFirstIteration)
        wr.clipmap->startUAVFeedback();
    };
  }));
  result.push_back(ns.registerNode("complete_prepass", DAFG_PP_NODE_SRC,
    [](dafg::Registry registry) { complete_prepass_of_gbuffer_rendering_region(registry); }));
  result.push_back(ns.registerNode("end", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    end_gbuffer_rendering_region(registry);

    return [](dafg::multiplexing::Index multiplexing_index) {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      bool isFirstIteration = multiplexing_index == dafg::multiplexing::Index{};

      if (wr.clipmap && isFirstIteration)
        wr.clipmap->endUAVFeedback();
    };
  }));

  return result;
}

extern ConVarT<bool, false> vehicle_cockpit_in_depth_prepass;

static auto request_opaque_prepass(dafg::Registry registry)
{
  render_to_gbuffer_prepass(registry);

  auto [cameraHndl, stateRequest] = request_common_opaque_state(registry);

  return eastl::pair{cameraHndl, stateRequest};
}

static auto request_opaque_pass(dafg::Registry registry)
{
  render_to_gbuffer(registry).vrsRate(VRS_RATE_TEXTURE_NAME);

  auto [cameraHndl, stateRequest] = request_common_opaque_state(registry);

  return cameraHndl;
}

static auto request_opaque_triangle_size(dafg::Registry registry)
{
  registry.allowAsyncPipelines().requestRenderPass().depthRo("gbuf_depth").color({"triangle_size_tex"});

  auto [cameraHndl, stateRequest] = request_common_opaque_state(registry);

  return cameraHndl;
}


eastl::fixed_vector<dafg::NodeHandle, 2> makeOpaqueMainNodes(dafg::NameSpace ns, bool prepassEnabled, MainNodeRenderPass mode)
{
  eastl::fixed_vector<dafg::NodeHandle, 2> result;
  bool debugTriangle = mode == MainNodeRenderPass::TriangleSizeDebugPass;
  result.push_back(ns.registerNode("rendinst_node", DAFG_PP_NODE_SRC, [debugTriangle](dafg::Registry registry) {
    auto cameraHndl = debugTriangle ? request_opaque_triangle_size(registry) : request_opaque_pass(registry);

    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();
    registry.read("ri_update_token").blob<OrderingToken>();

    if (!debugTriangle)
    {
      // Burnt ground decals are applied to rendinst clipmap
      auto burntGroundNs = registry.root() / "burnt_ground";
      burntGroundNs.read("burnt_ground_decals_buf").buffer().atStage(dafg::Stage::PS).bindToShaderVar().optional();
      burntGroundNs.read("burnt_ground_decals_indices_buf")
        .buffer()
        .atStage(dafg::Stage::PS)
        .bindToShaderVar("burnt_ground_decals_indices_buf")
        .optional();
    }

    return [cameraHndl, strmCtxHndl, debugTriangle](const dafg::multiplexing::Index &multiplexing_index) {
      CameraViewVisibilityMgr *camJobsMgr = cameraHndl.ref().jobsMgr;
      camJobsMgr->waitVisibility(RENDER_MAIN);
      const camera_in_camera::ApplyMasterState camcam{multiplexing_index};

      RenderPrecise renderPrecise(cameraHndl.ref().viewTm, cameraHndl.ref().cameraWorldPos);
      auto *riExRenderer = camJobsMgr->waitAsyncRIGenExtraOpaqueRender();
      const rendinst::RenderPass renderPass = debugTriangle ? rendinst::RenderPass::TriangleSizeDebug : rendinst::RenderPass::Normal;
      rendinst::render::renderRIGen(renderPass, camJobsMgr->getRiMainVisibility(), cameraHndl.ref().viewItm,
        rendinst::LayerFlag::Opaque, rendinst::OptimizeDepthPass::No, /*count_multiply*/ 1, rendinst::AtestStage::All,
        debugTriangle ? nullptr : riExRenderer, strmCtxHndl.ref());
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      if (!debugTriangle && wr.clipmap)
        wr.clipmap->increaseUAVAtomicPrefix();
    };
  }));

  result.push_back(ns.registerNode("ri_trees_node", DAFG_PP_NODE_SRC, [prepassEnabled, debugTriangle](dafg::Registry registry) {
    auto cameraHndl = debugTriangle ? request_opaque_triangle_size(registry) : request_opaque_pass(registry);

    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();
    registry.read("ri_update_token").blob<OrderingToken>();

    return [cameraHndl, strmCtxHndl, prepassEnabled, debugTriangle](const dafg::multiplexing::Index &multiplexing_index) {
      CameraViewVisibilityMgr *camJobsMgr = cameraHndl.ref().jobsMgr;
      camJobsMgr->waitVisibility(RENDER_MAIN);
      const camera_in_camera::ApplyMasterState camcam{multiplexing_index};

      RenderPrecise renderPrecise(cameraHndl.ref().viewTm, cameraHndl.ref().cameraWorldPos);
      const rendinst::RenderPass renderPass = debugTriangle ? rendinst::RenderPass::TriangleSizeDebug : rendinst::RenderPass::Normal;
      rendinst::render::renderRIGen(renderPass, camJobsMgr->getRiMainVisibility(), cameraHndl.ref().viewItm,
        rendinst::LayerFlag::NotExtra, prepassEnabled ? rendinst::OptimizeDepthPass::Yes : rendinst::OptimizeDepthPass::No, 1,
        rendinst::AtestStage::NoAtest, nullptr, strmCtxHndl.ref());
    };
  }));

  return result;
}

eastl::fixed_vector<dafg::NodeHandle, 8> makeOpaqueStaticNodes(bool prepassEnabled)
{
  eastl::fixed_vector<dafg::NodeHandle, 8> result;

  auto ns = dafg::root() / "opaque" / "statics";

  if (prepassEnabled)
  {
    result.push_back(ns.registerNode("riex_prepass", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
      auto [cameraHndl, stateRequest] = request_opaque_prepass(registry);

      // TODO: Refactor. RI code around blocks is very weird. Sometimes
      // they set the correct block on their own, sometimes they expect
      // the caller to set the block :/
      int rendinstDepthSceneBlockid = ShaderGlobal::getBlockId("rendinst_depth_scene");

      auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();

      return [cameraHndl = cameraHndl, strmCtxHndl, rendinstDepthSceneBlockid](const dafg::multiplexing::Index &index) {
        const camera_in_camera::ApplyMasterState camcam{index};
        CameraViewVisibilityMgr *camJobsMgr = cameraHndl.ref().jobsMgr;
        RiGenVisibility *riMainVisibility = camJobsMgr->getRiMainVisibility();
        G_ASSERT(riMainVisibility);

        RenderPrecise renderPrecise(cameraHndl.ref().viewTm, cameraHndl.ref().cameraWorldPos);
        SCENE_LAYER_GUARD(rendinstDepthSceneBlockid);

        camJobsMgr->waitVisibility(RENDER_MAIN);
        if (async_riex_opaque.get()) // wait until vb is filled by async opaque render (prepass re-uses same buf struct)
          camJobsMgr->waitAsyncRIGenExtraOpaqueRenderVbFill();

        rendinst::render::renderRIGenOptimizationDepth(rendinst::RenderPass::Depth, riMainVisibility, cameraHndl.ref().viewItm,
          rendinst::IgnoreOptimizationLimits::No, rendinst::SkipTrees::Yes, rendinst::RenderGpuObjects::Yes, 1U, strmCtxHndl.ref());
      };
    }));

    result.push_back(ns.registerNode("ri_trees_prepass", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
      auto [cameraHndl, _] = request_opaque_prepass(registry);
      registry.read("ri_update_token").blob<OrderingToken>();

      return [cameraHndl = cameraHndl](const dafg::multiplexing::Index &multiplexing_index) {
        const camera_in_camera::ApplyMasterState camcam{multiplexing_index};

        RenderPrecise renderPrecise(cameraHndl.ref().viewTm, cameraHndl.ref().cameraWorldPos);
        CameraViewVisibilityMgr *camJobsMgr = cameraHndl.ref().jobsMgr;
        RiGenVisibility *riMainVisibility = camJobsMgr->getRiMainVisibility();
        G_ASSERT(riMainVisibility);

        camJobsMgr->waitVisibility(RENDER_MAIN);
        rendinst::render::renderRITreeDepth(riMainVisibility, cameraHndl.ref().viewItm);
      };
    }));

    result.push_back(ns.registerNode("vehicle_cockpit_prepass", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
      auto cameraHndl = request_opaque_pass(registry);
      auto strmCtxHndl = registry.read("tex_ctx").blob<TexStreamingContext>().handle();

      return [cameraHndl = cameraHndl, strmCtxHndl, dyn_model_render_passVarId = ::get_shader_variable_id("dyn_model_render_pass")](
               const dafg::multiplexing::Index &multiplexing_index) {
        if (!vehicle_cockpit_in_depth_prepass.get())
          return;
        const camera_in_camera::ApplyMasterState camcam{multiplexing_index};

        // TODO: this shouldn't be within /opaque/statics :/
        ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Color));
        g_entity_mgr->broadcastEventImmediate(VehicleCockpitPrepass(cameraHndl.ref().viewTm, strmCtxHndl.ref()));
      };
    }));
  }

  result.push_back(ns.registerNode("bin_scene_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto cameraHndl = request_opaque_pass(registry);

    return [cameraHndl](const dafg::multiplexing::Index &multiplexing_index) {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

      auto *binScene = wr.binScene;

      if (!binScene)
        return;

      const camera_in_camera::ApplyMasterState camcam{multiplexing_index};

      const auto &cameraPos = cameraHndl.ref().viewItm.getcol(3);
      const auto &cullingFrustum = cameraHndl.ref().noJitterFrustum;

      VisibilityFinder vf;
      Occlusion *occlusion = cameraHndl.ref().jobsMgr->getOcclusion();
      vf.set(v_ldu(&cameraPos.x), cullingFrustum, 0.0f, 0.0f, 1.0f, 1.0f, occlusion);

      binScene->render(vf, 0, 0xFFFFFFFF);
    };
  }));

  result.push_back(ns.registerNode("cables_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    request_opaque_pass(registry);

    return [](const dafg::multiplexing::Index &multiplexing_index) {
      auto *cablesMgr = get_cables_mgr();

      if (!cablesMgr)
        return;

      const camera_in_camera::ApplyMasterState camcam{multiplexing_index};

      cablesMgr->render(Cables::RENDER_PASS_OPAQUE);
    };
  }));

  result.push_back(ns.registerNode("gi_collision_debug", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto cameraHndl = request_opaque_pass(registry);

    return [cameraHndl]() {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      wr.renderGiCollision(cameraHndl.ref().viewItm, cameraHndl.ref().noJitterFrustum);
    };
  }));

  for (auto &&n : makeOpaqueMainNodes(ns, prepassEnabled, MainNodeRenderPass::MainColorPass))
    result.push_back(eastl::move(n));

  return result;
}

eastl::fixed_vector<dafg::NodeHandle, 2> makeControlOpaqueDynamicRendinstNodes(const char *prev_region_ns)
{
  eastl::fixed_vector<dafg::NodeHandle, 2> result;

  auto ns = dafg::root() / "opaque" / "dynamic_rendinst";

  result.push_back(ns.registerNode("begin", DAFG_PP_NODE_SRC, [prev_region_ns](dafg::Registry registry) {
    auto readFromNs = registry.root() / "opaque" / prev_region_ns;
    start_gbuffer_rendering_region(registry, readFromNs, false);
  }));
  result.push_back(ns.registerNode("end", DAFG_PP_NODE_SRC, [](dafg::Registry registry) { end_gbuffer_rendering_region(registry); }));

  return result;
}

dafg::NodeHandle makeOpaqueDynamicRendinstNode()
{
  auto ns = dafg::root() / "opaque" / "dynamic_rendinst";

  return ns.registerNode("rendinst_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto cameraHndl = request_opaque_pass(registry);

    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();
    registry.read("ri_update_token").blob<OrderingToken>();

    return [cameraHndl, strmCtxHndl](const dafg::multiplexing::Index &multiplexing_index) {
      CameraViewVisibilityMgr *camJobsMgr = cameraHndl.ref().jobsMgr;
      camJobsMgr->waitVisibility(RENDER_MAIN);
      const camera_in_camera::ApplyMasterState camcam{multiplexing_index};

      RenderPrecise renderPrecise(cameraHndl.ref().viewTm, cameraHndl.ref().cameraWorldPos);
      rendinst::render::renderRIGen(rendinst::RenderPass::Normal, camJobsMgr->getRiMainVisibility(), cameraHndl.ref().viewItm,
        rendinst::LayerFlag::Opaque, rendinst::OptimizeDepthPass::No, /*count_multiply*/ 1, rendinst::AtestStage::All, nullptr,
        strmCtxHndl.ref(), rendinst::RiExtraRenderingSubset::OnlyDynamic);
    };
  });
}

ECS_TAG(render, dev)
static void create_opaque_triangle_size_debug_node_es(const CreateTriangleDebugNodes &evt)
{
  if (!evt.systems.isRI && !evt.systems.isTree)
    return;

  auto nodes = makeOpaqueMainNodes(evt.getNameSpace(), true, MainNodeRenderPass::TriangleSizeDebugPass);

  if (evt.systems.isRI)
    evt.nodes->push_back(eastl::move(nodes[0]));

  if (evt.systems.isTree)
    evt.nodes->push_back(eastl::move(nodes[1]));
}

ECS_TAG(render)
ECS_ON_EVENT(OnCameraNodeConstruction)
static void create_opaque_static_nodes_es(const OnCameraNodeConstruction &evt)
{
  auto nodes = makeOpaqueStaticNodes(evt.hasOpaquePrepass);
  for (auto &n : nodes)
    evt.nodes->push_back(eastl::move(n));

  for (auto &&n : makeControlOpaqueCloseupsNodes())
    evt.nodes->push_back(eastl::move(n));

  evt.nodes->push_back(eastl::move(makeOpaqueDynamicRendinstNode()));

  const bool dynamicsAfterStatics = resolveHZBBeforeDynamic.get();
  if (dynamicsAfterStatics)
  {
    for (auto &&n : makeControlOpaqueStaticsNodes("closeups"))
      evt.nodes->push_back(eastl::move(n));

    for (auto &&n : makeControlOpaqueDynamicRendinstNodes("statics"))
      evt.nodes->push_back(eastl::move(n));

    for (auto &&n : makeControlOpaqueDynamicsNodes("dynamic_rendinst"))
      evt.nodes->push_back(eastl::move(n));

    for (auto &&n : makeControlOpaqueDecorationsNode("dynamics"))
      evt.nodes->push_back(eastl::move(n));
  }
  else
  {
    for (auto &&n : makeControlOpaqueDynamicsNodes("closeups"))
      evt.nodes->push_back(eastl::move(n));

    for (auto &&n : makeControlOpaqueStaticsNodes("dynamics"))
      evt.nodes->push_back(eastl::move(n));

    for (auto &&n : makeControlOpaqueDynamicRendinstNodes("statics"))
      evt.nodes->push_back(eastl::move(n));

    for (auto &&n : makeControlOpaqueDecorationsNode("dynamic_rendinst"))
      evt.nodes->push_back(eastl::move(n));
  }
}
