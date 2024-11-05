// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/rayTrace/dag_drvRayTrace.h>

#include "driver.h"
#include "resource_memory.h"

#include "resource_manager/basic_buffer.h"


namespace drv3d_dx12
{

#if D3D_HAS_RAY_TRACING
struct RaytraceAccelerationStructure
{
  // Warning: ASes are suballocated, this resource may also contain other ASes!
  ID3D12Resource *asHeapResource = {};
  // Only present for top-level ASes
  D3D12_CPU_DESCRIPTOR_HANDLE descriptor = {};
  D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = {};
  // Actual size of this suballocation (might be larger than requested)
  uint32_t size = {};
  // The raw size requested by the user
  uint32_t requestedSize = {};
  // Stuff required for graceful deallocation
  uint16_t slotInAsHeap = {};
  uint16_t asHeapIdx = {};
};
#endif

} // namespace drv3d_dx12
