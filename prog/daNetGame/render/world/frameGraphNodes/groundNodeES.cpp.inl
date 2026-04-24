// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <render/world/frameGraphHelpers.h>
#include <render/daFrameGraph/daFG.h>
#include <util/dag_convar.h>
#include <ecs/render/renderPasses.h>
#include <render/world/defaultVrsSettings.h>
#include <render/viewVecs.h>
#include <landMesh/virtualtexture.h>
#include <frustumCulling/frustumPlanes.h>

#define INSIDE_RENDERER 1
#include "../private_worldRenderer.h"
#include "frameGraphNodes.h"
#include <render/world/renderPrecise.h>
#include <render/renderEvent.h>
#include <render/world/wrDispatcher.h>


dafg::NodeHandle makeGroundNode(bool early)
{
  auto ns = dafg::root() / "opaque" / "statics";

  return ns.registerNode("ground_node", DAFG_PP_NODE_SRC, [renderEarly = early](dafg::Registry registry) {
    if (renderEarly)
      registry.setPriority(dafg::PRIO_AS_EARLY_AS_POSSIBLE);
    else
      registry.setPriority(dafg::PRIO_AS_LATE_AS_POSSIBLE);

    auto camera = use_camera_in_camera(registry);
    auto cameraHndl = CameraViewShvars{camera}.bindViewVecs().toHandle();
    use_camera_in_camera_jitter_frustum_plane_shader_vars(registry);

    registry.requestState().setFrameBlock("global_frame").allowWireframe();
    render_to_gbuffer(registry).vrsRate(VRS_RATE_TEXTURE_NAME);

    auto burntGroundNs = registry.root() / "burnt_ground";
    burntGroundNs.read("burnt_ground_decals_buf").buffer().atStage(dafg::Stage::PS).bindToShaderVar().optional();
    burntGroundNs.read("burnt_ground_decals_indices_buf")
      .buffer()
      .atStage(dafg::Stage::PS)
      .bindToShaderVar("burnt_ground_decals_indices_buf")
      .optional();

    return [cameraHndl = cameraHndl](const dafg::multiplexing::Index &multiplexing_index) {
      const camera_in_camera::ApplyMasterState camcam{multiplexing_index};

      bool isFirstIteration = multiplexing_index == dafg::multiplexing::Index{};
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      {
        RenderPrecise renderPrecise(cameraHndl.ref().viewTm, cameraHndl.ref().cameraWorldPos);
        CameraViewVisibilityMgr *camJobsMgr = cameraHndl.ref().jobsMgr;
        camJobsMgr->waitGroundVisibility();
        wr.renderGround(camJobsMgr->getLandMeshCullingData(), isFirstIteration);
        if (wr.clipmap)
          wr.clipmap->increaseUAVAtomicPrefix();
      }
    };
  });
}

ECS_TAG(render)
ECS_ON_EVENT(OnCameraNodeConstruction)
static void create_ground_nodes_es(const OnCameraNodeConstruction &evt)
{
  const bool tiledRenderArch = ::dgs_get_settings()
                                 ->getBlockByNameEx("graphics")
                                 ->getBool("tiledRenderArch", d3d::get_driver_desc().caps.hasTileBasedArchitecture);

  evt.nodes->push_back(makeGroundNode(tiledRenderArch));
  evt.nodes->push_back(dafg::register_node("clipmap_copy_uav_feedback_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.multiplex(dafg::multiplexing::Mode::None);
    registry.orderMeAfter("after_world_render_node");
    registry.executionHas(dafg::SideEffects::External);
    return [] {
      if (WRDispatcher::getClipmap())
        WRDispatcher::getClipmap()->copyUAVFeedback();
    };
  }));
}
