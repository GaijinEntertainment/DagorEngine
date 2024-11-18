// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "lensFlareNodes.h"
#include "lensFlareRenderer.h"

#include <render/daBfg/bfg.h>
#include <EASTL/shared_ptr.h>

dabfg::NodeHandle create_lens_flare_render_node(const LensFlareRenderer *renderer, const LensFlareQualityParameters &quality)
{
  return get_lens_flare_namespace().registerNode("render", DABFG_PP_NODE_SRC, [renderer, quality](dabfg::Registry registry) {
    auto hasLensFlaresHndl = registry.readBlob<int>("has_lens_flares").handle();

    const auto resolution = registry.getResolution<2>("post_fx", quality.resolutionScaling);
    uint32_t rtFmt = TEXFMT_R11G11B10F;
    if (!(d3d::get_texformat_usage(rtFmt) & d3d::USAGE_RTARGET))
      rtFmt = TEXFMT_A16B16G16R16F;
    const uint32_t texFmt = rtFmt | TEXCF_RTARGET;
    auto lensFlareTexture = registry.createTexture2d("lens_flares", dabfg::History::No, {texFmt, resolution});
    registry.requestRenderPass().color({lensFlareTexture}).clear(lensFlareTexture, make_clear_value(0.f, 0.f, 0.f, 0.f));

    registry.requestState().setFrameBlock("global_frame");

    return [renderer, resolution, hasLensFlaresHndl]() {
      if (!hasLensFlaresHndl.ref())
        return;
      renderer->render(resolution.get());
    };
  });
}
