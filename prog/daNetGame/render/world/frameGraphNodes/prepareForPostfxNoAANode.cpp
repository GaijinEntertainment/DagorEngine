// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/resourceSlot/registerAccess.h>

#include "frameGraphNodes.h"

dabfg::NodeHandle makePrepareForPostfxNoAANode()
{
  return dabfg::register_node("prepare_for_postfx", DABFG_PP_NODE_SRC,
    [](dabfg::Registry registry) { registry.renameTexture("final_target_with_motion_blur", "frame_after_aa", dabfg::History::No); });
}
