// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <device_memory_class.h>
#include <pipeline.h>
#include <container_mutex_wrapper.h>
#include <bindless.h>
#include "host_shared_components.h"

#include <EASTL/vector.h>
#include <debug/dag_log.h>


namespace drv3d_dx12::resource_manager
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
    // deleted and replaced with a new one, do not report size change
    MOVED_BY_DRIVER,
  };

protected:
  struct BufferFreeWithReason
  {
    BufferState buffer;
    FreeReason freeReason = FreeReason::USER_DELETE;
  };
  struct PendingForCompletedFrameData : BaseType::PendingForCompletedFrameData
  {
    dag::Vector<BufferFreeWithReason> deletedBuffers;
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
    dag::Vector<ValueRange<uint64_t>> freeRanges;

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

    void rangeAllocate(ValueRange<uint64_t> range)
    {
      auto ref =
        eastl::find_if(begin(freeRanges), end(freeRanges), [range](auto free_range) { return range.isSubRangeOf(free_range); });
      G_ASSERT(ref != end(freeRanges));
      if (ref == end(freeRanges))
      {
        return;
      }

      auto followup = ref->cutOut(range);
      if (!followup.empty())
      {
        if (ref->empty())
        {
          *ref = followup;
        }
        else
        {
          freeRanges.insert(ref + 1, followup);
        }
      }
      else if (ref->empty())
      {
        freeRanges.erase(ref);
      }
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
    dag::Vector<Heap> bufferHeaps;
    dag::Vector<StandbyInfo> bufferHeapDiscardStandbyList;
    // list of buffers can be big, up to 2k, so saving the free slots should be more efficient than searching on each allocate
    dag::Vector<uint32_t> freeBufferSlots;

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

    size_t freeBufferHeap(BufferHeap *manager, uint32_t index, const char *name, bool update_defragmentation_generation)
    {
      auto &heap = bufferHeaps[index];
      auto memoryHeapID = heap.bufferMemory.getHeapID();
      size_t result = 0 == memoryHeapID.isAlias ? heap.bufferMemory.size() : 0;
      ResourceHeapProperties memoryProperties;
      memoryProperties.raw = memoryHeapID.group;
      manager->recordBufferHeapFreed(result, !memoryProperties.isCPUVisible(), name);
      heap.reset(manager, update_defragmentation_generation);
      freeBufferSlots.push_back(index);
      return result;
    }

    size_t freeBufferHeap(BufferHeap *manager, uint32_t index, ValueRange<uint64_t> range, const char *name)
    {
      if (!bufferHeaps[index].free(range))
      {
        return 0;
      }
      return freeBufferHeap(manager, index, name, false);
    }

    eastl::pair<BufferGlobalId, HRESULT> createBufferHeap(BufferHeap *manager, DXGIAdapter *adapter, ID3D12Device *device,
      uint64_t allocation_size, ResourceHeapProperties properties, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initial_state,
      const char *name, AllocationFlags allocation_flags = {})
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

      const auto allocation = manager->allocate(adapter, device, properties, allocInfo, allocation_flags);
      const ResourceHeapProperties allocatedProperties{allocation.getHeapID().group};

      if (!allocation)
      {
        return {result, E_OUTOFMEMORY};
      }

      auto errorCode = newHeap.create(device, desc, allocation, initial_state, allocatedProperties.isCPUVisible());
      if (DX12_CHECK_FAIL(errorCode))
      {
        manager->free(allocation, allocation_flags.test(AllocationFlag::DEFRAGMENTATION_OPERATION));
        return {result, errorCode};
      }

      newHeap.freeRanges.push_back(make_value_range<uint64_t>(0, allocation_size));
      newHeap.flags = flags;

      result = adoptBufferHeap(eastl::move(newHeap));
      G_ASSERT(result);

      char strBuf[64];
      if (!name)
      {
        sprintf_s(strBuf, "Buffer#%u", result.index());
        name = strBuf;
      }
      manager->recordBufferHeapAllocated(allocation_size, !allocatedProperties.isCPUVisible(), name);
      manager->updateMemoryRangeUse(allocation, result);

      return {result, S_OK};
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

  using BufferHeapStateWrapper = ContainerMutexWrapper<BufferHeapState, OSSpinlock>;
  BufferHeapStateWrapper bufferHeapState;

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

  BufferGlobalId tryCloneBuffer(DXGIAdapter *adapter, ID3D12Device *device, BufferGlobalId buffer_id,
    BufferHeapStateWrapper::AccessToken &bufferHeapStateAccess, AllocationFlags allocation_flags)
  {
    auto &heap = bufferHeapStateAccess->bufferHeaps[buffer_id.index()];
    auto &memory = heap.bufferMemory;
    if (heap.flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
      allocation_flags.set(AllocationFlag::IS_UAV);
    const auto [resIndex, errorCode] = bufferHeapStateAccess->createBufferHeap(this, adapter, device, memory.size(),
      getPropertiesFromMemory(memory), heap.flags, D3D12_RESOURCE_STATE_INITIAL_BUFFER_STATE, nullptr, allocation_flags);
    G_UNUSED(errorCode);
    return resIndex;
  }

  // Updates all entries of old_buffer in the standby list to use new_buffer instead.
  // Returns true if the whole buffer is part of the standby list
  bool moveStandbyBufferEntries(BufferGlobalId old_buffer, BufferGlobalId new_buffer,
    BufferHeapStateWrapper::AccessToken &bufferHeapStateAccess)
  {
    auto oldBufferIndex = old_buffer.index();
    auto newBufferIndex = new_buffer.index();

    auto &oldBuffer = bufferHeapStateAccess->bufferHeaps[oldBufferIndex];
    auto &newBuffer = bufferHeapStateAccess->bufferHeaps[newBufferIndex];

    uint32_t totalSize = 0;
    for (auto &si : bufferHeapStateAccess->bufferHeapDiscardStandbyList)
    {
      if (si.index != oldBufferIndex)
      {
        continue;
      }
      totalSize += si.range.size();
      oldBuffer.free(si.range);
      newBuffer.rangeAllocate(si.range);
      si.index = newBufferIndex;
    }

    return totalSize == oldBuffer.bufferMemory.size();
  }

  // Checks if the buffer is actually needed and if not it will be deleted immediately
  bool tidyCloneBuffer(BufferGlobalId buffer_id, BufferHeapStateWrapper::AccessToken &bufferHeapStateAccess)
  {
    auto &buffer = bufferHeapStateAccess->bufferHeaps[buffer_id.index()];
    // If either no free ranges are available or more than one indicate that something is used by someone
    if (buffer.freeRanges.empty() || (buffer.freeRanges.size() > 1))
    {
      return false;
    }
    // If one range is available then it has to match the buffer size to indicate no one is using this buffer
    if (buffer.freeRanges.front().size() != buffer.bufferMemory.size())
    {
      return false;
    }
    bufferHeapStateAccess->freeBufferHeap(this, buffer_id.index(), "move buffer", true);
    return true;
  }

  BufferState moveBuffer(const BufferState &current_buffer, BufferGlobalId new_buffer,
    BufferHeapStateWrapper::AccessToken &bufferHeapStateAccess)
  {
    auto &newBuffer = bufferHeapStateAccess->bufferHeaps[new_buffer.index()];

    BufferState newBufferState;
    newBufferState.buffer = newBuffer.buffer.Get();
    newBufferState.size = current_buffer.size;
    newBufferState.discardCount = current_buffer.discardCount;
    newBufferState.currentDiscardIndex = current_buffer.currentDiscardIndex;
    newBufferState.fistDiscardFrame = current_buffer.fistDiscardFrame;
    newBufferState.offset = current_buffer.offset;
    newBufferState.resourceId = new_buffer;
#if _TARGET_PC_WIN
    newBufferState.cpuPointer = newBuffer.getCPUPointer();
    if (newBufferState.cpuPointer)
    {
      newBufferState.cpuPointer += newBufferState.offset;
    }
#endif
    newBufferState.gpuPointer = newBuffer.getGPUPointer() + newBufferState.offset;

    newBuffer.rangeAllocate(newBufferState.usageRange());

    return newBufferState;
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
    return (0 == ((SBCF_BIND_UNORDERED | SBCF_USAGE_ACCELLERATION_STRUCTURE_BUILD_SCRATCH_SPACE) & cflags)) ||
           (0 == (cflags & any_read_flag_mask));
  }

  static bool is_optimized_discard(uint32_t cflags)
  {
    return ((0 == (cflags & SBCF_BIND_MASK)) && (SBCF_CPU_ACCESS_WRITE | SBCF_CPU_ACCESS_READ) == (SBCF_CPU_ACCESS_MASK & cflags));
  }

public:
  // NOTE needs context lock to be held for frame end data structure
  BufferState discardBuffer(DXGIAdapter *adapter, Device &device, BufferState to_discared, DeviceMemoryClass memory_class,
    FormatStore format, uint32_t struct_size, bool raw_view, bool struct_view, D3D12_RESOURCE_FLAGS flags, uint32_t cflags,
    const char *name, uint32_t frame_index, bool disable_sub_alloc, bool name_objects);

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

  BufferState allocateBuffer(DXGIAdapter *adapter, Device &device, uint64_t size, uint32_t structure_size, uint32_t discard_count,
    DeviceMemoryClass memory_class, D3D12_RESOURCE_FLAGS flags, uint32_t cflags, const char *name, bool disable_sub_alloc,
    bool name_objects);

  BufferState allocateBufferWithoutDefragmentation(DXGIAdapter *adapter, ID3D12Device *device, uint64_t size, uint32_t structure_size,
    uint32_t discard_count, DeviceMemoryClass memory_class, D3D12_RESOURCE_FLAGS flags, uint32_t cflags, const char *name,
    bool disable_sub_alloc, bool name_objects, const ResourceHeapProperties &heap_properties, AllocationFlags allocation_flags,
    HRESULT &error_code);

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

  template <typename T>
  void visitBuffers(T clb)
  {
    auto bufferHeapStateAccess = bufferHeapState.access();
    clb.beginVisit();
    clb.beginVisitBuffers();
    for (auto &buf : bufferHeapStateAccess->bufferHeaps)
    {
      if (!buf.buffer)
      {
        continue;
      }
      auto freeSum =
        eastl::accumulate(begin(buf.freeRanges), end(buf.freeRanges), 0, [](uint64_t v, const auto &r) { return v + r.size(); });
      bool isDiscardReady =
        end(bufferHeapStateAccess->bufferHeapDiscardStandbyList) !=
        eastl::find_if(begin(bufferHeapStateAccess->bufferHeapDiscardStandbyList),
          end(bufferHeapStateAccess->bufferHeapDiscardStandbyList), [id = buf.resId.index()](auto info) { return info.index == id; });
      clb.visitBuffer(buf.resId.index(), buf.bufferMemory.size(), freeSum, buf.flags, isDiscardReady);
      for (auto r : buf.freeRanges)
      {
        clb.visitBufferFreeRange(r);
      }
    }
    clb.endVisitBuffers();
    clb.beginVisitStandbyRange();
    for (auto &info : bufferHeapStateAccess->bufferHeapDiscardStandbyList)
    {
      clb.visitBufferStandbyRange(info.progress, info.index, info.range);
    }
    clb.endVisitStandbyRange();
    clb.endVisit();
  }

  void freeBufferOnFrameCompletion(BufferState &&buffer, FreeReason free_reason)
  {
    BufferFreeWithReason info;
    info.buffer = eastl::move(buffer);
    info.freeReason = free_reason;
    accessRecodingPendingFrameCompletion<PendingForCompletedFrameData>(
      [&](auto &data) { data.deletedBuffers.push_back(eastl::move(info)); });
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
} // namespace drv3d_dx12::resource_manager
