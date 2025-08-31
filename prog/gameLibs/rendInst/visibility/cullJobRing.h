// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_threadPool.h>

#include "visibility/cullJob.h"


namespace rendinst
{
inline constexpr int MAX_CULL_JOBS = 8;

struct CullJobRing
{
  const CullJobSharedState &info;
  uint32_t queue_pos = 0;
  threadpool::JobPriority prio = threadpool::PRIO_HIGH;
  StaticTab<CullJob, MAX_CULL_JOBS> jobs;

  CullJobRing(int num_jobs, const CullJobSharedState &jinfo) : info(jinfo)
  {
    G_FAST_ASSERT(num_jobs > 0);
    prio = is_main_thread() ? threadpool::PRIO_HIGH : threadpool::PRIO_NORMAL;
    jobs.resize(min(num_jobs, MAX_CULL_JOBS));
    for (int i = 0; i < jobs.size(); ++i)
      jobs[i].set(i, &info);
  }
  void start()
  {
    for (int i = 0; i < (int)jobs.size() - 1; ++i)
    {
      G_FAST_ASSERT(interlocked_relaxed_load(jobs[i].done)); // Either not started yet or already finished
      threadpool::add(&jobs[i], prio, queue_pos, threadpool::AddFlags::None);
    }
    threadpool::wake_up_one(); // Note: rest is woken up by first job
  }
  void finishWork()
  {
    jobs.back().doJob();
    bool haveAnyNonDoneJobs = false;
    for (int i = 0, n = (int)jobs.size() - 1; i < n && !haveAnyNonDoneJobs; ++i)
      haveAnyNonDoneJobs |= !interlocked_acquire_load(jobs[i].done);
    if (!haveAnyNonDoneJobs)
      return;

    DA_PROFILE_WAIT("wait_ri_cull_jobs");

    // Warning: scenes are read locked at this point, so any job that want to grab write lock for it will deadlock,
    // therefore try to avoid active wait if we can; can't do that if there are no enough threadpool workers available
    // since there are might be logical dependencies between jobs (e.g. vis. prepare job wait for occlusion/reproj jobs)
    intptr_t spins = SPINS_BEFORE_SLEEP * (threadpool::get_num_workers() / 3u);
    while (spins-- > 0)
      if (interlocked_acquire_load(jobs[jobs.size() - 2].done))
      {
        for (int i = 0, n = (int)jobs.size() - 2; i < n; ++i)
          threadpool::wait(&jobs[i]);
        return;
      }
      else
        cpu_yield();

    // Spins are exhausted -> do active wait starting from last added job
    for (int i = (int)jobs.size() - 2; i >= 0; --i)
      threadpool::barrier_active_wait_for_job(&jobs[i], prio, queue_pos);
  }
};

} // namespace rendinst
