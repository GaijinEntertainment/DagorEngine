//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_threadPool.h>
#include <debug/dag_assert.h>
#include <EASTL/algorithm.h>
#include <perfMon/dag_statDrv.h>
#include <stdlib.h> // alloca
#include <osApiWrappers/dag_atomic_types.h>
#include <osApiWrappers/dag_miscApi.h>
#include <supp/dag_alloca.h>

namespace threadpool
{
/*
  will basically call, in threads following loop

  for (uint i = begin; i<end; i+=quant)
    cb(i, min(end-i, quant), thread_id);

  parallel_for(uint32_t begin, uint32_t end, uint32_t quant,
               eastl::function<void(uint32_t tbegin, uint32_t tend, uint32_t thread_id)> cb,
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

namespace internal
{

enum class CallerRole
{
  ProcessUniformly,
  ProcessFirstChunkOnCaller,
};

template <CallerRole role, typename Cb>
inline void parallel_for_inline_impl(uint32_t begin, uint32_t end, uint32_t quant, Cb cb, uint32_t add_jobs_count, JobPriority prio,
  bool wake, uint32_t dapDescId)
{
  if (end == begin)
    return;
  add_jobs_count = add_jobs_count ? add_jobs_count : get_num_workers();
  add_jobs_count = eastl::min(add_jobs_count, (end - begin + quant - 1) / quant - 1); // one
  if (!add_jobs_count)                                                                // no job for workers!
  {
    for (uint32_t i = 0, ie = (end - begin) / quant; i != ie; ++i, begin += quant)
      cb(begin, begin + quant, 0);
    if (begin < end)
      cb(begin, end, 0);
    return;
  }
  TIME_PROFILE(parallel_for);
  struct ParallelForJob final : cpujobs::IJob
  {
    uint32_t end, quant, workerId, dapToken;
    Cb &cb;
    dag::AtomicInteger<uint32_t> &current;
    ParallelForJob(dag::AtomicInteger<uint32_t> &counter, Cb &cb_, uint32_t e, uint32_t q, uint32_t worker_id, uint32_t token) :
      current(counter), cb(cb_), end(e), quant(q), workerId(worker_id), dapToken(token)
    {}
    virtual const char *getJobName(bool &) const override { return "ParallelForJob"; }
    virtual void doJob() override
    {
      // would be nice to put marker here...
      if (current.load(dag::memory_order_relaxed) >= end)
        return;
      DA_PROFILE_EVENT_DESC(dapToken);
      for (uint32_t at = current.fetch_add(quant); at < end; at = current.fetch_add(quant))
        cb(at, at + eastl::min(quant, end - at), workerId);
    }
  };
  // To avoid false sharing on `job->done`, we allocate two (on x86) cache lines space
#if defined(__aarch64__) || defined(_M_ARM64)
  static constexpr size_t stack_per_job = eastl::max((size_t)64, sizeof(ParallelForJob));
#else
  static constexpr size_t stack_per_job = eastl::max((size_t)64 * 2, sizeof(ParallelForJob));
#endif
  const uint32_t counter_init = (role == CallerRole::ProcessFirstChunkOnCaller) ? (begin + quant) : begin;
  dag::AtomicInteger<uint32_t> current = {counter_init};
  const auto wakeOnAdd = (add_jobs_count <= 2 && wake) ? threadpool::AddFlags::WakeOnAdd : threadpool::AddFlags::None;
  uint32_t queue_pos;
  ParallelForJob *lastJob;
  alignas(ParallelForJob) char staticJobsStorage[threadpool::MAX_WORKER_COUNT * stack_per_job];
  // stride is stack_per_job (pads for false-sharing); alloca must match the stride, not
  // sizeof(ParallelForJob), or slot 1+ overruns the allocation into the caller's stack.
  char *jobsStorage = DAGOR_LIKELY(add_jobs_count <= threadpool::MAX_WORKER_COUNT)
                        ? staticJobsStorage
                        : (char *)alloca(add_jobs_count * stack_per_job), // To consider: framemem?
    *jobStorage = jobsStorage;

  static uint32_t def_desc = 0;
  if (!dapDescId)
  {
    uint32_t d = interlocked_relaxed_load(def_desc);
    if (!d)
      interlocked_relaxed_store(def_desc, d = DA_PROFILE_ADD_DESCRIPTION(__FILE__, __LINE__, "parallel_for_job"));
    dapDescId = d;
  }

  int i = 0;
  do
  {
    lastJob = new (jobStorage, _NEW_INPLACE) ParallelForJob(current, cb, end, quant, i + 1, dapDescId);
    add(lastJob, prio, queue_pos, wakeOnAdd | AddFlags::IgnoreNotDone); // we just allocated jobs, job->done == 1 for sure
    ++i;
    jobStorage += stack_per_job;
  } while (i < add_jobs_count);


  // To consider: instead of explicit wake_all & cb execution we can just call active wait and that's it.

  if (wake && wakeOnAdd == threadpool::AddFlags::None) // otherwise already woken
    wake_up_all();

  {
    DA_PROFILE_EVENT_DESC(dapDescId);
    if constexpr (role == CallerRole::ProcessFirstChunkOnCaller)
    {
      // first chunk runs directly on caller thread
      cb(begin, eastl::min(begin + quant, end), 0);
    }
    else
    {
      for (uint32_t at = current.fetch_add(quant); at < end; at = current.fetch_add(quant))
        cb(at, at + eastl::min(quant, end - at), 0);
    }
  }
  barrier_active_wait_for_job(lastJob, prio, queue_pos);
  // ok, all job _work_ is done here.
  // however, we still have to wait for all jobs to write there 'done' flag, otherwise they can spoil stack (when writing jobDone)

  jobStorage = jobsStorage;
  for (int i = 0; i < add_jobs_count; ++i, jobStorage += stack_per_job)
  {
    ParallelForJob *job;
    memcpy(&job, &jobStorage, sizeof(job)); // to avoid UB
    threadpool::wait(job);
    job->~ParallelForJob(); // does nothing
  }
}
} // namespace internal

template <typename Cb>
inline void parallel_for_inline(uint32_t begin, uint32_t end, uint32_t quant, Cb cb, uint32_t add_jobs_count = 0,
  JobPriority prio = PRIO_HIGH, bool wake = true,
  uint32_t dapDescId = 0) // wide load expects high priority
{
  internal::parallel_for_inline_impl<internal::CallerRole::ProcessUniformly>(begin, end, quant, eastl::move(cb), add_jobs_count, prio,
    wake, dapDescId);
}

// first chunk takes [begin, begin + quant) and runs it on the caller thread, the rest works as in parallel_for_inline
template <typename Cb>
inline void parallel_for_inline_caller_first(uint32_t begin, uint32_t end, uint32_t quant, Cb cb, uint32_t add_jobs_count = 0,
  JobPriority prio = PRIO_HIGH, bool wake = true, uint32_t dapDescId = 0)
{
  internal::parallel_for_inline_impl<internal::CallerRole::ProcessFirstChunkOnCaller>(begin, end, quant, eastl::move(cb),
    add_jobs_count, prio, wake, dapDescId);
}

}; // namespace threadpool
