// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "host_shared_components.h"
#include <device.h>


namespace drv3d_dx12::resource_manager
{
HostDeviceSharedMemoryRegion FramePushRingMemoryProvider::allocatePushMemory(DXGIAdapter *adapter, Device &device, uint32_t size,
  uint32_t alignment)
{
  auto result = pushRing.access()->allocate(this, adapter, device.getDevice(), size, alignment);
  // Here is a dangerous place for emergency defragmentation. We have a lot of push memory allocations from drv internal code (like
  // flushGraphics). It makes such call here deadlock and gpu-crash prone. So I think it is better to try to handle OOM here in another
  // way.
  if (checkForOOM(adapter, static_cast<bool>(result),
        OomReportData{"allocatePushMemory", nullptr, size, AllocationFlags{}.to_ulong(), getPushHeapProperties().raw}))
  {
    recordConstantRingUsed(size);
  }
  return result;
}

HostDeviceSharedMemoryRegion UploadRingMemoryProvider::allocateUploadRingMemory(DXGIAdapter *adapter, Device &device, uint32_t size,
  uint32_t alignment)
{
  auto result = uploadRing.access()->allocate(this, adapter, device.getDevice(), size, alignment);
  if (!result)
  {
    device.processEmergencyDefragmentation(getPushHeapProperties().raw, true, false, false);
    result = uploadRing.access()->allocate(this, adapter, device.getDevice(), size, alignment);
  }
  if (checkForOOM(adapter, static_cast<bool>(result),
        OomReportData{"allocateUploadRingMemory", nullptr, size, AllocationFlags{}.to_ulong(), getPushHeapProperties().raw}))
  {
    recordUploadRingUsed(size);
  }
  return result;
}

HostDeviceSharedMemoryRegion TemporaryUploadMemoryProvider::allocateTempUpload(DXGIAdapter *adapter, Device &device, size_t size,
  size_t alignment, bool &should_flush)
{
  should_flush = false;
  HRESULT errorCode = S_OK;
  const auto properties = getProperties(D3D12_RESOURCE_FLAG_NONE, TemporaryUploadMemoryImeplementation::memory_class,
    D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
  auto oomCheckOnExit = checkForOOMOnExit(
    adapter, [&errorCode]() { return !is_oom_error_code(errorCode); },
    OomReportData{"allocateTempUpload", nullptr, size, AllocationFlags{}.to_ulong(), properties.raw});
  HostDeviceSharedMemoryRegion result = tryAllocateTempUpload(adapter, device.getDevice(), size, alignment, should_flush, errorCode);
  if (!result)
  {
    device.processEmergencyDefragmentation(properties.raw, true, false, false);
    errorCode = S_OK;
    result = tryAllocateTempUpload(adapter, device.getDevice(), size, alignment, should_flush, errorCode);
  }
  recordTempBufferUsed(result.range.size());
  return result;
}

HostDeviceSharedMemoryRegion TemporaryUploadMemoryProvider::tryAllocateTempUploadForUploadBuffer(DXGIAdapter *adapter,
  ID3D12Device *device, size_t size, size_t alignment, HRESULT errorCode)
{
  HostDeviceSharedMemoryRegion result;
  auto tempBufferAccess = tempBuffer.access();
  if (tempBufferAccess->uploadBufferUsage > tempBufferAccess->uploadBufferUsageLimit)
  {
    ByteUnits currentUsage{tempBufferAccess->uploadBufferUsage};
    ByteUnits usageLimit{tempBufferAccess->uploadBufferUsageLimit};
    ByteUnits reqSize{size};
    // Out of budget, return empty region
    logdbg("DX12: Out of upload buffer pool, usage %.2f %s of %.2f %s, while trying to allocate "
           "%.2f %s",
      currentUsage.units(), currentUsage.name(), usageLimit.units(), usageLimit.name(), reqSize.units(), reqSize.name());
    return result;
  }
  eastl::tie(result, errorCode) = tempBufferAccess->allocate(this, adapter, device, size, alignment);
  if (!result)
  {
    // if allocate fails for some reason we just return the empty region
    return result;
  }
  tempBufferAccess->uploadBufferUsage += result.range.size();
  return result;
}

HostDeviceSharedMemoryRegion TemporaryUploadMemoryProvider::allocateTempUploadForUploadBuffer(DXGIAdapter *adapter, Device &device,
  size_t size, size_t alignment)
{
  HRESULT errorCode = S_OK;
  const auto properties = getProperties(D3D12_RESOURCE_FLAG_NONE, TemporaryUploadMemoryImeplementation::memory_class,
    D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
  auto oomCheckOnExit = checkForOOMOnExit(
    adapter, [&errorCode]() { return !is_oom_error_code(errorCode); },
    OomReportData{"allocateTempUploadForUploadBuffer", nullptr, size, AllocationFlags{}.to_ulong(), properties.raw});
  HostDeviceSharedMemoryRegion result = tryAllocateTempUploadForUploadBuffer(adapter, device.getDevice(), size, alignment, errorCode);
  if (is_oom_error_code(errorCode))
  {
    device.processEmergencyDefragmentation(properties.raw, true, false, false);
    errorCode = S_OK;
    result = tryAllocateTempUploadForUploadBuffer(adapter, device.getDevice(), size, alignment, errorCode);
  }
  if (!result)
    return result;
  recordTempBufferUsed(result.range.size());
  return result;
}

HostDeviceSharedMemoryRegion PersistentUploadMemoryProvider::allocatePersistentUploadMemory(DXGIAdapter *adapter, Device &device,
  size_t size, size_t alignment)
{
  auto result = uploadMemory.access()->allocate(this, adapter, device.getDevice(), size, alignment);
  const auto properties = getProperties(D3D12_RESOURCE_FLAG_NONE, PersistentUploadMemoryImplementation::memory_class,
    D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
  if (!result)
  {
    device.processEmergencyDefragmentation(properties.raw, true, false, false);
    result = uploadMemory.access()->allocate(this, adapter, device.getDevice(), size, alignment);
  }
  if (checkForOOM(adapter, static_cast<bool>(result),
        OomReportData{"allocatePersistentUploadMemory", nullptr, size, AllocationFlags{}.to_ulong(), properties.raw}))
  {
    recordPersistentUploadMemoryAllocated(size);
  }
  return result;
}

HostDeviceSharedMemoryRegion PersistentReadBackMemoryProvider::allocatePersistentReadBack(DXGIAdapter *adapter, Device &device,
  size_t size, size_t alignment)
{
  auto result = readBackMemory.access()->allocate(this, adapter, device.getDevice(), size, alignment);
  const auto properties = getProperties(D3D12_RESOURCE_FLAG_NONE, PersistentReadBackMemoryImplementation::memory_class,
    D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
  if (!result)
  {
    device.processEmergencyDefragmentation(properties.raw, true, false, false);
    result = readBackMemory.access()->allocate(this, adapter, device.getDevice(), size, alignment);
  }
  if (checkForOOM(adapter, static_cast<bool>(result),
        OomReportData{"allocatePersistentReadBack", nullptr, size, AllocationFlags{}.to_ulong(), properties.raw}))
  {
    recordPersistentReadBackMemoryAllocated(size);
  }
  return result;
}

HostDeviceSharedMemoryRegion PersistentBidirectionalMemoryProvider::allocatePersistentBidirectional(DXGIAdapter *adapter,
  Device &device, size_t size, size_t alignment)
{
  auto result = bidirectionalMemory.access()->allocate(this, adapter, device.getDevice(), size, alignment);
  const auto properties = getProperties(D3D12_RESOURCE_FLAG_NONE, PersistentBidirectionalMemoryImplementation::memory_class,
    D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
  if (!result)
  {
    device.processEmergencyDefragmentation(properties.raw, true, false, false);
    result = bidirectionalMemory.access()->allocate(this, adapter, device.getDevice(), size, alignment);
  }
  if (checkForOOM(adapter, static_cast<bool>(result),
        OomReportData{"allocatePersistentReadBack", nullptr, size, AllocationFlags{}.to_ulong(), properties.raw}))
  {
    recordPersistentBidirectionalMemoryAllocated(size);
  }
  return result;
}
} // namespace drv3d_dx12::resource_manager
