#pragma once

#include <supp/dag_comPtr.h>
#include <EASTL/vector.h>

#include "driver.h"
#include "constants.h"
#include "resource_memory.h"
#include "host_device_shared_memory_region.h"
#include "d3d12_error_handling.h"
#include "container_mutex_wrapper.h"

#include "resource_manager/basic_buffer.h"
#include "resource_manager/esram_components.h"


namespace drv3d_dx12
{
namespace resource_manager
{
class RingMemoryBase : public ESRamPageMappingProvider
{
  using BaseType = ESRamPageMappingProvider;

protected:
  struct CompletedFrameExecutionInfo : BaseType::CompletedFrameExecutionInfo
  {
    uint32_t historyIndex;
  };

  template <typename T>
  struct RingMemoryState : T
  {
    using HeapType = typename T::HeapType;
    struct RingSegment : BasicBuffer
    {
      // Offset where we allocate next chunk from
      uint32_t allocationOffset = 0;
      // Total allocated, bufferMemory.size() - allocationSize yields free memory
      uint32_t allocationSize = 0;
      // Allocation size since last snapshot
      uint32_t currentAllocation = 0;
      // Copy of currentAllocation before it was set to 0, used for telemetry
      uint32_t lastAllocation = 0;
      // number of times we have seen it unused
      uint32_t timeSinceUnused = 0;
      // coincides with latched frame index
      uint32_t allocationHistory[FRAME_FRAME_BACKLOG_LENGTH] = {};

      HostDeviceSharedMemoryRegion allocate(uint32_t size, uint32_t alignment)
      {
        HostDeviceSharedMemoryRegion result;

        auto allocationBegin = align_value(allocationOffset, alignment);
        auto allocationEnd = allocationBegin + size;

        auto freeMemory = bufferMemory.size() - allocationSize;
        auto freeBegin = allocationOffset;
        auto freeEnd = freeBegin + freeMemory;

        // wants to allocate more than we have free
        if (allocationEnd > freeEnd)
        {
          return result;
        }

        uint32_t extra = 0;
        if (allocationEnd > bufferMemory.size())
        {
          extra = bufferMemory.size() - freeBegin;
          // we have to wrap
          allocationBegin = 0;
          allocationEnd = size;
          freeBegin = 0;
          freeEnd -= bufferMemory.size();

          // check free space again after wrap
          if (allocationEnd > freeEnd)
          {
            return result;
          }
        }

        result.buffer = buffer.Get();
#if !_TARGET_XBOX
        result.gpuPointer = getGPUPointer() + allocationBegin;
#endif
        result.pointer = getCPUPointer() + allocationBegin;
        result.range = ValueRange<uint64_t>{allocationBegin, allocationEnd};

        allocationOffset = allocationEnd;
        allocationSize += extra + result.range.size();
        currentAllocation += extra + result.range.size();

        result.source = HostDeviceSharedMemoryRegion::Source::PUSH_RING;
        return result;
      }
    };

    // we always have at least one segment with min_ring_size
    // should we ever run out of segment space we allocate a
    // new segment. As soon as we no longer need it, we drop
    // the segment.
    eastl::vector<RingSegment> ringSegments;

    void finishRecording(uint32_t history_index)
    {
      G_ASSERT(history_index < FRAME_FRAME_BACKLOG_LENGTH);
      for (auto &segment : ringSegments)
      {
        segment.timeSinceUnused = segment.allocationSize > 0 ? 0 : (segment.timeSinceUnused + 1);
        segment.allocationHistory[history_index] = segment.currentAllocation;
        segment.lastAllocation = segment.currentAllocation;
        segment.currentAllocation = 0;
      }
    }

    void finishExecution(HeapType *heap, uint32_t history_index)
    {
      G_ASSERT(history_index < FRAME_FRAME_BACKLOG_LENGTH);

      for (auto &segment : ringSegments)
      {
        segment.allocationSize -= segment.allocationHistory[history_index];
        segment.allocationHistory[history_index] = 0;
      }

      while ((ringSegments.size() >= T::min_active_segments) && T::shouldTrim(heap))
      {
        // remove no longer needed segments one by one
        auto toBeRemoved = eastl::find_if(begin(ringSegments) + T::min_active_segments, end(ringSegments),
          [=](auto &segment) { return 0 == segment.allocationSize; });
        if (end(ringSegments) != toBeRemoved)
        {
          T::onSegmentRemove(heap, toBeRemoved->bufferMemory.size());
          toBeRemoved->reset(heap);
          ringSegments.erase(toBeRemoved);
        }
        else
        {
          break;
        }
      }
    }

    bool addSegment(HeapType *heap, DXGIAdapter *adapter, ID3D12Device *device, uint32_t size)
    {
      D3D12_RESOURCE_DESC desc;
      desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
      desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
      desc.Width = align_value(size, T::min_ring_size);
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

      auto initialState =
        heap->propertiesToInitialState(D3D12_RESOURCE_DIMENSION_BUFFER, D3D12_RESOURCE_FLAG_NONE, DeviceMemoryClass::PUSH_RING_BUFFER);

      auto memoryProperties = heap->getProperties(D3D12_RESOURCE_FLAG_NONE, DeviceMemoryClass::PUSH_RING_BUFFER, allocInfo.Alignment);

      auto allocation = heap->allocate(adapter, device, memoryProperties, allocInfo, {});
      if (!allocation)
      {
        return false;
      }

      RingSegment newSegment;
      auto errorCode = newSegment.create(device, desc, allocation, initialState, true);
      if (DX12_CHECK_FAIL(errorCode))
      {
        heap->free(allocation);
        return false;
      }
      auto &ringSegment = ringSegments.emplace_back(eastl::move(newSegment));

      T::onSegmentAdd(heap, ringSegment.bufferMemory, ringSegment.buffer.Get());

      return true;
    }

    // rotates segments by one to the left, eg new back becomes previous front
    void rotateSegments()
    {
      if (2 > ringSegments.size())
      {
        return;
      }
      if (2 == ringSegments.size())
      {
        eastl::swap(ringSegments.front(), ringSegments.back());
      }
      else
      {
        eastl::rotate(begin(ringSegments), begin(ringSegments) + 1, end(ringSegments));
      }
    }

    HostDeviceSharedMemoryRegion allocate(HeapType *heap, DXGIAdapter *adapter, ID3D12Device *device, uint32_t size,
      uint32_t alignment)
    {
      HostDeviceSharedMemoryRegion result;
      for (auto &segment : ringSegments)
      {
        result = segment.allocate(size, alignment);
        if (result)
        {
          return result;
        }
      }

      // no segment can supply the memory, need to allocate a new segment
      if (!addSegment(heap, adapter, device, size))
      {
        return result;
      }

      auto &newSegment = ringSegments.back();
      result = newSegment.allocate(size, alignment);
      G_ASSERT(static_cast<bool>(result));
      return result;
    }

    size_t currentMemorySize() const
    {
      return eastl::accumulate(begin(ringSegments), end(ringSegments), 0,
        [](size_t value, auto &segment) //
        { return value + segment.bufferMemory.size(); });
    }

    void shutdown(HeapType *heap)
    {
      for (auto &segment : ringSegments)
      {
        segment.reset(heap);
      }
      ringSegments.clear();
    }
  };
};

class FramePushRingMemoryProvider : public RingMemoryBase
{
  using BaseType = RingMemoryBase;

protected:
  struct FramePushRingMemoryImplementation
  {
    // using a ring of 16 MiBytes avoids basically any resizes
    constexpr static uint32_t min_ring_size = 16 * 1024 * 1024;
    // always keep one segment around
    constexpr static uint32_t min_active_segments = 1;

    using HeapType = FramePushRingMemoryProvider;

    static void onSegmentAdd(HeapType *heap, ResourceMemory memory, ID3D12Resource *buffer)
    {
      heap->updateMemoryRangeUse(memory, PushRingBufferReference{buffer});
      heap->recordConstantRingAllocated(memory.size());
    }
    static void onSegmentRemove(HeapType *heap, size_t size) { heap->recordConstantRingFreed(size); }
    static bool shouldTrim(HeapType *heap) { return heap->shouldTrimFramePushRingBuffer(); }
  };

  ContainerMutexWrapper<RingMemoryState<FramePushRingMemoryImplementation>, OSSpinlock> pushRing;

  void setup(const SetupInfo &info)
  {
    BaseType::setup(info);
    // The very first allocation is the first const ring buffer. This makes handling in case of memory shortage
    // simpler, as the very first segment, which we always *need*, is in the very first heap and so has to never
    // be moved. We only truncate additional ring buffers when possible.
    pushRing.access()->addSegment(this, info.getAdapter(), info.device, FramePushRingMemoryImplementation::min_ring_size);
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
  HostDeviceSharedMemoryRegion allocatePushMemory(DXGIAdapter *adapter, ID3D12Device *device, uint32_t size, uint32_t alignment)
  {
    auto result = pushRing.access()->allocate(this, adapter, device, size, alignment);
    if (checkForOOM(static_cast<bool>(result), "DX12: OOM during %s", "allocatePushMemory"))
    {
      recordConstantRingUsed(size);
    }
    return result;
  }

  void completeFrameRecording(const CompletedFrameRecordingInfo &info)
  {
    BaseType::completeFrameRecording(info);
    pushRing.access()->finishRecording(info.historyIndex);
  }

  void completeFrameExecution(const CompletedFrameExecutionInfo &info, PendingForCompletedFrameData &data)
  {
    BaseType::completeFrameExecution(info, data);
    pushRing.access()->finishExecution(this, info.historyIndex);
  }

  size_t getFramePushRingMemorySize() { return pushRing.access()->currentMemorySize(); }

  void freeHostDeviceSharedMemoryRegionOnFrameCompletion(HostDeviceSharedMemoryRegion mem)
  {
    G_ASSERT(HostDeviceSharedMemoryRegion::Source::PUSH_RING == mem.source);
    G_UNUSED(mem);
  }
};

class UploadRingMemoryProvider : public FramePushRingMemoryProvider
{
  using BaseType = FramePushRingMemoryProvider;

protected:
  struct UploadRingMemoryImplementation
  {
    // using a ring of 16 MiBytes avoids basically any resizes
    constexpr static uint32_t min_ring_size = 16 * 1024 * 1024;
    // always keep one segment around
    constexpr static uint32_t min_active_segments = 0;

    using HeapType = UploadRingMemoryProvider;

    static void onSegmentAdd(HeapType *heap, ResourceMemory memory, ID3D12Resource *buffer)
    {
      heap->updateMemoryRangeUse(memory, UploadRingBufferReference{buffer});
      heap->recordUploadRingAllocated(memory.size());
    }
    static void onSegmentRemove(HeapType *heap, size_t size) { heap->recordUploadRingFreed(size); }
    static bool shouldTrim(HeapType *heap) { return heap->shouldTrimUploadRingBuffer(); }
  };
  ContainerMutexWrapper<RingMemoryState<UploadRingMemoryImplementation>, OSSpinlock> uploadRing;

  void preRecovery()
  {
    uploadRing.access()->shutdown(this);

    BaseType::preRecovery();
  }

  void shutdown()
  {
    uploadRing.access()->shutdown(this);

    BaseType::shutdown();
  }

public:
  HostDeviceSharedMemoryRegion allocateUploadRingMemory(DXGIAdapter *adapter, ID3D12Device *device, uint32_t size, uint32_t alignment)
  {
    auto result = uploadRing.access()->allocate(this, adapter, device, size, alignment);
    if (checkForOOM(static_cast<bool>(result), "DX12: OOM during %s", "allocateUploadRingMemory"))
    {
      recordUploadRingUsed(size);
    }
    return result;
  }

  void completeFrameRecording(const CompletedFrameRecordingInfo &info)
  {
    BaseType::completeFrameRecording(info);
    uploadRing.access()->finishRecording(info.historyIndex);
  }

  void completeFrameExecution(const CompletedFrameExecutionInfo &info, PendingForCompletedFrameData &data)
  {
    BaseType::completeFrameExecution(info, data);
    uploadRing.access()->finishExecution(this, info.historyIndex);
  }

  size_t getUploadRingMemorySize() { return uploadRing.access()->currentMemorySize(); }
};

class TemporaryMemoryBase : public UploadRingMemoryProvider
{
  using BaseType = UploadRingMemoryProvider;

protected:
  template <typename T>
  struct TemporaryMemoryState : T
  {
    using HeapType = typename T::HeapType;
    static constexpr DeviceMemoryClass memory_class = T::memory_class;

    struct Buffer : BasicBuffer
    {
      uint32_t fillSize = 0;
      uint32_t allocations = 0;
    };
    Buffer currentBuffer;
    Buffer standbyBuffer;
    size_t nextBufferSize = T::min_buffer_size;
    size_t currentBufferUse = 0;
    size_t nextBufferSizeShrinkThreshold = T::min_buffer_size;
    uint32_t timesSinceUse = 0;

    eastl::vector<Buffer> buffers;

    HostDeviceSharedMemoryRegion allocate(HeapType *heap, DXGIAdapter *adapter, ID3D12Device *device, size_t size, size_t alignment)
    {
      HostDeviceSharedMemoryRegion result;
      size_t offset = (currentBuffer.fillSize + alignment - 1) & ~(alignment - 1);
      if (!currentBuffer || (offset + size > currentBuffer.bufferMemory.size()))
      {
        if (currentBuffer)
        {
          if (currentBuffer.allocations)
          {
            buffers.push_back(eastl::move(currentBuffer));
            currentBuffer.bufferMemory = {};
          }
          else
          {
            T::onSegmentRemove(heap, currentBuffer.bufferMemory.size());
            currentBuffer.reset(heap);
          }
        }

        if (standbyBuffer && standbyBuffer.bufferMemory.size() >= size)
        {
          currentBuffer = eastl::move(standbyBuffer);
          standbyBuffer.bufferMemory = {};
        }
        else
        {
          if (standbyBuffer)
          {
            T::onSegmentRemove(heap, standbyBuffer.bufferMemory.size());
            standbyBuffer.reset(heap);
          }
          currentBuffer.bufferMemory = {};

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

          auto allocation = heap->allocate(adapter, device, memoryProperties, allocInfo, {});

          if (!allocation)
          {
            return result;
          }

          auto errorCode = currentBuffer.create(device, desc, allocation, initialState, true);
          if (DX12_CHECK_FAIL(errorCode))
          {
            heap->free(allocation);
            return result;
          }

          T::onSegmentAdd(heap, currentBuffer.buffer.Get(), currentBuffer.bufferMemory);
        }
        currentBuffer.fillSize = 0;
        currentBuffer.allocations = 0;
        offset = 0;
      }
      result.buffer = currentBuffer.buffer.Get();
      result.pointer = currentBuffer.getCPUPointer() + offset;
#if !_TARGET_XBOX
      result.gpuPointer = currentBuffer.getGPUPointer() + offset;
#endif
      auto oldFill = currentBuffer.fillSize;
      currentBuffer.fillSize = offset + size;
      currentBuffer.allocations++;
      currentBufferUse += currentBuffer.fillSize - oldFill;
      G_ASSERT(result.pointer);
      G_ASSERT(result.buffer);
      result.range = make_value_range<uint64_t>(offset, size);
      return result;
    }

    void trim(HeapType *heap)
    {
      if (standbyBuffer)
      {
        T::onSegmentRemove(heap, standbyBuffer.bufferMemory.size());
      }
      standbyBuffer.reset(heap);
    }

    bool shouldSwapStandbyBuffer(Buffer &other)
    {
      if (!standbyBuffer)
      {
        return true;
      }
      if (other.bufferMemory.size() < nextBufferSize)
      {
        return false;
      }
      if (other.bufferMemory.size() > nextBufferSize * 2)
      {
        return false;
      }
      return standbyBuffer.bufferMemory.size() < other.bufferMemory.size();
    }

    void free(HeapType *heap, ID3D12Resource *ref)
    {
      if (ref == currentBuffer.buffer.Get())
      {
        if (!--currentBuffer.allocations)
        {
          currentBuffer.fillSize = 0;
        }
        return;
      }

      auto iter = eastl::find_if(begin(buffers), end(buffers),
        [ref](const auto &buf) //
        { return ref == buf.buffer.Get(); });
      G_ASSERT(iter != end(buffers));
      if (iter != end(buffers))
      {
        if (0 == --iter->allocations)
        {
          if (shouldSwapStandbyBuffer(*iter))
          {
            if (standbyBuffer)
            {
              T::onSegmentRemove(heap, standbyBuffer.bufferMemory.size());
            }
            standbyBuffer.reset(heap);
            standbyBuffer = eastl::move(*iter);
            standbyBuffer.fillSize = 0;
          }
          else
          {
            T::onSegmentRemove(heap, iter->bufferMemory.size());
            iter->reset(heap);
          }
          *iter = eastl::move(buffers.back());
          buffers.pop_back();
        }
      }
    }

    void free(HeapType *heap, const eastl::vector<ID3D12Resource *> &list)
    {
      for (auto ref : list)
      {
        free(heap, ref);
      }
    }

    void completeFrameRecording(HeapType *heap)
    {
      // keep size aligned to min alignment
      nextBufferSize = max(align_value<size_t>(currentBufferUse, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT),
        max(T::min_buffer_size, nextBufferSize));

      if (nextBufferSize > nextBufferSizeShrinkThreshold && currentBufferUse < nextBufferSize / 2)
      {
        nextBufferSize /= 2;
      }

      currentBufferUse = 0;
      if (!currentBuffer.allocations)
      {
        if (++timesSinceUse > T::drop_timeout)
        {
          if (standbyBuffer)
          {
            T::onSegmentRemove(heap, standbyBuffer.bufferMemory.size());
            standbyBuffer.reset(heap);
          }
          else if (currentBuffer)
          {
            T::onSegmentRemove(heap, currentBuffer.bufferMemory.size());
            currentBuffer.reset(heap);
          }
          timesSinceUse = 0;
        }
      }
    }

    size_t currentMemorySize() const
    {
      return eastl::accumulate(begin(buffers), end(buffers), currentBuffer.bufferMemory.size() + standbyBuffer.bufferMemory.size(),
        [](size_t value, auto &buffer) { return value + buffer.bufferMemory.size(); });
    }

    void shutdown(HeapType *heap)
    {
      for (auto &buf : buffers)
      {
        buf.reset(heap);
      }
      buffers.clear();
      standbyBuffer.reset(heap);
      currentBuffer.reset(heap);
      currentBuffer.fillSize = 0;
      currentBuffer.allocations = 0;

      currentBufferUse = 0;
    }

    // This checks if any buffer has still some allocations outstanding to be freed
    bool isAllFree() const
    {
      // This intentionally has no short cuts to report all buffers that are still used
      bool anySeen = false;
      if (currentBuffer.allocations > 0)
      {
        logdbg("DX12: Buffer %p has still %u allocations...", currentBuffer.buffer.Get(), currentBuffer.allocations);
        anySeen = true;
      }
      // Scan all pending buffers and see if any of them has still some allocations pending.
      for (auto &buffer : buffers)
      {
        if (buffer.allocations > 0)
        {
          logdbg("DX12: Buffer %p has still %u allocations...", buffer.buffer.Get(), buffer.allocations);
          anySeen = true;
        }
      }
      return !anySeen;
    }
  };
};

class TemporaryUploadMemoryProvider : public TemporaryMemoryBase
{
  using BaseType = TemporaryMemoryBase;

protected:
  struct PendingForCompletedFrameData : BaseType::PendingForCompletedFrameData
  {
    eastl::vector<ID3D12Resource *> uploadBufferRefs;
    uint32_t uploadBufferUsage = 0;
    uint32_t tempUsage = 0;
  };

  struct TemporaryUploadMemoryImeplementation
  {
    // never have less than one mibyte
    static constexpr size_t min_buffer_size = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT * 16;
    static constexpr uint32_t drop_timeout = FRAME_FRAME_BACKLOG_LENGTH * 256;
    static constexpr DeviceMemoryClass memory_class = DeviceMemoryClass::TEMPORARY_UPLOAD_BUFFER;

    using HeapType = TemporaryUploadMemoryProvider;

    void onSegmentAdd(HeapType *heap, ID3D12Resource *buffer, ResourceMemory mem)
    {
      heap->updateMemoryRangeUse(mem, TempUploadBufferReference{buffer});
      heap->recordTempBufferAllocated(mem.size());
    }

    void onSegmentRemove(HeapType *heap, size_t size) { heap->recordTempBufferFreed(size); }
  };

  struct TemporaryUploadMemoryInfo : TemporaryMemoryState<TemporaryUploadMemoryImeplementation>
  {
    using BaseType = TemporaryMemoryState<TemporaryUploadMemoryImeplementation>;

    uint32_t uploadBufferUsage = 0;
    // TODO make configurable
    uint32_t uploadBufferUsageLimit = 256 * 1024 * 1024;
    uint32_t tempUsage = 0;
    // TODO make configurable
    uint32_t tempUsageLimit = 256 << 20;

    void shutdown(TemporaryUploadMemoryProvider *heap)
    {
      uploadBufferUsage = 0;
      tempUsage = 0;

      BaseType::shutdown(heap);
    }
  };

  ContainerMutexWrapper<TemporaryUploadMemoryInfo, OSSpinlock> tempBuffer;

  void completeFrameExecution(const CompletedFrameExecutionInfo &info, PendingForCompletedFrameData &data)
  {
    // sort to make free a bit faster by doing one buffer at a time
    eastl::sort(begin(data.uploadBufferRefs), end(data.uploadBufferRefs));
    {
      auto tempBufferAccess = tempBuffer.access();
      tempBufferAccess->free(this, data.uploadBufferRefs);
      tempBufferAccess->uploadBufferUsage -= data.uploadBufferUsage;
      tempBufferAccess->tempUsage -= data.tempUsage;
    }
    data.uploadBufferRefs.clear();
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
        if (data.uploadBufferRefs.empty())
        {
          return;
        }
        eastl::sort(begin(data.uploadBufferRefs), end(data.uploadBufferRefs));
        {
          tempBufferAccess->free(this, data.uploadBufferRefs);
          tempBufferAccess->uploadBufferUsage -= data.uploadBufferUsage;
          tempBufferAccess->tempUsage -= data.tempUsage;
        }
        data.uploadBufferRefs.clear();
        data.uploadBufferUsage = 0;
        data.tempUsage = 0;
      });
      hasComplted = tempBufferAccess->isAllFree();
    }
    return baseCompleted && hasComplted;
  }

public:
  HostDeviceSharedMemoryRegion allocateTempUpload(DXGIAdapter *adapter, ID3D12Device *device, size_t size, size_t alignment,
    bool &should_flush)
  {
    should_flush = false;
    HostDeviceSharedMemoryRegion result;
    auto oomCheckOnExit =
      checkForOOMOnExit([&result]() { return static_cast<bool>(result); }, "DX12: OOM during %s", "allocateTempUpload");
    {
      auto tempBufferAccess = tempBuffer.access();
      result = tempBufferAccess->allocate(this, adapter, device, size, alignment);
      if (!result)
      {
        ByteUnits reqSize{size};
        logdbg("TemporaryUploadMemoryProvider::allocateTempUpload: Allocation failed, let's trim "
               "heaps and try again. Size: %.2f %s, Error code: 0x%08X",
          reqSize.units(), reqSize.name(), GetLastError());
        tempBufferAccess->trim(this);
        result = tempBufferAccess->allocate(this, adapter, device, size, alignment);
      }
      tempBufferAccess->tempUsage += result.range.size();
      should_flush = tempBufferAccess->tempUsage > tempBufferAccess->tempUsageLimit;

      if (should_flush)
      {
        ByteUnits currentUsage{tempBufferAccess->tempUsage};
        ByteUnits usageLimit{tempBufferAccess->tempUsageLimit};
        ByteUnits reqSize{size};
        logdbg("DX12: Out of temp upload pool budget, usage %.2f %s of %.2f %s, while allocating %.2f %s, flushing.",
          currentUsage.units(), currentUsage.name(), usageLimit.units(), usageLimit.name(), reqSize.units(), reqSize.name());
      }
    }
    recordTempBufferUsed(result.range.size());
    return result;
  }

  HostDeviceSharedMemoryRegion allocateTempUploadForUploadBuffer(DXGIAdapter *adapter, ID3D12Device *device, size_t size,
    size_t alignment)
  {
    HostDeviceSharedMemoryRegion result;
    bool wasOutOfBudget = false;
    auto oomCheckOnExit = checkForOOMOnExit([&result, &wasOutOfBudget]() { return static_cast<bool>(result) || wasOutOfBudget; },
      "DX12: OOM during %s", "allocateTempUploadForUploadBuffer");
    {
      auto tempBufferAccess = tempBuffer.access();
      if (tempBufferAccess->uploadBufferUsage > tempBufferAccess->uploadBufferUsageLimit)
      {
        ByteUnits currentUsage{tempBufferAccess->uploadBufferUsage};
        ByteUnits usageLimit{tempBufferAccess->uploadBufferUsageLimit};
        ByteUnits reqSize{size};
        // Out of budget, return empty region
        logdbg("DX12: Out of upload buffer pool, usage %.2f %s of %.2f %s, while trying to allocate "
               "%.2f %s",
          currentUsage.units(), currentUsage.name(), usageLimit.units(), usageLimit.name(), reqSize.units(), reqSize.name());
        wasOutOfBudget = true;

        return result;
      }
      result = tempBufferAccess->allocate(this, adapter, device, size, alignment);
      if (!result)
      {
        // if allocate fails for some reason we just return the empty region
        return result;
      }
      tempBufferAccess->uploadBufferUsage += result.range.size();
    }
    recordTempBufferUsed(result.range.size());
    return result;
  }

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
      accessRecodingPendingFrameCompletion<PendingForCompletedFrameData>([buffer = mem.buffer, size = mem.range.size()](auto &data) {
        data.uploadBufferRefs.push_back(buffer);
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
    accessRecodingPendingFrameCompletion<PendingForCompletedFrameData>([buffer = mem.buffer, size = mem.range.size()](auto &data) {
      data.uploadBufferRefs.push_back(buffer);
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
      eastl::vector<ValueRange<uint64_t>> freeRanges;

      HostDeviceSharedMemoryRegion allocate(size_t size, size_t alignment)
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
#if _TARGET_XBOX
          result.gpuPointer = getGPUPointer() + range.front();
#endif
          result.pointer = getCPUPointer() + range.front();
          result.range = range;
          result.source = source_type;
        }
        return result;
      }
      bool free(ValueRange<uint64_t> range)
      {
        free_list_insert_and_coalesce(freeRanges, range);
        return freeRanges.size() == 1 && freeRanges.front().size() == bufferMemory.size();
      }
    };

    eastl::vector<MemoryBufferHeap> buffers;

    void free(HeapType *heap, ID3D12Resource *res, ValueRange<uint64_t> range)
    {
      auto heapRef = eastl::find_if(begin(buffers), end(buffers),
        [res](const auto &heap) //
        { return res == heap.buffer.Get(); });
      G_ASSERT(heapRef != end(buffers));
      if (heapRef != end(buffers))
      {
        if (heapRef->free(range))
        {
          T::onSegmentRemove(heap, heapRef->bufferMemory.size());
          heapRef->reset(heap);

          buffers.erase(heapRef);
        }
      }
    }

    HostDeviceSharedMemoryRegion allocate(HeapType *heap, DXGIAdapter *adapter, ID3D12Device *device, size_t size, size_t alignment)
    {
      HostDeviceSharedMemoryRegion result;
      for (auto &buffer : buffers)
      {
        auto result = buffer.allocate(size, alignment);
        if (result)
        {
          return result;
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

      auto allocation = heap->allocate(adapter, device, memoryProperties, allocInfo, {});

      if (!allocation)
      {
        return result;
      }

      auto errorCode = newHeap.create(device, desc, allocation, initialState, true);
      if (DX12_CHECK_FAIL(errorCode))
      {
        heap->free(allocation);
        return result;
      }

      T::onSegmentAdd(heap, newHeap.bufferMemory, newHeap.buffer.Get());

      newHeap.freeRanges.push_back(make_value_range(0ull, desc.Width));
      result = newHeap.allocate(size, alignment);
      G_ASSERT(static_cast<bool>(result));
      buffers.push_back(eastl::move(newHeap));
      return result;
    }

    void shutdown(HeapType *heap)
    {
      for (auto &buffer : buffers)
      {
        buffer.reset(heap);
      }
      buffers.clear();
    }

    size_t currentMemorySize() const
    {
      return eastl::accumulate(begin(buffers), end(buffers), 0,
        [](size_t value, auto &buffer) //
        { return value + buffer.bufferMemory.size(); });
    }
  };
};

class PersistentUploadMemoryProvider : public PersistentMemoryBase
{
  using BaseType = PersistentMemoryBase;

protected:
  struct PendingForCompletedFrameData : BaseType::PendingForCompletedFrameData
  {
    eastl::vector<HostDeviceSharedMemoryRegion> uploadMemoryFrees;
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
  HostDeviceSharedMemoryRegion allocatePersistentUploadMemory(DXGIAdapter *adapter, ID3D12Device *device, size_t size,
    size_t alignment)
  {
    auto result = uploadMemory.access()->allocate(this, adapter, device, size, alignment);
    if (checkForOOM(static_cast<bool>(result), "DX12: OOM during %s", "allocatePersistentUploadMemory"))
    {
      recordPersistentUploadMemoryAllocated(size);
    }
    return result;
  }

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
    eastl::vector<HostDeviceSharedMemoryRegion> readBackFrees;
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
  HostDeviceSharedMemoryRegion allocatePersistentReadBack(DXGIAdapter *adapter, ID3D12Device *device, size_t size, size_t alignment)
  {
    auto result = readBackMemory.access()->allocate(this, adapter, device, size, alignment);
    if (checkForOOM(static_cast<bool>(result), "DX12: OOM during %s", "allocatePersistentReadBack"))
    {
      recordPersistentReadBackMemoryAllocated(size);
    }
    return result;
  }

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
    eastl::vector<HostDeviceSharedMemoryRegion> bidirectionalFrees;
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
  HostDeviceSharedMemoryRegion allocatePersistentBidirectional(DXGIAdapter *adapter, ID3D12Device *device, size_t size,
    size_t alignment)
  {
    auto result = bidirectionalMemory.access()->allocate(this, adapter, device, size, alignment);
    if (checkForOOM(static_cast<bool>(result), "DX12: OOM during %s", "allocatePersistentBidirectional"))
    {
      recordPersistentBidirectionalMemoryAllocated(size);
    }
    return result;
  }

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
} // namespace resource_manager
} // namespace drv3d_dx12
