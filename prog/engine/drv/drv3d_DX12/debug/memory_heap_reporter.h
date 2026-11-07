// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <driver.h>

#include <EASTL/string.h>


namespace drv3d_dx12::debug
{

#if DX12_ENABLE_MEMORY_HEAP_REPORTER

// type_name is kept by pointer, so it has to be a static string.
void register_memory_heap(D3DDevice *device, ID3D12Heap *heap, const char *type_name, uint32_t index);
void unregister_memory_heap(ID3D12Heap *heap);
// Call after the device error observer is joined, so a late page fault report still sees the heaps.
void clear_memory_heaps();
void report_memory_heap_at(uint64_t gpu_address, eastl::string &out);

#else

constexpr void register_memory_heap(D3DDevice *, ID3D12Heap *, const char *, uint32_t) {}
constexpr void unregister_memory_heap(ID3D12Heap *) {}
constexpr void clear_memory_heaps() {}
constexpr void report_memory_heap_at(uint64_t, eastl::string &) {}

#endif

} // namespace drv3d_dx12::debug
