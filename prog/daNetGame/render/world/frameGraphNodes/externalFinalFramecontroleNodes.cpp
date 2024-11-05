// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daBfg/bfg.h>

#include "frameGraphNodes.h"

#define INSIDE_RENDERER 1
#include "../private_worldRenderer.h"

eastl::array<dabfg::NodeHandle, 2> makeExternalFinalFrameControlNodes(bool requires_multisampling)
{
  return {dabfg::register_node("frame_after_postfx_producer", DABFG_PP_NODE_SRC,
            [=](dabfg::Registry registry) {
              auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
              registry.orderMeAfter("post_fx_node");
              // Same branches as in `prepare_postfx_target` node
              if (requires_multisampling)
              {
                // Rename FG-owned multiplexed texture, it will be passed to external finalTargetFrame later in sub/superSampling nodes
                registry.renameTexture("postfxed_frame", "frame_after_postfx", dabfg::History::No);
              }
              else if (wr.isFsrEnabled() || wr.isFXAAEnabled() || wr.isStaticUpsampleEnabled() || wr.isSSAAEnabled())
              {
                // These AA/upscalers use intermediate textures and write to external finalTargetFrame only in the end
                registry.registerTexture2d("frame_after_postfx",
                  [&wr](const dabfg::multiplexing::Index) -> ManagedTexView { return *wr.finalTargetFrame; });
              }
              else
              {
                // `postfxed_frame` is already external finalTargetFrame texture, so we can just rename it
                registry.renameTexture("postfxed_frame", "frame_after_postfx", dabfg::History::No);
              }
            }),
    dabfg::register_node("setup_frame_with_debug", DABFG_PP_NODE_SRC,
      [](dabfg::Registry registry) { registry.renameTexture("frame_after_postfx", "frame_with_debug", dabfg::History::No); })};
}
