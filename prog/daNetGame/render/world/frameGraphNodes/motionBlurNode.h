// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/daFrameGraph/daFG.h>

enum class MotionBlurNodeStatus
{
  UNINITIALIZED,
  DISABLED,
  ENABLED
};

struct MotionBlurNodePointers
{
  dafg::NodeHandle *accumulationNode;
  dafg::NodeHandle *combineNode;
  MotionBlurNodeStatus *status;
};

void enable_motion_blur_node(MotionBlurNodePointers pointers);
void disable_motion_blur_node(MotionBlurNodePointers pointers);
void recreate_motion_blur_nodes(MotionBlurNodePointers pointers);