// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "frameGraphNodes.h"

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <render/daFrameGraph/daFG.h>

#include <render/renderEvent.h>


dafg::NodeHandle makeTargetRenameBeforeMotionBlurNode()
{
  return dafg::register_node("target_rename_before_motion_blur_node", DAFG_PP_NODE_SRC,
    [](dafg::Registry registry) { registry.renameTexture("frame_after_distortion", "target_before_motion_blur"); });
}

ECS_TAG(render)
ECS_ON_EVENT(OnCameraNodeConstruction)
static void create_target_rename_before_motion_blur_node_es(const OnCameraNodeConstruction &evt)
{
  evt.nodes->push_back(makeTargetRenameBeforeMotionBlurNode());
}
