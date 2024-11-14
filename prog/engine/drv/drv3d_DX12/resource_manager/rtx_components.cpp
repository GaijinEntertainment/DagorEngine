// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "rtx_components.h"

#ifdef D3D_HAS_RAY_TRACING

#include "device.h"


namespace drv3d_dx12::resource_manager
{

RaytraceAccelerationStructureHeap RaytraceAccelerationStructureObjectProvider::allocAccelStructHeap(DXGIAdapter *adapter,
  Device &device, uint32_t size)
{
  auto properties = getProperties(D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, DeviceMemoryClass::DEVICE_RESIDENT_BUFFER,
    D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

  D3D12_RESOURCE_DESC desc;
  desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
  desc.Width = size;
  desc.Height = 1;
  desc.DepthOrArraySize = 1;
  desc.MipLevels = 1;
  desc.Format = DXGI_FORMAT_UNKNOWN;
  desc.SampleDesc.Count = 1;
  desc.SampleDesc.Quality = 0;
  desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

  D3D12_RESOURCE_ALLOCATION_INFO allocInfo;
  allocInfo.SizeInBytes = desc.Width;
  allocInfo.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

  HRESULT errorCode = S_OK;
  auto oomCheckOnExit = checkForOOMOnExit(
    adapter, [&errorCode]() { return !is_oom_error_code(errorCode); },
    OomReportData{"allocAccelStructHeap", nullptr, size, AllocationFlags{}.to_ulong(), properties.raw});

  auto memory = allocate(adapter, device.getDevice(), properties, allocInfo, {}, &errorCode);
  if (!memory)
  {
    device.processEmergencyDefragmentation(properties.raw, true, true, false);
    errorCode = S_OK;
    memory = allocate(adapter, device.getDevice(), properties, allocInfo, {}, &errorCode);
  }
  const ResourceHeapProperties allocatedProperties{memory.getHeapID().group};
  if (!memory)
    return {};

  RaytraceAccelerationStructureHeap heap;
  errorCode = heap.create(device.getDevice(), desc, memory, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
    allocatedProperties.isCPUVisible());
  if (DX12_CHECK_FAIL(errorCode))
  {
    free(memory, false);
    return {};
  }

  updateMemoryRangeUse(memory, RaytraceAccelerationStructureHeapReference{heap.buffer.Get()});

  memoryUsed += memory.size();
  recordRaytraceAccelerationStructureHeapAllocated(static_cast<uint32_t>(memory.size()));

  return heap;
}

void RaytraceAccelerationStructureObjectProvider::freeAccelStructHeap(RaytraceAccelerationStructureHeap &&heap)
{
  G_FAST_ASSERT(heap.freeSlots.all());
  G_FAST_ASSERT(heap.takenSlotCount == 0);
  recordRaytraceAccelerationStructureHeapFreed(static_cast<uint32_t>(heap.bufferMemory.size()));
  memoryUsed -= heap.bufferMemory.size();
  heap.reset(this);
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

drv3d_dx12::RaytraceAccelerationStructure *RaytraceAccelerationStructureObjectProvider::allocAccelStruct(DXGIAdapter *adapter,
  Device &device, uint32_t size)
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
    if (heap && slot < slotsInHeap)
    {
      slotIdx = slot;
      break;
    }
    ++heapIdx;
  }

  if (heapIdx == bucket.size())
  {
    heapIdx = 0;
    while (heapIdx < bucket.size() && bucket[heapIdx])
      ++heapIdx;

    if (heapIdx == bucket.size())
      bucket.emplace_back();

    // If AS doesn't fit in default heap size, basically make a dedicated
    // allocation for it. If only a single AS fits, again, dedicated alloc but of smaller size.
    // In both cases we should be "honest" about how much memory we are using
    // by aligning to 64K, because a "tail" of <64K size can never be used by anything else.
    const uint32_t heapSize = align_value(slotsInHeap > 1 ? slotsInHeap * alignedSize : alignedSize, RAYTRACE_HEAP_ALIGNMENT);

    bucket[heapIdx] = allocAccelStructHeap(adapter, device, heapSize);
  }

  auto &heap = bucket[heapIdx];

  // Allocation failed completely for some reason, should not happen probably?
  if (!heap)
    return nullptr;

  G_FAST_ASSERT(heap.freeSlots.test(slotIdx));
  heap.freeSlots.set(slotIdx, false);
  heap.takenSlotCount++;

  auto result = accelStructPool.allocate();
  result->asHeapResource = heap.buffer.Get();
  result->descriptor = {};
  result->gpuAddress = heap.getGPUPointer() + slotIdx * alignedSize;
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

  G_FAST_ASSERT(!heap.freeSlots.test(accelStruct->slotInAsHeap));
  heap.freeSlots.set(accelStruct->slotInAsHeap, true);
  if (--heap.takenSlotCount == 0)
  {
    freeAccelStructHeap(eastl::move(heap));
  }

  while (!bucket.empty() && !bucket.back())
    bucket.pop_back();

  if (bucket.empty())
    heapBuckets.erase(accelStruct->size);

  accelStructPool.free(accelStruct);
}

drv3d_dx12::RaytraceAccelerationStructure *drv3d_dx12::resource_manager::RaytraceAccelerationStructureObjectProvider::
  newRaytraceTopAccelerationStructure(DXGIAdapter *adapter, Device &device, uint64_t size)
{
  auto result = allocAccelStruct(adapter, device, size);

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
  newRaytraceBottomAccelerationStructure(DXGIAdapter *adapter, Device &device, uint64_t size)
{
  G_ASSERT(size < static_cast<uint64_t>(UINT32_MAX));

  auto result = allocAccelStruct(adapter, device, size);

  if (result)
  {
    recordRaytraceBottomStructureAllocated(size);
  }
  return result;
}
} // namespace drv3d_dx12::resource_manager
#endif