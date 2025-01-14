// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "buffer.h"
#include <drv/3d/dag_commands.h>
#include <debug/dag_debug.h>
#include <generic/dag_tab.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>

#include <vecmath/dag_vecMath.h>
#include <math/dag_vecMathCompatibility.h>
#include <util/dag_string.h>
#include <util/dag_watchdog.h>

#include <validation.h>

#include "driver.h"
#include "statStr.h"
#include "device_context.h"
#include "global_lock.h"

using namespace drv3d_vulkan;

GenericBufferInterface::GenericBufferInterface(uint32_t struct_size, uint32_t element_count, uint32_t flags, FormatStore format,
  bool managed, const char *stat_name) :
  structSize(struct_size), bufSize(struct_size * element_count), bufFlags(flags), uavFormat(format)
{
  G_ASSERTF(bufSize, "vulkan: trying to create empty buffer <%s>", stat_name);
  G_ASSERTF((bufFlags & (SBCF_FRAMEMEM | SBCF_USAGE_ACCELLERATION_STRUCTURE_BUILD_SCRATCH_SPACE)) !=
              (SBCF_FRAMEMEM | SBCF_USAGE_ACCELLERATION_STRUCTURE_BUILD_SCRATCH_SPACE),
    "vulkan: can't create RT scratch buffer as framemem");
  setResName(stat_name);
  G_ASSERTF(
    format == FormatStore(0) || ((flags & (SBCF_BIND_VERTEX | SBCF_BIND_INDEX | SBCF_MISC_STRUCTURED | SBCF_MISC_ALLOW_RAW)) == 0),
    "can't create buffer with texture format which is Structured, Vertex, Index or Raw");

  if (bufFlags & SBCF_MISC_ALLOW_RAW)
  {
    G_ASSERT(struct_size == 1 || struct_size == 4);
    structSize = 4;
  }

#if DAGOR_DBGLEVEL > 0
  if (bufFlags & SBCF_MISC_STRUCTURED)
  {
    if (!is_good_buffer_structure_size(structSize))
    {
      D3D_ERROR("The structure size of %u of buffer %s has a hardware unfriendly size and probably "
                "results in degraded performance. For some platforms it might requires the shader "
                "compiler to restructure the structure type to avoid layout violations and on other "
                "platforms it results in wasted memory as the memory manager has to insert extra "
                "padding to align the buffer properly.",
        structSize, getResName());
    }
  }

  if ((bufFlags & SBCF_ZEROMEM) && (bufFlags & SBCF_FRAMEMEM))
  {
    D3D_ERROR(
      "Using SBCF_ZEROMEM and SBCF_FRAMEMEM flags for buffer %s together don't make really any sense and it prevents the driver "
      "to apply optimizations on the buffer handling");
  }
#endif

  // allow option-disable true framemem implementation, due to lingering amount of bugs in client usage code
  if (!Globals::cfg.bits.allowFrameMem)
    bufFlags &= ~SBCF_FRAMEMEM;
  bool frameMemRequested = bufFlags & SBCF_FRAMEMEM;

  if ((FormatStore(0) != uavFormat) && frameMemRequested)
  {
    // silently drop the flag, because we do have "one_frame" class for sampled buffers
    // if it used in bad manner i.e. with variable discard size, we have D3D_ERROR there
    // D3D_ERROR("vulkan: framemem for sampled buffers are not supported, dropping framemem flag for <%s>", stat_name);
    bufFlags &= ~SBCF_FRAMEMEM;
    frameMemRequested = false;
  }

#if DAGOR_DBGLEVEL > 0
  // use default discard impl, it allows us to keep naming of buffer at framemem check time
  if (Globals::cfg.bits.debugFrameMemUsage)
    bufFlags &= ~SBCF_FRAMEMEM;
#endif

  if (!managed || (bufFlags & SBCF_FRAMEMEM))
    return;

  // pass framemem flag to object for debug purposes
  BufferMemoryFlags memFlags = BufferMemoryFlags::NONE;
  if (frameMemRequested)
    memFlags = BufferMemoryFlags::FRAMEMEM;
  else if (bufFlags & SBCF_USAGE_ACCELLERATION_STRUCTURE_BUILD_SCRATCH_SPACE)
    memFlags = BufferMemoryFlags::DEDICATED;
  ref = BufferRef{Buffer::create(bufSize, getMemoryClass(), 1, memFlags)};
  afterBufferResourceAllocated();
}

void GenericBufferInterface::useExternalResource(Buffer *resource)
{
  ref = BufferRef{resource};
  afterBufferResourceAllocated();
}

void GenericBufferInterface::afterBufferResourceAllocated()
{
  Globals::Dbg::naming.setBufName(ref.buffer, getResName());
  if (FormatStore(0) != uavFormat)
    ref.buffer->addBufferView(uavFormat);

  if (bufFlags & SBCF_ZEROMEM)
  {
    unsigned char *data = nullptr;
    if (lock(0, bufSize, (void **)&data, VBLOCK_WRITEONLY) && data)
    {
      memset(data, 0, bufSize);
      unlock();
    }
  }
}

GenericBufferInterface::~GenericBufferInterface()
{
  auto &ctx = Globals::ctx;

  if (stagingBuffer)
    ctx.destroyBuffer(stagingBuffer);

  if (bufFlags & SBCF_FRAMEMEM)
  {
    if (ref)
      Frontend::frameMemBuffers.release(ref.buffer);
    return;
  }

  ctx.destroyBuffer(ref.buffer);
}

int GenericBufferInterface::ressize() const { return int(bufSize); }

int GenericBufferInterface::getFlags() const { return int(bufFlags); }

void GenericBufferInterface::destroy() { free_buffer(this); }

int GenericBufferInterface::getElementSize() const { return structSize; }

int GenericBufferInterface::getNumElements() const { return structSize ? bufSize / structSize : 0; }

bool GenericBufferInterface::copyTo(Sbuffer *dst)
{
  auto dest = (GenericBufferInterface *)dst;
  auto &context = Globals::ctx;
  G_ASSERTF(bufSize <= ref.buffer->getBlockSize(), "the actual backing buffer is smaller (%d bytes) than the Buffer (%d bytes)",
    ref.buffer->getBlockSize(), bufSize);
  // reorder buffer update if we running it from external thread
  if (Globals::lock.isAcquired())
    context.copyBufferDiscardReorderable(ref, dest->ref, 0, 0, bufSize);
  else
    context.uploadBufferOrdered(ref, dest->ref, 0, 0, bufSize);
  return true;
}

bool GenericBufferInterface::copyTo(Sbuffer *dst, uint32_t dst_offset, uint32_t src_offset, uint32_t size_bytes)
{
  auto dest = (GenericBufferInterface *)dst;
  auto &context = Globals::ctx;
  // reorder buffer update if we running it from external thread
  if (Globals::lock.isAcquired())
    context.copyBufferDiscardReorderable(ref, dest->ref, src_offset, dst_offset, size_bytes);
  else
    context.uploadBufferOrdered(ref, dest->ref, src_offset, dst_offset, size_bytes);
  return true;
}

uint32_t GenericBufferInterface::acquireStagingBuffer()
{
  TIME_PROFILE(vulkan_acquire_staging_buf);
  // staging can be already allocated with async readback logic
  if (!stagingBuffer)
  {
    // alloc temp with best possible memory type and smallest size only covering
    // locked area
    stagingBuffer = Buffer::create(lockRange.size(), getTemporaryStagingBufferMemoryClass(), 1,
      bufferLockedForRead() ? BufferMemoryFlags::NONE : BufferMemoryFlags::TEMP);
    stagingBuffer->setStagingDebugName(ref.buffer);
  }
  return 0;
}

void GenericBufferInterface::disposeStagingBuffer()
{
  if (stagingBuffer)
  {
    auto &ctx = Globals::ctx;

    ctx.destroyBuffer(stagingBuffer);
    stagingBuffer = nullptr;
  }
}

void GenericBufferInterface::processDiscardFlag()
{
  if (bufferLockDiscardRequested())
  {
    auto &ctx = Globals::ctx;
    // FIXME: clang on windows with dbg config, generates invalid code when .back() is used, used .stop instead
    uint32_t dynamic_size = (bufFlags & SBCF_FRAMEMEM) > 0 ? lockRange.stop : 0;

    ref = ctx.discardBuffer(ref, getMemoryClass(), uavFormat, bufFlags, dynamic_size);
  }
}

void GenericBufferInterface::markAsyncCopyInProgress()
{
  auto &ctx = Globals::ctx;
  if (asyncCopyEvent.isRequested())
  {
    if (!asyncCopyEvent.isCompleted())
    {
      D3D_ERROR("vulkan: async readback is already requested for buffer [%s]", ref.buffer->getDebugName());
      TIME_PROFILE(vulkan_double_readback_wait);
      ctx.waitForIfPending(asyncCopyEvent);
    }
    // valid code path, we can "ignore" readback result sometimes
    // else
    //  D3D_ERROR("vulkan: async readback is not consumed [%s]", ref.buffer->getDebugName());
    asyncCopyEvent.reset();
  }
  asyncCopyEvent.request(ctx.getCurrentWorkItemId());
  lastLockFlags = 0;
}

void GenericBufferInterface::markAsyncCopyFinished()
{
  if (!asyncCopyEvent.isCompleted())
  {
    D3D_ERROR("vulkan: async readback is not yet completed for buffer [%s]", ref.buffer->getDebugName());
    TIME_PROFILE(vulkan_incomplete_readback_wait);
    Globals::ctx.waitForIfPending(asyncCopyEvent);
  }
  asyncCopyEvent.reset();
}

void GenericBufferInterface::blockingReadbackWait()
{
  TIME_PROFILE(vulkan_buffer_blocking_readback_wait);
  Globals::ctx.wait();
}

bool GenericBufferInterface::isDMAPathAvailable()
{
  if (!Globals::cfg.bits.allowDMAlockPath)
    return false;
  return ref.buffer->hasMappedMemory();
}

BufferRef GenericBufferInterface::fillFrameMemWithDummyData()
{
  G_ASSERTF(bufFlags & SBCF_FRAMEMEM, "vulkan: binding empty non framemem buffer %p:%s", this, getResName());
  // because we do like to create-bind-discard-draw, and we can't state-replace empty buffers
  // always allocate empty block when non discarded buffer is bound
  void *ptr;
  lock(0, 0, &ptr, VBLOCK_WRITEONLY | VBLOCK_DISCARD);
  unlock();
  return ref;
}
