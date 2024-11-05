// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/motionBlur.h>
#include <daECS/core/entityComponent.h>
#include <EASTL/utility.h>

ECS_DECLARE_BOXED_TYPE(MotionBlur);

enum class MotionBlurMode
{
  RAW_DEPTH,
  LINEAR_DEPTH,
  MOTION_VECTOR
};

eastl::pair<MotionBlur *, MotionBlurMode> query_motion_blur();