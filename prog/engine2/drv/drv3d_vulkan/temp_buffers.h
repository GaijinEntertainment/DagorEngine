// temp buffers for various frame-once data transfers
#pragma once

#include "buffer_resource.h"

namespace drv3d_vulkan
{

struct TempBufferInfo
{
  Buffer *buffer = nullptr;
  VkDeviceSize fill = 0;
};

class TempBufferHolder;

class TempBufferManager
{
  friend class TempBufferHolder;

  WinCritSec writeLock;
  std::atomic<int> buffersInUse;
  eastl::vector<TempBufferInfo> buffers;
  VkDeviceSize currentAllocSize = 0;
  VkDeviceSize maxAllocSize = 0;
  uint32_t pow2Alignment = 0;
  uint8_t managerIndex = 0;

  BufferSubAllocation allocate(Device &device, uint32_t unalignedSize);

public:
  TempBufferManager() { buffersInUse = 0; }
  ~TempBufferManager() = default;
  TempBufferManager(const TempBufferManager &) = delete;
  TempBufferManager &operator=(const TempBufferManager &) = delete;
  TempBufferManager(TempBufferManager &&) = delete;
  TempBufferManager &operator=(TempBufferManager &&) = delete;
  void setConfig(VkDeviceSize size, uint32_t alignment, uint8_t index);

  template <typename T>
  void onFrameEnd(T clb)
  {
    WinAutoLock lock(writeLock);

    if (maxAllocSize < currentAllocSize)
      maxAllocSize = currentAllocSize;
    else if (currentAllocSize < maxAllocSize / 2)
      maxAllocSize = nextPowerOfTwo(currentAllocSize + 1);

    currentAllocSize = 0;

    int pendingBuffers = buffersInUse;
    if (pendingBuffers > 0)
    {
      debug("vulkan: %u temp buffers holded over frame end, skipping cleanup!", pendingBuffers);
      return;
    }

    for (auto &&buffer : buffers)
      clb(buffer.buffer);
    buffers.clear();
  }

  enum
  {
    TYPE_UNIFORM = 0,
    TYPE_USER_VERTEX = 1,
    TYPE_USER_INDEX = 2,
    TYPE_UPDATE = 3,
    TYPE_COUNT = 4,
  };
};

class TempBufferHolder
{
  BufferSubAllocation subAlloc;
  TempBufferManager &manager;

#if DAGOR_DBGLEVEL > 0
  bool writesFlushed = false;
#endif

public:
  TempBufferHolder() = delete;
  TempBufferHolder(const TempBufferHolder &from) : manager(from.manager), subAlloc(from.subAlloc) { ++manager.buffersInUse; }

  TempBufferHolder(Device &device, TempBufferManager &_manager, uint32_t size, const void *src) : manager(_manager)
  {
    ++manager.buffersInUse;
    subAlloc = manager.allocate(device, size);
    memcpy(getPtr(), src, size);
    flushWrite();
  }

  TempBufferHolder(Device &device, TempBufferManager &_manager, uint32_t size) : manager(_manager)
  {
    ++manager.buffersInUse;
    subAlloc = manager.allocate(device, size);
  }

  ~TempBufferHolder()
  {
#if DAGOR_DBGLEVEL > 0
    G_ASSERTF(writesFlushed, "vulkan: temp buffer write was not flushed");
#endif
    --manager.buffersInUse;
  }

  void flushWrite()
  {
    subAlloc.buffer->markNonCoherentRange(dataOffset(), subAlloc.size, true);
#if DAGOR_DBGLEVEL > 0
    writesFlushed = true;
#endif
  }

  const BufferSubAllocation &get() { return subAlloc; }

  uint32_t dataOffset() { return subAlloc.buffer->dataOffset(subAlloc.offset); }

  void *getPtr() { return subAlloc.buffer->dataPointer(subAlloc.offset); }
};

} // namespace drv3d_vulkan
