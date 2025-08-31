// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/daFG.h>
#include <render/downsampleDepth.h>
#include <perfMon/dag_statDrv.h>
#include <3d/dag_textureIDHolder.h>
#include <memory/dag_framemem.h>

#include <EASTL/fixed_string.h>

#include "frameGraphNodes.h"
#include <render/world/frameGraphHelpers.h>

#define INSIDE_RENDERER 1
#include "../private_worldRenderer.h"


using SmallString = eastl::fixed_string<char, 42, false, framemem_allocator>;
dafg::NodeHandle makeDownsampleDepthWithWaterNode()
{
  return dafg::register_node("downsample_depth_with_water_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    read_gbuffer_motion(registry).optional();

    auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
    bool makeCheckerboardDepth = wr.hasFeature(FeatureRenderFlags::UPSCALE_SAMPLING_TEX);

    const bool hasStencilTest = renderer_has_feature(CAMERA_IN_CAMERA);
    const uint32_t depthFormat = get_gbuffer_depth_format(hasStencilTest);

    const char *depthNameBase = makeCheckerboardDepth ? "checkerboard_depth_with_water" : "far_downsampled_depth_with_water";
    const char *stencilSuffix = hasStencilTest ? "_no_stencil" : "";
    const SmallString depthName{SmallString::CtorSprintf{}, "%s%s", depthNameBase, stencilSuffix};

    auto downsampledDepthHdnl = registry.create(depthName.c_str(), dafg::History::No)
                                  .texture({depthFormat | TEXCF_RTARGET, registry.getResolution<2>("main_view", 0.5f)})
                                  .atStage(dafg::Stage::PS)
                                  .useAs(dafg::Usage::DEPTH_ATTACHMENT)
                                  .handle();

    auto depthHndl =
      registry.read("opaque_depth_with_water").texture().atStage(dafg::Stage::PS).useAs(dafg::Usage::SHADER_RESOURCE).handle();

    auto waterModeHndl = registry.readBlob<WaterRenderMode>("water_render_mode").handle();
    const auto mainViewResolution = registry.getResolution<2>("main_view");
    return [downsampledDepthHdnl, depthHndl, waterModeHndl, makeCheckerboardDepth, mainViewResolution] {
      if (waterModeHndl.ref() == WaterRenderMode::LATE)
        return;

      ManagedTexView downsampledDepthWithWaterTex = downsampledDepthHdnl.view();

      ManagedTexView depth = depthHndl.view();

      TIME_D3D_PROFILE(downsample_depth_after_water);

      TextureIDPair downsampleDst = TextureIDPair(downsampledDepthWithWaterTex.getTex2D(), downsampledDepthWithWaterTex.getTexId());

      auto [renderingWidth, renderingHeight] = mainViewResolution.get();

      if (makeCheckerboardDepth)
        downsample_depth::downsamplePS(TextureIDPair(depth.getTex2D(), depth.getTexId()), renderingWidth, renderingHeight,
          nullptr /*far_depth*/, nullptr /*close_depth*/, nullptr /*far_normals*/, nullptr /*motion_vectors*/, &downsampleDst);
      else
        downsample_depth::downsamplePS(TextureIDPair(depth.getTex2D(), depth.getTexId()), renderingWidth, renderingHeight,
          &downsampleDst, nullptr /*close_depth*/, nullptr /*far_normals*/, nullptr /*motion_vectors*/,
          nullptr /*checkerboard_depth*/);
    };
  });
}
