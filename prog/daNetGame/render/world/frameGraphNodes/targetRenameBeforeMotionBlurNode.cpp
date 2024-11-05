// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "frameGraphNodes.h"

#include <render/daBfg/bfg.h>

dabfg::NodeHandle makeTargetRenameBeforeMotionBlurNode()
{
  return dabfg::register_node("target_rename_before_motion_blur_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.renameTexture("target_for_transparency", "target_before_motion_blur", dabfg::History::No);
  });
}
