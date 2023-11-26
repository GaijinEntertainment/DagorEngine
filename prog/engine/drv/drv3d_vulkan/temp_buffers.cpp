#include "temp_buffers.h"
#include "device.h"

using namespace drv3d_vulkan;

void TempBufferManager::setConfig(VkDeviceSize size, uint32_t alignment, uint8_t index)
{
  G_ASSERT(((alignment - 1) & alignment) == 0);
  pow2Alignment = max<VkDeviceSize>(get_device().getDeviceProperties().properties.limits.optimalBufferCopyOffsetAlignment, alignment);
  maxAllocSize = size;
  managerIndex = index;
}

BufferSubAllocation TempBufferManager::allocate(Device &device, uint32_t unalignedSize)
{
  WinAutoLock lock(writeLock);

  uint32_t size = (unalignedSize + (pow2Alignment - 1)) & ~(pow2Alignment - 1);

  auto ref = eastl::find_if(begin(buffers), end(buffers),
    [=](const TempBufferInfo &info) //
    {
      auto space = info.buffer->getBlockSize() - info.fill;
      return space >= size;
    });
  if (ref == end(buffers))
  {
    // rescale max allocation size to store at least size and some extra
    if (size >= maxAllocSize)
      maxAllocSize *= (size / maxAllocSize) + 1;
    TempBufferInfo info;
    info.buffer =
      device.createBuffer(maxAllocSize, DeviceMemoryClass::DEVICE_RESIDENT_HOST_WRITE_ONLY_BUFFER, 1, BufferMemoryFlags::TEMP);
    if (get_device().getDebugLevel() > 0)
      info.buffer->setDebugName(String(128, "internal temp buffer type %u", managerIndex));
    ref = buffers.insert(end(buffers), info);
  }
  BufferSubAllocation result;
  result.buffer = ref->buffer;
  result.offset = ref->fill;
  result.size = size;
  ref->fill += size;
  currentAllocSize += size;
  return result;
}
