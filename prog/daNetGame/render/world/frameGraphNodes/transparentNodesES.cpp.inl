// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/daFG.h>
#include <ecs/render/updateStageRender.h>
#include <render/cascadeShadows.h>
#include <render/rendererFeatures.h>
#include <render/world/cameraParams.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include "frameGraphNodes.h"
#include <render/world/frameGraphHelpers.h>
#include <render/world/frameGraphNodes/prevFrameTexRequests.h>
#include <render/world/cameraViewVisibilityManager.h>
#include <render/world/shadowsManager.h>
#include <render/world/wrDispatcher.h>


dafg::NodeHandle makeTransparentEcsNode()
{
  auto nodeNs = dafg::root() / "transparent" / "close";
  return nodeNs.registerNode("ecs_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto cameraHndl = request_common_transparent_state(registry, nullptr, false).handle();
    registry.setPriority(TRANSPARENCY_NODE_PRIORITY_ECS);

    registry.requestState().allowWireframe().setFrameBlock("global_frame");

    read_prev_frame_tex(registry);
    registry.read("glass_depth_for_merge").texture().atStage(dafg::Stage::POST_RASTER).bindToShaderVar("glass_hole_mask").optional();
    registry.read("glass_target").texture().atStage(dafg::Stage::POST_RASTER).bindToShaderVar("glass_rt_reflection").optional();

    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();
    return [cameraHndl, strmCtxHndl](const dafg::multiplexing::Index &multiplexing_index) {
      if (!g_entity_mgr)
        return;

      const camera_in_camera::ApplyMasterState camcam{multiplexing_index};

      const auto &camera = cameraHndl.ref();
      Occlusion *occlusion = camera.jobsMgr->getOcclusion();
      const auto &texCtx = strmCtxHndl.ref();

      g_entity_mgr->broadcastEventImmediate(
        UpdateStageInfoRenderTrans(camera.viewTm, camera.jitterProjTm, camera.viewItm, occlusion, texCtx));
    };
  });
}

ECS_TAG(render)
ECS_ON_EVENT(OnCameraNodeConstruction)
static void create_transparents_ecs_nodes_es(const OnCameraNodeConstruction &evt)
{
  evt.nodes->push_back(makeTransparentEcsNode());

  // control nodes
  {
    auto beforeZoneNs = dafg::root() / "transparent" / "far";
    evt.nodes->push_back(beforeZoneNs.registerNode("begin", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
      const char *targetFrom;
      const char *depthFrom;

      targetFrom = "opaque_with_envi_and_water";
      depthFrom = "opaque_depth_with_water";

      registry.requestRenderPass()
        .color({registry.rename(targetFrom, "color_target").texture()})
        .depthRo(registry.rename(depthFrom, "depth").texture());
      registry.root().readTexture("csm_texture").optional().atStage(dafg::Stage::POST_RASTER);

      return []() {
        // This is the most convenient place to do this.
        debug_mesh::deactivate_mesh_coloring_master_override();

        // TODO: Maybe we are paranoid about "setCascadesToShader" and we can delete that.
        if (CascadeShadows *csm = WRDispatcher::getShadowsManager().getCascadeShadows())
          csm->setCascadesToShader();
      };
    }));
    evt.nodes->push_back(
      beforeZoneNs.registerNode("end", DAFG_PP_NODE_SRC, [](dafg::Registry registry) { end_transparent_region(registry); }));
  }
  {
    auto zoneNs = dafg::root() / "transparent" / "middle";
    evt.nodes->push_back(zoneNs.registerNode("begin", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
      auto prevNs = registry.root() / "transparent" / "far";
      start_transparent_region(registry, prevNs);
    }));
    evt.nodes->push_back(
      zoneNs.registerNode("end", DAFG_PP_NODE_SRC, [](dafg::Registry registry) { end_transparent_region(registry); }));
  }
  {
    auto afterZoneNs = dafg::root() / "transparent" / "close";
    evt.nodes->push_back(afterZoneNs.registerNode("begin", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
      auto prevNs = registry.root() / "transparent" / "middle";
      start_transparent_region(registry, prevNs);
    }));
    evt.nodes->push_back(afterZoneNs.registerNode("end", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
      end_transparent_region(registry);
      registry.create("acesfx_done_token").blob<OrderingToken>();
    }));
    evt.nodes->push_back(afterZoneNs.registerNode("start_fx_interframe_jobs", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
      registry.read("acesfx_done_token").blob<OrderingToken>();
      registry.executionHas(dafg::SideEffects::External);
      return &acesfx::start_fx_interframe_jobs;
    }));
  }

  // We expose the transparent attachements to the outside world
  evt.nodes->push_back(dafg::register_node("transparent_publish_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeBefore("water_ssr_late_node");
    auto prevNs = registry.root() / "transparent" / "close";
    prevNs.rename("color_target_done", "target_for_transparency");
    prevNs.rename("depth_done", "depth_before_water_late");
  }));

  evt.nodes->push_back(dafg::register_node("transparent_depth_publish_node", DAFG_PP_NODE_SRC,
    [](dafg::Registry registry) { registry.rename("depth_before_water_late", "depth_for_transparency"); }));
}