//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_threadPool.h>
#include <EASTL/fixed_function.h>

#include <supp/dag_define_KRNLIMP.h>
namespace threadpool
{
/*
  will basically call, in threads following loop

  for (uint i = begin; i<end; i+=quant)
    cb(i, min(end-i, quant), thread_id);

  parallel_for(uint32_t begin, uint32_t end, uint32_t quant,
               eastl::fixed_function<sizeof(void*)*2, void(uint32_t tbegin, uint32_t tend, uint32_t thread_id)> cb,
               uint32_t add_jobs_count = 0, JobPriority prio = PRIO_HIGH, bool wake = true);

  cb:
    call back function.
    must be reentrant, as it will be callled from different threads.
    * tbegin: in [begin, end) range
    * tend: in (tbegin, end) range
    * thread_id:
      allows parallel summation ad such without locking primitives
      you can be sure that it is never more outside [0, add_jobs_count ? add_jobs_count : get_num_workers()]

  add_jobs_count: if 0, same as get_num_workers().
    if you are calling from threads or you know some of threads are busy - can specify less threads
  prio: by default, uses highest priority, as it suppose to end asap
  wake: wakeup all threads. Better specify true, unless you know what are you doing
*/
KRNLIMP void parallel_for(uint32_t begin, uint32_t end, uint32_t quant,
  eastl::fixed_function<sizeof(void *) * 2, void(uint32_t tbegin, uint32_t tend, uint32_t thread_id)> cb, uint32_t add_jobs_count = 0,
  JobPriority prio = PRIO_HIGH, bool wake = true, uint32_t dapDescId = 0); // wide load expects high priority
}; // namespace threadpool

#include <supp/dag_undef_KRNLIMP.h>
