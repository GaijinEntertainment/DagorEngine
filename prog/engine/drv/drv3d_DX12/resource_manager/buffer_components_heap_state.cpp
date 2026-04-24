// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "buffer_components.h"

namespace drv3d_dx12::resource_manager
{

eastl::pair<BufferHeap::Heap *, ValueRange<uint64_t>> BufferHeap::BufferHeapState::trySuballocateFromExistingHeaps(
  ResourceHeapProperties properties, uint64_t size, D3D12_RESOURCE_FLAGS flags, uint32_t offset_alignment)
{
  return suballocator.trySuballocate(bufferHeaps, properties, size, flags, offset_alignment);
}

BufferGlobalId BufferHeap::BufferHeapState::adoptBufferHeap(Heap &&heap, bool can_suballocate)
{
  BufferGlobalId result;
  if (freeBufferSlots.empty())
  {
    heap.setResId(BufferGlobalId{static_cast<uint32_t>(bufferHeaps.size())});
    result = heap.getResId();
    bufferHeaps.push_back(eastl::move(heap));
  }
  else
  {
    heap.setResId(BufferGlobalId{freeBufferSlots.back()});
    freeBufferSlots.pop_back();
    result = heap.getResId();
    bufferHeaps[result.index()] = eastl::move(heap);
  }

  suballocator.onHeapCreated(bufferHeaps[result.index()], can_suballocate);
  return result;
}

size_t BufferHeap::BufferHeapState::freeBufferHeap(BufferHeap *manager, uint32_t index, const char *name, bool is_heaps_lock_required)
{
  auto &heap = bufferHeaps[index];
  suballocator.onHeapDestroyed(heap);
  size_t result = 0 == heap.getHeapID().isAlias ? heap.getBufferMemorySize() : 0;
  ResourceHeapProperties memoryProperties;
  memoryProperties.raw = heap.getHeapID().group;
  manager->recordBufferHeapFreed(result, memoryProperties.isOnDevice(manager->getFeatureSet()), name);
  heap.reset(manager, is_heaps_lock_required);
  freeBufferSlots.push_back(index);
  return result;
}

size_t BufferHeap::BufferHeapState::freeBufferHeap(BufferHeap *manager, uint32_t index, ValueRange<uint64_t> range, const char *name)
{
  if (!bufferHeaps[index].free(range))
  {
    return 0;
  }
  return freeBufferHeap(manager, index, name, true);
}

eastl::pair<BufferGlobalId, HRESULT> BufferHeap::BufferHeapState::createBufferHeapInMemory(BufferHeap *manager, ID3D12Device *device,
  uint64_t allocation_size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initial_state, const D3D12_RESOURCE_DESC &desc,
  const ResourceMemory &allocation, ResourceHeapProperties allocatedProperties, bool can_suballocate)
{
  BufferGlobalId result;
  Heap newHeap;
  auto errorCode = newHeap.create(device, desc, allocation, initial_state, allocatedProperties.isCPUVisible(manager->getFeatureSet()));
  if (DX12_CHECK_FAIL(errorCode))
  {
    return {result, errorCode};
  }

  newHeap.init(allocation_size);
  newHeap.setFlags(flags);

  result = adoptBufferHeap(eastl::move(newHeap), can_suballocate);
  G_ASSERT(result);

  return {result, S_OK};
}

eastl::pair<BufferGlobalId, HRESULT> BufferHeap::BufferHeapState::createBufferHeap(BufferHeap *manager, DXGIAdapter *adapter,
  ID3D12Device *device, uint64_t allocation_size, ResourceHeapProperties properties, D3D12_RESOURCE_FLAGS flags,
  D3D12_RESOURCE_STATES initial_state, const char *name, bool can_suballocate, AllocationFlags allocation_flags)
{
  BufferGlobalId buffer;
  Heap newHeap;

  auto [desc, allocInfo] = calculate_buffer_desc_allocation_info(device, allocation_size, flags);

  const auto allocation = manager->allocate(adapter, device, properties, allocInfo, allocation_flags);
  const ResourceHeapProperties allocatedProperties{allocation.getHeapID().group};

  if (!allocation)
  {
    return {buffer, E_OUTOFMEMORY};
  }

  HRESULT result = S_OK;
  eastl::tie(buffer, result) = createBufferHeapInMemory(manager, device, allocInfo.SizeInBytes, flags, initial_state, desc, allocation,
    allocatedProperties, can_suballocate);
  if (FAILED(result))
  {
    manager->free(allocation);
    return {buffer, result};
  }

  char strBuf[64];
  if (!name)
  {
    sprintf_s(strBuf, "Buffer#%u", buffer.index());
    name = strBuf;
  }
  manager->recordBufferHeapAllocated(allocation_size, allocatedProperties.isOnDevice(manager->getFeatureSet()), name);
  manager->updateMemoryRangeUse(allocation, buffer);

  return {buffer, S_OK};
}

eastl::pair<D3D12_RESOURCE_DESC, D3D12_RESOURCE_ALLOCATION_INFO> BufferHeap::calculate_buffer_desc_allocation_info(
  ID3D12Device *device, uint64_t allocation_size, D3D12_RESOURCE_FLAGS flags)
{
  D3D12_RESOURCE_DESC desc;
  desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
#if _TARGET_XBOX
  desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
#else
  desc.Alignment = (flags & D3D12_RESOURCE_FLAG_USE_TIGHT_ALIGNMENT) ? 0 : D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
#endif
  desc.Width = allocation_size;
  desc.Height = 1;
  desc.DepthOrArraySize = 1;
  desc.MipLevels = 1;
  desc.Format = DXGI_FORMAT_UNKNOWN;
  desc.SampleDesc.Count = 1;
  desc.SampleDesc.Quality = 0;
  desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  desc.Flags = flags;

  D3D12_RESOURCE_ALLOCATION_INFO allocInfo = device->GetResourceAllocationInfo(0, 1, &desc);
  if (allocInfo.SizeInBytes == UINT64_MAX)
  {
    allocInfo.SizeInBytes = desc.Width;
    allocInfo.Alignment = max(desc.Alignment, 256ull);
  }

  return {desc, allocInfo};
}

bool BufferHeap::BufferHeapState::isValidBuffer(const BufferState &buf)
{
  auto resIndex = buf.resourceId.index();
  return (resIndex < bufferHeaps.size() && bufferHeaps[resIndex].getResourcePtr() == buf.buffer);
}

size_t BufferHeap::BufferHeapState::freeBuffer(BufferHeap *manager, const BufferState &buf, BufferHeap::FreeReason free_reason,
  const char *name)
{
  G_UNUSED(free_reason);
  size_t memoryFreedSize = 0;
  auto resIndex = buf.resourceId.index();

  if (!isValidBuffer(buf))
  {
    logwarn("DX12: freeBuffer: Provided buffer %p and id %u does not match stored buffer %u (%u)", buf.buffer, buf.resourceId,
      resIndex < bufferHeaps.size() ? bufferHeaps[resIndex].getResourcePtr() : nullptr, bufferHeaps.size());
    return 0;
  }

  auto &heap = bufferHeaps[resIndex];
  if (0 != heap.getHeapID().isAlias)
  {
    G_ASSERTF(FreeReason::USER_DELETE == free_reason, "DX12: Buffer %s was discarded but was a "
                                                      "aliased / placed buffer, this is a "
                                                      "invalid use case!");
    // Alias/placed buffers own the entire heap entry; bypass the range-based free
    // which would fail because usageRange (logical size) < bufferMemory.size (allocation size).
    memoryFreedSize += freeBufferHeap(manager, resIndex, name, true);
  }
  else
  {
    memoryFreedSize += freeBufferHeap(manager, resIndex, buf.usageRange(), name);
  }
  return memoryFreedSize;
}

size_t BufferHeap::BufferHeapState::clear(ResourceMemoryHeapProvider *provider)
{
  // placed here to avoid massive Heap::free processing
  suballocator.clear();

  size_t totalMemoryFreed = 0;
  for (auto &heap : bufferHeaps)
  {
    if (heap)
    {
      // we don't count aliases now
      totalMemoryFreed += heap.getHeapID().isAlias == 0 ? heap.getBufferMemorySize() : 0;
      heap.reset(provider, true);
    }
  }

  bufferHeaps.clear();
  freeBufferSlots.clear();

  return totalMemoryFreed;
}

} // namespace drv3d_dx12::resource_manager