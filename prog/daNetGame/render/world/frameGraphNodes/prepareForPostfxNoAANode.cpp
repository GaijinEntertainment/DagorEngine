// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/resourceSlot/registerAccess.h>

#include "frameGraphNodes.h"

resource_slot::NodeHandleWithSlotsAccess makePrepareForPostfxNoAANode()
{
  return resource_slot::register_access("prepare_for_postfx", DABFG_PP_NODE_SRC,
    {resource_slot::Create{"postfx_input_slot", "frame_for_postfx"}}, [](resource_slot::State slotsState, dabfg::Registry registry) {
      registry.renameTexture("final_target_with_motion_blur", slotsState.resourceToCreateFor("postfx_input_slot"), dabfg::History::No);
    });
}
