// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/daFG.h>
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

eastl::fixed_vector<dafg::NodeHandle, 3> makeControlOpaqueDynamicsNodes(const char *prev_region_ns)
{
  eastl::fixed_vector<dafg::NodeHandle, 3> result;

  auto ns = dafg::root() / "opaque" / "dynamics";

  result.push_back(ns.registerNode("after_async_animchar", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    // Ensure that async animchar rendering (if it's enabled) starts
    // before we actually use it's results in dynmodel rendering
    registry.read("async_animchar_rendering_started_token").blob<OrderingToken>().optional();
    registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
  }));

  result.push_back(ns.registerNode("begin", DAFG_PP_NODE_SRC, [prev_region_ns](dafg::Registry registry) {
    registry.orderMeAfter("after_async_animchar");
    auto readFromNs = registry.root() / "opaque" / prev_region_ns;

    start_gbuffer_rendering_region(registry, readFromNs, false);

    // Ensure that animchar parts that intersect the camera are hidden
    // before starting to render anything dynamic. Optional is here
    // "just in case", as we will not break if the token is not present.
    registry.read("hidden_animchar_nodes_token").blob<OrderingToken>().optional();

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();

    return [cameraHndl]() {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      // Eye caustics (which are opaque dynamics) require CSM
      wr.shadowsManager.updateCsmData(cameraHndl.ref());
    };
  }));
  result.push_back(ns.registerNode("end", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.create("acesfx_start_update_token", dafg::History::No).blob<OrderingToken>();
    end_gbuffer_rendering_region(registry);
  }));

  return result;
}

dafg::NodeHandle makeOpaqueDynamicsNode()
{
  auto ns = dafg::root() / "opaque" / "dynamics";

  return ns.registerNode("main_dynamics_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto [cameraHndl, stateRequest] = request_common_opaque_state(registry);
    use_camera_in_camera_jitter_frustum_plane_shader_vars(registry);
    render_to_gbuffer(registry).vrsRate(VRS_RATE_TEXTURE_NAME);

    auto strmCtxHndl = registry.read("tex_ctx").blob<TexStreamingContext>().handle();

    return [cameraHndl = cameraHndl, strmCtxHndl, dyn_model_render_passVarId = ::get_shader_variable_id("dyn_model_render_pass")](
             const dafg::multiplexing::Index &multiplexing_index) {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      const auto &camera = cameraHndl.ref();

      ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Color));

      const uint32_t hints = UpdateStageInfoRender::RENDER_COLOR | UpdateStageInfoRender::RENDER_DEPTH |
                             UpdateStageInfoRender::RENDER_MAIN |
                             (wr.hasMotionVectors ? UpdateStageInfoRender::RENDER_MOTION_VECS : 0);

      const dynmodel_renderer::DynModelRenderingState *pState = nullptr;
      if (async_animchars_main.get())
      {
        camera.jobsMgr->waitAsyncAnimcharMainRender();
        pState = camera.jobsMgr->getAsyncAnimcharMainRenderState();
      }

      const camera_in_camera::ApplyMasterState camcam{multiplexing_index};

      // TODO: the plan here is to stop using ECS events for this and
      // create dedicated FG nodes instead. This will provide easier
      // access to various data required for rendering, like the
      // 10 different camera params we have in this event.
      // Async data will have to be waited for in the `begin` node
      // in that case.
      {
        TIME_D3D_PROFILE(ecs_render);
        Occlusion *occlusion = camera.jobsMgr->getOcclusion();
        g_entity_mgr->broadcastEventImmediate(UpdateStageInfoRender(hints, camera.jitterFrustum, camera.viewItm, camera.viewTm,
          camera.jitterProjTm, camera.viewItm.getcol(3), camera.negRoundedCamPos, camera.negRemainderCamPos, occlusion, RENDER_MAIN,
          pState, strmCtxHndl.ref()));
      }
    };
  });
}
