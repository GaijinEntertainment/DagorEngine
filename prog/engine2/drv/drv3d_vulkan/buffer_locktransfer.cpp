#include "buffer.h"

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

  disposeStagingBuffer();
  markDataUpdate();

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
  lockRange.reset(ofs_bytes, size_bytes ? (ofs_bytes + size_bytes) : bufSize);

  // adjust some broken flags
  if (0 == ((VBLOCK_WRITEONLY | VBLOCK_READONLY) & lastLockFlags))
  {
    // this is just awful
    G_ASSERT(((VBLOCK_DISCARD | VBLOCK_NOOVERWRITE) & lastLockFlags));
    lastLockFlags |= VBLOCK_WRITEONLY;
  }

  // manage discard blocks if needed
  processDiscardFlag();

  // use one of transfer approaches, depending on lock flags and buffer props
  if (bufferGpuTimelineUpdate() && updatedInCurrentWorkItem())
    lockPush(ptr);
  else if (isDMAPathAvailable())
    lockDMA(ptr);
  else
    lockStaging(ptr);

  if (!asyncCopyInProgress)
    Stat3D::updateLockVIBuf();

  return asyncCopyInProgress ? 0 : 1;
}

bool GenericBufferInterface::updateData(uint32_t ofs_bytes, uint32_t size_bytes, const void *__restrict src, uint32_t lockFlags)
{
  G_ASSERT_RETURN(size_bytes != 0, false);
  if ((lockFlags & (VBLOCK_DISCARD | VBLOCK_NOOVERWRITE)) || isDMAPathAvailable() || !updatedInCurrentWorkItem())
  {
    // in those cases we need to use locking to keep everything consistent
    return updateDataWithLock(ofs_bytes, size_bytes, src, lockFlags);
  }
  else
  {
    // only usable in case when we want to overwrite portions at a certain point and there are no
    // local copies
    markDataUpdate();

    auto &device = get_device();
    auto &ctx = device.getContext();
    auto buf = ctx.copyToTempBuffer(TempBufferManager::TYPE_UPDATE, size_bytes, (void *)src);
    // reorder buffer update if we running it from external thread
    if (is_global_lock_acquired())
      ctx.copyBuffer(buf.get().buffer, buffer, buf.get().offset, ofs_bytes, size_bytes);
    else
      ctx.uploadBufferOrdered(buf.get().buffer, buffer, buf.get().offset, ofs_bytes, size_bytes);

    return true;
  }
}

// DMA path - for host & device visible buffers

void GenericBufferInterface::lockDMA(void **ptr)
{
  // gpu can only change data via unordered access
  if (bufferLockedForGPUReadback())
  {
    auto &ctx = get_device().getContext();

    // even with mapped memory we need to properly flush caches and
    // sync with the host
    lockReadback(
      ptr, [&]() { ctx.flushBufferToHost(buffer, lockRange); },
      [&]() { buffer->markNonCoherentRange(lockRange.front(), lockRange.size(), false); });
  }

  if (ptr)
    *ptr = buffer->dataPointer(lockRange.front());
}

void GenericBufferInterface::unlockWriteDMA() { buffer->markNonCoherentRange(lockRange.front(), lockRange.size(), true); }

// staging path - for device visible buffers

void GenericBufferInterface::lockStaging(void **ptr)
{
  uint32_t stageOffset = acquireStagingBuffer();

  if (bufferLockedForRead())
  {
    auto &ctx = get_device().getContext();

    lockReadback(
      ptr, [&]() { ctx.downloadBuffer(buffer, stagingBuffer, lockRange.front(), stageOffset, lockRange.size()); },
      [&]() { stagingBuffer->markNonCoherentRange(stageOffset, lockRange.size(), false); });
  }

  if (ptr)
    *ptr = stagingBuffer->dataPointer(stageOffset);
}

void GenericBufferInterface::unlockWriteStaging()
{
  auto &ctx = get_device().getContext();

  uint32_t stageOffset = 0;
  if (stagingBufferIsPermanent())
    stageOffset = lockRange.front();

  stagingBuffer->markNonCoherentRange(stageOffset, lockRange.size(), true);
  ctx.uploadBuffer(stagingBuffer, buffer, stageOffset, lockRange.front(), lockRange.size());
}

// Push path - for ordered update of device visible buffers

void GenericBufferInterface::lockPush(void **ptr)
{
  auto &ctx = get_device().getContext();

  pushAllocation = ctx.allocTempBuffer(TempBufferManager::TYPE_UPDATE, lockRange.size());
  if (ptr)
    *ptr = pushAllocation->getPtr();
}

void GenericBufferInterface::unlockWritePush()
{
  auto &ctx = get_device().getContext();

  pushAllocation->flushWrite();
  if (is_global_lock_acquired())
    ctx.copyBuffer(pushAllocation->get().buffer, buffer, pushAllocation->get().offset, lockRange.front(), lockRange.size());
  else
    ctx.uploadBufferOrdered(pushAllocation->get().buffer, buffer, pushAllocation->get().offset, lockRange.front(), lockRange.size());
  ctx.freeTempBuffer(pushAllocation);
  pushAllocation = nullptr;

  if (bufFlags & SBCF_CPU_ACCESS_READ)
  {
    if (stagingBuffer)
      ctx.flushBufferToHost(stagingBuffer, lockRange);
    else if (isDMAPathAvailable())
      ctx.flushBufferToHost(buffer, lockRange);
  }
}
