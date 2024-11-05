// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// simple execution state part that uses POD fields and no tracking is required

#include <drv/3d/dag_commands.h>
#include "image_resource.h"
#include "buffer_resource.h"
#include <util/dag_stdint.h>
#include "vk_wrapped_handles.h"

namespace drv3d_vulkan
{

struct ExecutionPODState
{
  uint32_t *asyncPipelineCompileFeedbackPtr = nullptr;
  // signals that we want to compile pipeline, but it will not be used with current state
  bool nonDrawCompile = false;
};

} // namespace drv3d_vulkan
