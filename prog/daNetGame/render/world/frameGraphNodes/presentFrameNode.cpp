// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daBfg/bfg.h>

#include "frameGraphNodes.h"

#define INSIDE_RENDERER 1
#include "../private_worldRenderer.h"

dabfg::NodeHandle makeFrameToPresentProducerNode()
{
  return dabfg::register_node("present_frame_producer", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.multiplex(dabfg::multiplexing::Mode::Viewport);
    auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
    registry.registerTexture2d("frame_to_present",
      [&wr](const dabfg::multiplexing::Index) -> ManagedTexView { return *wr.finalTargetFrame; });
  });
}
