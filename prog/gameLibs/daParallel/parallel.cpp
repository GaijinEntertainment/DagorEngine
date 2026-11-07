// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "parallel_api.h"
#include <osApiWrappers/dag_atomic_types.h>
#include <util/dag_globDef.h>
#ifndef DAPARALLEL
#define DAPARALLEL 1
#endif
#if DAPARALLEL
#include <util/dag_parallelFor.h>
#endif

namespace daparallel
{
static dag::AtomicInteger<uint32_t> active_threads = 0, peak_threads = 0;
static dag::AtomicFlag count_threads;
static thread_local uint32_t body_depth = 0;

uint32_t worker_count(int threads)
{
#if DAPARALLEL
  const uint32_t pool = threadpool::get_num_workers();
  return 1 + (threads <= 0 ? pool : min(uint32_t(threads - 1), pool));
#else
  G_UNUSED(threads);
  return 1;
#endif
}

void parallel_run(uint32_t workers, uint32_t n, uint32_t quant, dag::FunctionRef<void(uint32_t, uint32_t, uint32_t)> cb)
{
  const bool count = count_threads.test(dag::memory_order_relaxed);
  // a thread counts once in nested loops
  auto counted = [&](uint32_t begin, uint32_t end, uint32_t worker) {
    if (!body_depth++)
    {
      const uint32_t now = active_threads.fetch_add(1) + 1;
      for (uint32_t peak = peak_threads.load(); now > peak && !peak_threads.compare_exchange_weak(peak, now);)
        ;
    }
    cb(begin, end, worker);
    if (!--body_depth)
      active_threads.fetch_sub(1);
  };
#if DAPARALLEL
  if (workers > 1)
  {
    if (count)
      threadpool::parallel_for(0, n, quant, counted, workers - 1);
    else
      threadpool::parallel_for(0, n, quant, cb, workers - 1);
    return;
  }
#else
  G_UNUSED(workers);
#endif
  for (uint32_t begin = 0; begin < n; begin += quant)
    count ? counted(begin, min(begin + quant, n), 0) : cb(begin, min(begin + quant, n), 0);
}

uint32_t parallel_peak_threads(bool reset)
{
  count_threads.test_and_set(dag::memory_order_relaxed);
  const uint32_t peak = peak_threads.load();
  if (reset)
    peak_threads.store(0);
  return peak;
}
} // namespace daparallel
