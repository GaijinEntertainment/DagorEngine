// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daBfg/bfg.h>
#include <render/world/cameraParams.h>

#include "frameGraphNodes.h"

dabfg::NodeHandle makeShowSceneDebugNode()
{
  return dabfg::register_node("show_scene_debug_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    auto cameraParamsHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto colorHndl =
      registry.modifyTexture("frame_with_debug").atStage(dabfg::Stage::PS).useAs(dabfg::Usage::COLOR_ATTACHMENT).handle();
    auto depthHndl = registry
                       .readTexture("depth_for_postfx") // FIXME: There should be modify for the depth, because it is used as RW depth
                       .atStage(dabfg::Stage::PS)
                       .useAs(dabfg::Usage::DEPTH_ATTACHMENT)
                       .handle();
    registry.readTexture("far_downsampled_depth").atStage(dabfg::Stage::PS).bindToShaderVar("downsampled_far_depth_tex");
    return [cameraParamsHndl, colorHndl, depthHndl] {
      extern void render_scene_debug(BaseTexture * target, BaseTexture * depth, const CameraParams &camera);

      ManagedTexView depth = depthHndl.view();
      ManagedTexView color = colorHndl.view();

      TextureInfo depthInfo;
      depth->getinfo(depthInfo);
      TextureInfo colorInfo;
      color->getinfo(colorInfo);

      const bool canUseDepth = depthInfo.w == colorInfo.w && depthInfo.h == colorInfo.h;

      render_scene_debug(color.getTex2D(), canUseDepth ? depth.getTex2D() : nullptr, cameraParamsHndl.ref());
    };
  });
}
