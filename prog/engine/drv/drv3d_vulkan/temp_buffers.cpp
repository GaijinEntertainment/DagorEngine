// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "temp_buffers.h"
#include "globals.h"
#include "buffer_props.h"
#include "driver_config.h"
#include "device_context.h"
#include "global_lock.h"
#include "frontend_pod_state.h"
#include "backend/cmd/buffers.h"
#include "backend/cmd/resources.h"

using namespace drv3d_vulkan;

void TempBufferManager::setConfig(VkDeviceSize size, uint32_t alignment, uint8_t index)
{
  G_ASSERT(((alignment - 1) & alignment) == 0);
  pow2Alignment = max<VkDeviceSize>(Globals::VK::bufProps.minimal, alignment);
  maxAllocSize = size;
  managerIndex = index;
}

void TempBufferManager::waitIfExtraAlloc(uint32_t unalignedSize)
{
  if (Globals::cfg.tempBufferOverAllocWaitMs == 0)
    return;

  uint32_t size = (unalignedSize + (pow2Alignment - 1)) & ~(pow2Alignment - 1);
  bool shouldWait = false;
  {
    WinAutoLock lock(writeLock);

    // one temp buffer is always viable without extra wait
    if (buffers.empty())
      return;

    auto ref = eastl::find_if(begin(buffers), end(buffers),
      [=](const TempBufferInfo &info) //
      {
        auto space = info.buffer->getBlockSize() - info.fill;
        return space >= size;
      });

    shouldWait = ref == end(buffers);
  }

  if (shouldWait)
  {
    // facing this marker means temp buffers allocate from "cold" memory
    // which is slow and may create random spikes, which should be avoided
    // there is different approaches to fix this issue
    //  - check that buffer data transfers are not too big / using wrong path
    //  - check that ring allocator page size is reasonable sized
    //  - check that this and other data transfers are not being on hold for long time
    //  - adjust wait time if overallocation is ok but wait time is too big/small
    DA_PROFILE_WAIT("vulkan_temp_buffers_overalloc");
    sleep_msec(Globals::cfg.tempBufferOverAllocWaitMs);
  }
}

BufferRef TempBufferManager::allocate(uint32_t unalignedSize)
{
  waitIfExtraAlloc(unalignedSize);
  ++buffersInUse;
  uint32_t size = (unalignedSize + (pow2Alignment - 1)) & ~(pow2Alignment - 1);
  G_ASSERTF(!Globals::lock.isAcquired(), "vulkan: use framemem buffers instead of temp buffers under GPU lock");

  WinAutoLock lock(writeLock);
  auto ref = eastl::find_if(begin(buffers), end(buffers),
    [=](const TempBufferInfo &info) //
    {
      auto space = info.buffer->getBlockSize() - info.fill;
      return space >= size;
    });
  if (ref == end(buffers))
  {
    TempBufferInfo info;
    info.buffer = Buffer::create((size >= maxAllocSize ? size : 0) + maxAllocSize,
      DeviceMemoryClass::HOST_RESIDENT_HOST_READ_WRITE_BUFFER, 1, BufferMemoryFlags::TEMP);
    if (Globals::cfg.debugLevel > 0)
      info.buffer->setDebugName(String(128, "internal temp buffer type %u", managerIndex));
    ref = buffers.insert(end(buffers), info);
  }

  BufferRef result{ref->buffer, size, (uint32_t)ref->fill};
  ref->fill += size;
  currentAllocSize += size;
  return result;
}

TempBufferHolder *TempBufferManager::allocatePooled(uint32_t size)
{
  WinAutoLock poolLock(poolGuard);
  return pool.allocate(size);
}

TempBufferHolder *TempBufferManager::allocatePooled(const BufferRef &framemem_ref)
{
  WinAutoLock poolLock(poolGuard);
  return pool.allocate(framemem_ref);
}

void TempBufferManager::freePooled(TempBufferHolder *temp_buff_holder)
{
  WinAutoLock poolLock(poolGuard);
  pool.free(temp_buff_holder);
}

void TempBufferManager::shutdown()
{
  // leaked temp buffers will be already reported per frame basis
  // no need to verify that pool is empty
  pool.freeAll();
}

TempBufferHolder::TempBufferHolder(const TempBufferHolder &from) : ref(from.ref) { ++Frontend::tempBuffers.buffersInUse; }

TempBufferHolder::TempBufferHolder(uint32_t size, const void *src)
{
  ref = Frontend::tempBuffers.allocate(size);
  memcpy(getPtr(), src, size);
  flushWrite();
}

TempBufferHolder::TempBufferHolder(uint32_t size) { ref = Frontend::tempBuffers.allocate(size); }

TempBufferHolder::TempBufferHolder(const BufferRef &ext_ref) : ref(ext_ref) { ++Frontend::tempBuffers.buffersInUse; }

TempBufferHolder::~TempBufferHolder()
{
#if DAGOR_DBGLEVEL > 0
  G_ASSERTF(writesFlushed, "vulkan: temp buffer write was not flushed");
#endif
  --Frontend::tempBuffers.buffersInUse;
}

void FramememBufferManager::init()
{
  for (Buffer *&i : buffers)
    i = nullptr;
  destroyQue.clear();
}

void FramememBufferManager::shutdown()
{
  G_ASSERTF(destroyQue.size() == 0, "vulkan: should land in shutdown after onFrameEnd call");
  purge();
}

BufferRef FramememBufferManager::acquire(uint32_t size, DeviceMemoryClass mem_class)
{
  // will also trigger TSAN warning if we try to present in parallel to discard, see assert below,
  // that's invalid for framemem buffer! fix in user code!
  volatile uint32_t frame = Frontend::State::pod.frameIndex;
  size = Globals::VK::bufProps.alignSize(size);
#if DAGOR_DBGLEVEL > 0
  totalUsage += size;
#endif

  bool nonMappedRing = (mem_class == DeviceMemoryClass::DEVICE_RESIDENT_BUFFER) && Globals::Mem::pool.hasDedicatedMemory();
  WinAutoLock lock(writeLock);
  Buffer *&buf = nonMappedRing ? getNonMappedBuf(mem_class, frame) : getBuf(mem_class, frame);

  if (!buf || !buf->onDiscardFramemem(frame, size))
  {
    uint32_t discardCount = 1;
    uint32_t discardBlockSz = Globals::cfg.framememBlockSize;
    if (buf)
    {
      discardCount += buf->getDiscardBlocks();
      destroyQue.push_back(buf);
    }
    discardCount += size / discardBlockSz + 1;

    buf = Buffer::create(discardBlockSz, mem_class, discardCount, BufferMemoryFlags::FRAMEMEM);
    // first CmdAddRefFrameMem handled at creation time to avoid lock order inversion
    // between manager write lock and front lock
    buf->markDiscardUsageRange(frame, size);
    if (Globals::cfg.debugLevel > 0)
      buf->setDebugName(String(32, "framemem_%u", frameToRingIdx(frame)));
  }
  if (nonMappedRing)
    buf->ensureNoMappedMemory();
  BufferRef ret{buf, size};
  // make acquires unique over multiple frames as some framemem buffers may be leaved bound, yet not updated
  // causing discard state replacement to fail, due to non unique buf+offset combos
  ret.frameReference = frame;
  // invalid because no one can read data reliably from buffer under such conditions
  G_ASSERTF(frame == Frontend::State::pod.frameIndex,
    "vulkan: submitting frames and doing discard at same time for framemem buffer is invalid!");
  return ret;
}

BufferRef FramememBufferManager::acquire(uint32_t size, uint32_t flags)
{
  return acquire(size, BufferDescription::memoryClassFromCflags(flags));
}

void FramememBufferManager::getMemUsageInfo(uint32_t &allocated, uint32_t &used_currently)
{
  WinAutoLock lock(writeLock);

  allocated = 0;
  used_currently = 0;
#if DAGOR_DBGLEVEL > 0
  for (uint32_t i : lastTotalUsages)
    used_currently += i;
#endif

  for (Buffer *i : buffers)
  {
    if (!i)
      continue;
    allocated += i->getTotalSize();
  }
}

void FramememBufferManager::purge()
{
  WinAutoLock lock(writeLock);

  for (Buffer *&i : buffers)
  {
    if (i)
    {
      Globals::ctx.dispatchCmd<CmdReleaseFrameMem>({i});
      Globals::ctx.dispatchCmd<CmdDestroyBuffer>({i});
    }
    i = nullptr;
  }
}

void FramememBufferManager::onFrameEnd()
{
  WinAutoLock lock(writeLock);

#if DAGOR_DBGLEVEL > 0
  lastTotalUsages[frameToRingIdx(Frontend::State::pod.frameIndex)] = totalUsage;
  totalUsage = 0;
#endif
  for (Buffer *i : destroyQue)
  {
    Globals::ctx.dispatchCmdNoLock<CmdReleaseFrameMem>({i});
    Globals::ctx.dispatchCmdNoLock<CmdDestroyBuffer>({i});
  }
  destroyQue.clear();
}

void FramememBufferManager::swapRefCounters(Buffer *src, Buffer *dst)
{
  if (src)
    Globals::ctx.dispatchCmd<CmdReleaseFrameMem>({src});
  Globals::ctx.dispatchCmd<CmdAddRefFrameMem>({dst});
}

void FramememBufferManager::release(Buffer *obj) { Globals::ctx.dispatchCmd<CmdReleaseFrameMem>({obj}); }
