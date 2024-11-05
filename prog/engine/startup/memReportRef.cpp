// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_memReport.h>
#include <memory/dag_memStat.h>
#include <3d/tql.h>
#include <util/memReport.h>

size_t memreport::sysmem_used_ref = 0, memreport::sharedmem_used_ref = 0, memreport::sysmem_ptrs_ref = 0;
volatile int memreport::sysmem_used_max_kb = 0;

void memreport::set_sysmem_reference()
{
  sysmem_used_ref = dagor_memory_stat::get_memory_allocated(true);
  sysmem_ptrs_ref = dagor_memory_stat::get_memchunk_count(true);

#if MEMREPORT_CRT_INCLUDES_GPUMEM
  DECL_AND_GET_GPU_METRICS(true);
  sharedmem_used_ref = size_t(mem_used_discardable_kb + mem_used_persistent_kb) << 10;
#endif
}
void memreport::reset_sysmem_reference() { sysmem_used_ref = 0, sharedmem_used_ref = 0, sysmem_ptrs_ref = 0; }
