// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "rtx_components.h"

#ifdef D3D_HAS_RAY_TRACING

#include "device.h"


namespace drv3d_dx12::resource_manager
{
::raytrace::AccelerationStructurePool drv3d_dx12::resource_manager::RaytraceAccelerationStructurePoolProvider::
  createAccelerationStructurePool(Device &device, const ::raytrace::AccelerationStructurePoolCreateInfo &info)
{
  auto newPool = eastl::make_unique<RayTraceAccelerationStructurePool>();
  newPool->sizeInBytes = info.sizeInBytes;

  D3D12_HEAP_PROPERTIES memoryProperties = {
    .Type = D3D12_HEAP_TYPE_DEFAULT,
    .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
    .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
    .CreationNodeMask = 0,
    .VisibleNodeMask = 0,
  };

  D3D12_RESOURCE_DESC desc = {
    .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
    .Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
    .Width = info.sizeInBytes,
    .Height = 1,
    .DepthOrArraySize = 1,
    .MipLevels = 1,
    .Format = DXGI_FORMAT_UNKNOWN,
    .SampleDesc =
      {
        .Count = 1,
        .Quality = 0,
      },
    .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
    .Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
  };

  device.getDevice()->CreateCommittedResource(&memoryProperties, D3D12_HEAP_FLAG_NONE, &desc,
    D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, COM_ARGS(&newPool->poolResource));

  if (newPool->poolResource)
  {
    newPool->baseAddress = newPool->poolResource->GetGPUVirtualAddress();
  }

  auto result = reinterpret_cast<::raytrace::AccelerationStructurePool>(newPool.get());
  pools.access()->push_back(eastl::move(newPool));

  recordRaytraceAccelerationStructurePoolAllocated(info.sizeInBytes);
  return result;
}

RaytraceAccelerationStructure *drv3d_dx12::resource_manager::RaytraceAccelerationStructurePoolProvider::createAccelerationStructure(
  Device &device, ::raytrace::AccelerationStructurePool pool, const ::raytrace::TopAccelerationStructurePlacementInfo &info)
{
  auto asPool = reinterpret_cast<RayTraceAccelerationStructurePool *>(pool);
  auto newAs = asPool->subStructures.allocate();
  newAs->asHeapResource = asPool->poolResource.Get();
  newAs->gpuAddress = asPool->baseAddress + info.offsetInBytes;
  newAs->size = info.sizeInBytes;
  newAs->requestedSize = info.sizeInBytes;

  D3D12_SHADER_RESOURCE_VIEW_DESC desc = {
    .Format = DXGI_FORMAT_UNKNOWN,
    .ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE,
    .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
    .RaytracingAccelerationStructure =
      {
        .Location = newAs->gpuAddress,
      },
  };
  newAs->descriptor = allocateBufferSRVDescriptor(device.getDevice());
  device.getDevice()->CreateShaderResourceView(nullptr /*must be null*/, &desc, newAs->descriptor);

  recordRaytraceTopStructureAllocated(info.sizeInBytes);
  return newAs;
}

RaytraceAccelerationStructure *drv3d_dx12::resource_manager::RaytraceAccelerationStructurePoolProvider::createAccelerationStructure(
  ::raytrace::AccelerationStructurePool pool, const ::raytrace::BottomAccelerationStructurePlacementInfo &info)
{
  auto asPool = reinterpret_cast<RayTraceAccelerationStructurePool *>(pool);
  auto newAs = asPool->subStructures.allocate();
  newAs->asHeapResource = asPool->poolResource.Get();
  newAs->gpuAddress = asPool->baseAddress + info.offsetInBytes;
  newAs->size = info.sizeInBytes;
  newAs->requestedSize = info.sizeInBytes;

  recordRaytraceBottomStructureAllocated(info.sizeInBytes);
  return newAs;
}

RaytraceAccelerationStructureHeap RaytraceAccelerationStructureObjectProvider::allocAccelStructHeap(Device &device, uint32_t size)
{

  ::raytrace::AccelerationStructurePoolCreateInfo poolCreateInfo = {
    .debugName = "DriverManagedPool",
    .sizeInBytes = size,
  };

  RaytraceAccelerationStructureHeap heap;
  heap.pool = reinterpret_cast<RayTraceAccelerationStructurePool *>(createAccelerationStructurePool(device, poolCreateInfo));

  memoryUsed += heap.pool->sizeInBytes;

  return heap;
}

void RaytraceAccelerationStructureObjectProvider::freeAccelStructHeap(RaytraceAccelerationStructureHeap &&heap)
{
  G_FAST_ASSERT(heap.freeSlots.all());
  G_FAST_ASSERT(heap.takenSlotCount == 0);
  // recordRaytraceAccelerationStructureHeapFreed(static_cast<uint32_t>(heap.bufferMemory.size()));
  memoryUsed -= heap.pool->sizeInBytes;

  // we remove the pool directly as here we already had the time to complete all frames using this pool and
  // we are already under the lock for the frame related data
  removePool(heap.pool);
}

static uint32_t align_as_size(uint32_t size)
{
  static constexpr uint32_t POW2_UP_TO = RAYTRACE_HEAP_ALIGNMENT / 2;
  if (size < RAYTRACE_AS_ALIGNMENT)
    size = RAYTRACE_AS_ALIGNMENT;
  else if (size < POW2_UP_TO)
    size = get_bigger_pow2(size);
  else
    size = align_value(size, POW2_UP_TO);

  return size;
}

drv3d_dx12::RaytraceAccelerationStructure *RaytraceAccelerationStructureObjectProvider::allocAccelStruct(Device &device, uint32_t size)
{
  OSSpinlockScopedLock lock{rtasSpinlock};

  const uint32_t alignedSize = align_as_size(size);

  auto &bucket = heapBuckets[alignedSize];

  uint16_t heapIdx = 0;
  uint16_t slotIdx = 0;

  // NOTE: this MIGHT be 0, but it works out to a single heap per huge resource
  const uint16_t slotsInHeap = RAYTRACE_HEAP_SIZE / alignedSize;
  G_FAST_ASSERT(slotsInHeap <= RaytraceAccelerationStructureHeap::SLOTS);

  for (auto &heap : bucket)
  {
    const uint16_t slot = heap.freeSlots.find_first();
    if (heap.pool && slot < slotsInHeap)
    {
      slotIdx = slot;
      break;
    }
    ++heapIdx;
  }

  if (heapIdx == bucket.size())
  {
    heapIdx = 0;
    while (heapIdx < bucket.size() && bucket[heapIdx].pool)
      ++heapIdx;

    if (heapIdx == bucket.size())
      bucket.emplace_back();

    // If AS doesn't fit in default heap size, basically make a dedicated
    // allocation for it. If only a single AS fits, again, dedicated alloc but of smaller size.
    // In both cases we should be "honest" about how much memory we are using
    // by aligning to 64K, because a "tail" of <64K size can never be used by anything else.
    const uint32_t heapSize = align_value(slotsInHeap > 1 ? slotsInHeap * alignedSize : alignedSize, RAYTRACE_HEAP_ALIGNMENT);

    bucket[heapIdx] = allocAccelStructHeap(device, heapSize);
  }

  auto &heap = bucket[heapIdx];

  // Allocation failed completely for some reason, should not happen probably?
  if (!heap.pool)
    return nullptr;

  G_FAST_ASSERT(heap.freeSlots.test(slotIdx));
  heap.freeSlots.set(slotIdx, false);
  heap.takenSlotCount++;

  auto result = heap.pool->subStructures.allocate();
  result->asHeapResource = heap.pool->poolResource.Get();
  result->descriptor = {};
  result->gpuAddress = heap.pool->baseAddress + slotIdx * alignedSize;
  result->size = alignedSize;
  result->slotInAsHeap = slotIdx;
  result->asHeapIdx = heapIdx;
  result->requestedSize = size;
  return result;
}

void RaytraceAccelerationStructureObjectProvider::freeAccelStruct(RaytraceAccelerationStructure *accelStruct)
{
  OSSpinlockScopedLock lock{rtasSpinlock};

  auto &bucket = heapBuckets[accelStruct->size];
  auto &heap = bucket[accelStruct->asHeapIdx];

  // need struct size after the object is freed
  auto structSize = accelStruct->size;
  G_FAST_ASSERT(!heap.freeSlots.test(accelStruct->slotInAsHeap));
  heap.freeSlots.set(accelStruct->slotInAsHeap, true);
  heap.pool->subStructures.free(accelStruct);
  if (--heap.takenSlotCount == 0)
  {
    freeAccelStructHeap(eastl::exchange(heap, {}));
  }

  while (!bucket.empty() && !bucket.back().pool)
    bucket.pop_back();

  if (bucket.empty())
    heapBuckets.erase(structSize);
}

drv3d_dx12::RaytraceAccelerationStructure *drv3d_dx12::resource_manager::RaytraceAccelerationStructureObjectProvider::
  newRaytraceTopAccelerationStructure(Device &device, uint64_t size)
{
  auto result = allocAccelStruct(device, size);

  if (result)
  {
    D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    desc.RaytracingAccelerationStructure.Location = result->gpuAddress;
    result->descriptor = allocateBufferSRVDescriptor(device.getDevice());
    device.getDevice()->CreateShaderResourceView(nullptr /*must be null*/, &desc, result->descriptor);

    recordRaytraceTopStructureAllocated(size);
  }
  return result;
}

drv3d_dx12::RaytraceAccelerationStructure *drv3d_dx12::resource_manager::RaytraceAccelerationStructureObjectProvider::
  newRaytraceBottomAccelerationStructure(Device &device, uint64_t size)
{
  G_ASSERT(size < static_cast<uint64_t>(UINT32_MAX));

  auto result = allocAccelStruct(device, size);

  if (result)
  {
    recordRaytraceBottomStructureAllocated(size);
  }
  return result;
}
} // namespace drv3d_dx12::resource_manager
#endif