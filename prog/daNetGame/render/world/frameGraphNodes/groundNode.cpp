// Copyright (C) Gaijin Games KFT.  All rights reserved.

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


dafg::NodeHandle makeGroundNode(bool early)
{
  auto ns = dafg::root() / "opaque" / "statics";

  return ns.registerNode("ground_node", DAFG_PP_NODE_SRC, [renderEarly = early](dafg::Registry registry) {
    if (renderEarly)
      registry.setPriority(dafg::PRIO_AS_EARLY_AS_POSSIBLE);
    else
      registry.setPriority(dafg::PRIO_AS_LATE_AS_POSSIBLE);

    auto cameraHndl = use_camera_in_camera(registry);
    use_camera_in_camera_jitter_frustum_plane_shader_vars(registry);

    registry.requestState().setFrameBlock("global_frame").allowWireframe();
    render_to_gbuffer(registry).vrsRate(VRS_RATE_TEXTURE_NAME);

    return [cameraHndl = cameraHndl](const dafg::multiplexing::Index &multiplexing_index) {
      const camera_in_camera::ApplyMasterState camcam{multiplexing_index, cameraHndl.ref()};

      if (camera_in_camera::is_main_view(multiplexing_index))
        set_viewvecs_to_shader(cameraHndl.ref().viewTm, cameraHndl.ref().jitterProjTm);

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
