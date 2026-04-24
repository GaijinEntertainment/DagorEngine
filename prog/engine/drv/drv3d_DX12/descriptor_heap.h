// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "d3d12_error_handling.h"
#include "d3d12_utils.h"
#include "driver.h"

#include <dag/dag_vector.h>
#include <EASTL/unique_ptr.h>
#include <generic/dag_bitset.h>
#include <supp/dag_comPtr.h>
#include <value_range.h>
#include <drv/3d/dag_bindless.h>


namespace drv3d_dx12
{
template <typename Policy>
class DescriptorHeap
{
  struct SubHeap : Policy::SubHeapData
  {
    ComPtr<ID3D12DescriptorHeap> heap;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuBegin;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuEnd;

    bool isPartOf(D3D12_CPU_DESCRIPTOR_HANDLE handle, uint32_t slot_size)
    {
      // nvidia does some funky stuff with their UAV/SRV/CBV descriptors (just very stupid, tbh).
      // Instead of pointers they hand out base id's, with that heaps overlap in their address
      // range and the only way to find out which heap it belongs to is that there is no
      // remainder after subtracting the base and dividing by slot size.
      if ((handle.ptr - cpuBegin.ptr) % slot_size)
        return false;
      return handle.ptr >= cpuBegin.ptr && handle.ptr < cpuEnd.ptr;
    }
  };

  dag::Vector<SubHeap> heaps;
  typename Policy::HeapData data;

public:
  typedef Policy ThisPolicy;

  DescriptorHeap() = default;
  ~DescriptorHeap() = default;

  DescriptorHeap(const DescriptorHeap &) = default;
  DescriptorHeap &operator=(const DescriptorHeap &) = default;

  DescriptorHeap(DescriptorHeap &&) = default;
  DescriptorHeap &operator=(DescriptorHeap &&) = default;

  void init(const typename ThisPolicy::InitData &init) { data = typename ThisPolicy::HeapData{init}; }

  void shutdown()
  {
    heaps.clear();
    data.reset();
  }

  D3D12_CPU_DESCRIPTOR_HANDLE allocate(ID3D12Device *device)
  {
    D3D12_CPU_DESCRIPTOR_HANDLE result = {};
    for (auto &&heap : heaps)
    {
      uint32_t slot = heap.allocate();
      if (heap.isValidSlot(slot))
      {
        result.ptr = heap.cpuBegin.ptr + data.slotSize() * slot;
        return result;
      }
    }

    auto &target = heaps.emplace_back();
    target.init(data);
    target.heap = ThisPolicy::allocateHeap(device, data, target);
    if (!target.heap)
    {
      if (device->GetDeviceRemovedReason() == S_OK)
      {
        logdbg("DX12: heap count was %u", heaps.size());
        DAG_FATAL("DX12: DescriptorHeap::allocate: failed to allocate descriptor");
      }
      else
      {
        logdbg("DX12: Trying to 'DescriptorHeap::allocate' and recovery was not started yet");
      }

      heaps.pop_back();
      return result;
    }
    target.cpuBegin = target.heap->GetCPUDescriptorHandleForHeapStart();
    target.cpuEnd = target.cpuBegin;
    target.cpuEnd.ptr += target.getTotalSlots() * data.slotSize();
    uint32_t slot = target.allocate();
    result.ptr = target.cpuBegin.ptr + data.slotSize() * slot;
    return result;
  }
  void free(D3D12_CPU_DESCRIPTOR_HANDLE slot)
  {
    for (auto &&heap : heaps)
    {
      if (heap.isPartOf(slot, data.slotSize()))
      {
        uint32_t index = (slot.ptr - heap.cpuBegin.ptr) / data.slotSize();
        heap.free(index);
        return;
      }
    }
    G_ASSERT("DX12: DescriptorHeap::free unable to find heap for descriptor");
  }

  void freeAll()
  {
    for (auto &&heap : heaps)
      heap.freeAll();
  }
};

// Base policy for block allocating heaps, allocation state is tracked with a bitfield
// of N bits, where N the block size is. So use block sizes of integer types for
// Best mapping (default is 64).
// If Gpu is true, the heaps are also GPU visible. Note block heaps are usually
// too small to be efficient for GPU visible heaps and require many resets of
// the heap pointer on a command buffer (which is slow!).
template <D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t BlockSize = 64, bool Gpu = false>
struct BasicBlockHeap
{
  typedef UINT InitData;
  struct HeapData
  {
    HeapData() = default;
    ~HeapData() = default;

    HeapData(const HeapData &) = default;
    HeapData &operator=(const HeapData &) = default;

    HeapData(const InitData &init) : slotBytes{init} {}

    UINT slotBytes = 0;

    UINT slotSize() const { return slotBytes; }
    void reset() { slotBytes = 0; }
    void beginMerge() {}
    template <typename T>
    void addHeapForMerge(T &)
    {}
    void endMerge() {}
    constexpr bool allowMerge() const { return false; }
  };
  struct SubHeapData
  {
    Bitset<BlockSize> freeMap;
    uint32_t allocate() { return static_cast<uint32_t>(freeMap.find_first_and_reset()); }
    constexpr bool isValidSlot(uint32_t slot) const { return slot < BlockSize; }
    void free(uint32_t slot) { freeMap.set(slot); }
    void init(const HeapData &) { freeMap.set(); }
    constexpr uint32_t getTotalSlots() const { return BlockSize; }
    void freeAll() { freeMap.set(); }
  };

  static ComPtr<ID3D12DescriptorHeap> allocateHeap(ID3D12Device *device, const HeapData &, SubHeapData &)
  {
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = Type;
    desc.NumDescriptors = BlockSize;
    if (Gpu)
      desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ComPtr<ID3D12DescriptorHeap> result;
    DX12_CHECK_RESULT(device->CreateDescriptorHeap(&desc, COM_ARGS(&result)));

    return result;
  }
};

template <size_t BlockSize = 64>
using ShaderResourceViewStagingPolicy = BasicBlockHeap<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, BlockSize>;
using SamplerStagingPolicy = BasicBlockHeap<D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER>;
using RenderTargetViewPolicy = BasicBlockHeap<D3D12_DESCRIPTOR_HEAP_TYPE_RTV>;
using DepthStencilViewPolicy = BasicBlockHeap<D3D12_DESCRIPTOR_HEAP_TYPE_DSV>;

// Base policy for free list managed heaps. This base policy supports merging
// and is best suited for GPU use and resize after each frame they where used.
template <D3D12_DESCRIPTOR_HEAP_TYPE Type, bool Gpu = true>
struct BasicFreeListHeap
{
  struct InitData
  {
    UINT slotBytes = 0;
    UINT initialSize = 0;
    UINT maxHeapSize = 0;

    InitData() = default;
    ~InitData() = default;

    InitData(const InitData &) = default;
    InitData &operator=(const InitData &) = default;

    InitData(InitData &) = default;
    InitData &operator=(InitData &) = default;

    InitData(UINT slot_size, UINT slot_count, UINT max_size) : slotBytes{slot_size}, initialSize{slot_count}, maxHeapSize{max_size} {}
  };
  struct HeapData
  {
    HeapData() = default;
    ~HeapData() = default;

    HeapData(const HeapData &) = default;
    HeapData &operator=(const HeapData &) = default;

    HeapData(const InitData &init) : slotBytes{init.slotBytes}, subHeapSize{init.initialSize}, maxHeapSize{init.maxHeapSize} {}

    UINT slotBytes = 0;
    UINT subHeapSize = 0;
    UINT maxHeapSize = 0;

    UINT slotSize() const { return slotBytes; }
    void reset()
    {
      slotBytes = 0;
      subHeapSize = 0;
    }
    void beginMerge() { subHeapSize = 0; }
    void endMerge()
    {
      if (subHeapSize > maxHeapSize)
        subHeapSize = maxHeapSize;
      // TODO: round to next something?
    }
    // if we hit the per heap limit, do not merge
    bool allowMerge() const { return subHeapSize < maxHeapSize; }
  };

  struct SubHeapData
  {
    dag::Vector<ValueRange<uint32_t>> freeSlots;
    uint32_t totalSlots;
    uint32_t allocate()
    {
      if (freeSlots.empty())
        return totalSlots;
      auto &block = freeSlots.back();
      auto slot = block.front();
      block.pop_front();
      if (block.empty())
        freeSlots.pop_back();
      return slot;
    }
    constexpr bool isValidSlot(uint32_t slot) const { return slot < totalSlots; }
    void free(uint32_t slot)
    {
      freeSlots.push_back({slot, slot + 1});
      // try to merge ranges
      for (size_t b = 0; b < freeSlots.size() - 1; ++b)
      {
        auto &prev = freeSlots[b];
        size_t i = b + 1;
        while (i < freeSlots.size())
        {
          if (prev.isContigous(freeSlots[i]))
          {
            prev.merge(freeSlots[i]);
            freeSlots[i] = freeSlots.back();
            freeSlots.pop_back();
          }
          else
          {
            ++i;
          }
        }
      }
    }
    void init(const HeapData &data)
    {
      totalSlots = data.subHeapSize;
      freeSlots.push_back({0, totalSlots});
    }
    constexpr uint32_t getTotalSlots() const { return totalSlots; }
    void freeAll()
    {
      freeSlots.clear();
      freeSlots.push_back({0, totalSlots});
    }
  };

  static ComPtr<ID3D12DescriptorHeap> allocateHeap(ID3D12Device *device, const HeapData &, SubHeapData &heap)
  {
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = Type;
    desc.NumDescriptors = heap.totalSlots;
    if (Gpu)
    {
      desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    }
    ComPtr<ID3D12DescriptorHeap> result;
    DX12_CHECK_RESULT(device->CreateDescriptorHeap(&desc, COM_ARGS(&result)));

    return result;
  }
};

typedef BasicFreeListHeap<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV> ShaderResourceViewDynamicPolicy;
// TODO: may have a different policy for this?
typedef BasicFreeListHeap<D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER> SamplerDynamicPolicy;

constexpr uint32_t SRV_HEAP_SIZE = D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_1 / 2;
constexpr uint32_t SAMPLER_HEAP_SIZE = D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE;

// Base class for ManagedDescriptorHeaps
// It manages the descriptor heap object, the cpu, gpu address calculations and object state
// updates.
class ManagedDescriptorHeapBase
{
  ComPtr<ID3D12DescriptorHeap> heap;
  D3D12_GPU_DESCRIPTOR_HANDLE gpuBaseAddress = {};
  D3D12_CPU_DESCRIPTOR_HANDLE cpuBaseAddress = {};
  uint32_t entrySize = 0;

protected:
  ManagedDescriptorHeapBase(ID3D12Device *device, uint32_t entry_size, uint32_t heap_size, D3D12_DESCRIPTOR_HEAP_TYPE type) :
    entrySize{entry_size}
  {
    D3D12_DESCRIPTOR_HEAP_DESC desc;
    desc.Type = type;
    desc.NumDescriptors = heap_size;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    desc.NodeMask = 0;
    if (DX12_CHECK_OK(device->CreateDescriptorHeap(&desc, COM_ARGS(&heap))))
    {
      gpuBaseAddress = heap->GetGPUDescriptorHandleForHeapStart();
      cpuBaseAddress = heap->GetCPUDescriptorHandleForHeapStart();
    }
  }

public:
  ManagedDescriptorHeapBase() = default;
  ~ManagedDescriptorHeapBase() = default;
  ManagedDescriptorHeapBase(const ManagedDescriptorHeapBase &) = delete;
  ManagedDescriptorHeapBase &operator=(const ManagedDescriptorHeapBase &) = delete;
  ManagedDescriptorHeapBase(ManagedDescriptorHeapBase &&) = default;
  ManagedDescriptorHeapBase &operator=(ManagedDescriptorHeapBase &&) = default;

  ID3D12DescriptorHeap *getHandle() { return heap.Get(); }
  D3D12_GPU_DESCRIPTOR_HANDLE getGpuAddress(uint32_t index) const { return gpuBaseAddress + (index * entrySize); }
  D3D12_CPU_DESCRIPTOR_HANDLE getCpuAddress(uint32_t index) const { return cpuBaseAddress + (index * entrySize); }
  void updateDescriptors(ID3D12Device *device, uint32_t dst_offset, const D3D12_CPU_DESCRIPTOR_HANDLE *desc, uint32_t count,
    D3D12_DESCRIPTOR_HEAP_TYPE type)
  {
    auto dstStart = getCpuAddress(dst_offset);
    // nullptr on src ranges means its a set of ranges with a length of 1
    device->CopyDescriptors(1, &dstStart, &count, count, desc, nullptr, type);
  }
  // copies 'count' successive from a different heap, starting at 'heap_ptr'
  // This variant is much faster as its a straight memcpy instead of a scatter/gather
  void updateDescriptors(ID3D12Device *device, uint32_t dst_offset, const D3D12_CPU_DESCRIPTOR_HANDLE heap_ptr, uint32_t count,
    D3D12_DESCRIPTOR_HEAP_TYPE type)
  {
    auto dstStart = getCpuAddress(dst_offset);
    device->CopyDescriptorsSimple(count, dstStart, heap_ptr, type);
  }
  void updateDescriptor(ID3D12Device *device, uint32_t dst_offset, D3D12_CPU_DESCRIPTOR_HANDLE desc, D3D12_DESCRIPTOR_HEAP_TYPE type)
  {
    device->CopyDescriptorsSimple(1, getCpuAddress(dst_offset), desc, type);
  }
  uint32_t getDescriptorSize() const { return entrySize; }
};

// For small tables we can use an array in place.
template <uint32_t HeapSize, bool AsArray = true>
class ManagedDescriptorHeapAllocationTableStore
{
  D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorTable[HeapSize] = {};

public:
  const D3D12_CPU_DESCRIPTOR_HANDLE *getCpuDescriptorTable() const { return cpuDescriptorTable; }
  D3D12_CPU_DESCRIPTOR_HANDLE *getCpuDescriptorTable() { return cpuDescriptorTable; }
};

#ifdef _MSC_VER
// For big array we need to store it in heap space, msvc will use excessive amount of ram during compilation otherwise.
template <uint32_t HeapSize>
class ManagedDescriptorHeapAllocationTableStore<HeapSize, false>
{
  eastl::unique_ptr<D3D12_CPU_DESCRIPTOR_HANDLE> cpuDescriptorTable{new D3D12_CPU_DESCRIPTOR_HANDLE[HeapSize]};

public:
  const D3D12_CPU_DESCRIPTOR_HANDLE *getCpuDescriptorTable() const { return cpuDescriptorTable.get(); }
  D3D12_CPU_DESCRIPTOR_HANDLE *getCpuDescriptorTable() { return cpuDescriptorTable.get(); }
};
#endif

union DescriptorHeapIndex
{
  uint32_t raw = 0;
  struct
  {
    uint32_t isInvalid : 1;
    uint32_t subHeapIndex : 7;
    uint32_t descriptorIndex : 24;
  };

  explicit operator bool() const { return 0 == isInvalid; }

  static DescriptorHeapIndex make_invalid()
  {
    DescriptorHeapIndex result;
    result.isInvalid = 1;
    return result;
  }

  static DescriptorHeapIndex make(uint32_t heap, uint32_t descriptor)
  {
    DescriptorHeapIndex result;
    result.isInvalid = 0;
    result.subHeapIndex = heap;
    result.descriptorIndex = descriptor;
    return result;
  }

  static DescriptorHeapIndex make(uint32_t descriptor)
  {
    DescriptorHeapIndex result;
    result.isInvalid = 0;
    result.subHeapIndex = 0;
    result.descriptorIndex = descriptor;
    return result;
  }

  static DescriptorHeapIndex make(bool is_valid, uint32_t descriptor)
  {
    DescriptorHeapIndex result;
    result.isInvalid = is_valid ? 0u : 1u;
    result.subHeapIndex = 0;
    result.descriptorIndex = descriptor;
    return result;
  }

  static DescriptorHeapIndex set_heap(DescriptorHeapIndex base, uint32_t heap)
  {
    DescriptorHeapIndex result;
    result.raw = base.raw;
    result.subHeapIndex = heap;
    return result;
  }
};

struct DescriptorHeapRange
{
  DescriptorHeapIndex base;
  uint32_t count = 0;

  explicit operator bool() const { return static_cast<bool>(base); }

  static DescriptorHeapRange make(DescriptorHeapIndex base, uint32_t count)
  {
    DescriptorHeapRange result;
    result.base = base;
    result.count = count;
    return result;
  }
};

enum class DescriptorReservationResult
{
  CurrentHeap,
  NewHeap,
};

class ShaderResourceViewDescriptorHeapManager
{
  struct Heap : public ManagedDescriptorHeapBase
  {
    Heap() = delete;
    ~Heap() = default;

    Heap(const Heap &) = delete;
    Heap &operator=(const Heap &) = delete;

    Heap(Heap &&) = default;
    Heap &operator=(Heap &&) = default;

    Heap(ID3D12Device *device, uint32_t entry_size) :
      ManagedDescriptorHeapBase{device, entry_size, SRV_HEAP_SIZE, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV}
    {}

    uint32_t allocationCount = ::bindless::MAX_RESOURCE_INDEX_COUNT;
    uint32_t bindlessRev = 0;

    void clear()
    {
      G_ASSERT_LOG(allocationCount <= SRV_HEAP_SIZE, "There are more descriptors used, than descriptor heap can handle (%d out of %d)",
        allocationCount, SRV_HEAP_SIZE);
      allocationCount = ::bindless::MAX_RESOURCE_INDEX_COUNT;
    }

    DescriptorHeapIndex append(ID3D12Device *device, const D3D12_CPU_DESCRIPTOR_HANDLE *descriptors, uint32_t count)
    {
      auto index = allocationCount;
      allocationCount += count;
      auto dstStart = getCpuAddress(index);
      device->CopyDescriptors(1, &dstStart, &count, count, descriptors, nullptr, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
      return DescriptorHeapIndex::make(index);
    }

    DescriptorHeapIndex append(ID3D12Device *device, D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
    {
      auto index = allocationCount++;
      device->CopyDescriptorsSimple(1, getCpuAddress(index), descriptor, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
      return DescriptorHeapIndex::make(index);
    }

    DescriptorHeapIndex append(ID3D12Device *device, D3D12_CPU_DESCRIPTOR_HANDLE base_ptr, uint32_t count)
    {
      auto index = allocationCount;
      allocationCount += count;
      device->CopyDescriptorsSimple(count, getCpuAddress(index), base_ptr, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

      return DescriptorHeapIndex::make(index);
    }

    DescriptorHeapIndex allocateDescriptors(uint32_t count)
    {
      auto index = allocationCount;
      allocationCount += count;
      return DescriptorHeapIndex::make(index);
    }

    /// returns true when enough space is available
    bool reserveSpace(uint32_t size) const { return (SRV_HEAP_SIZE - allocationCount) >= size; }

    void updateBindlessSegment(ID3D12Device *device, D3D12_CPU_DESCRIPTOR_HANDLE base_ptr, uint32_t size, uint32_t rev)
    {
      if (bindlessRev == rev)
      {
        return;
      }
      device->CopyDescriptorsSimple(size, getCpuAddress(0), base_ptr, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
  };
  dag::Vector<Heap> heaps;
  uint32_t activeHeapIndex = 0;
  uint32_t entrySize = 0;
  uint32_t bindlessRev = 0;

  void nextHeap(ID3D12Device *device)
  {
    ++activeHeapIndex;
    if (activeHeapIndex >= heaps.size())
    {
      heaps.emplace_back(device, entrySize);
    }
    bindlessRev = 0;
  }

public:
  ShaderResourceViewDescriptorHeapManager() = default;
  ~ShaderResourceViewDescriptorHeapManager() = default;

  ShaderResourceViewDescriptorHeapManager(const ShaderResourceViewDescriptorHeapManager &) = delete;
  ShaderResourceViewDescriptorHeapManager &operator=(const ShaderResourceViewDescriptorHeapManager &) = delete;

  ShaderResourceViewDescriptorHeapManager(ShaderResourceViewDescriptorHeapManager &&) = default;
  ShaderResourceViewDescriptorHeapManager &operator=(ShaderResourceViewDescriptorHeapManager &&) = default;

  ShaderResourceViewDescriptorHeapManager(ID3D12Device *device, uint32_t entry_size) : entrySize{entry_size}
  {
    // create initial heap so we never have zero
    heaps.emplace_back(device, entry_size);
  }

  void clearScratchSegments()
  {
    activeHeapIndex = 0;
    bindlessRev = 0;

    for (auto &heap : heaps)
    {
      heap.clear();
    }
  }

  void onFlush() {}

  ID3D12DescriptorHeap *getActiveHandle() { return heaps[activeHeapIndex].getHandle(); }

  D3D12_GPU_DESCRIPTOR_HANDLE getBindlessGpuAddress() const { return heaps[activeHeapIndex].getGpuAddress(0); }

  D3D12_GPU_DESCRIPTOR_HANDLE getGpuAddress(DescriptorHeapIndex index) const
  {
    G_ASSERT(static_cast<bool>(index));
    return heaps[index.subHeapIndex].getGpuAddress(index.descriptorIndex);
  }

  D3D12_GPU_DESCRIPTOR_HANDLE getGpuAddress(DescriptorHeapRange range) const { return getGpuAddress(range.base); }

  D3D12_CPU_DESCRIPTOR_HANDLE getCpuAddress(DescriptorHeapIndex index) const
  {
    G_ASSERT(static_cast<bool>(index));
    auto result = heaps[index.subHeapIndex].getCpuAddress(index.descriptorIndex);
    return result;
  }

  D3D12_CPU_DESCRIPTOR_HANDLE getCpuAddress(DescriptorHeapRange range) const { return getCpuAddress(range.base); }

  DescriptorHeapIndex findInUAVScratchSegment(const D3D12_CPU_DESCRIPTOR_HANDLE *, uint32_t) const
  {
    return DescriptorHeapIndex::make_invalid();
  }

  DescriptorHeapIndex findInUAVScratchSegment(D3D12_CPU_DESCRIPTOR_HANDLE) const { return DescriptorHeapIndex::make_invalid(); }

  DescriptorHeapIndex appendToUAVScratchSegment(ID3D12Device *device, const D3D12_CPU_DESCRIPTOR_HANDLE *descriptors, uint32_t count)
  {
    return DescriptorHeapIndex::set_heap(heaps[activeHeapIndex].append(device, descriptors, count), activeHeapIndex);
  }

  DescriptorHeapIndex appendToUAVScratchSegment(ID3D12Device *device, D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
  {
    return DescriptorHeapIndex::set_heap(heaps[activeHeapIndex].append(device, descriptor), activeHeapIndex);
  }

  bool highSRVScratchSegmentUsage() const { return false; }

  DescriptorHeapIndex findInSRVScratchSegment(const D3D12_CPU_DESCRIPTOR_HANDLE *, uint32_t) const
  {
    return DescriptorHeapIndex::make_invalid();
  }

  DescriptorHeapIndex findInSRVScratchSegment(D3D12_CPU_DESCRIPTOR_HANDLE) const { return DescriptorHeapIndex::make_invalid(); }

  DescriptorHeapIndex appendToSRVScratchSegment(ID3D12Device *device, const D3D12_CPU_DESCRIPTOR_HANDLE *descriptors, uint32_t count)
  {
    return DescriptorHeapIndex::set_heap(heaps[activeHeapIndex].append(device, descriptors, count), activeHeapIndex);
  }

  DescriptorHeapIndex appendToSRVScratchSegment(ID3D12Device *device, D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
  {
    return DescriptorHeapIndex::set_heap(heaps[activeHeapIndex].append(device, descriptor), activeHeapIndex);
  }

  void flushBindlessToHeaps(ID3D12Device *device, D3D12_CPU_DESCRIPTOR_HANDLE base_ptr, uint32_t size, uint32_t rev)
  {
    // with size of 0, GDK will crash, as MS decided to implemented it with a do while loop that copies in revers which
    // breaks when supplied with 0.
    if (bindlessRev == rev || 0 == size)
    {
      return;
    }

    for (auto &heap : heaps)
    {
      heap.updateBindlessSegment(device, base_ptr, size, rev);
    }
    bindlessRev = rev;
  }

  DescriptorHeapIndex appendToConstScratchSegment(ID3D12Device *device, D3D12_CPU_DESCRIPTOR_HANDLE descriptors, uint32_t count)
  {
    return DescriptorHeapIndex::set_heap(heaps[activeHeapIndex].append(device, descriptors, count), activeHeapIndex);
  }

  DescriptorHeapIndex allocateConstBufferDescriptors(uint32_t count)
  {
    return DescriptorHeapIndex::set_heap(heaps[activeHeapIndex].allocateDescriptors(count), activeHeapIndex);
  }

  DescriptorReservationResult reserveSpace(ID3D12Device *device, uint32_t slot_count)
  {
    if (heaps[activeHeapIndex].reserveSpace(slot_count))
    {
      return DescriptorReservationResult::CurrentHeap;
    }

    nextHeap(device);
    return DescriptorReservationResult::NewHeap;
  }
};

class SamplerDescriptorHeapManager
{
  struct Heap : public ManagedDescriptorHeapBase
  {
    // Top ::bindless::MAX_SAMPLER_INDEX_COUNT are off limits and reserved for bindless
    static constexpr uint32_t scratch_space_offset = ::bindless::MAX_SAMPLER_INDEX_COUNT;
    static constexpr uint32_t scratch_space_size = SAMPLER_HEAP_SIZE - scratch_space_offset;

    D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorTable[scratch_space_size]{};
    uint32_t allocationCount = 0;
    uint32_t bindlessSamplersCommitted = 0;

    Heap() = delete;
    ~Heap() = default;

    Heap(const Heap &) = delete;
    Heap &operator=(const Heap &) = delete;

    Heap(Heap &&) = default;
    Heap &operator=(Heap &&) = default;

    Heap(ID3D12Device *device, uint32_t entry_size) :
      ManagedDescriptorHeapBase{device, entry_size, SAMPLER_HEAP_SIZE, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER}
    {}

    void resetScratchSpace() { allocationCount = 0; }
    uint32_t freeCount() const { return scratch_space_size - allocationCount; }

    DescriptorHeapIndex findInScratchSegment(const D3D12_CPU_DESCRIPTOR_HANDLE *descriptors, uint32_t count) const
    {
      // we allocate from back to front, so free count after allocate is the start point of the allocated range
      auto from = cpuDescriptorTable;
      auto to = cpuDescriptorTable + allocationCount;
      auto at = eastl::search(from, to, descriptors, descriptors + count);
      return DescriptorHeapIndex::make(at != to, scratch_space_offset + (at - cpuDescriptorTable));
    }

    DescriptorHeapIndex appendToScratchSegment(ID3D12Device *device, const D3D12_CPU_DESCRIPTOR_HANDLE *descriptors, uint32_t count)
    {
      // we allocate from back to front, so free count after allocate is the start point of the allocated range
      auto loc = allocationCount;
      allocationCount += count;

      eastl::copy(descriptors, descriptors + count, cpuDescriptorTable + loc);
      auto dstStart = getCpuAddress(scratch_space_offset + loc);
      // nullptr on src ranges means its a set of ranges with a length of 1
      device->CopyDescriptors(1, &dstStart, &count, count, descriptors, nullptr, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
      return DescriptorHeapIndex::make(scratch_space_offset + loc);
    }

    void updateBindlessSegment(ID3D12Device *device, D3D12_CPU_DESCRIPTOR_HANDLE base_ptr, uint32_t size, uint32_t)
    {
      if (bindlessSamplersCommitted == size)
      {
        return;
      }

      device->CopyDescriptorsSimple(size, getCpuAddress(0), base_ptr, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
      bindlessSamplersCommitted = size;
    }
  };
  dag::Vector<Heap> heaps;
  uint32_t activeHeapIndex = 0;
  uint32_t entrySize = 0;
  uint32_t nextBindlessFlush = 0;
  uint32_t bindlessSamplersCommitted = 0;

public:
  SamplerDescriptorHeapManager() = default;
  ~SamplerDescriptorHeapManager() = default;

  SamplerDescriptorHeapManager(const SamplerDescriptorHeapManager &) = delete;
  SamplerDescriptorHeapManager &operator=(const SamplerDescriptorHeapManager &) = delete;

  SamplerDescriptorHeapManager(SamplerDescriptorHeapManager &&) = default;
  SamplerDescriptorHeapManager &operator=(SamplerDescriptorHeapManager &&) = default;

  SamplerDescriptorHeapManager(ID3D12Device *device, uint32_t entry_size) : entrySize{entry_size}
  {
    // create initial heap so we never have zero
    heaps.emplace_back(device, entry_size);
  }

  ID3D12DescriptorHeap *getActiveHandle() { return heaps[activeHeapIndex].getHandle(); }

  D3D12_GPU_DESCRIPTOR_HANDLE getBindlessGpuAddress() const { return heaps[activeHeapIndex].getGpuAddress(0); }

  D3D12_GPU_DESCRIPTOR_HANDLE getGpuAddress(DescriptorHeapIndex index) const
  {
    G_ASSERT(static_cast<bool>(index));
    return heaps[index.subHeapIndex].getGpuAddress(index.descriptorIndex);
  }

  D3D12_GPU_DESCRIPTOR_HANDLE getGpuAddress(DescriptorHeapRange range) const { return getGpuAddress(range.base); }

  DescriptorHeapIndex findInScratchSegment(const D3D12_CPU_DESCRIPTOR_HANDLE *descriptors, uint32_t count)
  {
    return DescriptorHeapIndex::set_heap(heaps[activeHeapIndex].findInScratchSegment(descriptors, count), activeHeapIndex);
  }
  DescriptorHeapIndex appendToScratchSegment(ID3D12Device *device, const D3D12_CPU_DESCRIPTOR_HANDLE *descriptors, uint32_t count)
  {
    return DescriptorHeapIndex::set_heap(heaps[activeHeapIndex].appendToScratchSegment(device, descriptors, count), activeHeapIndex);
  }

  DescriptorHeapRange migrateToActiveScratchSegment(ID3D12Device *device, DescriptorHeapRange range)
  {
    G_ASSERT(static_cast<bool>(range));
    auto count = range.count;
    // first see if active is the same as the source, if yes we don't do anything
    auto sHeapIndex = range.base.subHeapIndex;
    if (sHeapIndex == activeHeapIndex)
    {
      return range;
    }
    auto sIndex = range.base.descriptorIndex;

    auto &sHeap = heaps[sHeapIndex];
    auto dIndex = findInScratchSegment(sHeap.cpuDescriptorTable + sIndex, count);
    if (!dIndex)
    {
      dIndex = appendToScratchSegment(device, sHeap.cpuDescriptorTable + sIndex, count);
    }
    return DescriptorHeapRange::make(dIndex, count);
  }
  void clearScratchSegment()
  {
    for (auto &&heap : heaps)
    {
      heap.resetScratchSpace();
    }
    activeHeapIndex = 0;
  }

  bool ensureScratchSegmentSpace(ID3D12Device *device, uint32_t size)
  {
    G_ASSERTF(size < SAMPLER_HEAP_SIZE, "Can not reserve %u descriptors, because its more than a single "
                                        "heap can provide");
    if (heaps[activeHeapIndex].freeCount() >= size)
      return false;

    auto oldActiveHeapIndex = activeHeapIndex++;

    if (activeHeapIndex >= heaps.size())
      heaps.emplace_back(device, entrySize);

    return oldActiveHeapIndex != activeHeapIndex;
  }

  void flushBindlessToHeaps(ID3D12Device *device, D3D12_CPU_DESCRIPTOR_HANDLE base_ptr, uint32_t size)
  {
    if (bindlessSamplersCommitted != size)
    {
      bindlessSamplersCommitted = size;
      nextBindlessFlush = 0;
    }
    for (; nextBindlessFlush <= activeHeapIndex; ++nextBindlessFlush)
    {
      heaps[nextBindlessFlush].updateBindlessSegment(device, base_ptr, size, entrySize);
    }
  }
};

} // namespace drv3d_dx12
