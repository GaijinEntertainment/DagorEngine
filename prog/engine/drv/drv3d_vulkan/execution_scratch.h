// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// collection of various temporal information that used inside single work item execution, but need memory backing

#include <drv/3d/dag_commands.h>
#include <util/dag_stdint.h>
#include <EASTL/vector.h>

#include "vk_wrapped_handles.h"
#include "buffer_ref.h"

namespace drv3d_vulkan
{

class Image;

struct ExecutionScratch
{
  eastl::vector<FrameEvents *> frameEventCallbacks;
  eastl::vector<Image *> imageResidenceRestores;

  struct DiscardNotify
  {
    BufferRef oldBuf;
    BufferRef newBuf;
    uint32_t flags;
  };
  eastl::vector<DiscardNotify> delayedDiscards;
  eastl::vector<VulkanCommandBufferHandle> cmdListsToSubmit;
};

} // namespace drv3d_vulkan
