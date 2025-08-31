// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "frameGraphNodes.h"

#include <render/daFrameGraph/daFG.h>

dafg::NodeHandle makeTargetRenameBeforeMotionBlurNode()
{
  return dafg::register_node("target_rename_before_motion_blur_node", DAFG_PP_NODE_SRC,
    [](dafg::Registry registry) { registry.renameTexture("target_after_debug", "target_before_motion_blur", dafg::History::No); });
}
