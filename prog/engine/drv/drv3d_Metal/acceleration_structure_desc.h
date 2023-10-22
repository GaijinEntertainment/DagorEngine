#pragma once

#include "acceleration_structure.h"

namespace acceleration_structure_descriptors
{
API_AVAILABLE(ios(15.0), macos(11.0))
MTLInstanceAccelerationStructureDescriptor *getTLASDescriptor(Sbuffer *instance_descriptor_buffer, uint32_t instance_count,
  RaytraceBuildFlags struct_flags, RaytraceBuildFlags build_flags, bool update);
API_AVAILABLE(ios(15.0), macos(11.0))
MTLPrimitiveAccelerationStructureDescriptor *getBLASDescriptor(RaytraceGeometryDescription *desc, uint32_t count,
  RaytraceBuildFlags struct_flags, RaytraceBuildFlags build_flags, bool update);
} // namespace acceleration_structure_descriptors
