// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "device.h"
#include "device_context.h"

using namespace drv3d_dx12;

BufferState resource_manager::BufferHeap::discardBuffer(DXGIAdapter *adapter, Device &device, BufferState to_discared,
  DeviceMemoryClass memory_class, FormatStore format, uint32_t struct_size, bool raw_view, bool struct_view,
  D3D12_RESOURCE_FLAGS flags, uint32_t cflags, const char *name, uint32_t frame_index, bool disable_sub_alloc, bool name_objects)
{
  auto &ctx = device.getContext();

  // writes to staging only buffers are implemented as discards, saves one buffer copy
  // but we can't count them as discard traffic, so filter them out
  const bool isOptimizedDiscard = is_optimized_discard(cflags);
  BufferState result;
  auto nextDiscardIndex = to_discared.currentDiscardIndex + 1;
  if (nextDiscardIndex < to_discared.discardCount)
  {
    if (to_discared.srvs)
      ctx.updateBindlessReferences(to_discared.currentSRV(), to_discared.srvs[nextDiscardIndex]);

    // move to result for simpler later handling and RVO
    result = eastl::move(to_discared);
    // advance discard index
    result.currentDiscardIndex = nextDiscardIndex;

    if (!isOptimizedDiscard)
    {
      recordBufferDiscard(result.size, true);
    }
    else
    {
      recordDriverBufferDiscard(result.size, true);
    }
  }
  else
  {
    auto freeReason = FreeReason::DISCARD_DIFFERENT_FRAME;
    auto nextDiscardRange = to_discared.discardCount;
    // still the same frame we started discarding and we are already out of space?
    if (0 == to_discared.fistDiscardFrame || frame_index == to_discared.fistDiscardFrame)
    {
      // double discard space for next run
      nextDiscardRange *= 2;
      freeReason = FreeReason::DISCARD_SAME_FRAME;
    }

    result = allocateBuffer(adapter, device, to_discared.size, struct_size, nextDiscardRange, memory_class, flags, cflags, name,
      disable_sub_alloc, name_objects);
    if (result)
    {
      result.resourceId.inerhitStatusBits(to_discared.resourceId);

      if (to_discared.srvs)
      {
        if (raw_view)
        {
          createBufferRawSRV(device.getDevice(), result);
        }
        else if (struct_view)
        {
          createBufferStructureSRV(device.getDevice(), result, struct_size);
        }
        else
        {
          createBufferTextureSRV(device.getDevice(), result, format);
        }
      }
      if (to_discared.uavs)
      {
        if (raw_view)
        {
          createBufferRawUAV(device.getDevice(), result);
        }
        else if (struct_view)
        {
          createBufferStructureUAV(device.getDevice(), result, struct_size);
        }
        else
        {
          createBufferTextureUAV(device.getDevice(), result, format);
        }
      }

      if (to_discared.srvs)
        ctx.updateBindlessReferences(to_discared.currentSRV(), result.currentSRV());
      freeBufferOnFrameCompletion(eastl::move(to_discared), freeReason);

      if (!isOptimizedDiscard)
      {
        recordBufferDiscard(result.size, false);
      }
      else
      {
        recordDriverBufferDiscard(result.size, false);
      }
    }
    else
    {
      result = eastl::move(to_discared);
      // only case when create buffer fails if when an error happened and we need to reset
      // start over on the same buffer to keep everything from crashing
      result.currentDiscardIndex = 0;
    }
  }
  if (0 == result.fistDiscardFrame)
  {
    result.fistDiscardFrame = frame_index;
  }
  return result;
}

BufferState resource_manager::BufferHeap::allocateBuffer(DXGIAdapter *adapter, Device &device, uint64_t size, uint32_t structure_size,
  uint32_t discard_count, DeviceMemoryClass memory_class, D3D12_RESOURCE_FLAGS flags, uint32_t cflags, const char *name,
  bool disable_sub_alloc, bool name_objects)
{
  auto heapProperties = getProperties(flags, memory_class, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
  AllocationFlags allocationFlags{};
  if (flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
    allocationFlags.set(AllocationFlag::IS_UAV);

  HRESULT errorCode = S_OK;
  auto oomCheckOnExit = checkForOOMOnExit(
    adapter, [&errorCode]() { return !is_oom_error_code(errorCode); },
    OomReportData{"allocateBuffer", name, size, allocationFlags.to_ulong(), heapProperties.raw});

  auto result = allocateBufferWithoutDefragmentation(adapter, device.getDevice(), size, structure_size, discard_count, memory_class,
    flags, cflags, name, disable_sub_alloc, name_objects, heapProperties, allocationFlags, errorCode);

  if (!result)
  {
    device.processEmergencyDefragmentation(heapProperties.raw, true, flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, false);
    errorCode = S_OK;
    result = allocateBufferWithoutDefragmentation(adapter, device.getDevice(), size, structure_size, discard_count, memory_class,
      flags, cflags, name, disable_sub_alloc, name_objects, heapProperties, allocationFlags, errorCode);
  }
  return result;
}

BufferState resource_manager::BufferHeap::allocateBufferWithoutDefragmentation(DXGIAdapter *adapter, ID3D12Device *device,
  uint64_t size, uint32_t structure_size, uint32_t discard_count, DeviceMemoryClass memory_class, D3D12_RESOURCE_FLAGS flags,
  uint32_t cflags, const char *name, bool disable_sub_alloc, bool name_objects, const ResourceHeapProperties &heap_properties,
  AllocationFlags allocation_flags, HRESULT &error_code)
{
  const bool canUseSubAlloc = !disable_sub_alloc && can_use_sub_alloc(cflags);


  Heap *selectedHeap = nullptr;
  ValueRange<uint64_t> allocationRange;
  size_t memoryAllocatedSize = 0;
  G_ASSERTF(discard_count > 0, "discard count has to be at least one");
  uint64_t payloadSize = size;
  auto offsetAlignment = calculateOffsetAlignment(cflags, max<uint32_t>(1, structure_size));
  BufferState result;

  if (canUseSubAlloc)
  {
    payloadSize = align_value<uint64_t>(payloadSize, offsetAlignment);

    auto bufferHeapStateAccess = bufferHeapState.access();
    // First try to allocate from ready list
    eastl::tie(selectedHeap, allocationRange) =
      bufferHeapStateAccess->tryAllocateFromReadyList(heap_properties, payloadSize, flags, offsetAlignment, true);

    // Second try to allocate from existing heaps
    if (!selectedHeap)
    {
      eastl::tie(selectedHeap, allocationRange) =
        bufferHeapStateAccess->trySuballocateFromExistingHeaps(heap_properties, payloadSize, flags, offsetAlignment);
    }

    // Third create a new heap
    if (!selectedHeap)
    {
      const auto initialState = propertiesToInitialState(D3D12_RESOURCE_DIMENSION_BUFFER, flags, memory_class);
      BufferGlobalId resIndex{};
      eastl::tie(resIndex, error_code) = bufferHeapStateAccess->createBufferHeap(this, adapter, device,
        align_value<uint64_t>(payloadSize * discard_count, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT), heap_properties, flags,
        initialState, nullptr, allocation_flags);
      if (!resIndex)
      {
        return result;
      }

      auto heapIndex = resIndex.index();

      if (heapIndex < bufferHeapStateAccess->bufferHeaps.size())
      {
        selectedHeap = &bufferHeapStateAccess->bufferHeaps[heapIndex];
        memoryAllocatedSize = selectedHeap->bufferMemory.size();
        allocationRange = make_value_range<uint64_t>(0, payloadSize);
        if (selectedHeap->freeRanges.front().stop == payloadSize)
        {
          selectedHeap->freeRanges.pop_back();
        }
        else
        {
          selectedHeap->freeRanges.front().start = payloadSize;
        }
        if (name_objects)
        {
#if DX12_DOES_SET_DEBUG_NAMES
          wchar_t stringBuf[MAX_OBJECT_NAME_LENGTH];
          // As buffers can supply multiple engine buffers, it make little sense to name it after
          // one of them right now we simply name it by the heap index.
          swprintf_s(stringBuf, L"Buffer#%u", heapIndex);
          DX12_SET_DEBUG_OBJ_NAME(selectedHeap->buffer, stringBuf);
#endif
        }
      }
    }

    if (selectedHeap)
    {
      result.buffer = selectedHeap->buffer.Get();
      result.size = payloadSize;
      // With sub alloc we only allocate one segment at a time, possibly results in better ready
      // list usage and less buffers needed in total.
      result.discardCount = 1;
      result.resourceId = selectedHeap->resId;
#if _TARGET_PC_WIN
      result.cpuPointer = selectedHeap->getCPUPointer();
      if (result.cpuPointer)
      {
        result.cpuPointer += allocationRange.front();
      }
#endif
      result.gpuPointer = selectedHeap->getGPUPointer() + allocationRange.front();
      result.offset = allocationRange.front();
    }
  }
  else
  {
    if (discard_count > 1)
    {
      payloadSize = align_value<uint64_t>(payloadSize, offsetAlignment);
#if DX12_REPORT_BUFFER_PADDING
      auto padd = payloadSize - size;
      if (padd)
      {
        logdbg("DX12: Adding %u bytes of padding because of buffer alignment of %u", padd, offsetAlignment);
      }
#endif

      // if we have to pay possibly lots of overhead, try to cram as many discards into it as
      // possible
      discard_count = align_value<uint64_t>(payloadSize * discard_count, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT) / payloadSize;
    }
    else
    {
#if DX12_REPORT_BUFFER_PADDING
      auto padd = payloadSize - size;
      if (padd)
      {
        logdbg("DX12: Adding %u bytes of padding because min buffer size of %u", padd, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
      }
#endif
    }
    uint64_t totalSize = payloadSize * discard_count;

    {
      auto bufferHeapStateAccess = bufferHeapState.access();

      eastl::tie(selectedHeap, allocationRange) =
        bufferHeapStateAccess->tryAllocateFromReadyList(heap_properties, totalSize, flags, offsetAlignment, false);

      if (!selectedHeap)
      {
        const auto initialState = propertiesToInitialState(D3D12_RESOURCE_DIMENSION_BUFFER, flags, memory_class);
        BufferGlobalId resIndex{};
        eastl::tie(resIndex, error_code) = bufferHeapStateAccess->createBufferHeap(this, adapter, device,
          align_value<uint64_t>(totalSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT), heap_properties, flags, initialState, name,
          allocation_flags);
        if (!resIndex)
        {
          return result;
        }

        auto heapIndex = resIndex.index();

        if (heapIndex < bufferHeapStateAccess->bufferHeaps.size())
        {
          selectedHeap = &bufferHeapStateAccess->bufferHeaps[heapIndex];
          memoryAllocatedSize = selectedHeap->bufferMemory.size();
          allocationRange = make_value_range<uint64_t>(0, totalSize);
          if (selectedHeap->freeRanges.front().stop == totalSize)
          {
            selectedHeap->freeRanges.pop_back();
          }
          else
          {
            selectedHeap->freeRanges.front().start = totalSize;
          }
        }
      }

      if (selectedHeap)
      {
        result.buffer = selectedHeap->buffer.Get();
        result.size = payloadSize;
        result.discardCount = discard_count;
        result.resourceId = selectedHeap->resId;
#if _TARGET_PC_WIN
        result.cpuPointer = selectedHeap->getCPUPointer();
        if (result.cpuPointer)
        {
          result.cpuPointer += allocationRange.front();
        }
#endif
        result.gpuPointer = selectedHeap->getGPUPointer() + allocationRange.front();
        result.offset = allocationRange.front();
      }
    }

    if (result.buffer && name_objects)
    {
#if DX12_DOES_SET_DEBUG_NAMES
      wchar_t stringBuf[MAX_OBJECT_NAME_LENGTH];
      DX12_SET_DEBUG_OBJ_NAME(result.buffer, lazyToWchar(name, stringBuf, MAX_OBJECT_NAME_LENGTH));
#endif
    }
  }

  // has to be reported outside of the locked region, otherwise we may deadlock when the manager
  // decides to shuffle things around
  if (memoryAllocatedSize > 0)
  {
    notifyBufferMemoryAllocate(memoryAllocatedSize, true);
  }

  return result;
}
