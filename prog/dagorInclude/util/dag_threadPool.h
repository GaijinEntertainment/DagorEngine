//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_atomic.h>
#include <generic/dag_enumBitMask.h>

#include <supp/dag_define_KRNLIMP.h>

namespace cpujobs
{
class IJob;
}

namespace threadpool
{
enum JobPriority
{
  PRIO_HIGH,
  PRIO_NORMAL,
  PRIO_LOW,

  NUM_PRIO,
};

static constexpr JobPriority PRIO_DEFAULT = PRIO_NORMAL;

struct JobEnvironment
{
  uint16_t coreId = 0;
  void *d3dContext = NULL;
  void *localData = NULL;

  JobEnvironment() {}
};

class IJobNode : public cpujobs::IJob
{
public:
  void *jobDataPtr = NULL;
  uint32_t jobDataSize = 0;
  volatile int jobCount = 1; // atomically decremented by workers

  IJobNode() {}
  virtual ~IJobNode() {}
  virtual void doJob() /*final*/ {}  // block this, incompatible (PVS false positive V762)
  virtual void releaseJob() final {} // not called

  //! called by job manager to perform task; executed in context of worker thread
  virtual void doJob(JobEnvironment *job_env, void *job_data_ptr, unsigned job_index) = 0; //-V762

  void setJobData(void *ptr, size_t data_size_per_job)
  {
    jobDataPtr = ptr;
    jobDataSize = uint32_t(data_size_per_job);
  }
};

// if num_workers == 0, jobs will be executed synchronously
// queue_size must be power of 2
KRNLIMP void init_ex(int num_workers, int queue_sizes[NUM_PRIO], size_t stack_size = 64 << 10,
  uint64_t max_workers_prio = 0); // max per-worker prio (2 bits per worker)

inline void init(int num_workers, int queue_size, size_t stack_size = 64 << 10, uint64_t max_workers_prio = 0)
{
  int q_sizes[] = {queue_size, queue_size, queue_size};
  init_ex(num_workers, q_sizes, stack_size, max_workers_prio);
}

KRNLIMP void shutdown();

KRNLIMP JobEnvironment *get_job_env(int worker_index);

KRNLIMP int get_num_workers(); // Returns 0 if not inited/shutdowned
KRNLIMP int get_queue_size(JobPriority prio);

enum class AddFlags : uint32_t
{
  None = 0,
  WakeOnAdd = 1,
  IgnoreNotDone = 2,
  Default = WakeOnAdd
};
DAGOR_ENABLE_ENUM_BITMASK(AddFlags)

KRNLIMP bool add(cpujobs::IJob *j, JobPriority prio, uint32_t &queue_pos, AddFlags flags = AddFlags::Default);

// will actively perform jobs of same priority, if there are any in queue earlier than target, otherwise - waits. Allows implementing
// simple barrier.
KRNLIMP void barrier_active_wait_for_job_ool(cpujobs::IJob *j, JobPriority prio, uint32_t queue_pos, uint32_t wait_token);
inline void barrier_active_wait_for_job(cpujobs::IJob *j, JobPriority prio, uint32_t queue_pos, uint32_t wait_token = 0)
{
  if (!interlocked_acquire_load(j->done))
    barrier_active_wait_for_job_ool(j, prio, queue_pos, wait_token);
}

// Note : j->releaseJob() is not called!
inline bool add(cpujobs::IJob *j, JobPriority prio = PRIO_DEFAULT, bool wake_up = true)
{
  uint32_t pos;
  return add(j, prio, pos, wake_up ? AddFlags::WakeOnAdd : AddFlags::None);
}
KRNLIMP bool add_node(threadpool::IJobNode *j, uint32_t start_index, uint32_t num_jobs, JobPriority prio = PRIO_DEFAULT,
  bool wake_up = true);

// If lowest_prio_to_perform is in [PRIO_HIGH..NUM_PRIO] - waiting will be busy doing other jobs of higher priorities (NUM_PRIO mean
// all) The reasons to do that depends on caller estimations of waiting times
KRNLIMP void wait_ool(cpujobs::IJob *j, uint32_t profile_token, int lowest_prio_to_perform);
inline void wait(cpujobs::IJob *j, uint32_t profile_token = 0, int lowest_prio_to_perform = -1)
{
  if (!interlocked_acquire_load(j->done))
    wait_ool(j, profile_token, lowest_prio_to_perform);
}

KRNLIMP void wake_up_one();                                                          // Wake one (any) currently sleeping worker
KRNLIMP void wake_up_all();                                                          // Wake up all of the currently sleeping workers.
KRNLIMP void wake_up_all_delayed(JobPriority prio = PRIO_DEFAULT, bool wake = true); // Same functionality as prev, but waking up
                                                                                     // itself performed in job

// will return [0 ... num_workers) for worker thread and -1 otherwise
KRNLIMP int get_current_worker_id();
}; // namespace threadpool

#include <supp/dag_undef_KRNLIMP.h>
