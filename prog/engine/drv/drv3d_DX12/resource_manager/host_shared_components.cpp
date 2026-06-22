// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "host_shared_components.h"
#include <device.h>

namespace drv3d_dx12::resource_manager
{
HostDeviceSharedMemoryRegion FramePushRingMemoryProvider::allocatePushMemory(DXGIAdapter *adapter, Device &device, uint32_t size,
  uint32_t alignment)
{
  G_UNUSED(adapter);
  G_UNUSED(device);
  auto result = pushRing.access()->allocate(size, alignment);
  if (result)
  {
    recordConstantRingUsed(size);
  }
  return result;
}

HostDeviceSharedMemoryRegion TemporaryUploadMemoryProvider::allocatePushMemory(DXGIAdapter *adapter, Device &device, uint32_t size,
  uint32_t alignment)
{
  static constexpr uint32_t large_push_threshold = 2 * 1024 * 1024;
  // Small allocations try the push ring first; large ones skip it to avoid wasting ring space.
  if (size <= large_push_threshold)
  {
    auto result = BaseType::allocatePushMemory(adapter, device, size, alignment);
    if (result)
    {
      return result;
    }
  }

  const auto properties = getProperties(D3D12_RESOURCE_FLAG_NONE, TemporaryUploadMemoryImeplementation::memory_class,
    D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

  bool shouldFlush = false;
  return tryAllocateTempUpload(adapter, device.getDevice(), size, alignment, shouldFlush)
    .or_else([&, this](auto) {
      device.processEmergencyDefragmentation(

        properties.raw, true, false, false, size, false);
      return tryAllocateTempUpload(adapter, device.getDevice(), size, alignment, shouldFlush);
    })
    .transform([&, this](auto value) {
      recordTempBufferUsed(value.range.size());

      // Record for automatic cleanup on frame completion, as push memory callers do not free explicitly.
      accessRecodingPendingFrameCompletion<PendingForCompletedFrameData>(
        [buffer = value.buffer, allocSize = value.range.size()](auto &data) {
          data.uploadBufferRefs.push_back(buffer);
          data.tempUsage += allocSize;
        });
      return value;
    })
    .or_else([&, this](auto error) -> HostDeviceSharedMemoryRegionAllocationResult {
      checkForOOM(adapter, error,
        OomReportData{"allocatePushMemory(large)", nullptr, size, AllocationFlags{}.toUlong(), properties.raw});
      return dag::Unexpected{error};
    })
    .value_or({});
}

HostDeviceSharedMemoryRegion TemporaryUploadMemoryProvider::allocateTempUpload(DXGIAdapter *adapter, Device &device, size_t size,
  size_t alignment, bool &should_flush)
{
  should_flush = false;

  const auto properties = getProperties(D3D12_RESOURCE_FLAG_NONE, TemporaryUploadMemoryImeplementation::memory_class,
    D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

  return tryAllocateTempUpload(adapter, device.getDevice(), size, alignment, should_flush)
    .or_else([&, this](auto) {
      device.processEmergencyDefragmentation(properties.raw, true, false, false, size, false);
      return tryAllocateTempUpload(adapter, device.getDevice(), size, alignment, should_flush);
    })
    .transform([&, this](auto value) {
      recordTempBufferUsed(value.range.size());
      return value;
    })
    // have to resort to use or_else instead of transform_error as some VC compiler versions just fail to lookup the constructor from
    // value
    .or_else([&, this](auto error) -> HostDeviceSharedMemoryRegionAllocationResult {
      checkForOOM(adapter, error, OomReportData{"allocateTempUpload", nullptr, size, AllocationFlags{}.toUlong(), properties.raw});
      return dag::Unexpected{error};
    })
    .value_or({});
}

TemporaryUploadMemoryProvider::HostDeviceSharedMemoryRegionAllocationResult TemporaryUploadMemoryProvider::
  tryAllocateTempUploadForUploadBuffer(DXGIAdapter *adapter, ID3D12Device *device, size_t size, size_t alignment)
{
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
    return dag::Unexpected<MemoryAllocationError>{{.errorCode = E_OUTOFMEMORY}};
  }

  auto allocationResult = tempBufferAccess->allocate(this, adapter, device, size, alignment);
  if (!allocationResult)
  {
    return allocationResult;
  }
  tempBufferAccess->uploadBufferUsage += allocationResult->range.size();
  return allocationResult;
}

HostDeviceSharedMemoryRegion TemporaryUploadMemoryProvider::allocateTempUploadForUploadBuffer(DXGIAdapter *adapter, Device &device,
  size_t size, size_t alignment)
{
  const auto properties = getProperties(D3D12_RESOURCE_FLAG_NONE, TemporaryUploadMemoryImeplementation::memory_class,
    D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

  return tryAllocateTempUploadForUploadBuffer(adapter, device.getDevice(), size, alignment)
    .or_else([&, this](auto) {
      device.processEmergencyDefragmentation(properties.raw, true, false, false, size, false);
      return tryAllocateTempUploadForUploadBuffer(adapter, device.getDevice(), size, alignment);
    })
    .transform([&, this](auto &&value) {
      recordTempBufferUsed(value.range.size());
      return value;
    })
    // have to resort to use or_else instead of transform_error as some VC compiler versions just fail to lookup the constructor from
    // value
    .or_else([&, this](auto error) -> HostDeviceSharedMemoryRegionAllocationResult {
      checkForOOM(adapter, error,
        OomReportData{"allocateTempUploadForUploadBuffer", nullptr, size, AllocationFlags{}.toUlong(), properties.raw});
      return dag::Unexpected{error};
    })
    .value_or({});
}

HostDeviceSharedMemoryRegion PersistentUploadMemoryProvider::allocatePersistentUploadMemory(DXGIAdapter *adapter, Device &device,
  size_t size, size_t alignment)
{
  auto result = uploadMemory.access()->allocate(this, adapter, device.getDevice(), size, alignment);
  const auto properties = getProperties(D3D12_RESOURCE_FLAG_NONE, PersistentUploadMemoryImplementation::memory_class,
    D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
  if (!result)
  {
    device.processEmergencyDefragmentation(properties.raw, true, false, false, size, false);
    result = uploadMemory.access()->allocate(this, adapter, device.getDevice(), size, alignment);
  }
  eastl::optional<MemoryAllocationError> errorInfo;
  if (!result)
  {
    errorInfo = MemoryAllocationError{.errorCode = E_OUTOFMEMORY};
  }
  if (checkForOOM(adapter, errorInfo,
        OomReportData{"allocatePersistentUploadMemory", nullptr, size, AllocationFlags{}.toUlong(), properties.raw}))
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
    device.processEmergencyDefragmentation(properties.raw, true, false, false, size, false);
    result = readBackMemory.access()->allocate(this, adapter, device.getDevice(), size, alignment);
  }
  eastl::optional<MemoryAllocationError> errorInfo;
  if (!result)
  {
    errorInfo = MemoryAllocationError{.errorCode = E_OUTOFMEMORY};
  }
  if (checkForOOM(adapter, errorInfo,
        OomReportData{"allocatePersistentReadBack", nullptr, size, AllocationFlags{}.toUlong(), properties.raw}))
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
    device.processEmergencyDefragmentation(properties.raw, true, false, false, size, false);
    result = bidirectionalMemory.access()->allocate(this, adapter, device.getDevice(), size, alignment);
  }
  eastl::optional<MemoryAllocationError> errorInfo;
  if (!result)
  {
    errorInfo = MemoryAllocationError{.errorCode = E_OUTOFMEMORY};
  }
  if (checkForOOM(adapter, errorInfo,
        OomReportData{"allocatePersistentReadBack", nullptr, size, AllocationFlags{}.toUlong(), properties.raw}))
  {
    recordPersistentBidirectionalMemoryAllocated(size);
  }
  return result;
}
} // namespace drv3d_dx12::resource_manager
