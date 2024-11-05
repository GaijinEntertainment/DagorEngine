// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/daBfg/bfg.h>
#include <render/rendererFeatures.h>

inline eastl::optional<dabfg::VirtualResourceHandle<const BaseTexture, true, false>> read_prev_frame_tex(dabfg::Registry registry,
  dabfg::Stage stage = dabfg::Stage::PS)
{
  const char *prevFrameTexName = nullptr;
  const char *prevFrameSamplerName = nullptr;
  if (renderer_has_feature(FeatureRenderFlags::PREV_OPAQUE_TEX))
  {
    prevFrameSamplerName = "prev_frame_sampler";
    prevFrameTexName = "prev_frame_tex";
  }
  else if (!renderer_has_feature(FeatureRenderFlags::FORWARD_RENDERING))
  {
    prevFrameSamplerName = "gbuf_sampler";
    prevFrameTexName = "gbuf_1";
  }
  registry.read(prevFrameSamplerName).blob<d3d::SamplerHandle>().bindToShaderVar("prev_frame_tex_samplerstate").optional();
  if (prevFrameTexName)
    return registry.root().read(prevFrameTexName).texture().atStage(stage).bindToShaderVar("prev_frame_tex").handle();
  return eastl::nullopt;
}