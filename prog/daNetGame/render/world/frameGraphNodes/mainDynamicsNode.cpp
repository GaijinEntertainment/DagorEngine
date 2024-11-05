// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daBfg/bfg.h>
#include <util/dag_convar.h>
#include <ecs/render/renderPasses.h>
#include <render/world/defaultVrsSettings.h>
#include <render/world/frameGraphHelpers.h>
#include <ecs/render/updateStageRender.h>
#include <render/world/dynModelRenderPass.h>
#include <render/world/dynModelRenderer.h>
#include <frustumCulling/frustumPlanes.h>

#define INSIDE_RENDERER 1
#include "../private_worldRenderer.h"
#include "frameGraphNodes.h"
#include <daECS/core/entityManager.h>


extern ConVarT<bool, false> async_animchars_main;

eastl::fixed_vector<dabfg::NodeHandle, 2> makeControlOpaqueDynamicsNodes(const char *prev_region_ns)
{
  eastl::fixed_vector<dabfg::NodeHandle, 2> result;

  auto ns = dabfg::root() / "opaque" / "dynamics";

  result.push_back(ns.registerNode("begin", DABFG_PP_NODE_SRC, [prev_region_ns](dabfg::Registry registry) {
    auto readFromNs = registry.root() / "opaque" / prev_region_ns;

    start_gbuffer_rendering_region(registry, readFromNs, false);

    // Ensure that async animchar rendering (if it's enabled) starts
    // before we actually use it's results in dynmodel rendering
    registry.read("async_animchar_rendering_started_token").blob<OrderingToken>().optional();

    // Ensure that animchar parts that intersect the camera are hidden
    // before starting to render anything dynamic. Optional is here
    // "just in case", as we will not break if the token is not present.
    registry.read("hidden_animchar_nodes_token").blob<OrderingToken>().optional();

    return []() {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      // Eye caustics (which are opaque dynamics) require CSM
      wr.shadowsManager.updateCsmData();
    };
  }));
  result.push_back(
    ns.registerNode("end", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) { end_gbuffer_rendering_region(registry); }));

  return result;
}

extern void wait_async_animchar_main_render(); // defined in animCharRenderES.cpp.inl
extern const char *fmt_csm_render_pass_name(int cascade, char tmps[], int csm_task_id = 0);

dabfg::NodeHandle makeOpaqueDynamicsNode()
{
  auto ns = dabfg::root() / "opaque" / "dynamics";

  return ns.registerNode("main_dynamics_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.create("main_dynamics_update_token", dabfg::History::No).blob<OrderingToken>(); // For virtual dep of `acesfx_update_node`

    auto state = registry.requestState().allowWireframe().setFrameBlock("global_frame");

    auto pass = render_to_gbuffer(registry);

    use_default_vrs(pass, state);


    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();

    auto strmCtxHndl = registry.read("tex_ctx").blob<TexStreamingContext>().handle();
    auto occlusionHndl = registry.read("current_occlusion").blob<Occlusion *>().handle();
    use_jitter_frustum_plane_shader_vars(registry);

    return [cameraHndl, strmCtxHndl, occlusionHndl, dyn_model_render_passVarId = ::get_shader_variable_id("dyn_model_render_pass")] {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      const auto &camera = cameraHndl.ref();

      ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Color));

      const uint32_t hints = UpdateStageInfoRender::RENDER_COLOR | UpdateStageInfoRender::RENDER_DEPTH |
                             UpdateStageInfoRender::RENDER_MAIN |
                             (wr.hasMotionVectors ? UpdateStageInfoRender::RENDER_MOTION_VECS : 0);

      const dynmodel_renderer::DynModelRenderingState *pState = nullptr;
      if (async_animchars_main.get())
      {
        wait_async_animchar_main_render();
        char tmps[] = "csm#000";
        pState = dynmodel_renderer::get_state(fmt_csm_render_pass_name(0, tmps)); // See `start_async_animchar_main_render`
      }

      // TODO: the plan here is to stop using ECS events for this and
      // create dedicated FG nodes instead. This will provide easier
      // access to various data required for rendering, like the
      // 10 different camera params we have in this event.
      // Async data will have to be waited for in the `begin` node
      // in that case.
      {
        TIME_D3D_PROFILE(ecs_render);
        g_entity_mgr->broadcastEventImmediate(UpdateStageInfoRender(hints, camera.jitterFrustum, camera.viewItm, camera.viewTm,
          camera.jitterProjTm, camera.viewItm.getcol(3), camera.negRoundedCamPos, camera.negRemainderCamPos, occlusionHndl.ref(),
          RENDER_MAIN, pState, strmCtxHndl.ref()));
      }
    };
  });
}
