// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "lensFlareNodes.h"

#include <render/lensFlare/render/lensFlareRenderer.h>
#include <render/daFrameGraph/daFG.h>
#include <EASTL/shared_ptr.h>

dafg::NodeHandle create_lens_flare_render_node(const LensFlareRenderer *renderer, const LensFlareQualityParameters &quality)
{
  return get_lens_flare_namespace().registerNode("render", DAFG_PP_NODE_SRC, [renderer, quality](dafg::Registry registry) {
    auto hasLensFlaresHndl = registry.readBlob<int>("has_lens_flares").handle();

    const auto resolution = registry.getResolution<2>("post_fx", quality.resolutionScaling);
    uint32_t rtFmt = TEXFMT_R11G11B10F;
    if (!(d3d::get_texformat_usage(rtFmt) & d3d::USAGE_RTARGET))
      rtFmt = TEXFMT_A16B16G16R16F;
    const uint32_t texFmt = rtFmt | TEXCF_RTARGET;
    auto lensFlareTexture =
      registry.createTexture2d("lens_flares", dafg::History::No, {texFmt, resolution}).clear(make_clear_value(0.f, 0.f, 0.f, 0.f));
    registry.requestRenderPass().color({lensFlareTexture});

    registry.requestState().setFrameBlock("global_frame");

    return [renderer, resolution, hasLensFlaresHndl]() {
      if (!hasLensFlaresHndl.ref())
        return;
      renderer->render(resolution.get());
    };
  });
}
