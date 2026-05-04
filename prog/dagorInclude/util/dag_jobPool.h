//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_threadPool.h>
#include <osApiWrappers/dag_atomic_types.h>
#include <osApiWrappers/dag_spinlock.h>
#include <EASTL/memory.h>
#include <EASTL/deque.h>
#include <mutex>


namespace threadpool
{

// Allows adding lambdas with add(), and then waiting for all jobs to finish with wait().
struct JobPool
{
  struct JobPoolWorker;
  OSSpinlock mutex;
  eastl::deque<cpujobs::IJob *> DAG_TS_GUARDED_BY(mutex) queue;
  dag::Vector<JobPoolWorker *> workers;
  bool DAG_TS_GUARDED_BY(mutex) done = false;

  struct JobPoolWorker : cpujobs::IJob
  {
    JobPool *pool;
    bool idle = false;
    bool running = true;
    JobPoolWorker(JobPool *p) : pool(p) {}
    virtual void doJob() override
    {
      while (true)
      {
        cpujobs::IJob *job = nullptr;
        {
          OSSpinlockScopedLock guard(pool->mutex);
          if (pool->queue.empty())
          {
            idle = true;
            if (pool->done)
              break;
          }
          else
          {
            idle = false;
            job = pool->queue.front();
            pool->queue.pop_front();
          }
        }
        if (job)
        {
          job->doJob();
          job->releaseJob();
        }
        else
          sleep_usec(1);
      }
      {
        OSSpinlockScopedLock guard(pool->mutex);
        running = false;
      }
    }
    virtual const char *getJobName(bool &) const override { return "JobPoolWorker"; }
    virtual void releaseJob() override {}
  };

  JobPool(int num_workers, JobPriority prio = PRIO_HIGH)
  {
    num_workers = min(num_workers, get_num_workers());
    num_workers = max(num_workers, 0);
    workers.reserve(num_workers);
    for (int i = 0; i < num_workers; i++)
    {
      auto w = new JobPoolWorker(this);
      uint32_t queuePos;
      if (!threadpool::add(w, prio, queuePos))
      {
        delete w;
        break;
      }
      workers.push_back(w);
    }
  }

  ~JobPool() { stop(); }

  void addJob(cpujobs::IJob *job)
  {
    if (!job)
      return;
    if (workers.empty())
    {
      job->doJob();
      job->releaseJob();
    }
    else
    {
      OSSpinlockScopedLock guard(mutex);
      queue.push_back(job);
    }
  }

  template <typename F>
  void add(F &&func)
  {
    struct JobPoolJob final : cpujobs::IJob
    {
      F func;
      JobPoolJob(F &&f) : func(eastl::forward<F>(f)) {}
      virtual void doJob() override { func(); }
      virtual void releaseJob() override { delete this; }
      virtual const char *getJobName(bool &) const override { return "JobPoolJob"; }
    };

    auto job = new JobPoolJob(eastl::forward<F>(func));
    G_ASSERT(job);
    addJob(job);
  }

  void wait()
  {
    while (true)
    {
      cpujobs::IJob *job = nullptr;
      {
        OSSpinlockScopedLock guard(mutex);
        if (queue.empty())
        {
          if (eastl::all_of(workers.begin(), workers.end(), [](auto w) { return w->idle; }))
            break;
        }
        else
        {
          job = queue.front();
          queue.pop_front();
        }
      }
      if (job)
      {
        job->doJob();
        job->releaseJob();
      }
      else
        sleep_usec(1);
    }
  }

  void stop()
  {
    {
      OSSpinlockScopedLock guard(mutex);
      done = true;
    }
    wait();
    while (true)
    {
      {
        OSSpinlockScopedLock guard(mutex);
        if (eastl::all_of(workers.begin(), workers.end(), [](auto w) { return !w->running; }))
        {
          G_ASSERT(queue.empty());
          queue.clear();
          queue.shrink_to_fit();
          break;
        }
      }
      sleep_usec(1);
    }
    clear_all_ptr_items(workers);
  }
};

} // namespace threadpool
