// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "raytrace_scratch_buffer.h"
#include "globals.h"
#include "debug_naming.h"
#include "device_context.h"

using namespace drv3d_vulkan;

#if D3D_HAS_RAY_TRACING

void RaytraceScratchBuffer::shutdown()
{
  if (buf)
    Globals::ctx.destroyBuffer(buf);
  buf = nullptr;
}

void RaytraceScratchBuffer::ensureRoomForBuild(VkDeviceSize size)
{
  size = (size + alignmentMask) & ~alignmentMask;
  WinAutoLock lock(mutex);

  if (buf)
  {
    if (buf->getBlockSize() >= size)
      return;
    Globals::ctx.destroyBuffer(buf);
  }

  buf = Buffer::create(size, DeviceMemoryClass::DEVICE_RESIDENT_BUFFER, 1, BufferMemoryFlags::NONE);
  Globals::Dbg::naming.setBufName(buf, "RT scratch buffer");
}

#endif