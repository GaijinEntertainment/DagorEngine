// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/rayTrace/dag_drvRayTrace.h>

#if D3D_HAS_RAY_TRACING

#include <driver.h>

namespace drv3d_dx12
{
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

} // namespace drv3d_dx12

#endif
