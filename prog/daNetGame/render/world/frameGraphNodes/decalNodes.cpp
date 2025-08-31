// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/daFG.h>

#include <render/world/defaultVrsSettings.h>
#include <render/renderEvent.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/cameraParams.h>
#include <render/world/cameraViewVisibilityManager.h>
#include <daECS/core/entityManager.h>
#include <render/viewVecs.h>
#include "frameGraphNodes.h"

// TODO: both of these nodes should only be created when the relevant
// feature is enabled, but that's blocked by worldRendererForward.

dafg::NodeHandle makeDecalsOnStaticNode()
{
  auto ns = dafg::root() / "opaque" / "statics";
  return ns.registerNode("decals", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    render_to_gbuffer_but_sample_depth(registry).vrsRate(VRS_RATE_TEXTURE_NAME);

    auto [cameraHndl, _] = request_common_opaque_state(registry);
    auto texCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();

    return [cameraHndl = cameraHndl, texCtxHndl](const dafg::multiplexing::Index &multiplexing_index) {
      const camera_in_camera::ApplyMasterState camcam(multiplexing_index);
      const CameraParams &camera = cameraHndl.ref();
      set_viewvecs_to_shader(camera.viewTm, camera.jitterProjTm);

      g_entity_mgr->broadcastEventImmediate(
        OnRenderDecals(camera.viewTm, camera.viewItm, camera.cameraWorldPos, texCtxHndl.ref(), camera.jobsMgr->getRiMainVisibility()));
    };
  });
}

dafg::NodeHandle makeDecalsOnDynamicNode()
{
  // TODO: these are actually dynamic decals, but they need to be
  // rendered on some stuff in decarations too, I think.
  auto ns = dafg::root() / "opaque" / "decorations";
  return ns.registerNode("decals", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    render_to_gbuffer_but_sample_depth(registry).vrsRate(VRS_RATE_TEXTURE_NAME);

    request_common_opaque_state(registry);

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto texCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();

    return [cameraHndl, texCtxHndl](const dafg::multiplexing::Index &multiplexing_index) {
      if (!renderer_has_feature(FeatureRenderFlags::DECALS))
        return;
      const camera_in_camera::ApplyMasterState camcam(multiplexing_index);
      const CameraParams &camera = cameraHndl.ref();
      g_entity_mgr->broadcastEventImmediate(RenderDecalsOnDynamic(camera.viewTm, camera.cameraWorldPos, camera.jitterFrustum,
        camera.jobsMgr->getOcclusion(), texCtxHndl.ref()));
    };
  });
}
