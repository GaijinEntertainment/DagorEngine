#include "buffer.h"
#include <3d/dag_drv3dCmd.h>
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

using namespace drv3d_vulkan;

GenericBufferInterface::GenericBufferInterface(uint32_t struct_size, uint32_t element_count, uint32_t flags, FormatStore format,
  const char *stat_name) :
  structSize(struct_size), bufSize(struct_size * element_count), bufFlags(flags), uavFormat(format)
{
  setResName(stat_name);
  G_ASSERTF(
    format == FormatStore(0) || ((flags & (SBCF_BIND_VERTEX | SBCF_BIND_INDEX | SBCF_MISC_STRUCTURED | SBCF_MISC_ALLOW_RAW)) == 0),
    "can't create buffer with texture format which is Structured, Vertex, Index or Raw");

  Device &device = get_device();

  buffer = device.createBuffer(bufSize, getMemoryClass(), getInitialDiscardCount(), BufferMemoryFlags::NONE);

  if (!buffer->isResident())
    return;

  device.setBufName(buffer, getResName());

  if (!isDMAPathAvailable() && stagingBufferIsPermanent())
  {
    stagingBuffer =
      device.createBuffer(bufSize, getPermanentStagingBufferMemoryClass(), getInitialDiscardCount(), BufferMemoryFlags::NONE);
    stagingBuffer->setStagingDebugName(buffer);
  }

  if (bufFlags & SBCF_MISC_ALLOW_RAW)
  {
    G_ASSERT(struct_size == 1 || struct_size == 4);
    structSize = 4;
    uavFormat = FormatStore::fromCreateFlags(TEXFMT_R32UI);
  }

#if DAGOR_DBGLEVEL > 0
  if (bufFlags & SBCF_MISC_STRUCTURED)
  {
    if (!is_good_buffer_structure_size(structSize))
    {
      logerr("The structure size of %u of buffer %s has a hardware unfriendly size and probably "
             "results in degraded performance. For some platforms it might requires the shader "
             "compiler to restructure the structure type to avoid layout violations and on other "
             "platforms it results in wasted memory as the memory manager has to insert extra "
             "padding to align the buffer properly.",
        structSize, getResName());
    }
  }
#endif

  if (FormatStore(0) != uavFormat)
    device.addBufferView(buffer, uavFormat);

  if (flags & SBCF_ZEROMEM)
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
  auto &ctx = get_device().getContext();

  if (stagingBuffer)
    ctx.destroyBuffer(stagingBuffer);

  ctx.destroyBuffer(buffer);
}

int GenericBufferInterface::ressize() const { return int(bufSize); }

int GenericBufferInterface::getFlags() const { return int(bufFlags); }

void GenericBufferInterface::destroy() { free_buffer(this); }

int GenericBufferInterface::getElementSize() const { return structSize; }

int GenericBufferInterface::getNumElements() const { return structSize ? bufSize / structSize : 0; }

bool GenericBufferInterface::copyTo(Sbuffer *dst)
{
  auto dest = (GenericBufferInterface *)dst;
  auto &device = get_device();
  auto &context = device.getContext();
  context.copyBuffer(buffer, dest->buffer, 0, 0, bufSize);
  return true;
}

bool GenericBufferInterface::copyTo(Sbuffer *dst, uint32_t dst_offset, uint32_t src_offset, uint32_t size_bytes)
{
  auto dest = (GenericBufferInterface *)dst;
  auto &device = get_device();
  auto &context = device.getContext();
  context.copyBuffer(buffer, dest->buffer, src_offset, dst_offset, size_bytes);
  return true;
}

uint32_t GenericBufferInterface::acquireStagingBuffer()
{
  if (!stagingBufferIsPermanent())
  {
    // staging can be already allocated with async readback logic
    if (!stagingBuffer)
    {
      // alloc temp with best possible memory type and smallest size only covering
      // locked area
      stagingBuffer = get_device().createBuffer(lockRange.size(), getTemporaryStagingBufferMemoryClass(), 1,
        stagingBufferIsPermanent() ? BufferMemoryFlags::NONE : BufferMemoryFlags::TEMP);
      stagingBuffer->setStagingDebugName(buffer);
    }
    return 0;
  }
  else
    return lockRange.front();
}

void GenericBufferInterface::disposeStagingBuffer()
{
  if (!stagingBufferIsPermanent() && stagingBuffer)
  {
    auto &ctx = get_device().getContext();

    ctx.destroyBuffer(stagingBuffer);
    stagingBuffer = nullptr;
  }
}

void GenericBufferInterface::processDiscardFlag()
{
  if (bufferLockDiscardRequested())
  {
    auto &ctx = get_device().getContext();

    buffer = ctx.discardBuffer(buffer, getMemoryClass(), uavFormat, bufFlags);
    if (!isDMAPathAvailable() && stagingBufferIsPermanent())
      stagingBuffer = ctx.discardBuffer(stagingBuffer, getPermanentStagingBufferMemoryClass(), FormatStore(), bufFlags);
  }
}

void GenericBufferInterface::markDataUpdate() { lastUpdateWorkId = get_device().getContext().getCurrentWorkItemId(); }

bool GenericBufferInterface::updatedInCurrentWorkItem()
{
  return lastUpdateWorkId == get_device().getContext().getCurrentWorkItemId();
}

void GenericBufferInterface::markAsyncCopyInProgress()
{
  asyncCopyInProgress = true;
  lastLockFlags = 0;
}

void GenericBufferInterface::markAsyncCopyFinished()
{
  // TODO: check that assumed finished copy is really finished
  asyncCopyInProgress = false;
}

void GenericBufferInterface::blockingReadbackWait() { get_device().getContext().wait(); }

bool GenericBufferInterface::isDMAPathAvailable()
{
  if (!get_device().getFeatures().test(DeviceFeaturesConfig::ALLOW_BUFFER_DMA_LOCK_PATH))
    return false;
  return buffer->hasMappedMemory();
}
