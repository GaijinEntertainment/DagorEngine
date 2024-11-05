// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// struct for containment & managment of RT scratch buffer

#include <drv/3d/rayTrace/dag_drvRayTrace.h> // for D3D_HAS_RAY_TRACING
#include <osApiWrappers/dag_critSec.h>

#include "buffer_resource.h"

namespace drv3d_vulkan
{

class RaytraceScratchBuffer
{
  RaytraceScratchBuffer(const RaytraceScratchBuffer &);

#if D3D_HAS_RAY_TRACING
  static constexpr VkDeviceSize alignmentMask = 1024 - 1;
  Buffer *buf = nullptr;
  WinCritSec mutex;

public:
  RaytraceScratchBuffer() = default;
  void shutdown();
  void ensureRoomForBuild(VkDeviceSize size);

  Buffer *get()
  {
    // FIXME: potentional race (it is backed up by delayed destruction in frame)
    WinAutoLock lock(mutex);
    return buf;
  }
#else
public:
  RaytraceScratchBuffer() = default;
  void shutdown(){};
#endif
};

} // namespace drv3d_vulkan
