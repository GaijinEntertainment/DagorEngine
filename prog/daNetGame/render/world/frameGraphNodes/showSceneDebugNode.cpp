// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/daFG.h>
#include <render/world/cameraParams.h>
#include "frameGraphNodes.h"

dafg::NodeHandle makeShowSceneDebugNode()
{
  return dafg::register_node("show_scene_debug_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto debugNs = registry.root() / "debug";
    auto cameraParamsHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto colorHndl = debugNs.modifyTexture("target_for_debug").atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT).handle();
    auto depthHndl = registry
                       .readTexture("depth_for_postfx") // FIXME: There should be modify for the depth, because it is used as RW depth
                       .atStage(dafg::Stage::PS)
                       .useAs(dafg::Usage::DEPTH_ATTACHMENT)
                       .handle();
    auto upsampledDepthHndl =
      registry.readTexture("depth_for_debug").atStage(dafg::Stage::PS).useAs(dafg::Usage::DEPTH_ATTACHMENT).optional().handle();
    registry.readTexture("far_downsampled_depth").atStage(dafg::Stage::PS).bindToShaderVar("downsampled_far_depth_tex");
    return [cameraParamsHndl, colorHndl, depthHndl, upsampledDepthHndl] {
      extern void render_scene_debug(BaseTexture * target, BaseTexture * depth, const CameraParams &camera);

      BaseTexture *depth = upsampledDepthHndl.get() ? upsampledDepthHndl.get() : depthHndl.get();
      BaseTexture *color = colorHndl.get();

      TextureInfo depthInfo;
      depth->getinfo(depthInfo);
      TextureInfo colorInfo;
      color->getinfo(colorInfo);

      const bool canUseDepth = depthInfo.w == colorInfo.w && depthInfo.h == colorInfo.h;

      render_scene_debug(color, canUseDepth ? depth : nullptr, cameraParamsHndl.ref());
    };
  });
}
