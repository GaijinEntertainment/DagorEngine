// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/resourceSlot/registerAccess.h>

#include "frameGraphNodes.h"

dafg::NodeHandle makePrepareForPostfxNoAANode()
{
  return dafg::register_node("prepare_for_postfx", DAFG_PP_NODE_SRC,
    [](dafg::Registry registry) { registry.renameTexture("final_target_with_motion_blur", "frame_after_aa", dafg::History::No); });
}
