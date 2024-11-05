// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/world/frameGraphHelpers.h>
#include <render/daBfg/bfg.h>
#include <util/dag_convar.h>
#include <ecs/render/renderPasses.h>
#include <render/world/defaultVrsSettings.h>
#include <frustumCulling/frustumPlanes.h>

#define INSIDE_RENDERER 1
#include "../private_worldRenderer.h"
#include "frameGraphNodes.h"
#include <render/world/renderPrecise.h>


dabfg::NodeHandle makeGroundNode(bool early)
{
  auto ns = dabfg::root() / "opaque" / "statics";

  return ns.registerNode("ground_node", DABFG_PP_NODE_SRC, [renderEarly = early](dabfg::Registry registry) {
    if (renderEarly)
      registry.setPriority(dabfg::PRIO_AS_EARLY_AS_POSSIBLE);
    else
      registry.setPriority(dabfg::PRIO_AS_LATE_AS_POSSIBLE);

    auto state = registry.requestState().setFrameBlock("global_frame").allowWireframe();

    auto pass = render_to_gbuffer(registry);

    use_default_vrs(pass, state);

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera")
                        .bindAsView<&CameraParams::viewTm>()
                        .bindAsProj<&CameraParams::jitterProjTm>()
                        .handle();

    use_jitter_frustum_plane_shader_vars(registry);

    return [cameraHndl](dabfg::multiplexing::Index multiplexing_index) {
      bool isFirstIteration = multiplexing_index == dabfg::multiplexing::Index{};
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      {
        RenderPrecise renderPrecise(cameraHndl.ref().viewTm, cameraHndl.ref().cameraWorldPos);
        wr.renderGround(isFirstIteration);
      }
    };
  });
}
