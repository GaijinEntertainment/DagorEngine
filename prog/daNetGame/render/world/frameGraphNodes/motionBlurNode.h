// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/daBfg/bfg.h>

enum class MotionBlurNodeStatus
{
  UNINITIALIZED,
  DISABLED,
  ENABLED
};

struct MotionBlurNodePointers
{
  dabfg::NodeHandle *accumulationNode;
  dabfg::NodeHandle *combineNode;
  MotionBlurNodeStatus *status;
};

void enable_motion_blur_node(MotionBlurNodePointers pointers);
void disable_motion_blur_node(MotionBlurNodePointers pointers);
void recreate_motion_blur_nodes(MotionBlurNodePointers pointers);