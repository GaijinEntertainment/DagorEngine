// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// simple frontend state part that uses POD fields and no tracking is required
#include <util/dag_stdint.h>

namespace drv3d_vulkan
{

struct FrontendPODState
{
  // frame index increments on present
  uint32_t frameIndex = 0;
  // allocation count based trigger point for streaming pre clean
  uint32_t lastVisibleAllocations = 0;
  // async pipe feedback counter
  uint32_t summaryAsyncPipelineCompilationFeedback = 0;
  // draw actions counter, always incrementing for simplicity
  uint32_t drawsCount = 0;
  // counts passed native render passes, always incrementing for simplicity
  uint32_t nativeRenderPassesCount = 0;
};

} // namespace drv3d_vulkan
