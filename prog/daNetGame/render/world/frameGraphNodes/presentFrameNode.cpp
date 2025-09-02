// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/daFG.h>

#include "frameGraphNodes.h"
#include <render/world/frameGraphHelpers.h>

#define INSIDE_RENDERER 1
#include "../private_worldRenderer.h"

dafg::NodeHandle makeFrameToPresentProducerNode()
{
  return dafg::register_node("present_frame_producer", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.multiplex(dafg::multiplexing::Mode::Viewport);
    auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
    registry.registerTexture("frame_to_present",
      [&wr](const dafg::multiplexing::Index) -> ManagedTexView { return *wr.finalTargetFrame; });
  });
}
