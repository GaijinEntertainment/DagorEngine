// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>
#include "buffer_ref.h"

namespace drv3d_vulkan
{

struct CmdNotifyBufferDiscard
{
  BufferRef oldBuf;
  BufferRef newBuf;
  uint32_t bufFlags;
};

struct CmdAddRefFrameMem
{
  Buffer *buf;
};

struct CmdReleaseFrameMem
{
  Buffer *buf;
};

} // namespace drv3d_vulkan
