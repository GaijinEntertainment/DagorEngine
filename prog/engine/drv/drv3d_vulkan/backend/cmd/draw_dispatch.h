// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>
#include "vulkan_api.h"
#include "buffer_ref.h"

namespace drv3d_vulkan
{

struct CmdDispatch
{
  uint32_t x;
  uint32_t y;
  uint32_t z;
};

struct CmdDispatchIndirect
{
  BufferRef buffer;
  uint32_t offset;
};

struct CmdClearView
{
  int clearFlags;
};

struct CmdDrawIndirect
{
  VkPrimitiveTopology top;
  uint32_t count;
  BufferRef buffer;
  uint32_t offset;
  uint32_t stride;
};

struct CmdDrawIndexedIndirect
{
  VkPrimitiveTopology top;
  uint32_t count;
  BufferRef buffer;
  uint32_t offset;
  uint32_t stride;
};

struct CmdDraw
{
  VkPrimitiveTopology top;
  uint32_t start;
  uint32_t count;
  uint32_t firstInstance;
  uint32_t instanceCount;
};

struct CmdDrawIndexed
{
  VkPrimitiveTopology top;
  uint32_t indexStart;
  uint32_t count;
  int32_t vertexBase;
  uint32_t firstInstance;
  uint32_t instanceCount;
};

} // namespace drv3d_vulkan
