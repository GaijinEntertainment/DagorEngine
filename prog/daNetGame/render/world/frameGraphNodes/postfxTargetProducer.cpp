// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/daFG.h>
#include <render/hdrRender.h>
#include <render/world/frameGraphHelpers.h>

#include "frameGraphNodes.h"

#define INSIDE_RENDERER 1
#include "../private_worldRenderer.h"

dafg::NodeHandle makePostfxTargetProducerNode(bool requires_multisampling)
{
  return dafg::register_node("prepare_postfx_target", DAFG_PP_NODE_SRC, [requires_multisampling](dafg::Registry registry) {
    registry.multiplex(requires_multisampling ? dafg::multiplexing::Mode::FullMultiplex : dafg::multiplexing::Mode::Viewport);
    auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
    const char *TEX_NAME = "postfxed_frame";
    if (requires_multisampling) // No resolution scaling here
      registry.createTexture2d(TEX_NAME, dafg::History::No,
        {TEXCF_RTARGET | get_frame_render_target_format(), registry.getResolution<2>("post_fx")});
    else if (wr.isFsrEnabled())
    {
      uint texFmt = TEXFMT_R8G8B8A8;
      if (hdrrender::is_hdr_enabled())
      {
        TextureInfo textureInfo;
        hdrrender::get_render_target_tex()->getinfo(textureInfo);
        texFmt = textureInfo.cflg & TEXFMT_MASK;
      }
      registry.createTexture2d(TEX_NAME, dafg::History::No, {TEXCF_RTARGET | texFmt, registry.getResolution<2>("post_fx")});
    }
    else if (wr.isFXAAEnabled())
    {
      registry.createTexture2d(TEX_NAME, dafg::History::No,
        {TEXCF_RTARGET | get_frame_render_target_format(), registry.getResolution<2>("post_fx")});
      wr.applyFXAASettings();
    }
    else if (wr.isStaticUpsampleEnabled() || wr.isSSAAEnabled())
      registry.createTexture2d(TEX_NAME, dafg::History::No,
        {TEXCF_RTARGET | get_frame_render_target_format(), registry.getResolution<2>("main_view")});
    else
      registry.registerTexture(TEX_NAME, [&wr](const dafg::multiplexing::Index) { return ManagedTexView(*wr.finalTargetFrame); });
  });
}
