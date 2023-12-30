#pragma once

#include <EASTL/vector.h>
#include <3d/rayTrace/dag_drvRayTrace.h>
#include <osApiWrappers/dag_spinlock.h>
#include <generic/dag_objectPool.h>

#include "driver.h"
#include "container_mutex_wrapper.h"

#include "resource_manager/raytrace_acceleration_structure.h"
#include "resource_manager/buffer_components.h"


namespace drv3d_dx12
{

namespace resource_manager
{

#if D3D_HAS_RAY_TRACING
class RaytraceScratchBufferProvider : public BufferHeap
{
  using BaseType = BufferHeap;

protected:
  struct PendingForCompletedFrameData : BaseType::PendingForCompletedFrameData
  {
    eastl::vector<BasicBuffer> deletedRaytraceScratchBuffers;
  };

  RaytraceScratchBufferProvider() = default;
  ~RaytraceScratchBufferProvider() = default;
  RaytraceScratchBufferProvider(const RaytraceScratchBufferProvider &) = delete;
  RaytraceScratchBufferProvider &operator=(const RaytraceScratchBufferProvider &) = delete;
  RaytraceScratchBufferProvider(RaytraceScratchBufferProvider &&) = delete;
  RaytraceScratchBufferProvider &operator=(RaytraceScratchBufferProvider &&) = delete;

  ContainerMutexWrapper<BasicBuffer, OSSpinlock> scratchBuffer;

  void completeFrameExecution(const CompletedFrameExecutionInfo &info, PendingForCompletedFrameData &data)
  {
    for (auto &buffer : data.deletedRaytraceScratchBuffers)
    {
      recordRaytraceScratchBufferFreed(buffer.bufferMemory.size());
      buffer.reset(this);
    }
    data.deletedRaytraceScratchBuffers.clear();

    BaseType::completeFrameExecution(info, data);
  }

  void shutdown()
  {
    scratchBuffer.access()->reset(this);
    BaseType::shutdown();
  }

  void preRecovery()
  {
    scratchBuffer.access()->reset(this);
    BaseType::preRecovery();
  }

public:
  void ensureRaytraceScratchBufferSize(DXGIAdapter *adapter, ID3D12Device *device, uint64_t size)
  {
    HRESULT errorCode = S_OK;
    auto oomCheckOnExit = checkForOOMOnExit([&errorCode]() { return !is_oom_error_code(errorCode); }, "DX12: OOM during %s",
      "ensureRaytraceScratchBufferSize");

    auto scratchBufferAccess = scratchBuffer.access();
    if (scratchBufferAccess->bufferMemory.size() >= size)
    {
      return;
    }
    if (*scratchBufferAccess)
    {
      accessRecodingPendingFrameCompletion<PendingForCompletedFrameData>(
        [&](auto &data) { data.deletedRaytraceScratchBuffers.push_back(eastl::move(*scratchBufferAccess)); });

      *scratchBufferAccess = {};
    }

    D3D12_RESOURCE_DESC desc;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Width = align_value<uint64_t>(size, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_RESOURCE_ALLOCATION_INFO allocInfo;
    allocInfo.SizeInBytes = desc.Width;
    allocInfo.Alignment = desc.Alignment;

    auto properties = getProperties(D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, DeviceMemoryClass::DEVICE_RESIDENT_BUFFER,
      D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

    auto allocation = allocate(adapter, device, properties, allocInfo, {});

    if (!allocation)
    {
      errorCode = E_OUTOFMEMORY;
      return;
    }

    errorCode =
      scratchBufferAccess->create(device, desc, allocation, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, properties.isCPUVisible());
    if (DX12_CHECK_FAIL(errorCode))
    {
      free(allocation);
      return;
    }

    updateMemoryRangeUse(allocation, RaytraceScratchBufferReference{scratchBufferAccess->buffer.Get()});
    recordRaytraceScratchBufferAllocated(scratchBufferAccess->bufferMemory.size());
  }

  BufferResourceReferenceAndAddress getRaytraceScratchBuffer()
  {
    auto scratchBufferAccess = scratchBuffer.access();
    BufferResourceReferenceAndAddress result;
    result.buffer = scratchBufferAccess->buffer.Get();
    result.gpuPointer = scratchBufferAccess->getGPUPointer();
    return result;
  }
};

class RaytraceAccelerationStructureObjectProvider : public RaytraceScratchBufferProvider
{
  using BaseType = RaytraceScratchBufferProvider;

protected:
  ContainerMutexWrapper<ObjectPool<RaytraceAccelerationStructure>, OSSpinlock> raytraceAccelerationStructurePool;

  struct PendingForCompletedFrameData : BaseType::PendingForCompletedFrameData
  {
    eastl::vector<RaytraceAccelerationStructure *> deletedBottomAccelerationStructure;
    eastl::vector<RaytraceAccelerationStructure *> deletedTopAccelerationStructure;
  };

  void completeFrameExecution(const CompletedFrameExecutionInfo &info, PendingForCompletedFrameData &data)
  {
    {
      auto raytraceAccelerationStructurePoolAccess = raytraceAccelerationStructurePool.access();
      for (auto as : data.deletedTopAccelerationStructure)
      {
        freeBufferSRVDescriptor(as->handle);
        recordRaytraceTopStructureFreed(as->size());
        as->reset(this);
        raytraceAccelerationStructurePoolAccess->free(as);
      }
      for (auto as : data.deletedBottomAccelerationStructure)
      {
        recordRaytraceBottomStructureFreed(as->size());
        as->reset(this);
        raytraceAccelerationStructurePoolAccess->free(as);
      }
    }
    data.deletedTopAccelerationStructure.clear();
    data.deletedBottomAccelerationStructure.clear();

    BaseType::completeFrameExecution(info, data);
  }

  void shutdown()
  {
    BaseType::shutdown();
    raytraceAccelerationStructurePool.access()->freeAll();
  }

  void preRecovery()
  {
    BaseType::preRecovery();
    raytraceAccelerationStructurePool.access()->freeAll();
  }

  RaytraceAccelerationStructure *newRaytraceAccelerationStructure(DXGIAdapter *adapter, ID3D12Device *device, uint64_t size)
  {
    auto properties = getProperties(D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, DeviceMemoryClass::DEVICE_RESIDENT_BUFFER,
      D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

    D3D12_RESOURCE_DESC desc;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Width = align_value<uint64_t>(size, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_RESOURCE_ALLOCATION_INFO allocInfo;
    allocInfo.SizeInBytes = desc.Width;
    allocInfo.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

    RaytraceAccelerationStructure *newStruct = nullptr;
    HRESULT errorCode = S_OK;
    auto oomCheckOnExit = checkForOOMOnExit(
      [&errorCode, &newStruct]() { return (!is_oom_error_code(errorCode)) && newStruct && newStruct->getResourceHandle(); },
      "DX12: OOM during %s", "newRaytraceAccelerationStructure");

    auto memory = allocate(adapter, device, properties, allocInfo, {});
    if (!memory)
    {
      errorCode = E_OUTOFMEMORY;
      return newStruct;
    }

    newStruct = raytraceAccelerationStructurePool.access()->allocate();
    errorCode =
      newStruct->create(device, desc, memory, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, properties.isCPUVisible());
    if (DX12_CHECK_FAIL(errorCode))
    {
      raytraceAccelerationStructurePool.access()->free(newStruct);
      free(memory);
      newStruct = nullptr;
    }

    return newStruct;
  }

public:
  RaytraceAccelerationStructure *newRaytraceTopAccelerationStructure(DXGIAdapter *adapter, ID3D12Device *device, uint64_t size)
  {
    auto result = newRaytraceAccelerationStructure(adapter, device, size);
    if (result)
    {
      D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
      desc.Format = DXGI_FORMAT_UNKNOWN;
      desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
      desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
      desc.RaytracingAccelerationStructure.Location = result->getGPUPointer();
      result->handle = allocateBufferSRVDescriptor(device);
      device->CreateShaderResourceView(nullptr /*must be null*/, &desc, result->handle);
      updateMemoryRangeUse(result->getMemory(), RaytraceTopLevelAccelerationStructureRefnerence{result});
      recordRaytraceTopStructureAllocated(size);
    }
    return result;
  }

  RaytraceAccelerationStructure *newRaytraceBottomAccelerationStructure(DXGIAdapter *adapter, ID3D12Device *device, uint64_t size)
  {
    auto result = newRaytraceAccelerationStructure(adapter, device, size);
    if (result)
    {
      updateMemoryRangeUse(result->getMemory(), RaytraceBottomLevelAccelerationStructureRefnerence{result});
      recordRaytraceBottomStructureAllocated(size);
    }
    return result;
  }

  void deleteRaytraceTopAccelerationStructureOnFrameCompletion(RaytraceAccelerationStructure *ras)
  {
    accessRecodingPendingFrameCompletion<PendingForCompletedFrameData>(
      [=](auto &data) { data.deletedTopAccelerationStructure.push_back(ras); });
  }
  void deleteRaytraceBottomAccelerationStructureOnFrameCompletion(RaytraceAccelerationStructure *ras)
  {
    accessRecodingPendingFrameCompletion<PendingForCompletedFrameData>(
      [=](auto &data) { data.deletedBottomAccelerationStructure.push_back(ras); });
  }
};
#else
using RaytraceScratchBufferProvider = BufferHeap;
using RaytraceAccelerationStructureObjectProvider = RaytraceScratchBufferProvider;
#endif

} // namespace resource_manager
} // namespace drv3d_dx12
