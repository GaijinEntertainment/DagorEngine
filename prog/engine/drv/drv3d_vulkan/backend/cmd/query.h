// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>
#include "driver.h"
#include "queries.h"
#include "buffer_resource.h"

namespace drv3d_vulkan
{

struct CmdBeginSurvey
{
  VulkanQueryPoolHandle pool;
  uint32_t index;
};

struct CmdEndSurvey
{
  VulkanQueryPoolHandle pool;
  uint32_t index;
  Buffer *buffer;
};

struct CmdInsertTimesampQuery
{};

struct CmdStartOcclusionQuery
{
  QueryIndex queryIndex;
};

struct CmdEndOcclusionQuery
{
  QueryIndex queryIndex;
};

} // namespace drv3d_vulkan
