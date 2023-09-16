#pragma once

#include "device.h"
#include <atomic>

namespace drv3d_vulkan
{
class GenericBufferInterface final : public Sbuffer
{
  uint32_t bufSize = 0;
  uint32_t bufFlags = 0;
  ValueRange<uint32_t> lockRange;
  Buffer *buffer = nullptr;
  Buffer *stagingBuffer = nullptr;
  uint16_t structSize = 0;
  FormatStore uavFormat;
  uint8_t lastLockFlags = 0;
  bool asyncCopyInProgress = false;
  size_t lastUpdateWorkId = -1;
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
  uint32_t getInitialDiscardCount() const
  {
    if (bufFlags & SBCF_DYNAMIC)
    {
      if (bufFlags & SBCF_BIND_CONSTANT)
        return 5;
      return 2;
    }
    else if (isStagingBuffer())
    {
      return 2;
    }

    return 1;
  }
  DeviceMemoryClass getMemoryClass() const
  {
    if (bufFlags & (SBCF_CPU_ACCESS_WRITE | SBCF_DYNAMIC))
    {
      if (bufFlags & SBCF_BIND_CONSTANT)
      {
        if (bufFlags & SBCF_CPU_ACCESS_READ)
          return DeviceMemoryClass::HOST_RESIDENT_HOST_READ_WRITE_BUFFER;
        else
          return DeviceMemoryClass::DEVICE_RESIDENT_HOST_WRITE_ONLY_BUFFER;
      }
      else
      {
        if (bufFlags & SBCF_CPU_ACCESS_READ)
          return DeviceMemoryClass::HOST_RESIDENT_HOST_READ_WRITE_BUFFER;
        else
          return DeviceMemoryClass::HOST_RESIDENT_HOST_WRITE_ONLY_BUFFER;
      }
    }
    return DeviceMemoryClass::DEVICE_RESIDENT_BUFFER;
  }
  DeviceMemoryClass getPermanentStagingBufferMemoryClass() const
  {
    if (0 != (bufFlags & SBCF_CPU_ACCESS_READ))
    {
      if (0 != (bufFlags & SBCF_CPU_ACCESS_WRITE))
        return DeviceMemoryClass::HOST_RESIDENT_HOST_READ_WRITE_BUFFER;
      else
        return DeviceMemoryClass::HOST_RESIDENT_HOST_READ_ONLY_BUFFER;
    }
    return DeviceMemoryClass::DEVICE_RESIDENT_HOST_WRITE_ONLY_BUFFER;
  }
  DeviceMemoryClass getTemporaryStagingBufferMemoryClass() const
  {
    if (bufferLockedForRead())
    {
      if (bufferLockedForWrite())
        return DeviceMemoryClass::HOST_RESIDENT_HOST_READ_WRITE_BUFFER;
      else
        return DeviceMemoryClass::HOST_RESIDENT_HOST_READ_ONLY_BUFFER;
    }
    return DeviceMemoryClass::DEVICE_RESIDENT_HOST_WRITE_ONLY_BUFFER;
  }
  bool isStagingBuffer() const
  {
    // if it can not be used for anything and its read/write its a staging buffer (dx11 driver requires
    // r/w so we do it too).
    return (0 == (bufFlags & SBCF_BIND_MASK)) && (SBCF_CPU_ACCESS_WRITE | SBCF_CPU_ACCESS_READ) == (SBCF_CPU_ACCESS_MASK & bufFlags);
  }
  bool stagingBufferIsPermanent() const
  {
    // TODO: reevaluate this, this is always false
    // - see getMemoryClass
    return 0 != (bufFlags & SBCF_DYNAMIC);
  }

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

  void markDataUpdate();
  bool updatedInCurrentWorkItem();

  void markAsyncCopyInProgress();
  void markAsyncCopyFinished();
  void blockingReadbackWait();
  template <typename RbStarter, typename RbFinisher>
  void lockReadback(void **ptr, RbStarter startReadback, RbFinisher finishReadback)
  {
    if (ptr)
    {
      if (!asyncCopyInProgress)
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

public:
  GenericBufferInterface(uint32_t struct_size, uint32_t element_count, uint32_t flags, FormatStore format, const char *stat_name);
  ~GenericBufferInterface();

  int ressize() const override;
  int getFlags() const override;
  void destroy() override;
  int unlock() override;
  int lock(unsigned ofs_bytes, unsigned size_bytes, void **ptr, int flags) override;
  int getElementSize() const override;
  int getNumElements() const override;
  bool copyTo(Sbuffer *dst) override;
  bool copyTo(Sbuffer *dst, uint32_t dst_offset, uint32_t src_offset, uint32_t size_bytes) override;
  bool updateData(uint32_t ofs_bytes, uint32_t size_bytes, const void *__restrict src, uint32_t lockFlags) override;

  Buffer *getDeviceBuffer() const { return buffer; }
  VkIndexType getIndexType() const { return bufFlags & SBCF_INDEX32 ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16; }
};
void free_buffer(GenericBufferInterface *buffer);
} // namespace drv3d_vulkan