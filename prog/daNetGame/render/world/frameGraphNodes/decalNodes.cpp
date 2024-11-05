// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daBfg/bfg.h>

#include <render/world/defaultVrsSettings.h>
#include <render/renderEvent.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/cameraParams.h>
#include <daECS/core/entityManager.h>
#include <render/viewVecs.h>
#include "frameGraphNodes.h"

// TODO: both of these nodes should only be created when the relevant
// feature is enabled, but that's blocked by worldRendererForward.

dabfg::NodeHandle makeDecalsOnStaticNode()
{
  auto ns = dabfg::root() / "opaque" / "statics";
  return ns.registerNode("decals", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    auto pass = render_to_gbuffer_but_sample_depth(registry);

    auto state = registry.requestState().setFrameBlock("global_frame").allowWireframe();

    use_default_vrs(pass, state);

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera")
                        .bindAsView<&CameraParams::viewTm>()
                        .bindAsProj<&CameraParams::jitterProjTm>()
                        .handle();
    return [cameraHndl] {
      set_viewvecs_to_shader(cameraHndl.ref().viewTm, cameraHndl.ref().jitterProjTm);
      g_entity_mgr->broadcastEventImmediate(
        OnRenderDecals(cameraHndl.ref().viewTm, cameraHndl.ref().jitterProjTm, cameraHndl.ref().cameraWorldPos));
    };
  });
}

dabfg::NodeHandle makeDecalsOnDynamicNode()
{
  // TODO: these are actually dynamic decals, but they need to be
  // rendered on some stuff in decarations too, I think.
  auto ns = dabfg::root() / "opaque" / "decorations";
  return ns.registerNode("decals", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    auto pass = render_to_gbuffer_but_sample_depth(registry);

    auto state = registry.requestState().setFrameBlock("global_frame").allowWireframe();

    registry.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewTm>().bindAsProj<&CameraParams::jitterProjTm>();

    use_default_vrs(pass, state);

    return []() {
      if (!renderer_has_feature(FeatureRenderFlags::DECALS))
        return;
      g_entity_mgr->broadcastEventImmediate(RenderDecalsOnDynamic());
    };
  });
}
