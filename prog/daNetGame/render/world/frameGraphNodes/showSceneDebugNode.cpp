// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/daFG.h>
#include <render/world/cameraParams.h>
#include <render/world/frameGraphHelpers.h>

#include "frameGraphNodes.h"

dafg::NodeHandle makeUpsampleDepthForSceneDebugNode()
{
  return dafg::register_node("upsample_depth_for_scene_debug_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto downsampledDepthHndl =
      registry.readTexture("depth_for_postfx").atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    auto depthHndl = registry.create("depth_for_scene_debug", dafg::History::No)
                       .texture({get_gbuffer_depth_format() | TEXCF_RTARGET, registry.getResolution<2>("display")})
                       .atStage(dafg::Stage::PS)
                       .useAs(dafg::Usage::DEPTH_ATTACHMENT)
                       .handle();

    return [depthHndl, downsampledDepthHndl, renderer = PostFxRenderer("copy_depth")] {
      SCOPE_RENDER_TARGET;
      d3d::settex(15, downsampledDepthHndl.view().getTex2D());
      d3d::set_sampler(STAGE_PS, 15, d3d::request_sampler({}));
      d3d::set_render_target(nullptr, 0);
      d3d::set_depth(depthHndl.view().getTex2D(), DepthAccess::RW);
      renderer.render();
    };
  });
}

dafg::NodeHandle makeShowSceneDebugNode()
{
  return dafg::register_node("show_scene_debug_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto cameraParamsHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto colorHndl = registry.modifyTexture("frame_with_debug").atStage(dafg::Stage::PS).useAs(dafg::Usage::COLOR_ATTACHMENT).handle();
    auto depthHndl = registry
                       .readTexture("depth_for_postfx") // FIXME: There should be modify for the depth, because it is used as RW depth
                       .atStage(dafg::Stage::PS)
                       .useAs(dafg::Usage::DEPTH_ATTACHMENT)
                       .handle();
    auto upsampledDepthHndl =
      registry.readTexture("depth_for_scene_debug").atStage(dafg::Stage::PS).useAs(dafg::Usage::DEPTH_ATTACHMENT).optional().handle();
    registry.readTexture("far_downsampled_depth").atStage(dafg::Stage::PS).bindToShaderVar("downsampled_far_depth_tex");
    return [cameraParamsHndl, colorHndl, depthHndl, upsampledDepthHndl] {
      extern void render_scene_debug(BaseTexture * target, BaseTexture * depth, const CameraParams &camera);

      ManagedTexView depth = upsampledDepthHndl.get() ? upsampledDepthHndl.view() : depthHndl.view();
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
