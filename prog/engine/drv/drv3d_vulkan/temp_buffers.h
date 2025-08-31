// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// temp buffers for various frame-once data transfers

#include <generic/dag_relocatableFixedVector.h>
#include <osApiWrappers/dag_critSec.h>
#include <debug/dag_assert.h>
#include <atomic>

#include "buffer_resource.h"
#include "buffer_ref.h"
#include <generic/dag_objectPool.h>

namespace drv3d_vulkan
{

class TempBufferManager;

class TempBufferHolder
{
  BufferRef ref;

#if DAGOR_DBGLEVEL > 0
  bool writesFlushed = false;
#endif

public:
  TempBufferHolder() = delete;
  TempBufferHolder(const TempBufferHolder &from);
  TempBufferHolder(uint32_t size, const void *src);
  TempBufferHolder(uint32_t size);
  TempBufferHolder(const BufferRef &ext_ref);
  ~TempBufferHolder();

  void flushWrite()
  {
    ref.markNonCoherentRange(0, ref.visibleDataSize, true);
#if DAGOR_DBGLEVEL > 0
    writesFlushed = true;
#endif
  }

  BufferRef getRef() { return ref; }
  uint32_t bufOffset() { return ref.bufOffset(0); }
  void *getPtr() { return ref.ptrOffset(0); }
};

struct TempBufferInfo
{
  Buffer *buffer = nullptr;
  VkDeviceSize fill = 0;
};

class TempBufferManager
{
  friend class TempBufferHolder;

  WinCritSec writeLock;
  std::atomic<int> buffersInUse;
  dag::Vector<TempBufferInfo> buffers;
  VkDeviceSize currentAllocSize = 0;
  VkDeviceSize maxAllocSize = 0;
  VkDeviceSize lastAllocSize = 0;
  uint32_t pow2Alignment = 0;
  uint8_t managerIndex = 0;

  BufferRef allocate(uint32_t unalignedSize);

  WinCritSec poolGuard;
  ObjectPool<TempBufferHolder> pool;

public:
  TempBufferManager() { buffersInUse = 0; }
  ~TempBufferManager() = default;
  TempBufferManager(const TempBufferManager &) = delete;
  TempBufferManager &operator=(const TempBufferManager &) = delete;
  TempBufferManager(TempBufferManager &&) = delete;
  TempBufferManager &operator=(TempBufferManager &&) = delete;
  void setConfig(VkDeviceSize size, uint32_t alignment, uint8_t index);

  void waitIfExtraAlloc(uint32_t unalignedSize);

  VkDeviceSize getLastAllocSize()
  {
    WinAutoLock lock(writeLock);
    return lastAllocSize;
  }

  template <typename T>
  void onFrameEnd(T clb)
  {
    WinAutoLock lock(writeLock);

    lastAllocSize = currentAllocSize;
    currentAllocSize = 0;

    int pendingBuffers = buffersInUse;
    if (pendingBuffers > 0)
      return;

    for (auto &&buffer : buffers)
      clb(buffer.buffer);
    buffers.clear();
  }

  TempBufferHolder *allocatePooled(uint32_t size);
  TempBufferHolder *allocatePooled(const BufferRef &framemem_ref);
  void freePooled(TempBufferHolder *temp_buff_holder);
  void shutdown();
};

class FramememBufferManager
{
  WinCritSec writeLock;
  Buffer *buffers[Buffer::pending_discard_frames * (uint32_t)DeviceMemoryClass::COUNT] = {};
  dag::RelocatableFixedVector<Buffer *, Buffer::pending_discard_frames *(uint32_t)DeviceMemoryClass::COUNT> destroyQue;

  FramememBufferManager(const FramememBufferManager &);
  uint32_t frameToRingIdx(uint32_t frame) { return frame % Buffer::pending_discard_frames; }
  Buffer *&getBuf(DeviceMemoryClass dmc, uint32_t frame)
  {
    return buffers[frameToRingIdx(frame) * (uint32_t)DeviceMemoryClass::COUNT + (uint32_t)dmc];
  }

public:
  FramememBufferManager() {}
  ~FramememBufferManager() {}

  void init();
  void shutdown();
  BufferRef acquire(uint32_t size, DeviceMemoryClass mem_class);
  BufferRef acquire(uint32_t size, uint32_t flags);
  void getMemUsageInfo(uint32_t &allocated, uint32_t &used_currently);
  void purge();
  void onFrameEnd();
  void swapRefCounters(Buffer *src, Buffer *dst);
  void release(Buffer *obj);
};

} // namespace drv3d_vulkan
