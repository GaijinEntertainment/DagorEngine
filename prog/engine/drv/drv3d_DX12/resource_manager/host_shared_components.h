// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "basic_buffer.h"
#include "esram_components.h"
#include <constants.h>
#include <container_mutex_wrapper.h>
#include <d3d12_error_handling.h>
#include <driver.h>
#include <host_device_shared_memory_region.h>
#include <resource_memory.h>

#include <supp/dag_comPtr.h>

namespace drv3d_dx12::resource_manager
{
class FramePushRingMemoryProvider : public ESRamPageMappingProvider
{
  using BaseType = ESRamPageMappingProvider;

protected:
  using HostDeviceSharedMemoryRegionAllocationResult = dag::Expected<HostDeviceSharedMemoryRegion, MemoryAllocationError>;

  struct CompletedFrameExecutionInfo : BaseType::CompletedFrameExecutionInfo
  {
    uint32_t historyIndex;
  };

  struct PushRingMemoryState
  {
    using HeapType = FramePushRingMemoryProvider;

    static constexpr uint32_t min_ring_size = 2 * 1024 * 1024;
    static constexpr uint32_t default_ring_size = 16 * 1024 * 1024;
    static constexpr uint32_t default_large_push_threshold = 2 * 1024 * 1024;

    struct RingSegment
    {
      ComPtr<ID3D12Resource> buffer;
      ResourceMemoryLocationWithGPUAndCPUAddress baseLocation;
      uint64_t bufferSize = 0;
      // Offset where we allocate next chunk from
      uint32_t allocationOffset = 0;
      // Total allocated, bufferSize - allocationSize yields free memory
      uint32_t allocationSize = 0;
      // Allocation size since last snapshot
      uint32_t currentAllocation = 0;
      // coincides with latched frame index
      uint32_t allocationHistory[FRAME_FRAME_BACKLOG_LENGTH] = {};

      ID3D12Resource *getResourcePtr() const { return buffer.Get(); }
      D3D12_GPU_VIRTUAL_ADDRESS getGPUPointer() const { return baseLocation.gpuAddress; }
      uint8_t *getCPUPointer() const { return baseLocation.cpuAddress; }
      uint64_t getBufferMemorySize() const { return bufferSize; }

      /// Empty when the ring has no room left, which is an ordinary outcome; the caller falls back
      /// to the temporary upload memory.
      eastl::optional<HostDeviceSharedMemoryRegion> allocate(uint32_t size, uint32_t alignment)
      {
        HostDeviceSharedMemoryRegion result;

        auto allocationBegin = align_value(allocationOffset, alignment);
        auto allocationEnd = allocationBegin + size;

        auto freeMemory = bufferSize - allocationSize;
        auto freeBegin = allocationOffset;
        auto freeEnd = freeBegin + freeMemory;

        // wants to allocate more than we have free
        if (allocationEnd > freeEnd)
        {
          return {};
        }

        uint32_t newAllocationSize;
        if (allocationEnd > bufferSize)
        {
          const uint32_t extra = bufferSize - freeBegin;
          // we have to wrap
          allocationBegin = 0;
          allocationEnd = size;
          freeEnd -= bufferSize;

          // check free space again after wrap
          if (allocationEnd > freeEnd)
          {
            return {};
          }

          newAllocationSize = extra + size;
        }
        else
        {
          newAllocationSize = allocationEnd - allocationOffset;
        }

        result.buffer = buffer.Get();
        result.memoryLocation = baseLocation + allocationBegin;
        result.range = ValueRange<uint64_t>{allocationBegin, allocationEnd};

        allocationOffset = allocationEnd;
        allocationSize += newAllocationSize;
        currentAllocation += newAllocationSize;

        result.source = HostDeviceSharedMemoryRegion::Source::PUSH_RING;
        return result;
      }

      void finishRecording(uint32_t history_index)
      {
        allocationHistory[history_index] = currentAllocation;
        currentAllocation = 0;
      }

      void finishExecution(uint32_t history_index)
      {
        allocationSize -= allocationHistory[history_index];
        allocationHistory[history_index] = 0;
      }

      void reset()
      {
        buffer.Reset();
        baseLocation = {};
        bufferSize = 0;
      }
    };

    // Single ring buffer, allocated during setup.
    // When the ring can not provide memory, the caller falls back to temporary upload memory.
    RingSegment segment;

    void finishRecording(uint32_t history_index)
    {
      G_ASSERT(history_index < FRAME_FRAME_BACKLOG_LENGTH);
      segment.finishRecording(history_index);
    }

    void finishExecution(uint32_t history_index)
    {
      G_ASSERT(history_index < FRAME_FRAME_BACKLOG_LENGTH);
      segment.finishExecution(history_index);
    }

    void create(HeapType *heap, ID3D12Device *device, uint32_t size)
    {
      segment = {};
#if _TARGET_PC_WIN
      auto memoryProperties = heap->getPushHeapProperties();
      const auto &fs = heap->getFeatureSet();

      const D3D12_HEAP_PROPERTIES heapProperties = {
        .Type = memoryProperties.getHeapType(),
        .CPUPageProperty = memoryProperties.getCpuPageProperty(fs),
        .MemoryPoolPreference = memoryProperties.getMemoryPool(fs),
        .CreationNodeMask = 0,
        .VisibleNodeMask = 0,
      };

      const auto initialState = HeapType::propertiesToInitialState(D3D12_RESOURCE_DIMENSION_BUFFER, D3D12_RESOURCE_FLAG_NONE,
        DeviceMemoryClass::PUSH_RING_BUFFER);
#else
      const D3D12_HEAP_PROPERTIES heapProperties = {
        .Type = D3D12_HEAP_TYPE_UPLOAD,
        .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
        .CreationNodeMask = 0,
        .VisibleNodeMask = 0,
      };

      const auto initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
#endif

      const D3D12_RESOURCE_DESC desc = {
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
        .Width = align_value(size, min_ring_size),
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_UNKNOWN,
        .SampleDesc = {.Count = 1, .Quality = 0},
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        .Flags = D3D12_RESOURCE_FLAG_NONE,
      };

      ByteUnits ringSizeUnits{desc.Width};
      logdbg("DX12: Allocating push ring buffer of %.2f %s", ringSizeUnits.units(), ringSizeUnits.name());
      if (!DX12_CHECK_OK(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc, initialState, nullptr,
            COM_ARGS(&segment.buffer))))
      {
        DAG_FATAL("DX12: Failed to create push ring buffer of %u bytes", size);
      }

      segment.bufferSize = desc.Width;
      segment.baseLocation.gpuAddress = segment.buffer->GetGPUVirtualAddress();

      D3D12_RANGE emptyRange{};
      uint8_t *mappedPtr = nullptr;
      segment.buffer->Map(0, &emptyRange, reinterpret_cast<void **>(&mappedPtr));
      segment.baseLocation.cpuAddress = mappedPtr;

      heap->recordConstantRingAllocated(desc.Width);
    }

    eastl::optional<HostDeviceSharedMemoryRegion> allocate(uint32_t size, uint32_t alignment, uint32_t frame_index)
    {
      // Per-frame allocation limit to prevent one frame from consuming the entire ring,
      // which would cause oscillation between ring and temp upload fallback.
      // Per-frame usage after this allocation.
      uint32_t frameUsage = segment.currentAllocation + size;
      // Hard cap: no single frame may use more than half the buffer.
      if (frameUsage > segment.bufferSize / 2)
      {
        return {};
      }
      // Look-ahead: assume the next frame needs roughly the same total as this frame.
      // The next oldest frame completion frees at least minFreed bytes.
      const uint32_t minFreed = segment.allocationHistory[(frame_index + 1) % FRAME_FRAME_BACKLOG_LENGTH];
      const uint32_t available = static_cast<uint32_t>(segment.bufferSize) - segment.allocationSize + minFreed;
      if (frameUsage + frameUsage > available)
      {
        return {};
      }
      return segment.allocate(size, alignment);
    }

    size_t currentMemorySize() const { return segment.getBufferMemorySize(); }

    void shutdown(HeapType *heap)
    {
      heap->recordConstantRingFreed(segment.getBufferMemorySize());
      segment.reset();
    }
  };

  using PushRingMemoryStateWrapper = ContainerMutexWrapper<PushRingMemoryState, OSSpinlock>;
  PushRingMemoryStateWrapper pushRing;
  uint32_t largePushThreshold = PushRingMemoryState::default_large_push_threshold;

  struct SetupInfo : BaseType::SetupInfo
  {
    uint32_t pushRingSize = PushRingMemoryState::default_ring_size;
    uint32_t largePushThreshold = PushRingMemoryState::default_large_push_threshold;
  };

  void setup(const SetupInfo &info)
  {
    BaseType::setup(info);
    uint32_t ringSize = max(info.pushRingSize, PushRingMemoryState::min_ring_size);
    largePushThreshold = min(info.largePushThreshold, ringSize / 3);
    pushRing.access()->create(this, info.device, ringSize);
    ByteUnits thresholdUnits{largePushThreshold};
    logdbg("DX12: Push ring large push threshold set to %.2f %s", thresholdUnits.units(), thresholdUnits.name());
  }

  void preRecovery()
  {
    pushRing.access()->shutdown(this);
    BaseType::preRecovery();
  }

  void shutdown()
  {
    pushRing.access()->shutdown(this);
    BaseType::shutdown();
  }

  struct CompletedFrameRecordingInfo : BaseType::CompletedFrameRecordingInfo
  {
    uint32_t historyIndex;
  };

public:
  /// Empty when the request is too large for the ring or the ring is full for this frame. Both are
  /// ordinary outcomes, the derived provider then serves the request from temporary upload memory.
  eastl::optional<HostDeviceSharedMemoryRegion> allocatePushMemory(DXGIAdapter *adapter, Device &device, uint32_t size,
    uint32_t alignment, uint32_t frame_index);

  ResourceHeapProperties getPushHeapProperties()
  {
    return getProperties(D3D12_RESOURCE_FLAG_NONE, DeviceMemoryClass::PUSH_RING_BUFFER, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
  }

  void completeFrameRecording(const CompletedFrameRecordingInfo &info)
  {
    BaseType::completeFrameRecording(info);
    pushRing.access()->finishRecording(info.historyIndex);
  }

  void completeFrameExecution(const CompletedFrameExecutionInfo &info, PendingForCompletedFrameData &data)
  {
    BaseType::completeFrameExecution(info, data);
    pushRing.access()->finishExecution(info.historyIndex);
  }

  size_t getFramePushRingMemorySize() { return pushRing.access()->currentMemorySize(); }

  void freeHostDeviceSharedMemoryRegionOnFrameCompletion(HostDeviceSharedMemoryRegion mem)
  {
    G_ASSERT(HostDeviceSharedMemoryRegion::Source::PUSH_RING == mem.source);
    G_UNUSED(mem);
  }
};

class TemporaryUploadMemoryProvider : public FramePushRingMemoryProvider
{
  using BaseType = FramePushRingMemoryProvider;

protected:
  struct PendingForCompletedFrameData : BaseType::PendingForCompletedFrameData
  {
    struct FreeRange
    {
      ID3D12Resource *buffer;
      ValueRange<uint64_t> range;
    };
    dag::Vector<FreeRange> uploadBufferFrees;
    uint32_t uploadBufferUsage = 0;
    uint32_t tempUsage = 0;
  };

  struct TemporaryUploadMemoryInfo
  {
    using HeapType = TemporaryUploadMemoryProvider;
    // never have less than one mibyte
    static constexpr size_t min_buffer_size = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT * 16;
    static constexpr uint32_t drop_timeout = FRAME_FRAME_BACKLOG_LENGTH * 256;
    static constexpr DeviceMemoryClass memory_class = DeviceMemoryClass::TEMPORARY_UPLOAD_BUFFER;

    static void onSegmentAdd(HeapType *heap, ID3D12Resource *buffer, ResourceMemory mem)
    {
      heap->updateMemoryRangeUse(mem, TempUploadBufferReference{buffer});
      heap->recordTempBufferAllocated(mem.size());
    }
    static void onSegmentAddNoLock(HeapType *heap, ID3D12Resource *buffer, ResourceMemory mem)
    {
      heap->updateMemoryRangeUseNoLock(mem, TempUploadBufferReference{buffer});
      heap->recordTempBufferAllocated(mem.size());
    }
    static void onSegmentRemove(HeapType *heap, size_t size) { heap->recordTempBufferFreed(size); }

    struct Buffer : BasicBuffer
    {
      uint32_t allocationOffset = 0; // write head: next allocation starts here (after alignment)
      uint32_t allocationSize = 0;   // total bytes of live (not yet freed) allocations
      uint32_t freeOffset = 0;       // read head: oldest live data starts here
      dag::Vector<ValueRange<uint64_t>> deferredFrees;

      bool hasAllocations() const { return allocationSize > 0; }

      bool canFitAllocation(size_t size, size_t alignment) const
      {
        if (allocationOffset == freeOffset && hasAllocations()) // full
          return false;
        size_t offset = (allocationOffset + alignment - 1) & ~(alignment - 1);
        if (allocationOffset >= freeOffset)
        {
          // Non-wrapped: try fitting at the tail end
          if (offset + size <= getBufferMemorySize())
            return true;
          // Try wrapping to the beginning (offset 0 satisfies any power-of-2 alignment)
          return size <= freeOffset;
        }
        // Already wrapped: free space is [allocationOffset, freeOffset), must fit there
        return offset + size <= freeOffset;
      }

      void resetRing()
      {
        allocationOffset = 0;
        allocationSize = 0;
        freeOffset = 0;
        deferredFrees.clear();
      }

      // Bump-allocate from the ring. Returns the region and total ring space consumed
      // (data size + alignment padding + tail waste on wrap).
      eastl::pair<HostDeviceSharedMemoryRegion, size_t> allocateRing(size_t size, size_t alignment)
      {
        G_ASSERT(canFitAllocation(size, alignment));
        size_t offset = (allocationOffset + alignment - 1) & ~(alignment - 1);
        size_t consumed;

        if (offset + size > getBufferMemorySize())
        {
          // Wrap around: the tail [allocationOffset, bufferEnd) is unusable ring space.
          // Pre-insert it into deferredFrees so drainDeferredFrees skips over it.
          if (allocationOffset < getBufferMemorySize())
          {
            free_list_insert_and_coalesce(deferredFrees, ValueRange<uint64_t>{allocationOffset, getBufferMemorySize()});
          }
          consumed = getBufferMemorySize() - allocationOffset + size;
          offset = 0;
          allocationOffset = size;
        }
        else
        {
          // Normal path: alignment padding between previous end and this allocation
          // is ring space that no one will free explicitly.
          if (offset > allocationOffset)
          {
            free_list_insert_and_coalesce(deferredFrees, ValueRange<uint64_t>{allocationOffset, static_cast<uint64_t>(offset)});
          }
          consumed = offset + size - allocationOffset;
          allocationOffset = offset + size;
        }
        allocationSize += size;

        HostDeviceSharedMemoryRegion result;
        result.buffer = getResourcePtr();
        result.memoryLocation = static_cast<ResourceMemoryLocationWithGPUAndCPUAddress>(getBufferMemory()) + offset;
        result.range = make_value_range<uint64_t>(offset, size);
        return {result, consumed};
      }

      void freeRange(ValueRange<uint64_t> ring_range)
      {
        allocationSize -= ring_range.size();
        if (ring_range.front() == freeOffset)
        {
          // we want to advance by the ring_range size, back would be one short (as back is front + size - 1), so +1
          freeOffset = ring_range.back() + 1;
          if (freeOffset >= getBufferMemorySize())
            freeOffset = 0;
          drainDeferredFrees();
        }
        else
        {
          free_list_insert_and_coalesce(deferredFrees, ring_range);
        }
      }

      void drainDeferredFrees()
      {
        // After wrapping, deferred ranges are sorted by offset, but freeOffset may need
        // to jump from the buffer end back to 0. Use binary search instead of checking
        // only the front element.
        for (;;)
        {
          auto it = eastl::lower_bound(deferredFrees.begin(), deferredFrees.end(), freeOffset,
            [](const ValueRange<uint64_t> &range, uint64_t offset) { return range.front() < offset; });
          if (it == deferredFrees.end() || it->front() != freeOffset)
            break;
          freeOffset = it->back() + 1;
          deferredFrees.erase(it);
          // When draining tail waste that ends at the buffer boundary, wrap freeOffset
          if (freeOffset >= getBufferMemorySize())
            freeOffset = 0;
        }
      }
    };
    Buffer currentBuffer;
    Buffer standbyBuffer;
    size_t nextBufferSize = min_buffer_size;
    size_t currentBufferUse = 0;
    size_t nextBufferSizeShrinkThreshold = min_buffer_size;
    uint32_t timesSinceUse = 0;

    dag::Vector<Buffer> buffers;
    dag::Vector<Buffer> deletedBuffers;

    uint32_t uploadBufferUsage = 0;
    // TODO make configurable
    uint32_t uploadBufferUsageLimit = 256 * 1024 * 1024;
    uint32_t tempUsage = 0;
    // TODO make configurable
    uint32_t tempUsageLimit = 256 << 20;

    HostDeviceSharedMemoryRegionAllocationResult allocate(HeapType *heap, DXGIAdapter *adapter, ID3D12Device *device, size_t size,
      size_t alignment)
    {
      if (!currentBuffer || !currentBuffer.canFitAllocation(size, alignment))
      {
        if (currentBuffer)
        {
          if (currentBuffer.hasAllocations())
          {
            buffers.push_back(eastl::move(currentBuffer));
            // Move does not reset the memory info, clear it to avoid stale references.
            currentBuffer.setBufferMemory({});
          }
          else
          {
            onSegmentRemove(heap, currentBuffer.getBufferMemorySize());
            currentBuffer.reset(heap, true);
          }
        }

        if (standbyBuffer && standbyBuffer.getBufferMemorySize() >= size)
        {
          currentBuffer = eastl::move(standbyBuffer);
          // Move does not reset the memory info, clear it to avoid stale references.
          standbyBuffer.setBufferMemory({});
        }
        else
        {
          if (standbyBuffer)
          {
            onSegmentRemove(heap, standbyBuffer.getBufferMemorySize());
            standbyBuffer.reset(heap, true);
          }
          // Ensure no stale memory info remains before creating a new resource into currentBuffer.
          currentBuffer.setBufferMemory({});

          D3D12_RESOURCE_DESC desc;
          desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
          desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
          desc.Width = max(nextBufferSize, align_value<size_t>(size, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT));
          desc.Height = 1;
          desc.DepthOrArraySize = 1;
          desc.MipLevels = 1;
          desc.Format = DXGI_FORMAT_UNKNOWN;
          desc.SampleDesc.Count = 1;
          desc.SampleDesc.Quality = 0;
          desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
          desc.Flags = D3D12_RESOURCE_FLAG_NONE;

          D3D12_RESOURCE_ALLOCATION_INFO allocInfo;
          allocInfo.SizeInBytes = desc.Width;
          allocInfo.Alignment = desc.Alignment;

          auto initialState = heap->propertiesToInitialState(D3D12_RESOURCE_DIMENSION_BUFFER, D3D12_RESOURCE_FLAG_NONE, memory_class);

          auto memoryProperties = heap->getProperties(D3D12_RESOURCE_FLAG_NONE, memory_class, allocInfo.Alignment);

          auto allocationResult = heap->allocate(adapter, device, memoryProperties, allocInfo, {});
          if (!allocationResult.has_value())
          {
            return dag::Unexpected{allocationResult.error()};
          }

          auto &allocation = allocationResult.value();

          HRESULT errorCode = currentBuffer.create(device, desc, allocation, initialState, true);
          if (DX12_CHECK_FAIL(errorCode))
          {
            heap->free(allocation);
            // TODO: This is not 100% correct, as the allocation for the pool went through but for some
            // reason the object creation failed, probably should implement something for this properly.
            // For other objects, like textures we are not reporting any oom error when the object
            // create did fail.
            return dag::Unexpected{heap->makeMemoryAllocationError(errorCode, desc.Width, memoryProperties)};
          }

          onSegmentAdd(heap, currentBuffer.getResourcePtr(), currentBuffer.getBufferMemory());
        }
        currentBuffer.resetRing();
      }
      auto [result, consumed] = currentBuffer.allocateRing(size, alignment);
      currentBufferUse += consumed;
      return result;
    }

    void trim(HeapType *heap)
    {
      if (standbyBuffer)
      {
        onSegmentRemove(heap, standbyBuffer.getBufferMemorySize());
      }
      standbyBuffer.reset(heap, true);
    }

    void trimNoLock(HeapType *heap)
    {
      if (standbyBuffer)
      {
        onSegmentRemove(heap, standbyBuffer.getBufferMemorySize());
      }
      standbyBuffer.reset(heap, false);
    }

    bool shouldSwapStandbyBuffer(Buffer &other)
    {
      if (!standbyBuffer)
      {
        return true;
      }
      if (other.getBufferMemorySize() < nextBufferSize)
      {
        return false;
      }
      if (other.getBufferMemorySize() > nextBufferSize * 2)
      {
        return false;
      }
      return standbyBuffer.getBufferMemorySize() < other.getBufferMemorySize();
    }

    template <typename Handler>
    static bool free(HeapType *heap, dag::Vector<Buffer> &buffer_set, ID3D12Resource *ref, ValueRange<uint64_t> ring_range,
      Handler temp_swap_handler)
    {
      auto iter = eastl::find_if(begin(buffer_set), end(buffer_set),
        [ref](const auto &buf) //
        { return ref == buf.getResourcePtr(); });
      if (iter == end(buffer_set))
      {
        return false;
      }
      iter->freeRange(ring_range);
      if (!iter->hasAllocations())
      {
        if (!temp_swap_handler(*iter))
        {
          onSegmentRemove(heap, iter->getBufferMemorySize());
          iter->reset(heap, true);
        }
        *iter = eastl::move(buffer_set.back());
        buffer_set.pop_back();
      }
      return true;
    }

    void free(HeapType *heap, ID3D12Resource *ref, ValueRange<uint64_t> ring_range)
    {
      if (ref == currentBuffer.getResourcePtr())
      {
        currentBuffer.freeRange(ring_range);
        if (!currentBuffer.hasAllocations())
        {
          currentBuffer.resetRing();
        }
        return;
      }

      if (free(heap, buffers, ref, ring_range, [this, heap](auto &buffer) {
            if (!shouldSwapStandbyBuffer(buffer))
            {
              return false;
            }
            if (standbyBuffer)
            {
              onSegmentRemove(heap, standbyBuffer.getBufferMemorySize());
            }
            standbyBuffer.reset(heap, true);
            standbyBuffer = eastl::move(buffer);
            standbyBuffer.resetRing();
            return true;
          }))
      {
        return;
      }

      if (free(heap, deletedBuffers, ref, ring_range, [](auto &) { return false; }))
      {
        return;
      }

      G_ASSERTF(false, "DX12: Tried to free temp ref %p, but no matching buffer was found", ref);
    }

    void free(HeapType *heap, const dag::Vector<PendingForCompletedFrameData::FreeRange> &list)
    {
      for (auto &entry : list)
      {
        free(heap, entry.buffer, entry.range);
      }
    }

    void completeFrameRecording(HeapType *heap)
    {
      // keep size aligned to min alignment
      nextBufferSize =
        max(align_value<size_t>(currentBufferUse, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT), max(min_buffer_size, nextBufferSize));

      if (nextBufferSize > nextBufferSizeShrinkThreshold && currentBufferUse < nextBufferSize / 2)
      {
        nextBufferSize /= 2;
      }

      currentBufferUse = 0;
      if (!currentBuffer.hasAllocations())
      {
        if (++timesSinceUse > drop_timeout)
        {
          if (standbyBuffer)
          {
            onSegmentRemove(heap, standbyBuffer.getBufferMemorySize());
            standbyBuffer.reset(heap, true);
          }
          else if (currentBuffer)
          {
            onSegmentRemove(heap, currentBuffer.getBufferMemorySize());
            currentBuffer.reset(heap, true);
          }
          timesSinceUse = 0;
        }
      }
      else
        timesSinceUse = 0;
    }

    size_t currentMemorySize() const
    {
      return eastl::accumulate(begin(buffers), end(buffers), currentBuffer.getBufferMemorySize() + standbyBuffer.getBufferMemorySize(),
        [](size_t value, auto &buffer) { return value + buffer.getBufferMemorySize(); });
    }

    void shutdown(HeapType *heap)
    {
      uploadBufferUsage = 0;
      tempUsage = 0;

      for (auto &buf : buffers)
      {
        buf.reset(heap, true);
      }
      buffers.clear();
      for (auto &buf : deletedBuffers)
      {
        logdbg("DX12: TemporaryUploadMemoryInfo::shutdown: A deleted buffer was still alive during "
               "shutdown, %p with %u bytes still allocated",
          buf.getResourcePtr(), buf.allocationSize);
        buf.reset(heap, true);
      }
      deletedBuffers.clear();
      standbyBuffer.reset(heap, true);
      currentBuffer.reset(heap, true);
      currentBuffer.resetRing();

      currentBufferUse = 0;
    }

    bool tryMoveStandbyBuffer(HeapType *heap, DXGIAdapter *adapter, ID3D12Device *device, ID3D12Resource *buffer,
      AllocationFlags allocation_flags)
    {
      if (standbyBuffer.getResourcePtr() != buffer)
      {
        return false;
      }

      D3D12_RESOURCE_DESC desc;
      desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
      desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
      desc.Width = standbyBuffer.getBufferMemorySize();
      desc.Height = 1;
      desc.DepthOrArraySize = 1;
      desc.MipLevels = 1;
      desc.Format = DXGI_FORMAT_UNKNOWN;
      desc.SampleDesc.Count = 1;
      desc.SampleDesc.Quality = 0;
      desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
      desc.Flags = D3D12_RESOURCE_FLAG_NONE;

      D3D12_RESOURCE_ALLOCATION_INFO allocInfo;
      allocInfo.SizeInBytes = desc.Width;
      allocInfo.Alignment = desc.Alignment;

      auto initialState = heap->propertiesToInitialState(D3D12_RESOURCE_DIMENSION_BUFFER, D3D12_RESOURCE_FLAG_NONE, memory_class);

      auto memoryProperties = heap->getProperties(D3D12_RESOURCE_FLAG_NONE, memory_class, allocInfo.Alignment);
      auto allocationResult = heap->allocate(adapter, device, memoryProperties, allocInfo, allocation_flags);
      if (!allocationResult.has_value())
      {
        return false;
      }

      auto &allocation = allocationResult.value();

      Buffer newStandbyBuffer;
      const auto errorCode = newStandbyBuffer.create(device, desc, allocation, initialState, true);
      if (DX12_CHECK_FAIL(errorCode))
      {
        heap->free(allocation);
        return false;
      }

      onSegmentRemove(heap, standbyBuffer.getBufferMemorySize());
      standbyBuffer.reset(heap, true);

      standbyBuffer = newStandbyBuffer;
      onSegmentAdd(heap, standbyBuffer.getResourcePtr(), standbyBuffer.getBufferMemory());
      return true;
    }

    eastl::pair<D3D12_RESOURCE_DESC, D3D12_RESOURCE_ALLOCATION_INFO> calculate_temp_buffer_desc_alloc_info(uint64_t size)
    {
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
      desc.Flags = D3D12_RESOURCE_FLAG_NONE;

      D3D12_RESOURCE_ALLOCATION_INFO allocInfo;
      allocInfo.SizeInBytes = desc.Width;
      allocInfo.Alignment = desc.Alignment;

      return {desc, allocInfo};
    }

    bool tryMoveBuffer(HeapType *heap, DXGIAdapter *adapter, ID3D12Device *device, Buffer &buffer, AllocationFlags allocation_flags)
    {
      auto [desc, allocInfo] = calculate_temp_buffer_desc_alloc_info(buffer.getBufferMemorySize());

      auto initialState = heap->propertiesToInitialState(D3D12_RESOURCE_DIMENSION_BUFFER, D3D12_RESOURCE_FLAG_NONE, memory_class);

      auto memoryProperties = heap->getProperties(D3D12_RESOURCE_FLAG_NONE, memory_class, allocInfo.Alignment);

      auto allocationResult = heap->allocate(adapter, device, memoryProperties, allocInfo, allocation_flags);
      if (!allocationResult.has_value())
      {
        return false;
      }

      auto &allocation = allocationResult.value();

      Buffer newBuffer;
      const auto errorCode = newBuffer.create(device, desc, allocation, initialState, true);
      if (DX12_CHECK_FAIL(errorCode))
      {
        heap->free(allocation);
        return false;
      }

      if (buffer.hasAllocations())
      {
        deletedBuffers.push_back(eastl::move(buffer));
      }
      else
      {
        onSegmentRemove(heap, buffer.getBufferMemorySize());
        buffer.reset(heap, true);
      }

      buffer = eastl::move(newBuffer);

      onSegmentAdd(heap, buffer.getResourcePtr(), buffer.getBufferMemory());
      return true;
    }

    bool tryMoveBufferToLocation(HeapType *heap, ID3D12Device *device, Buffer &buffer, HeapID heap_id, uint32_t free_range_index)
    {
      auto memory = buffer.getBufferMemory();
      heap->freeNoLock(memory, false);
      memory.heap = {}; // reset heap ptr to avoid double free
      buffer.setBufferMemory(memory);

      auto [desc, allocInfo] = calculate_temp_buffer_desc_alloc_info(buffer.getBufferMemorySize());
      auto allocationResult = heap->allocateMemoryInPlace(heap_id, free_range_index, allocInfo);
      if (!allocationResult.has_value())
      {
        return false;
      }
      auto &allocation = allocationResult.value();

      auto initialState = heap->propertiesToInitialState(D3D12_RESOURCE_DIMENSION_BUFFER, D3D12_RESOURCE_FLAG_NONE, memory_class);
      Buffer newBuffer;
      const auto errorCode = newBuffer.create(device, desc, allocation, initialState, true);
      if (DX12_CHECK_FAIL(errorCode))
      {
        heap->freeNoLock(allocation, false);
        return false;
      }

      if (buffer.hasAllocations())
      {
        deletedBuffers.push_back(eastl::move(buffer));
      }
      else
      {
        onSegmentRemove(heap, buffer.getBufferMemorySize());
        buffer.reset(heap, false);
      }

      buffer = eastl::move(newBuffer);

      onSegmentAddNoLock(heap, buffer.getResourcePtr(), buffer.getBufferMemory());
      return true;
    }

    bool tryMoveBuffer(HeapType *heap, DXGIAdapter *adapter, ID3D12Device *device, ID3D12Resource *buffer,
      AllocationFlags allocation_flags)
    {
      if (currentBuffer.getResourcePtr() == buffer)
      {
        return tryMoveBuffer(heap, adapter, device, currentBuffer, allocation_flags);
      }
      auto ref = eastl::find_if(begin(buffers), end(buffers),
        [buffer](auto &buf) //
        { return buffer == buf.getResourcePtr(); });
      if (ref == end(buffers))
      {
        return false;
      }
      return tryMoveBuffer(heap, adapter, device, *ref, allocation_flags);
    }

    bool tryMoveBufferToLocation(HeapType *heap, ID3D12Device *device, ID3D12Resource *buffer, HeapID heap_id,
      uint32_t free_range_index)
    {
      if (currentBuffer.getResourcePtr() == buffer)
      {
        return tryMoveBufferToLocation(heap, device, currentBuffer, heap_id, free_range_index);
      }
      auto ref = eastl::find_if(begin(buffers), end(buffers),
        [buffer](auto &buf) //
        { return buffer == buf.getResourcePtr(); });
      if (ref == end(buffers))
      {
        return false;
      }
      return tryMoveBufferToLocation(heap, device, *ref, heap_id, free_range_index);
    }

    // This checks if any buffer has still some allocations outstanding to be freed
    bool isAllFree() const
    {
      // This intentionally has no short cuts to report all buffers that are still used
      bool anySeen = false;
      if (currentBuffer.hasAllocations())
      {
        logdbg("DX12: Buffer %p has still %u bytes allocated...", currentBuffer.getResourcePtr(), currentBuffer.allocationSize);
        anySeen = true;
      }
      // Scan all pending buffers and see if any of them has still some allocations pending.
      for (auto &buffer : buffers)
      {
        if (buffer.hasAllocations())
        {
          logdbg("DX12: Buffer %p has still %u bytes allocated...", buffer.getResourcePtr(), buffer.allocationSize);
          anySeen = true;
        }
      }
      return !anySeen;
    }
  };

  using TempMemoryStateWrapper = ContainerMutexWrapper<TemporaryUploadMemoryInfo, OSSpinlock>;
  TempMemoryStateWrapper tempBuffer;

  bool tryMoveTemporaryUploadStandbyBuffer(DXGIAdapter *adapter, ID3D12Device *device, ID3D12Resource *buffer,
    AllocationFlags allocation_flags)
  {
    return tempBuffer.access()->tryMoveStandbyBuffer(this, adapter, device, buffer, allocation_flags);
  }

  bool tryMoveTemporaryUploadBuffer(DXGIAdapter *adapter, ID3D12Device *device, ID3D12Resource *buffer,
    AllocationFlags allocation_flags)
  {
    return tempBuffer.access()->tryMoveBuffer(this, adapter, device, buffer, allocation_flags);
  }

  void completeFrameExecution(const CompletedFrameExecutionInfo &info, PendingForCompletedFrameData &data)
  {
    // sort by buffer pointer to make free a bit faster by doing one buffer at a time
    eastl::sort(begin(data.uploadBufferFrees), end(data.uploadBufferFrees),
      [](const auto &a, const auto &b) { return a.buffer < b.buffer; });
    {
      auto tempBufferAccess = tempBuffer.access();
      tempBufferAccess->free(this, data.uploadBufferFrees);
      tempBufferAccess->uploadBufferUsage -= data.uploadBufferUsage;
      tempBufferAccess->tempUsage -= data.tempUsage;
    }
    data.uploadBufferFrees.clear();
    data.uploadBufferUsage = 0;
    data.tempUsage = 0;

    BaseType::completeFrameExecution(info, data);
  }

  void preRecovery()
  {
    tempBuffer.access()->shutdown(this);

    BaseType::preRecovery();
  }

  void shutdown()
  {
    tempBuffer.access()->shutdown(this);

    BaseType::shutdown();
  }

  template <typename T>
  bool allTemporaryUsesCompleted(T visitable)
  {
    bool baseCompleted = BaseType::allTemporaryUsesCompleted(visitable);
    bool hasComplted = true;
    {
      hasComplted = tempBuffer.access()->isAllFree();
    }
    if (!hasComplted)
    {
      auto tempBufferAccess = tempBuffer.access();
      visitable([this, &tempBufferAccess](PendingForCompletedFrameData &data) {
        if (data.uploadBufferFrees.empty())
        {
          return;
        }
        eastl::sort(begin(data.uploadBufferFrees), end(data.uploadBufferFrees),
          [](const auto &a, const auto &b) { return a.buffer < b.buffer; });
        {
          tempBufferAccess->free(this, data.uploadBufferFrees);
          tempBufferAccess->uploadBufferUsage -= data.uploadBufferUsage;
          tempBufferAccess->tempUsage -= data.tempUsage;
        }
        data.uploadBufferFrees.clear();
        data.uploadBufferUsage = 0;
        data.tempUsage = 0;
      });
      hasComplted = tempBufferAccess->isAllFree();
    }
    return baseCompleted && hasComplted;
  }

public:
  HostDeviceSharedMemoryRegionAllocationResult tryAllocateTempUpload(DXGIAdapter *adapter, ID3D12Device *device, size_t size,
    size_t alignment, bool &should_flush)
  {
    auto tempBufferAccess = tempBuffer.access();
    return tempBufferAccess->allocate(this, adapter, device, size, alignment)
      .or_else([&, this](auto) {
        ByteUnits reqSize{size};
        logdbg("TemporaryUploadMemoryProvider::allocateTempUpload: Allocation failed, let's trim "
               "heaps and try again. Size: %.2f %s, Error code: 0x%08X",
          reqSize.units(), reqSize.name(), GetLastError());
        tempBufferAccess->trim(this);
        return tempBufferAccess->allocate(this, adapter, device, size, alignment);
      })
      .and_then([&](auto &&value) -> HostDeviceSharedMemoryRegionAllocationResult {
        tempBufferAccess->tempUsage += value.range.size();
        should_flush = tempBufferAccess->tempUsage > tempBufferAccess->tempUsageLimit;

        if (should_flush)
        {
          ByteUnits currentUsage{tempBufferAccess->tempUsage};
          ByteUnits usageLimit{tempBufferAccess->tempUsageLimit};
          ByteUnits reqSize{size};
          logdbg("DX12: Out of temp upload pool budget, usage %.2f %s of %.2f %s, while allocating %.2f %s, flushing.",
            currentUsage.units(), currentUsage.name(), usageLimit.units(), usageLimit.name(), reqSize.units(), reqSize.name());
        }
        return eastl::move(value);
      });
  }

  // Large push allocations (> 2 MiB) are redirected to temporary upload memory to avoid bloating the push ring.
  // The temporary memory is automatically recorded for frame-completion cleanup.
  HostDeviceSharedMemoryRegionAllocationResult allocatePushMemory(DXGIAdapter *adapter, Device &device, uint32_t size,
    uint32_t alignment, uint32_t frame_index);

  HostDeviceSharedMemoryRegionAllocationResult allocateTempUpload(DXGIAdapter *adapter, Device &device, size_t size, size_t alignment,
    bool &should_flush);

  HostDeviceSharedMemoryRegionAllocationResult allocateTempUploadForUploadBuffer(DXGIAdapter *adapter, Device &device, size_t size,
    size_t alignment);

  HostDeviceSharedMemoryRegionAllocationResult tryAllocateTempUploadForUploadBuffer(DXGIAdapter *adapter, ID3D12Device *device,
    size_t size, size_t alignment);

  void completeFrameRecording(const CompletedFrameRecordingInfo &info)
  {
    tempBuffer.access()->completeFrameRecording(this);
    BaseType::completeFrameRecording(info);
  }

  void setTempBufferShrinkThresholdSize(size_t size)
  {
    auto tempBufferAccess = tempBuffer.access();
    tempBufferAccess->nextBufferSizeShrinkThreshold = max(size, tempBufferAccess->min_buffer_size);
  }

  size_t getTemporaryUploadMemorySize() { return tempBuffer.access()->currentMemorySize(); }

  void freeHostDeviceSharedMemoryRegionOnFrameCompletion(HostDeviceSharedMemoryRegion mem)
  {
    if (HostDeviceSharedMemoryRegion::Source::TEMPORARY == mem.source)
    {
      accessRecodingPendingFrameCompletion<PendingForCompletedFrameData>(
        [buffer = mem.buffer, range = mem.range, size = mem.range.size()](auto &data) {
          data.uploadBufferFrees.push_back({buffer, range});
          data.tempUsage += size;
        });
    }
    else
    {
      BaseType::freeHostDeviceSharedMemoryRegionOnFrameCompletion(mem);
    }
  }

  void freeHostDeviceSharedMemoryRegionForUploadBufferOnFrameCompletion(HostDeviceSharedMemoryRegion mem)
  {
    G_ASSERT(HostDeviceSharedMemoryRegion::Source::TEMPORARY == mem.source);
    accessRecodingPendingFrameCompletion<PendingForCompletedFrameData>(
      [buffer = mem.buffer, range = mem.range, size = mem.range.size()](auto &data) {
        data.uploadBufferFrees.push_back({buffer, range});
        data.uploadBufferUsage += size;
      });
  }
};

class PersistentMemoryBase : public TemporaryUploadMemoryProvider
{
  using BaseType = TemporaryUploadMemoryProvider;

protected:
  template <typename T>
  struct PersistentMemoryState : T
  {
    using HeapType = typename T::HeapType;
    static constexpr HostDeviceSharedMemoryRegion::Source source_type = T::source_type;
    static constexpr size_t buffer_alignment = T::buffer_alignment;
    static constexpr DeviceMemoryClass memory_class = T::memory_class;

    struct MemoryBufferHeap : BasicBuffer
    {
      dag::Vector<ValueRange<uint64_t>> freeRanges;

      /// Empty when no free range of this buffer fits, the caller then looks at the next buffer or
      /// creates a new one.
      eastl::optional<HostDeviceSharedMemoryRegion> allocate(size_t size, size_t alignment)
      {
        HostDeviceSharedMemoryRegion result;
        auto at = free_list_find_smallest_fit_aligned(freeRanges, size, alignment);
        if (at != end(freeRanges))
        {
          auto range = make_value_range<uint64_t>(align_value<size_t>(at->front(), alignment), size);
          auto p2 = at->cutOut(range);
          if (at->empty())
          {
            if (p2.empty())
            {
              freeRanges.erase(at);
            }
            else
            {
              *at = p2;
            }
          }
          else if (!p2.empty())
          {
            // move p2 after at, as on 'cutOut' this will always store the first of the two ranges
            freeRanges.insert(at + 1, p2);
          }

          result.buffer = buffer.Get();
          result.memoryLocation = static_cast<ResourceMemoryLocationWithGPUAndCPUAddress>(bufferMemory) + range.front();
          result.range = range;
          result.source = source_type;
          return result;
        }
        return {};
      }
      bool free(ValueRange<uint64_t> range)
      {
        free_list_insert_and_coalesce(freeRanges, range);
        return freeRanges.size() == 1 && freeRanges.front().size() == bufferMemory.size;
      }
    };

    dag::Vector<MemoryBufferHeap> buffers;

    void free(HeapType *heap, ID3D12Resource *res, ValueRange<uint64_t> range)
    {
      auto heapRef = eastl::find_if(begin(buffers), end(buffers),
        [res](const auto &heap) //
        { return res == heap.getResourcePtr(); });
      G_ASSERT(heapRef != end(buffers));
      if (heapRef != end(buffers))
      {
        if (heapRef->free(range))
        {
          T::onSegmentRemove(heap, heapRef->getBufferMemorySize());
          heapRef->reset(heap, true);

          buffers.erase(heapRef);
        }
      }
    }

    HostDeviceSharedMemoryRegionAllocationResult allocate(HeapType *heap, DXGIAdapter *adapter, ID3D12Device *device, size_t size,
      size_t alignment)
    {
      for (auto &buffer : buffers)
      {
        auto existingSpace = buffer.allocate(size, alignment);
        if (existingSpace)
        {
          return *existingSpace;
        }
      }

      MemoryBufferHeap newHeap;

      D3D12_RESOURCE_DESC desc;
      desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
      desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
      desc.Width = align_value<size_t>(size, buffer_alignment);
      desc.Height = 1;
      desc.DepthOrArraySize = 1;
      desc.MipLevels = 1;
      desc.Format = DXGI_FORMAT_UNKNOWN;
      desc.SampleDesc.Count = 1;
      desc.SampleDesc.Quality = 0;
      desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
      desc.Flags = D3D12_RESOURCE_FLAG_NONE;

      D3D12_RESOURCE_ALLOCATION_INFO allocInfo;
      allocInfo.SizeInBytes = desc.Width;
      allocInfo.Alignment = desc.Alignment;

      auto initialState = heap->propertiesToInitialState(D3D12_RESOURCE_DIMENSION_BUFFER, D3D12_RESOURCE_FLAG_NONE, memory_class);

      auto memoryProperties = heap->getProperties(D3D12_RESOURCE_FLAG_NONE, memory_class, allocInfo.Alignment);

      auto allocationResult = heap->allocate(adapter, device, memoryProperties, allocInfo, {});

      if (!allocationResult.has_value())
      {
        return dag::Unexpected{allocationResult.error()};
      }

      auto &allocation = allocationResult.value();

      auto errorCode = newHeap.create(device, desc, allocation, initialState, true);
      if (DX12_CHECK_FAIL(errorCode))
      {
        heap->free(allocation);
        return dag::Unexpected{heap->makeMemoryAllocationError(errorCode, desc.Width, memoryProperties)};
      }

      T::onSegmentAdd(heap, newHeap.getBufferMemory(), newHeap.getResourcePtr());

      newHeap.freeRanges.push_back(make_value_range(0ull, desc.Width));
      auto result = newHeap.allocate(size, alignment);
      G_ASSERT(result.has_value());
      buffers.push_back(eastl::move(newHeap));
      if (!result)
      {
        return unexpected_memory_allocation_error(E_FAIL);
      }
      return *result;
    }

    void shutdown(HeapType *heap)
    {
      for (auto &buffer : buffers)
      {
        buffer.reset(heap, true);
      }
      buffers.clear();
    }

    size_t currentMemorySize() const
    {
      return eastl::accumulate(begin(buffers), end(buffers), 0,
        [](size_t value, auto &buffer) //
        { return value + buffer.getBufferMemorySize(); });
    }
  };
};

class PersistentUploadMemoryProvider : public PersistentMemoryBase
{
  using BaseType = PersistentMemoryBase;

protected:
  struct PendingForCompletedFrameData : BaseType::PendingForCompletedFrameData
  {
    dag::Vector<HostDeviceSharedMemoryRegion> uploadMemoryFrees;
  };

  struct PersistentUploadMemoryImplementation
  {
    using HeapType = PersistentUploadMemoryProvider;
    static constexpr HostDeviceSharedMemoryRegion::Source source_type = HostDeviceSharedMemoryRegion::Source::PERSISTENT_UPLOAD;
    static constexpr size_t buffer_alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT * 256;
    static constexpr DeviceMemoryClass memory_class = DeviceMemoryClass::HOST_RESIDENT_HOST_WRITE_ONLY_BUFFER;

    static void onSegmentRemove(HeapType *heap, size_t size) { heap->recordPersistentUploadBufferFreed(size); }
    static void onSegmentAdd(HeapType *heap, ResourceMemory memory, ID3D12Resource *buffer)
    {
      heap->updateMemoryRangeUse(memory, PersistentUploadBufferReference{buffer});
      heap->recordPersistentUploadBufferAllocated(memory.size());
    }
    static void onSegmentAddNoLock(HeapType *heap, ResourceMemory memory, ID3D12Resource *buffer)
    {
      heap->updateMemoryRangeUseNoLock(memory, PersistentUploadBufferReference{buffer});
      heap->recordPersistentUploadBufferAllocated(memory.size());
    }
  };

  ContainerMutexWrapper<PersistentMemoryState<PersistentUploadMemoryImplementation>, OSSpinlock> uploadMemory;

  void completeFrameExecution(const CompletedFrameExecutionInfo &info, PendingForCompletedFrameData &data)
  {
    {
      auto uploadMemoryAccess = uploadMemory.access();
      // TODO data can be sorted by buffer pointer to improve search for heap
      for (auto &&buf : data.uploadMemoryFrees)
      {
        uploadMemoryAccess->free(this, buf.buffer, buf.range);
        recordPersistentUploadMemoryFreed(buf.range.size());
      }
    }
    data.uploadMemoryFrees.clear();
    BaseType::completeFrameExecution(info, data);
  }

  void preRecovery()
  {
    uploadMemory.access()->shutdown(this);

    BaseType::preRecovery();
  }

  void shutdown()
  {
    uploadMemory.access()->shutdown(this);

    BaseType::shutdown();
  }

public:
  HostDeviceSharedMemoryRegionAllocationResult allocatePersistentUploadMemory(DXGIAdapter *adapter, Device &device, size_t size,
    size_t alignment);

  size_t getPersistentUploadMemorySize() { return uploadMemory.access()->currentMemorySize(); }

  void freeHostDeviceSharedMemoryRegionOnFrameCompletion(HostDeviceSharedMemoryRegion mem)
  {
    if (HostDeviceSharedMemoryRegion::Source::PERSISTENT_UPLOAD == mem.source)
    {
      accessRecodingPendingFrameCompletion<PendingForCompletedFrameData>([=](auto &data) { data.uploadMemoryFrees.push_back(mem); });
    }
    else
    {
      BaseType::freeHostDeviceSharedMemoryRegionOnFrameCompletion(mem);
    }
  }
};

class PersistentReadBackMemoryProvider : public PersistentUploadMemoryProvider
{
  using BaseType = PersistentUploadMemoryProvider;

protected:
  struct PendingForCompletedFrameData : BaseType::PendingForCompletedFrameData
  {
    dag::Vector<HostDeviceSharedMemoryRegion> readBackFrees;
  };

  struct PersistentReadBackMemoryImplementation
  {
    using HeapType = PersistentReadBackMemoryProvider;
    static constexpr HostDeviceSharedMemoryRegion::Source source_type = HostDeviceSharedMemoryRegion::Source::PERSISTENT_READ_BACK;
    static constexpr size_t buffer_alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT * 16;
    static constexpr DeviceMemoryClass memory_class = DeviceMemoryClass::READ_BACK_BUFFER;

    static void onSegmentRemove(HeapType *heap, size_t size) { heap->recordPersistentReadBackBufferFreed(size); }
    static void onSegmentAdd(HeapType *heap, ResourceMemory memory, ID3D12Resource *buffer)
    {
      heap->updateMemoryRangeUse(memory, PersistentReadBackBufferReference{buffer});
      heap->recordPersistentReadBackBufferAllocated(memory.size());
    }
    static void onSegmentAddNoLock(HeapType *heap, ResourceMemory memory, ID3D12Resource *buffer)
    {
      heap->updateMemoryRangeUseNoLock(memory, PersistentReadBackBufferReference{buffer});
      heap->recordPersistentReadBackBufferAllocated(memory.size());
    }
  };

  ContainerMutexWrapper<PersistentMemoryState<PersistentReadBackMemoryImplementation>, OSSpinlock> readBackMemory;

  void completeFrameExecution(const CompletedFrameExecutionInfo &info, PendingForCompletedFrameData &data)
  {
    {
      auto readBackMemoryAccess = readBackMemory.access();
      // TODO data can be sorted by buffer pointer to improve search for heap
      for (auto &&buf : data.readBackFrees)
      {
        readBackMemoryAccess->free(this, buf.buffer, buf.range);
        recordPersistentReadBackMemoryFreed(buf.range.size());
      }
    }
    data.readBackFrees.clear();

    BaseType::completeFrameExecution(info, data);
  }

  void preRecovery()
  {
    readBackMemory.access()->shutdown(this);

    BaseType::preRecovery();
  }

  void shutdown()
  {
    readBackMemory.access()->shutdown(this);

    BaseType::shutdown();
  }

public:
  HostDeviceSharedMemoryRegionAllocationResult allocatePersistentReadBack(DXGIAdapter *adapter, Device &device, size_t size,
    size_t alignment);

  size_t getPersistentReadBackMemorySize() { return readBackMemory.access()->currentMemorySize(); }

  void freeHostDeviceSharedMemoryRegionOnFrameCompletion(HostDeviceSharedMemoryRegion mem)
  {
    if (HostDeviceSharedMemoryRegion::Source::PERSISTENT_READ_BACK == mem.source)
    {
      accessRecodingPendingFrameCompletion<PendingForCompletedFrameData>([=](auto &data) { data.readBackFrees.push_back(mem); });
    }
    else
    {
      BaseType::freeHostDeviceSharedMemoryRegionOnFrameCompletion(mem);
    }
  }
};

class PersistentBidirectionalMemoryProvider : public PersistentReadBackMemoryProvider
{
  using BaseType = PersistentReadBackMemoryProvider;

protected:
  struct PendingForCompletedFrameData : BaseType::PendingForCompletedFrameData
  {
    dag::Vector<HostDeviceSharedMemoryRegion> bidirectionalFrees;
  };

  struct PersistentBidirectionalMemoryImplementation
  {
    using HeapType = PersistentBidirectionalMemoryProvider;
    static constexpr HostDeviceSharedMemoryRegion::Source source_type = HostDeviceSharedMemoryRegion::Source::PERSISTENT_BIDIRECTIONAL;
    static constexpr size_t buffer_alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT * 128;
    static constexpr DeviceMemoryClass memory_class = DeviceMemoryClass::BIDIRECTIONAL_BUFFER;

    static void onSegmentRemove(HeapType *heap, size_t size) { heap->recordPersistentBidirectionalBufferFreed(size); }
    static void onSegmentAdd(HeapType *heap, ResourceMemory memory, ID3D12Resource *buffer)
    {
      heap->updateMemoryRangeUse(memory, PersistentBidirectionalBufferReference{buffer});
      heap->recordPersistentBidirectionalBufferAllocated(memory.size());
    }
    static void onSegmentAddNoLock(HeapType *heap, ResourceMemory memory, ID3D12Resource *buffer)
    {
      heap->updateMemoryRangeUseNoLock(memory, PersistentBidirectionalBufferReference{buffer});
      heap->recordPersistentBidirectionalBufferAllocated(memory.size());
    }
  };

  ContainerMutexWrapper<PersistentMemoryState<PersistentBidirectionalMemoryImplementation>, OSSpinlock> bidirectionalMemory;

  void completeFrameExecution(const CompletedFrameExecutionInfo &info, PendingForCompletedFrameData &data)
  {
    {
      auto bidirectionalMemoryAccess = bidirectionalMemory.access();
      // TODO data can be sorted by buffer pointer to improve search for heap
      for (auto &&buf : data.bidirectionalFrees)
      {
        bidirectionalMemoryAccess->free(this, buf.buffer, buf.range);
        recordPersistentBidirectionalMemoryFreed(buf.range.size());
      }
    }
    data.bidirectionalFrees.clear();

    BaseType::completeFrameExecution(info, data);
  }

  void preRecovery()
  {
    bidirectionalMemory.access()->shutdown(this);

    BaseType::preRecovery();
  }

  void shutdown()
  {
    bidirectionalMemory.access()->shutdown(this);

    BaseType::shutdown();
  }

public:
  HostDeviceSharedMemoryRegionAllocationResult allocatePersistentBidirectional(DXGIAdapter *adapter, Device &device, size_t size,
    size_t alignment);

  size_t getPersistentBidirectionalMemorySize() { return bidirectionalMemory.access()->currentMemorySize(); }

  void freeHostDeviceSharedMemoryRegionOnFrameCompletion(HostDeviceSharedMemoryRegion mem)
  {
    if (HostDeviceSharedMemoryRegion::Source::PERSISTENT_BIDIRECTIONAL == mem.source)
    {
      accessRecodingPendingFrameCompletion<PendingForCompletedFrameData>([=](auto &data) { data.bidirectionalFrees.push_back(mem); });
    }
    else
    {
      BaseType::freeHostDeviceSharedMemoryRegionOnFrameCompletion(mem);
    }
  }
};
} // namespace drv3d_dx12::resource_manager
