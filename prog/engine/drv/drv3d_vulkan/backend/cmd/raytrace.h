// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "buffer_ref.h"
#include "raytrace_as_resource.h"

namespace drv3d_vulkan
{

#if VULKAN_HAS_RAYTRACING
struct CmdTraceRays
{
  BufferRef rayGenTable;
  BufferRef missTable;
  BufferRef hitTable;
  BufferRef callableTable;
  uint32_t rayGenOffset;
  uint32_t missOffset;
  uint32_t missStride;
  uint32_t hitOffset;
  uint32_t hitStride;
  uint32_t callableOffset;
  uint32_t callableStride;
  uint32_t width;
  uint32_t height;
  uint32_t depth;
};

struct CmdRaytraceBuildStructures
{
  size_t index;
  uint32_t count;
};

struct CmdCopyRaytraceAccelerationStructure
{
  RaytraceAccelerationStructure *src;
  RaytraceAccelerationStructure *dst;
  bool compact;
};
#endif

} // namespace drv3d_vulkan
