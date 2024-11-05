// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "nodes.h"

#include <render/world/global_vars.h>
#include <render/world/wrDispatcher.h>
#include <render/world/cameraParams.h>
#include <render/world/shadowsManager.h>
#include <render/world/frameGraphHelpers.h>

#include <render/rendererFeatures.h>
#include <render/debugMesh.h>
#include <render/deferredRenderer.h>
#include <render/fx/fxRenderTags.h>
#include <render/fx/fx.h>

dabfg::NodeHandle mk_transparent_particles_mobile_node()
{
  return dabfg::register_node("transparent_particles_mobile_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    render_transparency(registry, "effects_depth_tex");
    registry.setPriority(TRANSPARENCY_NODE_PRIORITY_PARTICLES);
    registry.orderMeAfter("transparent_effects_setup_mobile");

    const auto depthHndl = registry.readTexture("depth").atStage(dabfg::Stage::PS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();

    registry.requestState().setFrameBlock("global_frame");

    return [depthHndl]() {
      acesfx::setDepthTex(&depthHndl.ref());
      acesfx::renderTransLowRes();

      acesfx::renderTransHighRes();
      acesfx::renderTransSpecial(ERT_TAG_ATEST);
    };
  });
}

dabfg::NodeHandle mk_transparent_effects_setup_mobile_node()
{
  return dabfg::register_node("transparent_effects_setup_mobile", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.setPriority(dabfg::PRIO_AS_LATE_AS_POSSIBLE);
    registry.requestState().setFrameBlock("global_frame");
    const auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();

    return [cameraHndl]() { acesfx::finish_update(cameraHndl.ref().jitterGlobtm); };
  });
}
