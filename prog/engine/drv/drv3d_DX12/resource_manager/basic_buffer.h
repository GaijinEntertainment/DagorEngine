// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "heap_components.h"
#include <d3d12_error_handling.h>
#include <driver.h>
#include <resource_memory.h>

#include <supp/dag_comPtr.h>


namespace drv3d_dx12::resource_manager
{
// Basic buffer with some common stuff, like handling differences in memory model between PC and consoles.
class BasicBuffer
{
protected:
  ComPtr<ID3D12Resource> buffer;
  ResourceMemoryWithGPUAndCPUAddress bufferMemory;

public:
  void setBufferMemory(const ResourceMemoryWithGPUAndCPUAddress &in_buffer_memory) { bufferMemory = in_buffer_memory; }
  const ResourceMemoryWithGPUAndCPUAddress &getBufferMemory() const { return bufferMemory; }

  D3D12_GPU_VIRTUAL_ADDRESS getGPUPointer() const { return bufferMemory.gpuAddress; }
  uint8_t *getCPUPointer() const { return bufferMemory.cpuAddress; }

  uint64_t getBufferMemorySize() const { return bufferMemory.size; }
  HeapID getHeapID() const { return bufferMemory.heapID; }

  ID3D12Resource *getResourcePtr() const { return buffer.Get(); }

  HRESULT create(ID3D12Device *device, const D3D12_RESOURCE_DESC &desc, ResourceMemory mem, D3D12_RESOURCE_STATES initial_state,
    bool map)
  {
    HRESULT errorCode = S_OK;
#if _TARGET_XBOX
    errorCode =
      DX12_CHECK_RESULT_NO_OOM_CHECK(xbox_create_placed_resource(device, mem.getAddress(), desc, initial_state, nullptr, buffer));
    if (DX12_CHECK_FAIL(errorCode))
    {
      return errorCode;
    }
#else
    errorCode = DX12_CHECK_RESULT_NO_OOM_CHECK(
      device->CreatePlacedResource(mem.getHeap(), mem.getOffset(), &desc, initial_state, nullptr, COM_ARGS(&buffer)));
    if (DX12_CHECK_FAIL(errorCode))
    {
      return errorCode;
    }
#endif
    bufferMemory.initializeFrom(mem, buffer.Get(), map);
    return errorCode;
  }

  void reset(ResourceMemoryHeapProvider *heap, bool update_defragmentation_generation = false)
  {
    if (bufferMemory && 0 == bufferMemory.heapID.isAlias)
    {
      heap->free(bufferMemory, update_defragmentation_generation);
      bufferMemory = {};
    }
    buffer.Reset();
  }

  explicit operator bool() const { return static_cast<bool>(buffer); }
};

} // namespace drv3d_dx12::resource_manager
