// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <device_memory_class.h>
#include <pipeline.h>
#include <container_mutex_wrapper.h>
#include <bindless.h>
#include "host_shared_components.h"
#include "heap_suballocator_impl.h"

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

  class HeapSuballocator;

  // keep them very long, usually standby read buffers total mem usage is less than 100mibytes
  // we can always trim them, should we run low on memory.
  static constexpr uint64_t standby_max_progress_age = 1024 * 4;
  class Heap : public BasicBuffer
  {
  public:
    bool hasNoAllocations() const { return freeRanges.size() == 1 && freeRanges.front().size() == bufferMemory.size; }

    const dag::Vector<ValueRange<uint64_t>> &getFreeRanges() const { return freeRanges; }
    uint64_t getMaxFreeRangeSize() const { return maxFreeRangeSize; }

    bool canSubAllocateFrom() const { return buffer && D3D12_RESOURCE_FLAG_NONE == flags && !getHeapID().isAlias; }

    void init(uint64_t allocation_size);
    void applyFirstAllocation(uint64_t payload_size);

    ValueRange<uint64_t> allocate(uint64_t size, uint64_t alignment);
    void rangeAllocate(ValueRange<uint64_t> range);
    bool free(ValueRange<uint64_t> range);

    uint32_t index() const { return resId.index(); }
    void setResId(BufferGlobalId in_res_id) { resId = in_res_id; }
    BufferGlobalId getResId() const { return resId; }

    D3D12_RESOURCE_FLAGS getFlags() const { return flags; }
    void setFlags(D3D12_RESOURCE_FLAGS in_flags) { flags = in_flags; }

    void setSuballocator(HeapSuballocator *in_suballocator) { suballocator = in_suballocator; }

  private:
    void updateMaxFreeRangeSize();
    void notifyMaxFreeRangeSizeChanged();

    // TODO can be derived from this pointer and properties
    BufferGlobalId resId;
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

    dag::Vector<ValueRange<uint64_t>> freeRanges;
    uint64_t maxFreeRangeSize = 0;

    HeapSuballocator *suballocator = nullptr;
  };

  class HeapSuballocator
  {
  public:
    void onHeapCreated(Heap &heap);
    void onHeapUpdated(const Heap &heap);
    void onHeapDestroyed(Heap &heap);

    void clear();

    eastl::pair<Heap *, ValueRange<uint64_t>> trySuballocate(dag::Vector<Heap> &buffer_heaps, ResourceHeapProperties properties,
      uint64_t size, D3D12_RESOURCE_FLAGS flags, uint32_t offset_alignment);

  private:
    static uint32_t getSignature(const Heap &heap);

    HeapSuballocatorImpl<ResourceHeapProperties::group_count> impl;
  };

  class BufferHeapState
  {
  public:
    struct StandbyInfo
    {
      uint64_t progress;
      uint32_t index;
      ValueRange<uint64_t> range;
    };

    eastl::pair<Heap *, ValueRange<uint64_t>> tryAllocateFromReadyList(ResourceHeapProperties properties, uint64_t size,
      D3D12_RESOURCE_FLAGS flags, uint64_t offset_alignment, bool allow_offset);

    eastl::pair<Heap *, ValueRange<uint64_t>> trySuballocateFromExistingHeaps(ResourceHeapProperties properties, uint64_t size,
      D3D12_RESOURCE_FLAGS flags, uint32_t offset_alignment);

    BufferGlobalId adoptBufferHeap(Heap &&heap);

    const dag::Vector<Heap> &getBufferHeaps() const { return bufferHeaps; }
    size_t getHeapCount() const { return bufferHeaps.size(); }
    const Heap &getConstHeap(uint32_t index) const { return bufferHeaps[index]; }

    Heap &getHeap(uint32_t index) { return bufferHeaps[index]; }

    const dag::Vector<StandbyInfo> &getBufferHeapDiscardStandbyList() const { return bufferHeapDiscardStandbyList; }

    size_t freeBufferHeap(BufferHeap *manager, uint32_t index, const char *name, bool update_defragmentation_generation);
    size_t freeBufferHeap(BufferHeap *manager, uint32_t index, ValueRange<uint64_t> range, const char *name);

    eastl::pair<BufferGlobalId, HRESULT> createBufferHeap(BufferHeap *manager, DXGIAdapter *adapter, ID3D12Device *device,
      uint64_t allocation_size, ResourceHeapProperties properties, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initial_state,
      const char *name, AllocationFlags allocation_flags = {});

    bool isValidBuffer(const BufferState &buf);

    size_t freeBuffer(BufferHeap *manager, const BufferState &buf, BufferHeap::FreeReason free_reason, uint64_t progress,
      const char *name);

    // Updates all entries of old_buffer in the standby list to use new_buffer instead.
    // Returns true if the whole buffer is part of the standby list
    bool moveStandbyBufferEntries(BufferGlobalId old_buffer, BufferGlobalId new_buffer);

    size_t clear(ResourceMemoryHeapProvider *provider);
    size_t cleanupDiscardStandbyList(BufferHeap *manager, uint64_t progressIndex);

    static bool shouldFreeBuffer(const Heap &heap, BufferHeap::FreeReason free_reason);

  private:
    dag::Vector<Heap> bufferHeaps;
    dag::Vector<StandbyInfo> bufferHeapDiscardStandbyList;
    // list of buffers can be big, up to 2k, so saving the free slots should be more efficient than searching on each allocate
    dag::Vector<uint32_t> freeBufferSlots;

    HeapSuballocator suballocator;
  };


  using BufferHeapStateWrapper = ContainerMutexWrapper<BufferHeapState, OSSpinlock>;
  BufferHeapStateWrapper bufferHeapState;

  void notifyBufferMemoryRelease(size_t sz);
  void notifyBufferMemoryAllocate(size_t sz, bool /*kick_off_shuffle*/);

  BufferGlobalId tryCloneBuffer(DXGIAdapter *adapter, ID3D12Device *device, BufferGlobalId buffer_id,
    BufferHeapStateWrapper::AccessToken &bufferHeapStateAccess, AllocationFlags allocation_flags);

  // Checks if the buffer is actually needed and if not it will be deleted immediately
  bool tidyCloneBuffer(BufferGlobalId buffer_id, BufferHeapStateWrapper::AccessToken &bufferHeapStateAccess);

  BufferState moveBuffer(const BufferState &current_buffer, BufferGlobalId new_buffer,
    BufferHeapStateWrapper::AccessToken &bufferHeapStateAccess);

  void shutdown();
  void preRecovery();

public:
  // NOTE needs context lock to be held for frame end data structure
  BufferState discardBuffer(DXGIAdapter *adapter, Device &device, BufferState to_discared, DeviceMemoryClass memory_class,
    FormatStore format, uint32_t struct_size, bool raw_view, bool struct_view, D3D12_RESOURCE_FLAGS flags, uint32_t cflags,
    const char *name, uint32_t frame_index, bool disable_sub_alloc);

  BufferState allocateBuffer(DXGIAdapter *adapter, Device &device, uint64_t size, uint32_t structure_size, uint32_t discard_count,
    DeviceMemoryClass memory_class, D3D12_RESOURCE_FLAGS flags, uint32_t cflags, const char *name, bool disable_sub_alloc);

  BufferState allocateBufferWithoutDefragmentation(DXGIAdapter *adapter, Device &device, uint64_t size, uint32_t structure_size,
    uint32_t discard_count, DeviceMemoryClass memory_class, D3D12_RESOURCE_FLAGS flags, uint32_t cflags, const char *name,
    bool disable_sub_alloc, const ResourceHeapProperties &heap_properties, AllocationFlags allocation_flags, HRESULT &error_code);

  void completeFrameExecution(const CompletedFrameExecutionInfo &info, PendingForCompletedFrameData &data);
  void freeBufferOnFrameCompletion(BufferState &&buffer, FreeReason free_reason);

  template <typename T>
  void visitBuffers(T clb)
  {
    auto bufferHeapStateAccess = bufferHeapState.access();
    clb.beginVisit();
    clb.beginVisitBuffers();
    for (auto &buf : bufferHeapStateAccess->getBufferHeaps())
    {
      if (!buf)
      {
        continue;
      }
      auto freeSum = eastl::accumulate(cbegin(buf.getFreeRanges()), cend(buf.getFreeRanges()), 0,
        [](uint64_t v, const auto &r) { return v + r.size(); });
      bool isDiscardReady =
        cend(bufferHeapStateAccess->getBufferHeapDiscardStandbyList()) !=
        eastl::find_if(cbegin(bufferHeapStateAccess->getBufferHeapDiscardStandbyList()),
          cend(bufferHeapStateAccess->getBufferHeapDiscardStandbyList()), [id = buf.index()](auto info) { return info.index == id; });
      clb.visitBuffer(buf.index(), buf.getBufferMemorySize(), freeSum, buf.getFlags(), isDiscardReady);
      for (auto r : buf.getFreeRanges())
      {
        clb.visitBufferFreeRange(r);
      }
    }
    clb.endVisitBuffers();
    clb.beginVisitStandbyRange();
    for (auto &info : bufferHeapStateAccess->getBufferHeapDiscardStandbyList())
    {
      clb.visitBufferStandbyRange(info.progress, info.index, info.range);
    }
    clb.endVisitStandbyRange();
    clb.endVisit();
  }
};
} // namespace drv3d_dx12::resource_manager
