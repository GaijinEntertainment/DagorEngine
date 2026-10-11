// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "rtx_components.h"

#ifdef D3D_HAS_RAY_TRACING

#include <device.h>
#include <debug/names.h>

#include <EASTL/algorithm.h>
#include <EASTL/numeric_limits.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>


namespace drv3d_dx12::resource_manager
{
RaytraceAccelerationStructurePoolProvider::AccelerationStructurePoolResult drv3d_dx12::resource_manager::
  RaytraceAccelerationStructurePoolProvider::createAccelerationStructurePool(Device &device,
    const ::raytrace::AccelerationStructurePoolCreateInfo &info)
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

  const auto errorCode = DX12_CHECK_RESULT(device.getDevice()->CreateCommittedResource(&memoryProperties, D3D12_HEAP_FLAG_NONE, &desc,
    D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, COM_ARGS(&newPool->poolResource)));
  if (FAILED(errorCode))
  {
    return dag::Unexpected{makeResourceCreationError(errorCode, info.sizeInBytes, ResourceHeapProperties{})};
  }

  if (info.debugName) // info.debugName is not unique across pools, so the serial stays part of the D3D12 name.
  {
    debug::name_resource(newPool->poolResource.Get(),
      debug::make_pool_object_name(debug::format_object_name("AccelerationStructurePool:%s", info.debugName)));
  }
  else
  {
    debug::name_resource(newPool->poolResource.Get(), debug::make_pool_object_name("AccelerationStructurePool"));
  }

  newPool->baseAddress = newPool->poolResource->GetGPUVirtualAddress();
  newPool->debugName = info.debugName;

  auto result = reinterpret_cast<::raytrace::AccelerationStructurePool>(newPool.get());
  pools.access()->push_back(eastl::move(newPool));

  recordRaytraceAccelerationStructurePoolAllocated(info.sizeInBytes);
  return result;
}

RaytraceAccelerationStructurePoolProvider::AccelerationStructureResult drv3d_dx12::resource_manager::
  RaytraceAccelerationStructurePoolProvider::createAccelerationStructure(Device &device, ::raytrace::AccelerationStructurePool pool,
    const ::raytrace::TopAccelerationStructurePlacementInfo &info)
{
  auto asPool = reinterpret_cast<RayTraceAccelerationStructurePool *>(pool);
  auto newAs = asPool->subStructures.allocate();
  newAs->asHeapResource = asPool->poolResource.Get();
  newAs->gpuAddress = asPool->baseAddress + info.offsetInBytes;
  newAs->size = info.sizeInBytes;
  newAs->requestedSize = info.sizeInBytes;
  newAs->type = RaytraceAccelerationStructure::Type::Top;

  D3D12_SHADER_RESOURCE_VIEW_DESC desc = {
    .Format = DXGI_FORMAT_UNKNOWN,
    .ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE,
    .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
    .RaytracingAccelerationStructure =
      {
        .Location = newAs->gpuAddress,
      },
  };
  return allocateBufferSRVDescriptor(device.getDevice())
    .transform([&, this](auto descriptor) {
      newAs->descriptor = descriptor;
      device.getDevice()->CreateShaderResourceView(nullptr /*must be null*/, &desc, newAs->descriptor);

      recordRaytraceTopStructureAllocated(info.sizeInBytes);
      return newAs;
    })
    .or_else([&](auto error) -> AccelerationStructureResult {
      asPool->subStructures.free(newAs);
      return dag::Unexpected{error};
    });
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
  newAs->type = RaytraceAccelerationStructure::Type::Bottom;

  recordRaytraceBottomStructureAllocated(info.sizeInBytes);
  return newAs;
}

RaytraceAccelerationStructure *drv3d_dx12::resource_manager::RaytraceAccelerationStructurePoolProvider::createAccelerationStructure(
  ::raytrace::AccelerationStructurePool pool, const ::raytrace::OpacityMicroMapTriangleArrayPlacementInfo &info)
{
  auto asPool = reinterpret_cast<RayTraceAccelerationStructurePool *>(pool);
  auto newAs = asPool->subStructures.allocate();
  newAs->asHeapResource = asPool->poolResource.Get();
  newAs->gpuAddress = asPool->baseAddress + info.offsetInBytes;
  newAs->size = info.sizeInBytes;
  newAs->requestedSize = info.sizeInBytes;
  newAs->type = RaytraceAccelerationStructure::Type::OpacityMicroMap;

  recordRaytraceOpacityMicroMapTriangleArrayAllocated(info.sizeInBytes);
  return newAs;
}

dag::Expected<RaytraceAccelerationStructureHeap, MemoryAllocationError> RaytraceAccelerationStructureObjectProvider::
  allocAccelStructHeap(Device &device, uint32_t aligned_size, uint16_t slot_count)
{
  // We should be "honest" about how much memory we are using by aligning to 64K, because a "tail"
  // of <64K size can never be used by anything else.
  ::raytrace::AccelerationStructurePoolCreateInfo poolCreateInfo = {
    .debugName = "DriverManagedPool",
    .sizeInBytes = align_value(slot_count * aligned_size, RAYTRACE_HEAP_ALIGNMENT),
  };

  auto poolResult = createAccelerationStructurePool(device, poolCreateInfo);
  if (!poolResult.has_value())
  {
    return dag::Unexpected{poolResult.error()};
  }

  RaytraceAccelerationStructureHeap heap;
  heap.pool = reinterpret_cast<RayTraceAccelerationStructurePool *>(poolResult.value());
  heap.pool->isDriverPool = true;
  heap.slotCount = slot_count;
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

// Each further heap of a bucket doubles, so a bucket whose working set keeps growing needs a
// logarithmic count of driver allocations for it, each of which is a kernel mode call.
static uint16_t heap_slot_count(uint32_t aligned_size, uint16_t live_heaps)
{
  uint32_t heapSize = RAYTRACE_HEAP_SIZE;
  for (uint16_t i = 0; i < live_heaps && heapSize < RAYTRACE_HEAP_MAX_SIZE; ++i)
    heapSize *= 2;

  // An AS bigger than the heap size gets a single slot, thus a heap of its own.
  return static_cast<uint16_t>(eastl::clamp<uint32_t>(heapSize / aligned_size, 1, RaytraceAccelerationStructureHeap::SLOTS));
}

RaytraceAccelerationStructureObjectProvider::AccelerationStructureResult RaytraceAccelerationStructureObjectProvider::allocAccelStruct(
  Device &device, uint32_t size, ResourceTagType tag, RaytraceAccelerationStructure::Type type)
{
  OSSpinlockScopedLock lock{rtasSpinlock};

  const uint32_t alignedSize = align_as_size(size);

  auto &bucket = heapBuckets[alignedSize];

  const uint16_t noHeap = static_cast<uint16_t>(bucket.size());
  uint16_t heapIdx = noHeap;
  uint16_t slotIdx = 0;
  uint16_t liveHeaps = 0;
  uint16_t emptyEntryIdx = noHeap;

  for (uint16_t i = 0; i < noHeap; ++i)
  {
    auto &heap = bucket[i];
    if (!heap.pool)
    {
      if (emptyEntryIdx == noHeap)
        emptyEntryIdx = i;
      continue;
    }
    ++liveHeaps;
    const uint16_t slot = heap.freeSlots.find_first();
    if (slot < heap.slotCount)
    {
      heapIdx = i;
      slotIdx = slot;
      break;
    }
  }

  if (heapIdx == noHeap)
  {
    heapIdx = emptyEntryIdx;
    if (heapIdx == noHeap)
      bucket.emplace_back();

    auto heapResult = allocAccelStructHeap(device, alignedSize, heap_slot_count(alignedSize, liveHeaps));
    if (!heapResult.has_value())
    {
      return dag::Unexpected{heapResult.error()};
    }
    bucket[heapIdx] = eastl::move(heapResult.value());
  }

  auto &heap = bucket[heapIdx];

  // Allocation failed completely for some reason, should not happen probably?
  if (!heap.pool)
    return unexpected_memory_allocation_error(E_FAIL);

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
  result->tag = tag;
  result->type = type;
  result->isAlive = true;
  return result;
}

void RaytraceAccelerationStructureObjectProvider::freeAccelStruct(RaytraceAccelerationStructure *accelStruct)
{
  OSSpinlockScopedLock lock{rtasSpinlock};

  auto &bucket = heapBuckets[accelStruct->size];

  if (DAGOR_UNLIKELY(accelStruct->asHeapIdx >= bucket.size() || !bucket[accelStruct->asHeapIdx].pool))
  {
    D3D_ERROR("DX12: Raytrace acceleration structure double free detected "
              "(size=%u, heapIdx=%u, slot=%u, type=%u)",
      accelStruct->size, accelStruct->asHeapIdx, accelStruct->slotInAsHeap, static_cast<uint32_t>(accelStruct->type));
    return;
  }

  auto &heap = bucket[accelStruct->asHeapIdx];

  // need struct size after the object is freed
  auto structSize = accelStruct->size;
  G_FAST_ASSERT(!heap.freeSlots.test(accelStruct->slotInAsHeap));
  heap.freeSlots.set(accelStruct->slotInAsHeap, true);
  heap.pool->subStructures.free(accelStruct);
  if (--heap.takenSlotCount != 0)
    return;

  if (emptyHeapKeepFrames)
  {
    heap.keepFramesLeft = emptyHeapKeepFrames;
    return;
  }

  freeAccelStructHeap(eastl::exchange(heap, {}));

  while (!bucket.empty() && !bucket.back().pool)
    bucket.pop_back();

  if (bucket.empty())
    heapBuckets.erase(structSize);
}

void RaytraceAccelerationStructureObjectProvider::retireEmptyAccelStructHeaps()
{
  OSSpinlockScopedLock lock{rtasSpinlock};

  for (auto iter = heapBuckets.begin(); iter != heapBuckets.end();)
  {
    auto &bucket = iter->second;
    for (auto &heap : bucket)
    {
      if (!heap.pool || heap.takenSlotCount)
        continue;
      G_FAST_ASSERT(heap.keepFramesLeft > 0);
      if (--heap.keepFramesLeft == 0)
        freeAccelStructHeap(eastl::exchange(heap, {}));
    }

    while (!bucket.empty() && !bucket.back().pool)
      bucket.pop_back();

    if (bucket.empty())
      iter = heapBuckets.erase(iter);
    else
      ++iter;
  }
}

void RaytraceAccelerationStructureObjectProvider::setup(const SetupInfo &info)
{
  BaseType::setup(info);

  const int keepFrames =
    ::dgs_get_settings()->getBlockByNameEx("dx12")->getInt("raytraceEmptyHeapKeepFrames", RAYTRACE_EMPTY_HEAP_KEEP_FRAMES);
  emptyHeapKeepFrames = eastl::clamp<int>(keepFrames, 0, eastl::numeric_limits<uint16_t>::max());
}

RaytraceAccelerationStructureObjectProvider::AccelerationStructureResult drv3d_dx12::resource_manager::
  RaytraceAccelerationStructureObjectProvider::newRaytraceTopAccelerationStructure(Device &device, uint64_t size, ResourceTagType tag)
{
  return allocAccelStruct(device, size, tag, RaytraceAccelerationStructure::Type::Top)
    .and_then([&, this](auto structure) -> AccelerationStructureResult {
      return allocateBufferSRVDescriptor(device.getDevice())
        .transform([&, this](auto descriptor) {
          D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
          desc.Format = DXGI_FORMAT_UNKNOWN;
          desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
          desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
          desc.RaytracingAccelerationStructure.Location = structure->gpuAddress;
          structure->descriptor = descriptor;
          device.getDevice()->CreateShaderResourceView(nullptr /*must be null*/, &desc, structure->descriptor);

          recordRaytraceTopStructureAllocated(size);
          return structure;
        })
        .or_else([&, this](auto error) -> AccelerationStructureResult {
          freeAccelStruct(structure);
          return dag::Unexpected{error};
        });
    });
}

RaytraceAccelerationStructureObjectProvider::AccelerationStructureResult drv3d_dx12::resource_manager::
  RaytraceAccelerationStructureObjectProvider::newRaytraceBottomAccelerationStructure(Device &device, uint64_t size,
    ResourceTagType tag)
{
  G_ASSERT(size < static_cast<uint64_t>(UINT32_MAX));

  return allocAccelStruct(device, size, tag, RaytraceAccelerationStructure::Type::Bottom).transform([&, this](auto structure) {
    recordRaytraceBottomStructureAllocated(size);
    return structure;
  });
}

RaytraceAccelerationStructureObjectProvider::AccelerationStructureResult drv3d_dx12::resource_manager::
  RaytraceAccelerationStructureObjectProvider::createOpacityMicroMapTriangleArray(Device &device, uint64_t size, ResourceTagType tag)
{
  G_ASSERT(size < static_cast<uint64_t>(UINT32_MAX));

  return allocAccelStruct(device, size, tag, RaytraceAccelerationStructure::Type::OpacityMicroMap)
    .transform([&, this](auto structure) {
      recordRaytraceOpacityMicroMapTriangleArrayAllocated(size);
      return structure;
    });
}
} // namespace drv3d_dx12::resource_manager
#endif