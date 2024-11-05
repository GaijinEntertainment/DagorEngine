// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <rendInst/rendInstGen.h>
#include <render/daBfg/bfg.h>
#include <render/world/frameGraphHelpers.h>
#include "frameGraphNodes.h"

dabfg::NodeHandle makeRendinstUpdateNode()
{
  auto rootNs = dabfg::root();
  return rootNs.registerNode("ri_update_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    // RI cell buffer may be outdated and need to update it before ri rendering to avoid splitting render pass
    registry.createBlob<OrderingToken>("ri_update_token", dabfg::History::No);
    return [] { rendinst::updateHeapVb(); };
  });
}
