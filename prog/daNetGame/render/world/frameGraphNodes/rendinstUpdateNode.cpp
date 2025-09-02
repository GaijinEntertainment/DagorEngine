// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <rendInst/rendInstGen.h>
#include <render/daFrameGraph/daFG.h>
#include <render/world/frameGraphHelpers.h>
#include "frameGraphNodes.h"

dafg::NodeHandle makeRendinstUpdateNode()
{
  auto rootNs = dafg::root();
  return rootNs.registerNode("ri_update_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    // RI cell buffer may be outdated and need to update it before ri rendering to avoid splitting render pass
    registry.createBlob<OrderingToken>("ri_update_token", dafg::History::No);
    return [] { rendinst::updateHeapVb(); };
  });
}
