// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_memReport.h>
#include <debug/dag_debug.h>
#include <memory/dag_memStat.h>
#include <perfMon/dag_cpuFreq.h>
#include <3d/tql.h>
#include <util/memReport.h>

static int last_report_t_msec = 0;

void memreport::dump_memory_usage_report(int time_interval_msec, bool sysmem, bool gpumem)
{
  if (last_report_t_msec && last_report_t_msec + time_interval_msec > get_time_msec())
    return;

  size_t total_mem = dagor_memory_stat::get_memory_allocated(true);
#if MEMREPORT_CRT_INCLUDES_GPUMEM
  DECL_AND_GET_GPU_METRICS(gpumem || sysmem);
  memreport::update_max_used(total_mem - (size_t(mem_used_discardable_kb + mem_used_persistent_kb) << 10));
#else
  DECL_AND_GET_GPU_METRICS(gpumem);
  memreport::update_max_used(total_mem);
#endif

  if (sysmem)
  {
    size_t dagor_mem = dagor_memory_stat::get_memory_allocated(false);
    size_t total_ptr = dagor_memory_stat::get_memchunk_count(true);
    size_t dagor_ptr = dagor_memory_stat::get_memchunk_count(false);
#if MEMREPORT_CRT_INCLUDES_GPUMEM
    total_mem -= size_t(mem_used_discardable_kb + mem_used_persistent_kb) << 10;
#endif
    debug("SYSmem: %dK in %d ptrs (+CRT %dK in %d ptrs) Max=%dM", dagor_mem >> 10, dagor_ptr, (total_mem - dagor_mem) >> 10,
      total_ptr - dagor_ptr, interlocked_acquire_load(sysmem_used_max_kb) >> 10);
    if (sysmem_used_ref)
      debug("  DIFF: %dK in %d ptrs [+base %dM]", ptrdiff_t(total_mem - (sysmem_used_ref - sharedmem_used_ref)) / 1024,
        total_ptr - sysmem_ptrs_ref, (sysmem_used_ref - sharedmem_used_ref) >> 20);
  }

  if (gpumem && tql::streaming_enabled)
  {
    debug("GPUmem: %dK  [%dK (%d tex) +%dK pers]", mem_used_discardable_kb + mem_used_persistent_kb, mem_used_discardable_kb,
      tex_used_discardable_cnt, mem_used_persistent_kb);
    debug("GPUmem: Quota=%dM  Max=%dM [%dM + %dM]", tql::mem_quota_kb >> 10, mem_used_sum_kb_max >> 10,
      mem_used_discardable_kb_max >> 10, mem_used_persistent_kb_max >> 10);
  }
  if (sysmem || gpumem)
    debug("----");

  last_report_t_msec = get_time_msec();
}
