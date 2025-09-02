// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "nodes.h"

#include <render/world/global_vars.h>
#include <render/world/wrDispatcher.h>
#include <render/world/cameraParams.h>
#include <render/world/shadowsManager.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/cameraViewVisibilityManager.h>

#include <render/rendererFeatures.h>
#include <render/debugMesh.h>
#include <render/deferredRenderer.h>
#include <render/fx/fxRenderTags.h>
#include <render/fx/fx.h>

dafg::NodeHandle mk_transparent_particles_mobile_node()
{
  return dafg::register_node("transparent_particles_mobile_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    request_common_transparent_state(registry, "effects_depth_tex");
    registry.setPriority(TRANSPARENCY_NODE_PRIORITY_PARTICLES);
    registry.orderMeAfter("transparent_effects_setup_mobile");

    const auto depthHndl = registry.readTexture("depth").atStage(dafg::Stage::PS).useAs(dafg::Usage::SHADER_RESOURCE).handle();

    registry.requestState().setFrameBlock("global_frame");

    return [depthHndl]() {
      acesfx::setDepthTex(&depthHndl.ref());
      acesfx::renderTransLowRes();

      acesfx::renderTransHighRes();
      acesfx::renderTransSpecial(ERT_TAG_ATEST);
      acesfx::renderTransSpecial(ERT_TAG_XRAY);
    };
  });
}

dafg::NodeHandle mk_transparent_effects_setup_mobile_node()
{
  return dafg::register_node("transparent_effects_setup_mobile", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.setPriority(dafg::PRIO_AS_LATE_AS_POSSIBLE);
    registry.requestState().setFrameBlock("global_frame");
    const auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    return [cameraHndl]() {
      Occlusion *occlusion = cameraHndl.ref().jobsMgr->getOcclusion();
      acesfx::finish_update(cameraHndl.ref().jitterGlobtm, occlusion);
    };
  });
}
