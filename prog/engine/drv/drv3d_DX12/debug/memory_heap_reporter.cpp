// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "memory_heap_reporter.h"

#if DX12_ENABLE_MEMORY_HEAP_REPORTER

#include <d3d12_error_handling.h>
#include <dag/dag_vector.h>
#include <debug/dag_log.h>
#include <osApiWrappers/dag_spinlock.h>
#include <osApiWrappers/dag_threadSafety.h>
#include <supp/dag_comPtr.h>

#include <cinttypes>
#include <EASTL/algorithm.h>


namespace drv3d_dx12::debug
{
namespace
{
struct MemoryHeapRecord
{
  ID3D12Heap *heap;
  const char *typeName;
  uint32_t index;
  uint64_t base;
  uint64_t size;
};

OSSpinlock memory_heap_records_mutex;
dag::Vector<MemoryHeapRecord> memory_heap_records DAG_TS_GUARDED_BY(memory_heap_records_mutex);

// GetGPUVirtualAddress may sign extend bit 47, DRED reports the address without it.
constexpr uint64_t strip_sign_extension(uint64_t gpu_address) { return gpu_address & ((1ull << 48) - 1); }

// ID3D12Heap has no GPU address query, a buffer placed at offset 0 gives the heap base.
uint64_t get_heap_gpu_base(D3DDevice *device, ID3D12Heap *heap, const D3D12_HEAP_DESC &heap_desc)
{
  if (heap_desc.Flags & D3D12_HEAP_FLAG_DENY_BUFFERS)
    return 0;

  D3D12_RESOURCE_DESC desc = {
    .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
    .Width = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
    .Height = 1,
    .DepthOrArraySize = 1,
    .MipLevels = 1,
    .SampleDesc = {.Count = 1},
    .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
  };
  ComPtr<ID3D12Resource> probe;
  if (FAILED(DX12_CHECK_RESULT_NO_OOM_CHECK(
        device->CreatePlacedResource(heap, 0, &desc, D3D12_RESOURCE_STATE_INITIAL_BUFFER_STATE, nullptr, COM_ARGS(&probe)))))
    return 0;

  probe->SetName(L"MemoryHeapAddressProbe");
  return strip_sign_extension(probe->GetGPUVirtualAddress());
}

void log_memory_heap(const MemoryHeapRecord &record, const char *event)
{
  if (record.base)
    logdbg("DX12: MemoryHeap.%s#%u %p %s, %" PRIu64 " bytes at GPU 0x%016" PRIX64 "-0x%016" PRIX64, record.typeName, record.index,
      record.heap, event, record.size, record.base, record.base + record.size);
  else
    logdbg("DX12: MemoryHeap.%s#%u %p %s, %" PRIu64 " bytes, no GPU range", record.typeName, record.index, record.heap, event,
      record.size);
}

} // namespace

void register_memory_heap(D3DDevice *device, ID3D12Heap *heap, const char *type_name, uint32_t index)
{
  const D3D12_HEAP_DESC heapDesc = heap->GetDesc();
  const MemoryHeapRecord record{heap, type_name, index, get_heap_gpu_base(device, heap, heapDesc), heapDesc.SizeInBytes};
  log_memory_heap(record, "created");
  OSSpinlockScopedLock lock{memory_heap_records_mutex};
  memory_heap_records.push_back(record);
}

void unregister_memory_heap(ID3D12Heap *heap)
{
  MemoryHeapRecord record;
  {
    OSSpinlockScopedLock lock{memory_heap_records_mutex};
    auto at = eastl::find_if(memory_heap_records.begin(), memory_heap_records.end(), [heap](auto &r) { return r.heap == heap; });
    if (at == memory_heap_records.end())
      return;
    record = *at;
    memory_heap_records.erase_unsorted(at);
  }
  log_memory_heap(record, "freed");
}

void clear_memory_heaps()
{
  dag::Vector<MemoryHeapRecord> records;
  {
    OSSpinlockScopedLock lock{memory_heap_records_mutex};
    records.swap(memory_heap_records);
  }
  for (const MemoryHeapRecord &record : records)
    log_memory_heap(record, "freed");
}

void report_memory_heap_at(uint64_t gpu_address, eastl::string &out)
{
  OSSpinlockScopedLock lock{memory_heap_records_mutex};
  const uint64_t address = strip_sign_extension(gpu_address);
  uint32_t withoutRange = 0;
  for (const MemoryHeapRecord &r : memory_heap_records)
  {
    if (!r.base)
      ++withoutRange;
    else if (address - r.base < r.size)
    {
      out.append_sprintf("Memory heap at the fault address: MemoryHeap.%s#%u %p at offset %" PRIu64 "\n", r.typeName, r.index, r.heap,
        address - r.base);
      return;
    }
  }
  out.append_sprintf(
    "Memory heap at the fault address: not in any memory heap with a known GPU range, %u memory heaps have no known range\n",
    withoutRange);
}

} // namespace drv3d_dx12::debug

#endif
