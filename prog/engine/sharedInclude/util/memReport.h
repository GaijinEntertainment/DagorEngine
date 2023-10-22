#pragma once

#include <osApiWrappers/dag_atomic.h>

namespace memreport
{
extern size_t sysmem_used_ref, sharedmem_used_ref, sysmem_ptrs_ref;
extern volatile int sysmem_used_max_kb;

static inline void update_max_used(ptrdiff_t cur_mem)
{
  int cur_mem_kb = int(cur_mem >> 10);
  int max_mem_kb = interlocked_acquire_load(sysmem_used_max_kb);
  while (cur_mem_kb > max_mem_kb)
    if (interlocked_compare_exchange(sysmem_used_max_kb, cur_mem_kb, max_mem_kb) == max_mem_kb)
      break;
    else
      max_mem_kb = interlocked_acquire_load(sysmem_used_max_kb);
}
} // namespace memreport

#define DECL_AND_GET_GPU_METRICS(NEED_GET)                                                                                      \
  int mem_used_discardable_kb = 0, mem_used_persistent_kb = 0, mem_used_discardable_kb_max = 0, mem_used_persistent_kb_max = 0, \
      mem_used_sum_kb_max = 0, tex_used_discardable_cnt = 0, tex_used_persistent_cnt = 0, max_mem_used_overdraft_kb = 0;        \
  if ((NEED_GET) && tql::streaming_enabled)                                                                                     \
  tql::get_tex_streaming_stats(mem_used_discardable_kb, mem_used_persistent_kb, mem_used_discardable_kb_max,                    \
    mem_used_persistent_kb_max, mem_used_sum_kb_max, tex_used_discardable_cnt, tex_used_persistent_cnt, max_mem_used_overdraft_kb)

#if _TARGET_XBOX
#define MEMREPORT_CRT_INCLUDES_GPUMEM 1
#else
#define MEMREPORT_CRT_INCLUDES_GPUMEM 0
#endif
