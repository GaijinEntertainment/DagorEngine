// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_buffers.h>
#include <atomic>
#include "util/value_range.h"
#include "buffer_resource.h"
#include "async_completion_state.h"
#include "temp_buffers.h"
#include "globals.h"
#include "debug_naming.h"

namespace drv3d_vulkan
{
class GenericBufferInterface final : public Sbuffer
{
  uint32_t bufSize = 0;
  uint32_t bufFlags = 0;
  ValueRange<uint32_t> lockRange;
  BufferRef ref = {};
  Buffer *stagingBuffer = nullptr;
  uint16_t structSize = 0;
  FormatStore uavFormat;
  uint8_t lastLockFlags = 0;
  AsyncCompletionState asyncCopyEvent;
  TempBufferHolder *pushAllocation;

  bool bufferLockedForRead() const { return lastLockFlags & VBLOCK_READONLY; }
  bool bufferLockedForWrite() const { return lastLockFlags & VBLOCK_WRITEONLY; }
  bool bufferLockedForGPUReadback() const { return (0 != (SBCF_BIND_UNORDERED & bufFlags)) && bufferLockedForRead(); }
  bool bufferLockDiscardRequested() const
  {
    return (lastLockFlags & VBLOCK_DISCARD) ||
           // a write to a staging buffer is a implicit discard request
           (bufferLockedForWrite() && !bufferLockedForRead() && isStagingBuffer());
  }
  bool bufferSyncUpdateRequested() const { return lastLockFlags & VBLOCK_NOOVERWRITE; }
  bool bufferGpuTimelineUpdate() const
  {
    return (!bufferLockedForRead() && bufferLockedForWrite() && !bufferLockDiscardRequested() && !bufferSyncUpdateRequested());
  }
  DeviceMemoryClass getMemoryClass() const { return BufferDescription::memoryClassFromCflags(bufFlags); }
  DeviceMemoryClass getPermanentStagingBufferMemoryClass() const
  {
    // keep permanent stagings in host heaps, do not waste shared host-device heap
    if (bufFlags & SBCF_CPU_ACCESS_READ)
      return (bufFlags & SBCF_CPU_ACCESS_WRITE) ? DeviceMemoryClass::HOST_RESIDENT_HOST_READ_WRITE_BUFFER
                                                : DeviceMemoryClass::HOST_RESIDENT_HOST_READ_ONLY_BUFFER;
    return DeviceMemoryClass::HOST_RESIDENT_HOST_WRITE_ONLY_BUFFER;
  }
  DeviceMemoryClass getTemporaryStagingBufferMemoryClass() const
  {
    if (bufferLockedForRead())
      return bufferLockedForWrite() ? DeviceMemoryClass::HOST_RESIDENT_HOST_READ_WRITE_BUFFER
                                    : DeviceMemoryClass::HOST_RESIDENT_HOST_READ_ONLY_BUFFER;
    return DeviceMemoryClass::HOST_RESIDENT_HOST_WRITE_ONLY_BUFFER;
  }
  bool isStagingBuffer() const { return bufFlags == SBCF_STAGING_BUFFER; }

  bool isDMAPathAvailable();

  void lockStaging(void **ptr);
  void lockDMA(void **ptr);
  void lockPush(void **ptr);
  void unlockWriteStaging();
  void unlockWritePush();
  void unlockWriteDMA();

  uint32_t acquireStagingBuffer();
  void disposeStagingBuffer();
  void processDiscardFlag();

  void markAsyncCopyInProgress();
  void markAsyncCopyFinished();
  void blockingReadbackWait();
  template <typename RbStarter, typename RbFinisher>
  void lockReadback(void **ptr, RbStarter startReadback, RbFinisher finishReadback)
  {
    if (ptr)
    {
      if (!asyncCopyEvent.isRequested())
      {
        startReadback();
        blockingReadbackWait();
      }
      else
        markAsyncCopyFinished();

      finishReadback();
    }
    else
    {
      startReadback();
      markAsyncCopyInProgress();
    }
  }

  void afterBufferResourceAllocated();

public:
  GenericBufferInterface(uint32_t struct_size, uint32_t element_count, uint32_t flags, FormatStore format, bool managed,
    const char *stat_name);
  ~GenericBufferInterface();

  int ressize() const override;
  int getFlags() const override;
  void destroy() override;
  int unlock() override;
  int lock(unsigned ofs_bytes, unsigned size_bytes, void **ptr, int flags) override;
  int getElementSize() const override;
  int getNumElements() const override;
  virtual void setResApiName(const char *name) const override { Globals::Dbg::naming.setBufName(ref.buffer, name); }
  bool copyTo(Sbuffer *dst) override;
  bool copyTo(Sbuffer *dst, uint32_t dst_offset, uint32_t src_offset, uint32_t size_bytes) override;
  bool updateData(uint32_t ofs_bytes, uint32_t size_bytes, const void *__restrict src, uint32_t lockFlags) override;
  void useExternalResource(Buffer *resource);
  BufferRef fillFrameMemWithDummyData();

  const BufferRef &getBufferRef() const { return ref; }
  VkIndexType getIndexType() const { return bufFlags & SBCF_INDEX32 ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16; }
};

GenericBufferInterface *allocate_buffer(uint32_t struct_size, uint32_t element_count, uint32_t flags, FormatStore format, bool managed,
  const char *stat_name);
void free_buffer(GenericBufferInterface *buffer);
} // namespace drv3d_vulkan