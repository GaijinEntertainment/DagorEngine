// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/world/frameGraphHelpers.h>
#include <render/daBfg/bfg.h>

#include <render/deferredRenderer.h>
#include <ecs/render/renderPasses.h>
#include <landMesh/virtualtexture.h>
#include <util/dag_convar.h>
#include <rendInst/rendInstExtraRender.h>
#include <rendInst/rendInstGenRender.h>
#include <render/renderEvent.h>
#include <daECS/core/entityManager.h>
#include <streaming/dag_streamingBase.h>
#include <render/cables.h>
#include <render/grass/grassRender.h>
#include <render/world/dynModelRenderPass.h>
#include <render/world/renderPrecise.h>
#include <shaders/dag_shaderBlock.h>

#define INSIDE_RENDERER 1
#include "../private_worldRenderer.h"
#include "frameGraphNodes.h"
#include "../defaultVrsSettings.h"


extern ConVarT<bool, false> async_riex_opaque;

eastl::fixed_vector<dabfg::NodeHandle, 3> makeControlOpaqueStaticsNodes(const char *prev_region_ns)
{
  eastl::fixed_vector<dabfg::NodeHandle, 3> result;

  auto ns = dabfg::root() / "opaque" / "statics";

  result.push_back(ns.registerNode("begin", DABFG_PP_NODE_SRC, [prev_region_ns](dabfg::Registry registry) {
    registry.read("shadow_visibility_token").blob<OrderingToken>();

    auto readFromNs = registry.root() / "opaque" / prev_region_ns;
    start_gbuffer_rendering_region(registry, readFromNs, true);

    // Ensure that the async animchar rendering (if it's enabled) is
    // started before static rendering and is done in parallel with it
    registry.read("async_animchar_rendering_started_token").blob<OrderingToken>().optional();

    return [](dabfg::multiplexing::Index multiplexing_index) {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      bool isFirstIteration = multiplexing_index == dabfg::multiplexing::Index{};
      // Ground, rendinsts and prefab (bin scene) use clipmap, so we
      // need feedback throughout statics rendering
      if (wr.clipmap && isFirstIteration)
        wr.clipmap->startUAVFeedback();
    };
  }));
  result.push_back(ns.registerNode("complete_prepass", DABFG_PP_NODE_SRC,
    [](dabfg::Registry registry) { complete_prepass_of_gbuffer_rendering_region(registry); }));
  result.push_back(ns.registerNode("end", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    end_gbuffer_rendering_region(registry);

    return [](dabfg::multiplexing::Index multiplexing_index) {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      bool isFirstIteration = multiplexing_index == dabfg::multiplexing::Index{};

      if (wr.clipmap && isFirstIteration)
        wr.clipmap->endUAVFeedback();
    };
  }));

  return result;
}

extern ConVarT<bool, false> prepass;
extern ConVarT<bool, false> vehicle_cockpit_in_depth_prepass;

static auto request_common_opaque_state(dabfg::Registry registry)
{
  auto stateRequest = registry.requestState().setFrameBlock("global_frame").allowWireframe();

  auto cameraHndl = registry.read("current_camera")
                      .blob<CameraParams>()
                      .bindAsView<&CameraParams::viewTm>()
                      .bindAsProj<&CameraParams::jitterProjTm>()
                      .handle();

  // For impostor billboards and stuff
  registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");

  return eastl::pair{cameraHndl, stateRequest};
}

static auto request_opaque_prepass(dabfg::Registry registry)
{
  auto pass = render_to_gbuffer_prepass(registry);
  auto [cameraHndl, stateRequest] = request_common_opaque_state(registry);

  use_default_vrs(pass, stateRequest);

  return eastl::pair{cameraHndl, stateRequest};
}

static auto request_opaque_pass(dabfg::Registry registry)
{
  auto pass = render_to_gbuffer(registry);

  auto [cameraHndl, stateRequest] = request_common_opaque_state(registry);

  use_default_vrs(pass, stateRequest);

  return cameraHndl;
}

eastl::fixed_vector<dabfg::NodeHandle, 8> makeOpaqueStaticNodes()
{
  eastl::fixed_vector<dabfg::NodeHandle, 8> result;

  auto ns = dabfg::root() / "opaque" / "statics";

  result.push_back(ns.registerNode("riex_prepass", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    auto [cameraHndl, stateRequest] = request_opaque_prepass(registry);

    // TODO: Refactor. RI code around blocks is very weird. Sometimes
    // they set the correct block on their own, sometimes they expect
    // the caller to set the block :/
    int rendinstDepthSceneBlockid = ShaderGlobal::getBlockId("rendinst_depth_scene");

    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();

    return [cameraHndl = cameraHndl, strmCtxHndl, rendinstDepthSceneBlockid]() {
      // TODO: this should be a static condition for node creation,
      // not a runtime condition
      if (!prepass.get())
        return;

      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      G_ASSERT(wr.rendinst_main_visibility);

      RenderPrecise renderPrecise(cameraHndl.ref().viewTm, cameraHndl.ref().cameraWorldPos);
      SCENE_LAYER_GUARD(rendinstDepthSceneBlockid);

      wr.waitVisibility(RENDER_MAIN);
      if (async_riex_opaque.get()) // wait until vb is filled by async opaque render (prepass re-uses same buf struct)
        rendinst::render::waitAsyncRIGenExtraOpaqueRenderVbFill(wr.rendinst_main_visibility);

      rendinst::render::renderRIGenOptimizationDepth(rendinst::RenderPass::Depth, wr.rendinst_main_visibility,
        cameraHndl.ref().viewItm, rendinst::IgnoreOptimizationLimits::No, rendinst::SkipTrees::Yes, rendinst::RenderGpuObjects::Yes,
        1U, strmCtxHndl.ref());
    };
  }));

  result.push_back(ns.registerNode("ri_trees_prepass", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    auto [cameraHndl, _] = request_opaque_prepass(registry);
    registry.read("ri_update_token").blob<OrderingToken>();

    return [cameraHndl = cameraHndl]() {
      if (!prepass.get())
        return;

      RenderPrecise renderPrecise(cameraHndl.ref().viewTm, cameraHndl.ref().cameraWorldPos);
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      G_ASSERT(wr.rendinst_main_visibility);

      wr.waitVisibility(RENDER_MAIN);
      rendinst::render::renderRITreeDepth(wr.rendinst_main_visibility, cameraHndl.ref().viewItm);
    };
  }));

  result.push_back(ns.registerNode("vehicle_cockpit_prepass", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    auto cameraHndl = request_opaque_pass(registry);

    return [cameraHndl = cameraHndl, dyn_model_render_passVarId = ::get_shader_variable_id("dyn_model_render_pass")]() {
      if (!prepass.get() || !vehicle_cockpit_in_depth_prepass.get())
        return;

      // TODO: this shouldn't be within /opaque/statics :/
      ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Color));
      g_entity_mgr->broadcastEventImmediate(VehicleCockpitPrepass(cameraHndl.ref().viewTm));
    };
  }));

  result.push_back(ns.registerNode("bin_scene_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    auto cameraHndl = request_opaque_pass(registry);

    return [cameraHndl]() {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

      auto *binScene = wr.binScene;

      if (!binScene)
        return;

      const auto &cameraPos = cameraHndl.ref().viewItm.getcol(3);
      const auto &cullingFrustum = cameraHndl.ref().noJitterFrustum;

      VisibilityFinder vf;
      vf.set(v_ldu(&cameraPos.x), cullingFrustum, 0.0f, 0.0f, 1.0f, 1.0f, current_occlusion);

      binScene->render(vf, 0, 0xFFFFFFFF);
    };
  }));

  result.push_back(ns.registerNode("rendinst_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    auto cameraHndl = request_opaque_pass(registry);

    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();
    registry.read("ri_update_token").blob<OrderingToken>();

    return [cameraHndl, strmCtxHndl]() {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

      wr.waitVisibility(RENDER_MAIN);
      RenderPrecise renderPrecise(cameraHndl.ref().viewTm, cameraHndl.ref().cameraWorldPos);
      auto *riExRenderer = rendinst::render::waitAsyncRIGenExtraOpaqueRender(wr.rendinst_main_visibility);
      rendinst::render::renderRIGen(rendinst::RenderPass::Normal, wr.rendinst_main_visibility, cameraHndl.ref().viewItm,
        rendinst::LayerFlag::Opaque, rendinst::OptimizeDepthPass::No, /*count_multiply*/ 1, rendinst::AtestStage::All, riExRenderer,
        strmCtxHndl.ref());
    };
  }));

  result.push_back(ns.registerNode("ri_trees_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    auto cameraHndl = request_opaque_pass(registry);

    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();
    registry.read("ri_update_token").blob<OrderingToken>();

    return [cameraHndl, strmCtxHndl]() {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

      wr.waitVisibility(RENDER_MAIN);
      RenderPrecise renderPrecise(cameraHndl.ref().viewTm, cameraHndl.ref().cameraWorldPos);
      rendinst::render::renderRIGen(rendinst::RenderPass::Normal, wr.rendinst_main_visibility, cameraHndl.ref().viewItm,
        rendinst::LayerFlag::NotExtra, prepass.get() ? rendinst::OptimizeDepthPass::Yes : rendinst::OptimizeDepthPass::No, 1,
        rendinst::AtestStage::NoAtest, nullptr, strmCtxHndl.ref());
    };
  }));

  result.push_back(ns.registerNode("cables_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    request_opaque_pass(registry);

    return []() {
      auto *cablesMgr = get_cables_mgr();

      if (!cablesMgr)
        return;

      cablesMgr->render(Cables::RENDER_PASS_OPAQUE);
    };
  }));

  result.push_back(ns.registerNode("gi_collision_debug", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    auto cameraHndl = request_opaque_pass(registry);

    return [cameraHndl]() {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      wr.renderGiCollision(cameraHndl.ref().viewItm, cameraHndl.ref().noJitterFrustum);
    };
  }));

  // Grass is not a static thing, but we want a grass prepass before we
  // start rendering the ground, which is static, because grass
  // occludes a lot of the ground.
  result.push_back(ns.registerNode("grass_prepass_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    // Grass is likely to be occluded by rendInsts and other statics
    // except for the ground, so we want it to run as late as possible.
    registry.setPriority(dabfg::PRIO_AS_LATE_AS_POSSIBLE);

    request_opaque_prepass(registry);

    return []() {
      if (!prepass.get())
        return;

      render_grass_prepass();
    };
  }));

  return result;
}
