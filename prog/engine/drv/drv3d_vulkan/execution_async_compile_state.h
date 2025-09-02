// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// execution state part for async pipelines compile

#include <drv/3d/dag_commands.h>
#include "image_resource.h"
#include "buffer_resource.h"
#include <util/dag_stdint.h>
#include "vk_wrapped_handles.h"

namespace drv3d_vulkan
{

struct ExecutionAsyncCompileState
{
  dag::Vector<uint32_t *> asyncPipelineCompileFeedbackStack;
  // counts number of skipped pipelines starting from last CmdAsyncPipeFeedbackBegin
  uint32_t currentSkippedCount = 0;
  // signals that we want to compile pipeline, but it will not be used with current state
  bool nonDrawCompile = false;
};

} // namespace drv3d_vulkan
