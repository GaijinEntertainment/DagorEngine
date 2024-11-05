// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daBfg/bfg.h>
#include <render/hdrRender.h>
#include <render/world/frameGraphHelpers.h>

#include "frameGraphNodes.h"

#define INSIDE_RENDERER 1
#include "../private_worldRenderer.h"

dabfg::NodeHandle makePostfxTargetProducerNode(bool requires_multisampling)
{
  return dabfg::register_node("prepare_postfx_target", DABFG_PP_NODE_SRC, [requires_multisampling](dabfg::Registry registry) {
    registry.multiplex(requires_multisampling ? dabfg::multiplexing::Mode::FullMultiplex : dabfg::multiplexing::Mode::Viewport);
    auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
    const char *TEX_NAME = "postfxed_frame";
    if (requires_multisampling) // No resolution scaling here
      registry.createTexture2d(TEX_NAME, dabfg::History::No,
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
      registry.createTexture2d(TEX_NAME, dabfg::History::No, {TEXCF_RTARGET | texFmt, registry.getResolution<2>("post_fx")});
    }
    else if (wr.isFXAAEnabled())
    {
      registry.createTexture2d(TEX_NAME, dabfg::History::No,
        {TEXCF_RTARGET | get_frame_render_target_format(), registry.getResolution<2>("post_fx")});
      wr.applyFXAASettings();
    }
    else if (wr.isStaticUpsampleEnabled() || wr.isSSAAEnabled())
      registry.createTexture2d(TEX_NAME, dabfg::History::No,
        {TEXCF_RTARGET | get_frame_render_target_format(), registry.getResolution<2>("main_view")});
    else
      registry.registerTexture2d(TEX_NAME, [&wr](const dabfg::multiplexing::Index) { return ManagedTexView(*wr.finalTargetFrame); });
  });
}
