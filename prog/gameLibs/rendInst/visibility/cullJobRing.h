#pragma once

#include <util/dag_threadPool.h>

#include "visibility/cullJob.h"


namespace rendinst
{
inline constexpr int MAX_CULL_JOBS = 8;

struct CullJobRing
{
  StaticTab<CullJob, MAX_CULL_JOBS> jobs;
  CullJobSharedState info;
  uint32_t queue_pos = 0;
  threadpool::JobPriority prio = threadpool::PRIO_HIGH;

  void start(int num_jobs, const CullJobSharedState &jinfo)
  {
    G_FAST_ASSERT(num_jobs > 0);
    num_jobs = min(num_jobs - 1, (int)MAX_CULL_JOBS);
    info = jinfo;
    prio = is_main_thread() ? threadpool::PRIO_HIGH : threadpool::PRIO_NORMAL;
    jobs.resize(num_jobs);
    for (int i = 0; i < jobs.size(); ++i)
    {
      jobs[i].set(i + 1, &info);
      threadpool::add(&jobs[i], prio, queue_pos, threadpool::AddFlags::None);
    }
    threadpool::wake_up_all();
  }
  void finishWork()
  {
    CullJob::perform_job(0, &info);
    if (jobs.empty()) // could be if 1 worker
      return;

    // Warning: scenes are read locked at this point, so any job that want to grab write lock for it will deadlock
    barrier_active_wait_for_job(&jobs.back(), prio, queue_pos);

    DA_PROFILE_WAIT("wait_ri_cull_jobs");
    for (auto &j : jobs)
      threadpool::wait(&j);
  }
};

} // namespace rendinst
