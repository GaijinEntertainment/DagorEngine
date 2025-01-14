// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <perfMon/dag_graphStat.h>

#include "buffer.h"
#include "device_context.h"
#include "global_lock.h"

using namespace drv3d_vulkan;

int GenericBufferInterface::unlock()
{
  G_ASSERT(lastLockFlags != 0);

  if (bufferLockedForWrite())
  {
    if (pushAllocation)
      unlockWritePush();
    else if (stagingBuffer)
      unlockWriteStaging();
    else if (isDMAPathAvailable())
      unlockWriteDMA();
    else
      // we did locked buffer for write, but pair for unlock is not found somehow
      G_ASSERTF(0, "vulkan: incorrect write unlock for buffer %p:%s", this, getResName());
  }

  if (!bufferLockedForRead())
    disposeStagingBuffer();

  lastLockFlags = 0;
  return 1;
}

int GenericBufferInterface::lock(unsigned ofs_bytes, unsigned size_bytes, void **ptr, int flags)
{
  checkLockParams(ofs_bytes, size_bytes, flags, bufFlags);

  G_ASSERTF(lastLockFlags == 0, "it seems lock was called on %p(%s) without unlocking the last lock", this, getResName());
  G_ASSERTF(ofs_bytes < bufSize, "locking of %p(%s) was out of range, buffer size is %u but offset was %u", this, getResName(),
    bufSize, ofs_bytes);
  G_ASSERTF(ofs_bytes + size_bytes <= bufSize, "locking of %p(%s) was out of range, buffer size is %u but locking range end was %u",
    this, getResName(), bufSize, ofs_bytes + size_bytes);
  G_ASSERTF((flags != 0), "buffer %p(%s) was locked without any locking flags", this, getResName());

  // save lock data
  lastLockFlags = static_cast<uint16_t>(flags);
  lockRange.reset(ofs_bytes, ofs_bytes + (size_bytes ? size_bytes : bufSize));

  // adjust some broken flags
  if (0 == ((VBLOCK_WRITEONLY | VBLOCK_READONLY) & lastLockFlags))
  {
    // this is just awful
    G_ASSERT(((VBLOCK_DISCARD | VBLOCK_NOOVERWRITE) & lastLockFlags));
    lastLockFlags |= VBLOCK_WRITEONLY;
  }

  // manage discard blocks if needed
  processDiscardFlag();

  G_ASSERTF(
    lockRange.back() <= ref.buffer->getBlockSize() || ((bufFlags & SBCF_FRAMEMEM) && lockRange.back() <= ref.buffer->getTotalSize()),
    "locked range (%d) is larger, than actual allocated buffer memory (%d)", lockRange.back(), ref.buffer->getBlockSize());

  G_ASSERTF(!stagingBuffer || (lockRange.back() <= stagingBuffer->getBlockSize()),
    "locked range (%d) is larger, than actual allocated buffer staging memory (%d)", lockRange.back(),
    lockRange.back() <= ref.buffer->getBlockSize());

  // use one of transfer approaches, depending on lock flags and buffer props
  if (bufferGpuTimelineUpdate())
    lockPush(ptr);
  else if (isDMAPathAvailable())
    lockDMA(ptr);
  else if (bufferLockedForRead())
    lockStaging(ptr);
  else
    lockPush(ptr);

  if (!asyncCopyEvent.isRequested())
    Stat3D::updateLockVIBuf();

  return asyncCopyEvent.isRequested() ? 0 : 1;
}

bool GenericBufferInterface::updateData(uint32_t ofs_bytes, uint32_t size_bytes, const void *__restrict src, uint32_t lockFlags)
{
  G_ASSERT_RETURN(size_bytes != 0, false);
  // discard can be processed only via lock
  // nooverwrite tells that caller are sure that there will be no conflicts
  if (lockFlags & (VBLOCK_DISCARD | VBLOCK_NOOVERWRITE))
    return updateDataWithLock(ofs_bytes, size_bytes, src, lockFlags);

  auto &ctx = Globals::ctx;
  // reorder buffer update if we running it from external thread
  if (Globals::lock.isAcquired())
    ctx.copyBufferDiscardReorderable(
      ctx.uploadToFrameMem(DeviceMemoryClass::HOST_RESIDENT_HOST_WRITE_ONLY_BUFFER, size_bytes, (void *)src), ref, 0, ofs_bytes,
      size_bytes);
  else
  {
    auto buf = TempBufferHolder(size_bytes, (void *)src);
    ctx.uploadBufferOrdered(buf.getRef(), ref, 0, ofs_bytes, size_bytes);
  }

  return true;
}

// DMA path - for host & device visible buffers

void GenericBufferInterface::lockDMA(void **ptr)
{
  // gpu can only change data via unordered access
  if (bufferLockedForGPUReadback())
  {
    auto &ctx = Globals::ctx;

    // even with mapped memory we need to properly flush caches and
    // sync with the host
    lockReadback(
      ptr, [&]() { ctx.flushBufferToHost(ref, lockRange); },
      [&]() { ref.markNonCoherentRange(lockRange.front(), lockRange.size(), false); });
  }

  if (ptr)
    *ptr = ref.ptrOffset(lockRange.front());
}

void GenericBufferInterface::unlockWriteDMA() { ref.markNonCoherentRange(lockRange.front(), lockRange.size(), true); }

// staging path - for device visible buffers

void GenericBufferInterface::lockStaging(void **ptr)
{
  uint32_t stageOffset = acquireStagingBuffer();

  if (bufferLockedForRead())
  {
    auto &ctx = Globals::ctx;

    lockReadback(
      ptr, [&]() { ctx.downloadBuffer(ref, stagingBuffer, lockRange.front(), stageOffset, lockRange.size()); },
      [&]() { stagingBuffer->markNonCoherentRangeLoc(stageOffset, lockRange.size(), false); });
  }

  if (ptr)
    *ptr = stagingBuffer->ptrOffsetLoc(stageOffset);
}

void GenericBufferInterface::unlockWriteStaging()
{
  auto &ctx = Globals::ctx;

  stagingBuffer->markNonCoherentRangeLoc(0, lockRange.size(), true);
  ctx.uploadBuffer(BufferRef{stagingBuffer}, ref, 0, lockRange.front(), lockRange.size());
}

// Push path - for ordered update of device visible buffers

void GenericBufferInterface::lockPush(void **ptr)
{
  if (Globals::lock.isAcquired())
    pushAllocation = Frontend::tempBuffers.allocatePooled(
      Frontend::frameMemBuffers.acquire(lockRange.size(), DeviceMemoryClass::HOST_RESIDENT_HOST_WRITE_ONLY_BUFFER));
  else
    pushAllocation = Frontend::tempBuffers.allocatePooled(lockRange.size());

  if (ptr)
    *ptr = pushAllocation->getPtr();
}

void GenericBufferInterface::unlockWritePush()
{
  auto &ctx = Globals::ctx;

  pushAllocation->flushWrite();
  if (!bufferGpuTimelineUpdate())
    ctx.uploadBuffer(pushAllocation->getRef(), ref, 0, lockRange.front(), lockRange.size());
  else if (Globals::lock.isAcquired())
    ctx.copyBufferDiscardReorderable(pushAllocation->getRef(), ref, 0, lockRange.front(), lockRange.size());
  else
    ctx.uploadBufferOrdered(pushAllocation->getRef(), ref, 0, lockRange.front(), lockRange.size());
  Frontend::tempBuffers.freePooled(pushAllocation);
  pushAllocation = nullptr;

  if (isDMAPathAvailable() && (bufFlags & SBCF_CPU_ACCESS_READ))
    ctx.flushBufferToHost(ref, lockRange);
}
