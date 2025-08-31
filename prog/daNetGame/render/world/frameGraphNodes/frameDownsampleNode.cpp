// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/daFG.h>
#include <render/mipRenderer.h>
#include <render/world/frameGraphHelpers.h>
#include <util/dag_convar.h>

#include "frameGraphNodes.h"

extern ConVarT<int, true> water_quality;

dafg::NodeHandle makeFrameDownsampleNode()
{
  return dafg::register_node("downsample_frame_with_sky", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    uint32_t rtFmt = get_frame_render_target_format();
    const auto resolution = registry.getResolution<2>("main_view", water_quality.get() == 2 ? 1.f : 0.5f);
    auto downsampledFrameHndl = registry
                                  .createTexture2d("prev_frame_tex", dafg::History::ClearZeroOnFirstFrame,
                                    {rtFmt | TEXCF_RTARGET | TEXCF_GENERATEMIPS, resolution, 4})
                                  .atStage(dafg::Stage::PS)
                                  .useAs(dafg::Usage::COLOR_ATTACHMENT)
                                  .handle();
    auto frameHndl = registry.read("opaque_with_envi").texture().atStage(dafg::Stage::UNKNOWN).useAs(dafg::Usage::UNKNOWN).handle();

    int mip_blur_filter_nan_validationVarId = get_shader_glob_var_id("mip_blur_filter_nan_validation", true);
    return [frameHndl, downsampledFrameHndl, resolution, mip_blur_filter_nan_validationVarId,
             mipRenderer = MipRenderer("bloom_filter_mipchain")] {
      ShaderGlobal::set_int(mip_blur_filter_nan_validationVarId, 1);
      mipRenderer.renderTo(frameHndl.view().getBaseTex(), downsampledFrameHndl.get(), resolution.get());
      ShaderGlobal::set_int(mip_blur_filter_nan_validationVarId, 0);
      mipRenderer.render(downsampledFrameHndl.get());
    };
  });
}

dafg::NodeHandle makeFrameDownsampleSamplerNode()
{
  return dafg::register_node("downsample_frame_sampler_init", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.create("prev_frame_sampler", dafg::History::No).blob<d3d::SamplerHandle>(d3d::request_sampler({}));
  });
}

dafg::NodeHandle makeNoFxFrameNode()
{
  return dafg::register_node("downsample_frame_for_water_early", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("prepare_water_node");

    auto frameHndl = registry.read("opaque_resolved").texture().atStage(dafg::Stage::UNKNOWN).useAs(dafg::Usage::UNKNOWN).handle();
    const auto resolution = registry.getResolution<2>("main_view", water_quality.get() == 2 ? 1.f : 0.5f);
    auto waterModeHndl = registry.readBlob<WaterRenderMode>("water_render_mode").handle();
    auto downsampledFrameHndl = registry.create("prev_frame_tex_for_water_early", dafg::History::No)
                                  .texture({TEXFMT_R11G11B10F | TEXCF_RTARGET | TEXCF_GENERATEMIPS, resolution, 4})
                                  .atStage(dafg::Stage::POST_RASTER)
                                  .useAs(dafg::Usage::COLOR_ATTACHMENT)
                                  .handle();

    return [frameHndl, downsampledFrameHndl, waterModeHndl, resolution, mipRenderer = MipRenderer("bloom_filter_mipchain")] {
      if (waterModeHndl.ref() != WaterRenderMode::EARLY_BEFORE_ENVI)
        return;
      mipRenderer.renderTo(frameHndl.view().getBaseTex(), downsampledFrameHndl.get(), resolution.get());
      mipRenderer.render(downsampledFrameHndl.get());
    };
  });
}