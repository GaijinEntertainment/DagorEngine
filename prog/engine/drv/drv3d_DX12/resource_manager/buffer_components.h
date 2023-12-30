#pragma once

#include <EASTL/vector.h>
#include <debug/dag_log.h>

#include "device_memory_class.h"
#include "pipeline.h"
#include "container_mutex_wrapper.h"
#include "bindless.h"

#include "resource_manager/host_shared_components.h"


namespace drv3d_dx12
{
namespace resource_manager
{
class BufferHeap : public PersistentBidirectionalMemoryProvider
{
  using BaseType = PersistentBidirectionalMemoryProvider;

public:
  enum class FreeReason
  {
    // regular delete where the user requested a deletion of a buffer resource
    USER_DELETE,
    // discard but buffer was too small to cover a whole frame of discards
    DISCARD_SAME_FRAME,
    // discard where buffer was large enough to at least cover a whole frame
    DISCARD_DIFFERENT_FRAME,
  };

protected:
  struct BufferFreeWithReason
  {
    BufferState buffer;
    FreeReason freeReason = FreeReason::USER_DELETE;
  };
  struct PendingForCompletedFrameData : BaseType::PendingForCompletedFrameData
  {
    eastl::vector<BufferFreeWithReason> deletedBuffers;
  };
  struct CompletedFrameExecutionInfo : BaseType::CompletedFrameExecutionInfo
  {
    frontend::BindlessManager *bindlessManager;
    uint64_t progressIndex;
    ID3D12Device *device;
  };
  // keep them very long, usually standby read buffers total mem usage is less than 100mibytes
  // we can always trim them, should we run low on memory.
  static constexpr uint64_t standby_max_progress_age = 1024 * 4;
  struct Heap : BasicBuffer
  {
    // TODO can be derived from this pointer and properties
    BufferGlobalId resId;
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
    eastl::vector<ValueRange<uint64_t>> freeRanges;

    bool free(ValueRange<uint64_t> range)
    {
      free_list_insert_and_coalesce(freeRanges, range);
      return freeRanges.front().size() == bufferMemory.size();
    }

    bool canSubAllocateFrom() const { return D3D12_RESOURCE_FLAG_NONE == flags; }

    // This allocate is a bit more simple than a general purpose allocation as it has the goal to
    // keep similarly aligned things in the same buffer heaps together So allocation from this will
    // fail if no free range is found that has a matching offset / address alignment.
    ValueRange<uint64_t> allocate(uint64_t size, uint64_t alignment)
    {
      if (freeRanges.empty())
      {
        return {};
      }
      auto ref = eastl::find_if(begin(freeRanges), end(freeRanges),
        [size, alignment](auto range) //
        { return (0 == (range.start % alignment)) && (range.size() >= size); });
      if (ref == end(freeRanges))
      {
        return {};
      }
      auto s = ref->start;
      if (ref->size() == size)
      {
        freeRanges.erase(ref);
      }
      else
      {
        ref->start += size;
      }
      return make_value_range<uint64_t>(s, size);
    }
  };
  struct StandbyInfo
  {
    uint64_t progress;
    uint32_t index;
    ValueRange<uint64_t> range;
  };
  struct BufferHeapState
  {
    eastl::vector<Heap> bufferHeaps;
    eastl::vector<StandbyInfo> bufferHeapDiscardStandbyList;
    // list of buffers can be big, up to 2k, so saving the free slots should be more efficient than searching on each allocate
    eastl::vector<uint32_t> freeBufferSlots;

    eastl::pair<Heap *, ValueRange<uint64_t>> tryAllocateFromReadyList(ResourceHeapProperties properties, uint64_t size,
      D3D12_RESOURCE_FLAGS flags, uint64_t offset_alignment, bool allow_offset)
    {
      Heap *selectedHeap = nullptr;
      ValueRange<uint64_t> allocationRange;
      // Do backward search so we always use the most recently added and we have a chance to free
      // older stuff. Also erase is a bit more efficient.
      auto ref = eastl::find_if(rbegin(bufferHeapDiscardStandbyList), rend(bufferHeapDiscardStandbyList),
        [this, size, flags, properties, offset_alignment, allow_offset](auto info) {
          auto &heap = bufferHeaps[info.index];
          if (flags != heap.flags)
          {
            return false;
          }
          auto memoryHeapID = heap.bufferMemory.getHeapID();
          ResourceHeapProperties memoryProperties;
          memoryProperties.raw = memoryHeapID.group;
          if (properties != memoryProperties)
          {
            return false;
          }
          if (!allow_offset && 0 != info.range.start)
          {
            return false;
          }
          if (size != info.range.size())
          {
            return false;
          }
          if (0 != (info.range.start % offset_alignment))
          {
            return false;
          }
          return true;
        });
      if (rend(bufferHeapDiscardStandbyList) != ref)
      {
        selectedHeap = &bufferHeaps[ref->index];
        allocationRange = ref->range;
        bufferHeapDiscardStandbyList.erase(ref);
      }
      return {selectedHeap, allocationRange};
    }

    eastl::pair<Heap *, ValueRange<uint64_t>> trySuballocateFromExistingHeaps(ResourceHeapProperties properties, uint64_t size,
      D3D12_RESOURCE_FLAGS flags, uint32_t offset_alignment)
    {
      Heap *selectedHeap = nullptr;
      ValueRange<uint64_t> allocationRange;
      for (auto &heap : bufferHeaps)
      {
        if (!heap)
        {
          continue;
        }
        if (!heap.canSubAllocateFrom())
        {
          continue;
        }
        if (heap.flags != flags)
        {
          continue;
        }
        auto memoryHeapID = heap.bufferMemory.getHeapID();
        if (memoryHeapID.isAlias)
        {
          continue;
        }
        ResourceHeapProperties memoryProperties;
        memoryProperties.raw = memoryHeapID.group;
        if (memoryProperties != properties)
        {
          continue;
        }
        auto range = heap.allocate(size, offset_alignment);
        if (range.empty())
        {
          continue;
        }
        selectedHeap = &heap;
        allocationRange = range;
        break;
      }
      return {selectedHeap, allocationRange};
    }

    BufferGlobalId adoptBufferHeap(Heap &&heap)
    {
      BufferGlobalId result;
      if (freeBufferSlots.empty())
      {
        heap.resId = BufferGlobalId{static_cast<uint32_t>(bufferHeaps.size())};
        result = heap.resId;
        bufferHeaps.push_back(eastl::move(heap));
      }
      else
      {
        heap.resId = BufferGlobalId{freeBufferSlots.back()};
        freeBufferSlots.pop_back();
        result = heap.resId;
        bufferHeaps[result.index()] = eastl::move(heap);
      }
      return result;
    }

    size_t freeBufferHeap(BufferHeap *manager, uint32_t index, ValueRange<uint64_t> range, const char *name)
    {
      auto &heap = bufferHeaps[index];
      auto memoryHeapID = heap.bufferMemory.getHeapID();
      if (!heap.free(range))
      {
        return 0;
      }
      size_t result = 0 == memoryHeapID.isAlias ? heap.bufferMemory.size() : 0;
      ResourceHeapProperties memoryProperties;
      memoryProperties.raw = memoryHeapID.group;
      manager->recordBufferHeapFreed(result, !memoryProperties.isCPUVisible(), name);
      heap.reset(manager);
      freeBufferSlots.push_back(index);
      return result;
    }

    BufferGlobalId createBufferHeap(BufferHeap *manager, DXGIAdapter *adapter, ID3D12Device *device, uint64_t allocation_size,
      ResourceHeapProperties properties, D3D12_RESOURCE_FLAGS flags, DeviceMemoryClass memory_class, const char *name)
    {
      BufferGlobalId result;
      Heap newHeap;

      D3D12_RESOURCE_DESC desc;
      desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
      desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
      desc.Width = allocation_size;
      desc.Height = 1;
      desc.DepthOrArraySize = 1;
      desc.MipLevels = 1;
      desc.Format = DXGI_FORMAT_UNKNOWN;
      desc.SampleDesc.Count = 1;
      desc.SampleDesc.Quality = 0;
      desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
      desc.Flags = flags;

      D3D12_RESOURCE_ALLOCATION_INFO allocInfo;
      allocInfo.SizeInBytes = desc.Width;
      allocInfo.Alignment = desc.Alignment;

      auto initialState = manager->propertiesToInitialState(D3D12_RESOURCE_DIMENSION_BUFFER, flags, memory_class);

      auto allocation = manager->allocate(adapter, device, properties, allocInfo, {});

      if (!allocation)
      {
        return result;
      }

      auto errorCode = newHeap.create(device, desc, allocation, initialState, properties.isCPUVisible());
      if (DX12_CHECK_FAIL(errorCode))
      {
        manager->free(allocation);
        return result;
      }

      newHeap.freeRanges.push_back(make_value_range<uint64_t>(0, allocation_size));
      newHeap.flags = flags;

      result = adoptBufferHeap(eastl::move(newHeap));

      char strBuf[64];
      if (!name)
      {
        sprintf_s(strBuf, "Buffer#%u", result.index());
        name = strBuf;
      }
      manager->recordBufferHeapAllocated(allocation_size, !properties.isCPUVisible(), name);
      manager->updateMemoryRangeUse(allocation, result);

      return result;
    }

    bool isValidBuffer(const BufferState &buf)
    {
      auto resIndex = buf.resourceId.index();
      return (resIndex < bufferHeaps.size() && bufferHeaps[resIndex].buffer.Get() == buf.buffer);
    }

    size_t freeBuffer(BufferHeap *manager, const BufferState &buf, BufferHeap::FreeReason free_reason, uint64_t progress,
      const char *name)
    {
      size_t memoryFreedSize = 0;
      auto resIndex = buf.resourceId.index();

      if (!isValidBuffer(buf))
      {
        logwarn("DX12: freeBuffer: Provided buffer %p and id %u does not match stored buffer %u (%u)", buf.buffer, buf.resourceId,
          resIndex < bufferHeaps.size() ? bufferHeaps[resIndex].buffer.Get() : nullptr, bufferHeaps.size());
        return 0;
      }

      auto &heap = bufferHeaps[resIndex];
      if (0 != heap.bufferMemory.getHeapID().isAlias)
      {
        G_ASSERTF(FreeReason::USER_DELETE == free_reason, "DX12: Buffer %s was discarded but was a "
                                                          "aliased / placed buffer, this is a "
                                                          "invalid use case!");
      }
      if (shouldFreeBuffer(heap, free_reason))
      {
        memoryFreedSize += freeBufferHeap(manager, resIndex, buf.usageRange(), name);
      }
      else
      {
        StandbyInfo info;
        info.progress = progress;
        info.index = resIndex;
        info.range = buf.usageRange();
        bufferHeapDiscardStandbyList.push_back(info);
      }
      return memoryFreedSize;
    }

    static bool shouldFreeBuffer(const Heap &heap, BufferHeap::FreeReason free_reason)
    {
      if (heap.canSubAllocateFrom())
      {
        return FreeReason::USER_DELETE == free_reason;
      }
      else
      {
        return FreeReason::DISCARD_DIFFERENT_FRAME != free_reason;
      }
    }
  };
  ContainerMutexWrapper<BufferHeapState, OSSpinlock> bufferHeapState;

  void notifyBufferMemoryRelease(size_t sz)
  {
    // TODO: probably not an issue on D12, but on others align everything to 1k may diverge from real usage with lots of sub 1k counts
    // getting lost?
    tql::on_buf_changed(false, -tql::sizeInKb(sz));
  }

  void notifyBufferMemoryAllocate(size_t sz, bool /*kick_off_shuffle*/)
  {
    auto kbz = tql::sizeInKb(sz);
    tql::on_buf_changed(true, kbz);
  }

  void shutdown()
  {
    size_t totalMemoryFreed = 0;
    {
      auto bufferHeapStateAccess = bufferHeapState.access();
      for (auto &heap : bufferHeapStateAccess->bufferHeaps)
      {
        if (heap.buffer)
        {
          totalMemoryFreed += heap.bufferMemory.size();
          heap.reset(this);
        }
      }

      bufferHeapStateAccess->bufferHeaps.clear();
      bufferHeapStateAccess->bufferHeapDiscardStandbyList.clear();
      bufferHeapStateAccess->freeBufferSlots.clear();
    }

    notifyBufferMemoryRelease(totalMemoryFreed);

    BaseType::shutdown();
  }

  void preRecovery()
  {
    size_t totalMemoryFreed = 0;
    {
      auto bufferHeapStateAccess = bufferHeapState.access();
      for (auto &heap : bufferHeapStateAccess->bufferHeaps)
      {
        if (heap.buffer)
        {
          totalMemoryFreed += heap.bufferMemory.size();
          heap.reset(this);
        }
      }

      bufferHeapStateAccess->bufferHeaps.clear();
      bufferHeapStateAccess->bufferHeapDiscardStandbyList.clear();
      bufferHeapStateAccess->freeBufferSlots.clear();
    }

    notifyBufferMemoryRelease(totalMemoryFreed);

    BaseType::preRecovery();
  }

  static bool can_use_sub_alloc(uint32_t cflags)
  {
    static constexpr uint32_t any_read_flag_mask =
      SBCF_BIND_VERTEX | SBCF_BIND_INDEX | SBCF_BIND_CONSTANT | SBCF_BIND_SHADER_RES | SBCF_MISC_DRAWINDIRECT;
    // Either the buffer can not be used as UAV or not as other read only type. Mixing UAV with a read type makes it difficult
    // to avoid state collisions.
    return (0 == (SBCF_BIND_UNORDERED & cflags)) || (0 == (cflags & any_read_flag_mask));
  }

  static bool is_optimized_discard(uint32_t cflags)
  {
    return ((0 == (cflags & SBCF_BIND_MASK)) && (SBCF_CPU_ACCESS_WRITE | SBCF_CPU_ACCESS_READ) == (SBCF_CPU_ACCESS_MASK & cflags));
  }

public:
  // NOTE needs context lock to be held for frame end data structure
  BufferState discardBuffer(DXGIAdapter *adapter, ID3D12Device *device, BufferState to_discared, DeviceMemoryClass memory_class,
    FormatStore format, uint32_t struct_size, bool raw_view, bool struct_view, D3D12_RESOURCE_FLAGS flags, uint32_t cflags,
    const char *name, uint32_t frame_index, bool disable_sub_alloc, bool name_objects)
  {
    // writes to staging only buffers are implemented as discards, saves one buffer copy
    // but we can't count them as discard traffic, so filter them out
    const bool isOptimizedDiscard = is_optimized_discard(cflags);
    BufferState result;
    auto nextDiscardIndex = to_discared.currentDiscardIndex + 1;
    if (nextDiscardIndex < to_discared.discardCount)
    {
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
            createBufferRawSRV(device, result);
          }
          else if (struct_view)
          {
            createBufferStructureSRV(device, result, struct_size);
          }
          else
          {
            createBufferTextureSRV(device, result, format);
          }
        }
        if (to_discared.uavs)
        {
          if (raw_view)
          {
            createBufferRawUAV(device, result);
          }
          else if (struct_view)
          {
            createBufferStructureUAV(device, result, struct_size);
          }
          else
          {
            createBufferTextureUAV(device, result, format);
          }
        }

        freeBufferOnFrameCompletion(eastl::move(to_discared), freeReason, true);

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

  BufferState allocateBuffer(DXGIAdapter *adapter, ID3D12Device *device, uint64_t size, uint32_t structure_size,
    uint32_t discard_count, DeviceMemoryClass memory_class, D3D12_RESOURCE_FLAGS flags, uint32_t cflags, const char *name,
    bool disable_sub_alloc, bool name_objects)
  {
    const bool canUseSubAlloc = !disable_sub_alloc && can_use_sub_alloc(cflags);

    auto heapProperties = getProperties(flags, memory_class, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

    Heap *selectedHeap = nullptr;
    ValueRange<uint64_t> allocationRange;
    size_t memoryAllocatedSize = 0;
    G_ASSERTF(discard_count > 0, "discard count has to be at least one");
    uint64_t payloadSize = size;
    auto offsetAlignment = calculateOffsetAlignment(cflags, max<uint32_t>(1, structure_size));
    BufferState result;
    auto oomCheckOnExit = checkForOOMOnExit([&result]() { return static_cast<bool>(result.buffer); }, "DX12: OOM during %s for <%s>",
      "allocateBuffer", name);
    if (canUseSubAlloc)
    {
      payloadSize = align_value<uint64_t>(payloadSize, offsetAlignment);

      auto bufferHeapStateAccess = bufferHeapState.access();
      // First try to allocate from ready list
      eastl::tie(selectedHeap, allocationRange) =
        bufferHeapStateAccess->tryAllocateFromReadyList(heapProperties, payloadSize, flags, offsetAlignment, true);

      // Second try to allocate from existing heaps
      if (!selectedHeap)
      {
        eastl::tie(selectedHeap, allocationRange) =
          bufferHeapStateAccess->trySuballocateFromExistingHeaps(heapProperties, payloadSize, flags, offsetAlignment);
      }

      // Third create a new heap
      if (!selectedHeap)
      {
        auto resIndex = bufferHeapStateAccess->createBufferHeap(this, adapter, device,
          align_value<uint64_t>(payloadSize * discard_count, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT), heapProperties, flags,
          memory_class, nullptr);
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
          bufferHeapStateAccess->tryAllocateFromReadyList(heapProperties, totalSize, flags, offsetAlignment, false);

        if (!selectedHeap)
        {
          auto resIndex = bufferHeapStateAccess->createBufferHeap(this, adapter, device,
            align_value<uint64_t>(totalSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT), heapProperties, flags, memory_class, name);
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

  void doFreeBufferOnFrameCompletion(BufferState &&buffer, FreeReason free_reason)
  {
    BufferFreeWithReason info;
    info.buffer = eastl::move(buffer);
    info.freeReason = free_reason;
    accessRecodingPendingFrameCompletion<PendingForCompletedFrameData>(
      [&](auto &data) { data.deletedBuffers.push_back(eastl::move(info)); });
  }

public:
  void completeFrameExecution(const CompletedFrameExecutionInfo &info, PendingForCompletedFrameData &data)
  {
    // do the free of old discard ready stuff before we free buffers that possibly increase the list
    // but are not removed because they can not be old enough to be removed.
    size_t memoryFreedSize = 0;
    {
      auto bufferHeapStateAccess = bufferHeapState.access();
      bufferHeapStateAccess->bufferHeapDiscardStandbyList.erase(
        eastl::remove_if(begin(bufferHeapStateAccess->bufferHeapDiscardStandbyList),
          end(bufferHeapStateAccess->bufferHeapDiscardStandbyList),
          [&memoryFreedSize, &bufferHeapStateAccess, this, pi = info.progressIndex](auto info) //
          {
            if (info.progress + standby_max_progress_age > pi)
            {
              return false;
            }
            memoryFreedSize += bufferHeapStateAccess->freeBufferHeap(this, info.index, info.range, "<ready list>");
            return true;
          }),
        end(bufferHeapStateAccess->bufferHeapDiscardStandbyList));

      char strBuf[MAX_OBJECT_NAME_LENGTH];
      for (auto &&freeInfo : data.deletedBuffers)
      {
        auto &buffer = freeInfo.buffer;
        if (buffer.srvs)
        {
          info.bindlessManager->unregisterBufferDescriptors({buffer.srvs.get(), buffer.srvs.get() + buffer.discardCount});
          freeBufferSRVDescriptors({buffer.srvs.get(), buffer.srvs.get() + buffer.discardCount});
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

  template <typename T>
  void visitBuffers(T clb)
  {
    char resnameBuffer[MAX_OBJECT_NAME_LENGTH];
    auto bufferHeapStateAccess = bufferHeapState.access();
    for (auto &buf : bufferHeapStateAccess->bufferHeaps)
    {
      if (!buf.buffer)
      {
        continue;
      }
      bool isDiscardReady =
        end(bufferHeapStateAccess->bufferHeapDiscardStandbyList) !=
        eastl::find_if(begin(bufferHeapStateAccess->bufferHeapDiscardStandbyList),
          end(bufferHeapStateAccess->bufferHeapDiscardStandbyList), [id = buf.resId.index()](auto info) { return info.index == id; });

      clb(get_resource_name(buf.buffer.Get(), resnameBuffer), buf.bufferMemory.size(), buf.bufferMemory.size(), isDiscardReady);
    }
  }

  void freeBufferOnFrameCompletion(BufferState &&buffer, FreeReason free_reason, bool is_still_valid)
  {
    if (is_still_valid)
    {
      doFreeBufferOnFrameCompletion(eastl::move(buffer), free_reason);
    }
    else
    {
      auto access = bufferHeapState.access();
      // Have to hold the access until enqueue is done, otherwise we may do it while the buffer table is reset.
      if (access->isValidBuffer(buffer))
      {
        doFreeBufferOnFrameCompletion(eastl::move(buffer), free_reason);
      }
    }
  }

  ResourceMemory getResourceMemoryForBuffer(BufferGlobalId index)
  {
    auto bufferHeapStateAccess = bufferHeapState.access();
    G_ASSERT(index.index() < bufferHeapStateAccess->bufferHeaps.size());
    if (index.index() < bufferHeapStateAccess->bufferHeaps.size())
    {
      return bufferHeapStateAccess->bufferHeaps[index.index()].bufferMemory;
    }
    return {};
  }
};
} // namespace resource_manager
} // namespace drv3d_dx12