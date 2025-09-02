// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "buffer_components.h"

namespace drv3d_dx12::resource_manager
{

eastl::pair<BufferHeap::Heap *, ValueRange<uint64_t>> BufferHeap::BufferHeapState::tryAllocateFromReadyList(
  ResourceHeapProperties properties, uint64_t size, D3D12_RESOURCE_FLAGS flags, uint64_t offset_alignment, bool allow_offset)
{
  Heap *selectedHeap = nullptr;
  ValueRange<uint64_t> allocationRange;
  // Do backward search so we always use the most recently added and we have a chance to free
  // older stuff. Also erase is a bit more efficient.
  auto ref = eastl::find_if(rbegin(bufferHeapDiscardStandbyList), rend(bufferHeapDiscardStandbyList),
    [this, size, flags, properties, offset_alignment, allow_offset](auto info) {
      auto &heap = bufferHeaps[info.index];
      if (flags != heap.getFlags())
      {
        return false;
      }
      ResourceHeapProperties memoryProperties;
      memoryProperties.raw = heap.getHeapID().group;
      if (properties != memoryProperties)
      {
        return false;
      }
      if (!allow_offset && 0 != info.range.start)
      {
        return false;
      }
      if (size != info.range.size())
      {
        return false;
      }
      if (0 != (info.range.start % offset_alignment))
      {
        return false;
      }
      return true;
    });
  if (rend(bufferHeapDiscardStandbyList) != ref)
  {
    selectedHeap = &bufferHeaps[ref->index];
    allocationRange = ref->range;
    bufferHeapDiscardStandbyList.erase(ref);
  }
  return {selectedHeap, allocationRange};
}

eastl::pair<BufferHeap::Heap *, ValueRange<uint64_t>> BufferHeap::BufferHeapState::trySuballocateFromExistingHeaps(
  ResourceHeapProperties properties, uint64_t size, D3D12_RESOURCE_FLAGS flags, uint32_t offset_alignment)
{
  return suballocator.trySuballocate(bufferHeaps, properties, size, flags, offset_alignment);
}

BufferGlobalId BufferHeap::BufferHeapState::adoptBufferHeap(Heap &&heap)
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

  suballocator.onHeapCreated(bufferHeaps[result.index()]);
  return result;
}

size_t BufferHeap::BufferHeapState::freeBufferHeap(BufferHeap *manager, uint32_t index, const char *name,
  bool update_defragmentation_generation)
{
  auto &heap = bufferHeaps[index];
  suballocator.onHeapDestroyed(heap);
  size_t result = 0 == heap.getHeapID().isAlias ? heap.getBufferMemorySize() : 0;
  ResourceHeapProperties memoryProperties;
  memoryProperties.raw = heap.getHeapID().group;
  manager->recordBufferHeapFreed(result, !memoryProperties.isCPUVisible(), name);
  heap.reset(manager, update_defragmentation_generation);
  freeBufferSlots.push_back(index);
  return result;
}

size_t BufferHeap::BufferHeapState::freeBufferHeap(BufferHeap *manager, uint32_t index, ValueRange<uint64_t> range, const char *name)
{
  if (!bufferHeaps[index].free(range))
  {
    return 0;
  }
  return freeBufferHeap(manager, index, name, false);
}

eastl::pair<BufferGlobalId, HRESULT> BufferHeap::BufferHeapState::createBufferHeap(BufferHeap *manager, DXGIAdapter *adapter,
  ID3D12Device *device, uint64_t allocation_size, ResourceHeapProperties properties, D3D12_RESOURCE_FLAGS flags,
  D3D12_RESOURCE_STATES initial_state, const char *name, AllocationFlags allocation_flags)
{
  BufferGlobalId result;
  Heap newHeap;

  D3D12_RESOURCE_DESC desc;
  desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
  desc.Width = allocation_size;
  desc.Height = 1;
  desc.DepthOrArraySize = 1;
  desc.MipLevels = 1;
  desc.Format = DXGI_FORMAT_UNKNOWN;
  desc.SampleDesc.Count = 1;
  desc.SampleDesc.Quality = 0;
  desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  desc.Flags = flags;

  D3D12_RESOURCE_ALLOCATION_INFO allocInfo;
  allocInfo.SizeInBytes = desc.Width;
  allocInfo.Alignment = desc.Alignment;

  const auto allocation = manager->allocate(adapter, device, properties, allocInfo, allocation_flags);
  const ResourceHeapProperties allocatedProperties{allocation.getHeapID().group};

  if (!allocation)
  {
    return {result, E_OUTOFMEMORY};
  }

  auto errorCode = newHeap.create(device, desc, allocation, initial_state, allocatedProperties.isCPUVisible());
  if (DX12_CHECK_FAIL(errorCode))
  {
    manager->free(allocation, allocation_flags.test(AllocationFlag::DEFRAGMENTATION_OPERATION));
    return {result, errorCode};
  }

  newHeap.init(allocation_size);
  newHeap.setFlags(flags);

  result = adoptBufferHeap(eastl::move(newHeap));
  G_ASSERT(result);

  char strBuf[64];
  if (!name)
  {
    sprintf_s(strBuf, "Buffer#%u", result.index());
    name = strBuf;
  }
  manager->recordBufferHeapAllocated(allocation_size, !allocatedProperties.isCPUVisible(), name);
  manager->updateMemoryRangeUse(allocation, result);

  return {result, S_OK};
}

bool BufferHeap::BufferHeapState::isValidBuffer(const BufferState &buf)
{
  auto resIndex = buf.resourceId.index();
  return (resIndex < bufferHeaps.size() && bufferHeaps[resIndex].getResourcePtr() == buf.buffer);
}

size_t BufferHeap::BufferHeapState::freeBuffer(BufferHeap *manager, const BufferState &buf, BufferHeap::FreeReason free_reason,
  uint64_t progress, const char *name)
{
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
  }
  if (shouldFreeBuffer(heap, free_reason))
  {
    memoryFreedSize += freeBufferHeap(manager, resIndex, buf.usageRange(), name);
  }
  else
  {
    StandbyInfo info;
    info.progress = progress;
    info.index = resIndex;
    info.range = buf.usageRange();
    bufferHeapDiscardStandbyList.push_back(info);
  }
  return memoryFreedSize;
}

bool BufferHeap::BufferHeapState::moveStandbyBufferEntries(BufferGlobalId old_buffer, BufferGlobalId new_buffer)
{
  auto oldBufferIndex = old_buffer.index();
  auto newBufferIndex = new_buffer.index();

  auto &oldBuffer = bufferHeaps[oldBufferIndex];
  auto &newBuffer = bufferHeaps[newBufferIndex];

  uint32_t totalSize = 0;
  for (auto &si : bufferHeapDiscardStandbyList)
  {
    if (si.index != oldBufferIndex)
    {
      continue;
    }
    totalSize += si.range.size();
    oldBuffer.free(si.range);
    newBuffer.rangeAllocate(si.range);
    si.index = newBufferIndex;
  }

  return totalSize == oldBuffer.getBufferMemorySize();
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
      heap.reset(provider);
    }
  }

  bufferHeaps.clear();
  bufferHeapDiscardStandbyList.clear();
  freeBufferSlots.clear();

  return totalMemoryFreed;
}

size_t BufferHeap::BufferHeapState::cleanupDiscardStandbyList(BufferHeap *manager, uint64_t progressIndex)
{
  size_t memoryFreedSize = 0;

  bufferHeapDiscardStandbyList.erase(eastl::remove_if(begin(bufferHeapDiscardStandbyList), end(bufferHeapDiscardStandbyList),
                                       [this, &memoryFreedSize, manager, pi = progressIndex](auto info) //
                                       {
                                         if (info.progress + standby_max_progress_age > pi)
                                         {
                                           return false;
                                         }
                                         memoryFreedSize += freeBufferHeap(manager, info.index, info.range, "<ready list>");
                                         return true;
                                       }),
    end(bufferHeapDiscardStandbyList));

  return memoryFreedSize;
}

bool BufferHeap::BufferHeapState::shouldFreeBuffer(const Heap &heap, BufferHeap::FreeReason free_reason)
{
  if (heap.canSubAllocateFrom())
  {
    return FreeReason::USER_DELETE == free_reason;
  }
  else
  {
    return FreeReason::DISCARD_DIFFERENT_FRAME != free_reason;
  }
}

} // namespace drv3d_dx12::resource_manager