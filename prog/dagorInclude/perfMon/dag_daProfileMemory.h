//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>
#include <supp/dag_define_KRNLIMP.h>

namespace da_profiler
{

// these are required link-time-resolved dependencies!
KRNLIMP void *unprofiled_mem_alloc(size_t);
KRNLIMP void unprofiled_mem_free(void *);

typedef uint64_t profile_mem_data_t;
static constexpr profile_mem_data_t invalid_memory_profile = 0;

class MemDumpCb
{
protected:
  virtual ~MemDumpCb() {}

public:
  // dump will be called with frame
  virtual void dump(profile_mem_data_t unique_id, const uintptr_t *stack, uint32_t stack_size, size_t allocated,
    size_t allocations) = 0;
};

// fills callstack of given uid, returns callstack len filled
KRNLIMP uint32_t profile_get_entry(profile_mem_data_t unique_id, uintptr_t *callstack, uint32_t max_callstack_size,
  size_t &allocations, size_t &allocated);

// doesn't return amount of memory allocated by profiler, but is very fast function
KRNLIMP void profile_memory_profiler_allocations(size_t &profiler_chunks, size_t &profiled_entries);

// relatively slow function, will iterate all entries to gather allocated memory
KRNLIMP void profile_memory_allocated(size_t &profiler_chunks, size_t &profiled_entries, size_t &profiled_memory,
  size_t &max_entry_sz);

// dumps all uniqure memory chunks
KRNLIMP void dump_memory_map(MemDumpCb &cb);

// to be called from memory allocator
KRNLIMP profile_mem_data_t profile_allocation(size_t n, const void *const *stack, unsigned stack_size);
KRNLIMP void profile_deallocation(size_t n, profile_mem_data_t data);
KRNLIMP void profile_reallocation(size_t old_sz, size_t new_sz, profile_mem_data_t data);
KRNLIMP profile_mem_data_t profile_allocation(size_t n, profile_mem_data_t existing_profile);

} // namespace da_profiler

#include <supp/dag_undef_KRNLIMP.h>
