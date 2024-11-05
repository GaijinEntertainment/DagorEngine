// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daBfg/bfg.h>
#include <render/downsampleDepth.h>
#include <perfMon/dag_statDrv.h>
#include <3d/dag_textureIDHolder.h>

#include "frameGraphNodes.h"
#include <render/world/frameGraphHelpers.h>

#define INSIDE_RENDERER 1
#include "../private_worldRenderer.h"

dabfg::NodeHandle makeDownsampleDepthWithWaterNode()
{
  return dabfg::register_node("downsample_depth_with_water_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    read_gbuffer_motion(registry).optional();

    auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
    bool makeCheckerboardDepth = wr.hasFeature(FeatureRenderFlags::UPSCALE_SAMPLING_TEX);

    auto downsampledDepthHdnl =
      registry.create(makeCheckerboardDepth ? "checkerboard_depth_with_water" : "far_downsampled_depth_with_water", dabfg::History::No)
        .texture({TEXFMT_DEPTH32 | TEXCF_RTARGET, registry.getResolution<2>("main_view", 0.5f)})
        .atStage(dabfg::Stage::PS)
        .useAs(dabfg::Usage::DEPTH_ATTACHMENT)
        .handle();

    auto depthHndl =
      registry.read("opaque_depth_with_water").texture().atStage(dabfg::Stage::PS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();

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
