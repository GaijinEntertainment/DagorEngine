// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#import "Metal/Metal.h"

#include <EASTL/vector.h>
#include <memory/dag_framemem.h>

namespace drv3d_metal
{
class Buffer;
}

namespace acceleration_structure_descriptors
{
MTLInstanceAccelerationStructureDescriptor *getTLASDescriptor(uint32_t elements, RaytraceBuildFlags flags);

MTLPrimitiveAccelerationStructureDescriptor *getBLASDescriptor(const RaytraceGeometryDescription *desc, uint32_t count,
  RaytraceBuildFlags flags, eastl::vector<drv3d_metal::Buffer *, framemem_allocator> *resources = nullptr);
} // namespace acceleration_structure_descriptors
