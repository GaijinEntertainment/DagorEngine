// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "buffer_components.h"
#include <device.h>
#include <device_context.h>


namespace drv3d_dx12::resource_manager
{

static bool can_use_sub_alloc(uint32_t cflags)
{
  constexpr uint32_t any_read_flag_mask =
    SBCF_BIND_VERTEX | SBCF_BIND_INDEX | SBCF_BIND_CONSTANT | SBCF_BIND_SHADER_RES | SBCF_MISC_DRAWINDIRECT;
  constexpr uint32_t uav_flag_mask = SBCF_BIND_UNORDERED | SBCF_USAGE_ACCELLERATION_STRUCTURE_BUILD_SCRATCH_SPACE;

  if (DAGOR_UNLIKELY(((SBCF_USAGE_STREAM_OUTPUT_COUNTER | SBCF_USAGE_STREAM_OUTPUT) & cflags) != 0))
  {
    // Stream output buffers can not be used as read or UAV buffers simultaneously
    return 0 == (any_read_flag_mask & cflags) && 0 == (uav_flag_mask & cflags);
  }

  // Either the buffer can not be used as UAV or not as other read only type. Mixing UAV with a read type makes it difficult
  // to avoid state collisions.
  return (0 == (uav_flag_mask & cflags)) || (0 == (any_read_flag_mask & cflags));
}

static bool is_optimized_discard(uint32_t cflags)
{
  return ((0 == (cflags & SBCF_BIND_MASK)) && (SBCF_CPU_ACCESS_WRITE | SBCF_CPU_ACCESS_READ) == (SBCF_CPU_ACCESS_MASK & cflags));
}

// find LCM (lowest common multiple)
static uint32_t findBestAlignmentOf(uint32_t value_a, uint32_t value_b)
{
  if (!value_a)
  {
    return value_b;
  }
  if (!value_b)
  {
    return value_a;
  }
  if (value_b == value_a)
  {
    return value_a;
  }
  uint32_t ab = value_a * value_b;
  // Greatest Common Denominator
  uint32_t gcd = value_a;
  uint32_t v = value_b;
  while (0 != v)
  {
    auto nextGCD = v;
    v = gcd % v;
    gcd = nextGCD;
  }

  return ab / gcd;
}

static uint32_t calculateOffsetAlignment(uint32_t cflags, uint32_t structure_size)
{
  uint32_t result = structure_size;
  if (cflags & SBCF_BIND_CONSTANT)
  {
    result = findBestAlignmentOf(result, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
  }
  else if (cflags & (SBCF_ALIGN16 | SBCF_MISC_ALLOW_RAW))
  {
    result = findBestAlignmentOf(result, sizeof(uint128_t));
  }
  else if (0 == (cflags & SBCF_BIND_MASK))
  {
    result = findBestAlignmentOf(result, sizeof(uint64_t));
  }
  else if (cflags & (SBCF_BIND_VERTEX | SBCF_BIND_INDEX | SBCF_MISC_DRAWINDIRECT))
  {
    result = findBestAlignmentOf(result, sizeof(uint32_t));
  }
  return result;
}

BufferState BufferHeap::discardBuffer(DXGIAdapter *adapter, Device &device, BufferState to_discared, DeviceMemoryClass memory_class,
  FormatStore format, uint32_t struct_size, bool raw_view, bool struct_view, D3D12_RESOURCE_FLAGS flags, uint32_t cflags,
  const char *name, uint32_t frame_index, bool disable_sub_alloc)
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
      disable_sub_alloc);
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

BufferState BufferHeap::allocateBuffer(DXGIAdapter *adapter, Device &device, uint64_t size, uint32_t structure_size,
  uint32_t discard_count, DeviceMemoryClass memory_class, D3D12_RESOURCE_FLAGS flags, uint32_t cflags, const char *name,
  bool disable_sub_alloc)
{
  auto heapProperties = getProperties(flags, memory_class, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
  AllocationFlags allocationFlags{};
  if (flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
    allocationFlags.set(AllocationFlag::IS_UAV);

  HRESULT errorCode = S_OK;
  auto oomCheckOnExit = checkForOOMOnExit(
    adapter, [&errorCode]() { return !is_oom_error_code(errorCode); },
    OomReportData{"allocateBuffer", name, size, allocationFlags.to_ulong(), heapProperties.raw});

  auto result = allocateBufferWithoutDefragmentation(adapter, device, size, structure_size, discard_count, memory_class, flags, cflags,
    name, disable_sub_alloc, heapProperties, allocationFlags, errorCode);

  if (!result)
  {
    device.processEmergencyDefragmentation(heapProperties.raw, true, flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, false);
    errorCode = S_OK;
    result = allocateBufferWithoutDefragmentation(adapter, device, size, structure_size, discard_count, memory_class, flags, cflags,
      name, disable_sub_alloc, heapProperties, allocationFlags, errorCode);
  }
  return result;
}

BufferState BufferHeap::allocateBufferWithoutDefragmentation(DXGIAdapter *adapter, Device &device, uint64_t size,
  uint32_t structure_size, uint32_t discard_count, DeviceMemoryClass memory_class, D3D12_RESOURCE_FLAGS flags, uint32_t cflags,
  const char *name, bool disable_sub_alloc, const ResourceHeapProperties &heap_properties, AllocationFlags allocation_flags,
  HRESULT &error_code)
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
      eastl::tie(resIndex, error_code) = bufferHeapStateAccess->createBufferHeap(this, adapter, device.getDevice(),
        align_value<uint64_t>(payloadSize * discard_count, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT), heap_properties, flags,
        initialState, nullptr, allocation_flags);
      if (!resIndex)
      {
        return result;
      }

      auto heapIndex = resIndex.index();

      if (heapIndex < bufferHeapStateAccess->getHeapCount())
      {
        selectedHeap = &bufferHeapStateAccess->getHeap(heapIndex);
        memoryAllocatedSize = selectedHeap->getBufferMemorySize();
        allocationRange = make_value_range<uint64_t>(0, payloadSize);
        selectedHeap->applyFirstAllocation(payloadSize);

        char stringBuf[MAX_OBJECT_NAME_LENGTH];
        // As buffers can supply multiple engine buffers, it make little sense to name it after
        // one of them right now we simply name it by the heap index.
        auto ln = sprintf_s(stringBuf, "Buffer#%u", heapIndex);
        device.nameResource(selectedHeap->getResourcePtr(), {stringBuf, static_cast<size_t>(ln)});
      }
    }

    if (selectedHeap)
    {
      result.buffer = selectedHeap->getResourcePtr();
      result.size = payloadSize;
      // With sub alloc we only allocate one segment at a time, possibly results in better ready
      // list usage and less buffers needed in total.
      result.discardCount = 1;
      result.resourceId = selectedHeap->getResId();
      result.memoryLocation =
        static_cast<ResourceMemoryLocationWithGPUAndCPUAddress>(selectedHeap->getBufferMemory()) + allocationRange.front();
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
      if (cflags & SBCF_BIND_CONSTANT)
      {
        // constant buffers size have to be aligned
        payloadSize = align_value<uint64_t>(payloadSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
      }
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
        eastl::tie(resIndex, error_code) = bufferHeapStateAccess->createBufferHeap(this, adapter, device.getDevice(),
          align_value<uint64_t>(totalSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT), heap_properties, flags, initialState, name,
          allocation_flags);
        if (!resIndex)
        {
          return result;
        }

        auto heapIndex = resIndex.index();

        if (heapIndex < bufferHeapStateAccess->getHeapCount())
        {
          selectedHeap = &bufferHeapStateAccess->getHeap(heapIndex);
          memoryAllocatedSize = selectedHeap->getBufferMemorySize();
          allocationRange = make_value_range<uint64_t>(0, totalSize);

          selectedHeap->applyFirstAllocation(totalSize);
        }
      }

      if (selectedHeap)
      {
        result.buffer = selectedHeap->getResourcePtr();
        result.size = payloadSize;
        result.discardCount = discard_count;
        result.resourceId = selectedHeap->getResId();
        result.memoryLocation =
          static_cast<ResourceMemoryLocationWithGPUAndCPUAddress>(selectedHeap->getBufferMemory()) + allocationRange.front();
        result.offset = allocationRange.front();
      }
    }

    if (result.buffer)
    {
      device.nameResource(result.buffer, name);
    }
  }

  // has to be reported outside of the locked region, otherwise we may deadlock when the manager
  // decides to shuffle things around
  if (memoryAllocatedSize > 0)
  {
    notifyBufferMemoryAllocate(memoryAllocatedSize, true);
  }

  G_ASSERT((cflags & SBCF_BIND_CONSTANT) == 0 || result.size % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0);

  return result;
}

void BufferHeap::notifyBufferMemoryRelease(size_t sz)
{
  // TODO: probably not an issue on D12, but on others align everything to 1k may diverge from real usage with lots of sub 1k counts
  // getting lost?
  tql::on_buf_changed(false, -tql::sizeInKb(sz));
}

void BufferHeap::notifyBufferMemoryAllocate(size_t sz, bool /*kick_off_shuffle*/)
{
  auto kbz = tql::sizeInKb(sz);
  tql::on_buf_changed(true, kbz);
}

BufferGlobalId BufferHeap::tryCloneBuffer(DXGIAdapter *adapter, ID3D12Device *device, BufferGlobalId buffer_id,
  BufferHeapStateWrapper::AccessToken &bufferHeapStateAccess, AllocationFlags allocation_flags)
{
  auto &heap = bufferHeapStateAccess->getConstHeap(buffer_id.index());
  auto &memory = heap.getBufferMemory();
  if (heap.getFlags() & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
    allocation_flags.set(AllocationFlag::IS_UAV);
  const auto [resIndex, errorCode] = bufferHeapStateAccess->createBufferHeap(this, adapter, device, memory.size,
    getPropertiesFromMemory(memory), heap.getFlags(), D3D12_RESOURCE_STATE_COPY_DEST, nullptr, allocation_flags);
  G_UNUSED(errorCode);
  return resIndex;
}

// Checks if the buffer is actually needed and if not it will be deleted immediately
bool BufferHeap::tidyCloneBuffer(BufferGlobalId buffer_id, BufferHeapStateWrapper::AccessToken &bufferHeapStateAccess)
{
  if (bufferHeapStateAccess->getConstHeap(buffer_id.index()).hasNoAllocations())
  {
    bufferHeapStateAccess->freeBufferHeap(this, buffer_id.index(), "move buffer", true);
    return true;
  }

  return false;
}

BufferState BufferHeap::moveBuffer(const BufferState &current_buffer, BufferGlobalId new_buffer,
  BufferHeapStateWrapper::AccessToken &bufferHeapStateAccess)
{
  auto &newBuffer = bufferHeapStateAccess->getHeap(new_buffer.index());

  BufferState newBufferState;
  newBufferState.buffer = newBuffer.getResourcePtr();
  newBufferState.size = current_buffer.size;
  newBufferState.discardCount = current_buffer.discardCount;
  newBufferState.currentDiscardIndex = current_buffer.currentDiscardIndex;
  newBufferState.fistDiscardFrame = current_buffer.fistDiscardFrame;
  newBufferState.offset = current_buffer.offset;
  newBufferState.resourceId = new_buffer;
  newBufferState.memoryLocation =
    static_cast<ResourceMemoryLocationWithGPUAndCPUAddress>(newBuffer.getBufferMemory()) + newBufferState.offset;

  newBuffer.rangeAllocate(newBufferState.usageRange());

  return newBufferState;
}

void BufferHeap::shutdown()
{
  size_t totalMemoryFreed = 0;
  {
    auto bufferHeapStateAccess = bufferHeapState.access();
    totalMemoryFreed = bufferHeapStateAccess->clear(this);
  }

  notifyBufferMemoryRelease(totalMemoryFreed);

  BaseType::shutdown();
}

void BufferHeap::preRecovery()
{
  size_t totalMemoryFreed = 0;
  {
    auto bufferHeapStateAccess = bufferHeapState.access();
    totalMemoryFreed = bufferHeapStateAccess->clear(this);
  }

  notifyBufferMemoryRelease(totalMemoryFreed);

  BaseType::preRecovery();
}

void BufferHeap::completeFrameExecution(const CompletedFrameExecutionInfo &info, PendingForCompletedFrameData &data)
{
  // do the free of old discard ready stuff before we free buffers that possibly increase the list
  // but are not removed because they can not be old enough to be removed.
  size_t memoryFreedSize = 0;
  {
    auto bufferHeapStateAccess = bufferHeapState.access();
    memoryFreedSize += bufferHeapStateAccess->cleanupDiscardStandbyList(this, info.progressIndex);

    char strBuf[MAX_OBJECT_NAME_LENGTH];
    for (auto &&freeInfo : data.deletedBuffers)
    {
      auto &buffer = freeInfo.buffer;
      if (buffer.srvs)
      {
        eastl::span<const D3D12_CPU_DESCRIPTOR_HANDLE> descriptors = {buffer.srvs.get(), buffer.srvs.get() + buffer.discardCount};
        for (const auto descriptor : descriptors)
        {
          G_UNUSED(descriptor);
          G_ASSERT(!info.bindlessManager->hasBufferViewReference(descriptor));
        }
        freeBufferSRVDescriptors(descriptors);
      }
      if (buffer.uavs)
      {
        freeBufferSRVDescriptors({buffer.uavs.get(), buffer.uavs.get() + buffer.discardCount});
      }
      if (buffer.uavForClear)
      {
        freeBufferSRVDescriptors({buffer.uavForClear.get(), buffer.uavForClear.get() + buffer.discardCount});
      }
      memoryFreedSize += bufferHeapStateAccess->freeBuffer(this, buffer, freeInfo.freeReason, info.progressIndex,
        get_resource_name(buffer.buffer, strBuf));
    }
  }
  data.deletedBuffers.clear();

  if (memoryFreedSize > 0)
  {
    notifyBufferMemoryRelease(memoryFreedSize);
  }

  BaseType::completeFrameExecution(info, data);
}

void BufferHeap::freeBufferOnFrameCompletion(BufferState &&buffer, FreeReason free_reason)
{
  BufferFreeWithReason info;
  info.buffer = eastl::move(buffer);
  info.freeReason = free_reason;
  accessRecodingPendingFrameCompletion<PendingForCompletedFrameData>(
    [&](auto &data) { data.deletedBuffers.push_back(eastl::move(info)); });
}

} // namespace drv3d_dx12::resource_manager