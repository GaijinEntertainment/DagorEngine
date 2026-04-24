// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <perfMon/dag_daProfileMemory.h>
#include <stdlib.h>

namespace da_profiler
{

void dump_memory_map(MemDumpCb &) {}

void profile_deallocation(size_t, profile_mem_data_t) {}

void profile_reallocation(size_t, size_t, profile_mem_data_t) {}

profile_mem_data_t profile_allocation(size_t, const void *const *, unsigned) { return invalid_memory_profile; }

profile_mem_data_t profile_allocation(size_t, profile_mem_data_t) { return invalid_memory_profile; }

void profile_memory_profiler_allocations(size_t &ac, size_t &ue) { ac = ue = 0; }

void profile_memory_allocated(size_t &pc, size_t &pe, size_t &pm, size_t &me) { pc = pe = pm = me = 0; }

uint32_t profile_get_entry(profile_mem_data_t, uintptr_t *, uint32_t, size_t &a, size_t &as)
{
  as = a = 0;
  return 0;
}


}; // namespace da_profiler

#define EXPORT_PULL dll_pull_memory_profile
#include <supp/exportPull.h>
