// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "driver.h"

#include <debug/dag_assert.h>
#include <value_range.h>


namespace drv3d_dx12
{

union HeapID
{
  using ValueType = uint32_t;
  static constexpr uint32_t alias_bits = 1;
  static constexpr uint32_t group_bits = 4;
  static constexpr uint32_t index_bits = (8 * sizeof(ValueType)) - group_bits - alias_bits;

  ValueType raw = 0;
  struct
  {
    ValueType isAlias : alias_bits;
    ValueType group : group_bits;
    ValueType index : index_bits;
  };
};

#if _TARGET_XBOX
struct VirtualFreeCaller
{
  void operator()(void *pointer) { VirtualFree(pointer, 0, MEM_RELEASE); }
};

class ResourceMemory
{
  uint8_t *heap = nullptr;
  uint64_t sz = 0;
  HeapID heapID;

public:
  ResourceMemory() = default;
  ~ResourceMemory() = default;

  ResourceMemory(const ResourceMemory &) = default;
  ResourceMemory &operator=(const ResourceMemory &) = default;

  ResourceMemory(ResourceMemory &&) = default;
  ResourceMemory &operator=(ResourceMemory &&) = default;

  ResourceMemory(uint8_t *h, uint64_t s, HeapID heap_id) : heap{h}, sz{s}, heapID{heap_id} {}

  explicit operator bool() const { return heap != nullptr; }

  uint64_t size() const { return sz; }

  uintptr_t getAddress() const { return reinterpret_cast<uintptr_t>(heap); }

  uint8_t *asPointer() const { return heap; }

  ResourceMemory subRange(uint64_t offset, uint64_t o_size) const
  {
    G_ASSERT(offset + o_size <= size());
    return {heap + offset, o_size, heapID};
  }

  ResourceMemory aliasSubRange(uint32_t new_index, uint64_t offset, uint64_t o_size) const
  {
    G_ASSERT(offset + o_size <= size());
    HeapID newHeapID = heapID;
    newHeapID.isAlias = 1;
    newHeapID.index = new_index;
    return {heap + offset, o_size, newHeapID};
  }

  bool isSubRangeOf(const ResourceMemory &mem) const
  {
    // NOTE: this can not check heapID as aliasing may change the heap id (from a real heap to a aliasing heap).
    return make_value_range(heap, size()).isSubRangeOf(make_value_range(mem.heap, mem.size()));
  }

  bool intersectsWith(const ResourceMemory &mem) const
  {
    return make_value_range(heap, size()).overlaps(make_value_range(mem.heap, mem.size()));
  }

  uint64_t calculateOffset(const ResourceMemory &sub) const { return sub.heap - heap; }

  HeapID getHeapID() const { return heapID; }

  operator ResourceMemoryRange() const { return {.range = make_value_range(heap, size())}; }

  operator ResourceMemoryLocation() const { return {.location = heap}; }
};

struct ResourceMemoryWithGPUAndCPUAddress
{
  union
  {
    uint8_t *heap = nullptr;
    D3D12_GPU_VIRTUAL_ADDRESS gpuAddress;
    uint8_t *cpuAddress;
  };
  uint64_t size = 0;
  HeapID heapID;

  void initializeFrom(const ResourceMemory &memory, ID3D12Resource *, bool)
  {
    heap = memory.asPointer();
    size = memory.size();
    heapID = memory.getHeapID();
  }

  explicit operator bool() const { return heap != nullptr; }

  operator ResourceMemoryRange() const { return {.range = make_value_range(heap, size)}; }
  operator ResourceMemoryLocation() const { return {.location = heap}; }
  operator ResourceMemory() const { return {heap, size, heapID}; }
  operator ResourceMemoryLocationWithGPUAndCPUAddress() const
  {
    return {
      .location = heap,
    };
  }
};
#else
class ResourceMemory
{
  ID3D12Heap *heap = nullptr;
  ValueRange<uint64_t> range;
  HeapID heapID;

public:
  ResourceMemory() = default;
  ~ResourceMemory() = default;

  ResourceMemory(const ResourceMemory &) = default;
  ResourceMemory &operator=(const ResourceMemory &) = default;

  ResourceMemory(ResourceMemory &&) = default;
  ResourceMemory &operator=(ResourceMemory &&) = default;

  ResourceMemory(ID3D12Heap *h, ValueRange<uint64_t> r, HeapID heap_id) : heap{h}, range{r}, heapID{heap_id} {}

  ID3D12Heap *getHeap() const { return heap; }

  ValueRange<uint64_t> getRange() const { return range; }

  explicit operator bool() const { return heap != nullptr; }

  uint64_t size() const { return range.size(); }

  uint64_t getOffset() const { return range.front(); }

  ResourceMemory subRange(uint64_t offset, uint64_t o_size) const
  {
    G_ASSERT(offset + o_size <= range.size());
    ResourceMemory r;
    r.heap = heap;
    r.range = make_value_range(getOffset() + offset, o_size);
    r.heapID = heapID;
    return r;
  }

  ResourceMemory aliasSubRange(uint32_t new_index, uint64_t offset, uint64_t o_size) const
  {
    G_ASSERT(offset + o_size <= range.size());
    ResourceMemory r;
    r.heap = heap;
    r.range = make_value_range(getOffset() + offset, o_size);
    r.heapID = heapID;
    r.heapID.isAlias = 1;
    r.heapID.index = new_index;
    return r;
  }

  bool isSubRangeOf(const ResourceMemory &mem) const
  {
    // NOTE: this can not check heapID as aliasing may change the heap id (from a real heap to a aliasing heap).
    if (mem.heap != heap)
    {
      return false;
    }
    return range.isSubRangeOf(mem.range);
  }

  bool intersectsWith(const ResourceMemory &mem) const { return (heap == mem.heap) && range.overlaps(mem.range); }

  uint64_t calculateOffset(const ResourceMemory &sub) const { return sub.range.start - range.start; }

  HeapID getHeapID() const { return heapID; }

  operator ResourceMemoryRange() const
  {
    return {
      .heap = heap,
      .range = range,
    };
  }

  operator ResourceMemoryLocation() const
  {
    return {
      .heap = heap,
      .offset = range.front(),
    };
  }
};

struct ResourceMemoryWithGPUAndCPUAddress
{
  ID3D12Heap *heap = nullptr;
  uint64_t offset = 0;
  uint64_t size = 0;
  HeapID heapID;
  D3D12_GPU_VIRTUAL_ADDRESS gpuAddress{};
  uint8_t *cpuAddress = nullptr;

  explicit operator bool() const { return heap != nullptr; }

  void initializeFrom(const ResourceMemory &memory, ID3D12Resource *res, bool can_map)
  {
    heap = memory.getHeap();
    offset = memory.getOffset();
    size = memory.size();
    heapID = memory.getHeapID();
    gpuAddress = res->GetGPUVirtualAddress();
    cpuAddress = nullptr;
    if (can_map)
    {
      D3D12_RANGE emptyRange{};
      res->Map(0, &emptyRange, reinterpret_cast<void **>(&cpuAddress));
    }
  }

  operator ResourceMemoryRange() const
  {
    return {
      .heap = heap,
      .range = make_value_range(offset, size),
    };
  }

  operator ResourceMemoryLocation() const
  {
    return {
      .heap = heap,
      .offset = offset,
    };
  }

  operator ResourceMemory() const { return {heap, make_value_range(offset, size), heapID}; }

  operator ResourceMemoryLocationWithGPUAndCPUAddress() const
  {
    return {
      .heap = heap,
      .offset = offset,
      .gpuAddress = gpuAddress,
      .cpuAddress = cpuAddress,
    };
  }
};

#endif

} // namespace drv3d_dx12
