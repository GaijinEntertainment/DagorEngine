// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#import "Metal/Metal.h"

namespace acceleration_structure_descriptors
{
API_AVAILABLE(ios(15.0), macos(11.0))
MTLInstanceAccelerationStructureDescriptor *getTLASDescriptor(uint32_t elements, RaytraceBuildFlags flags);

API_AVAILABLE(ios(15.0), macos(11.0))
MTLPrimitiveAccelerationStructureDescriptor *getBLASDescriptor(const RaytraceGeometryDescription *desc, uint32_t count,
  RaytraceBuildFlags flags);
} // namespace acceleration_structure_descriptors
