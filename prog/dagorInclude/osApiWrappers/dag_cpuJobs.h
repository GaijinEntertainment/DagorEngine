//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_stdint.h>
#include <supp/dag_define_COREIMP.h>


namespace cpujobs
{
template <typename T, bool MULTI_PRODUCER>
struct JobQueue;
extern volatile bool abortJobsRequested;

class IJob
{
public:
  IJob() : next(NULL), needRelease(1) {}
  virtual ~IJob() {}

  //! called by job manager to perform task; executed in context of worker thread
  virtual void doJob() = 0;

  //! called by job manager after job is performed; executed in context of main thread
  virtual void releaseJob() {}

  //! called by job manager to perform task; executed in context of worker thread
  virtual unsigned getJobTag() { return 0; };

  IJob *next;
  union
  {
    volatile int needRelease;
    volatile int done;
  };
};

static constexpr int DEFAULT_THREAD_PRIORITY = 0; // MS - THREAD_PRIORITY_NORMAL
static constexpr int THREAD_PRIORITY_LOWER_STEP = -1;
static constexpr int COREID_IMMEDIATE = 0x7FFFFFFF; // Pseudo-coreId to be used with add_job() to call {do,release}Job() immediately
static constexpr int DEFAULT_IDLE_TIME_SEC = 60;


//! initialize cpu jobs management system
//! specify force_core_count > 0 for disable autodetect
//! pass reserve_jobmgr_cores = true to unlock start_job_manager API
KRNLIMP void init(int force_core_count = -1, bool reserve_jobmgr_cores = true);
//! terminate cpu jobs management system, optionally cancelling all pending jobs
KRNLIMP void term(bool cancel_jobs, int timeout_msec = 10000);


//! returns cpujobs init status
KRNLIMP int is_inited();


//! return number of logical (SMT/HT) CPU cores
KRNLIMP int get_logical_core_count();
inline int get_core_count() { return get_logical_core_count(); } // compat alias

//! return number of physical CPU cores
KRNLIMP int get_physical_core_count();

//! return number of logical CPU cores per Last Level Cache (typically L3).
KRNLIMP int get_core_count_per_llc();

//! start job manager on specified Core. require reserve_jobmgr_cores = true passed in init
KRNLIMP bool start_job_manager(int core_id, int stack_sz = 64 << 10, const char *thread_name = NULL,
  int thread_priority = DEFAULT_THREAD_PRIORITY);

//! sends stop signal to job manager on specified Core
//! no jobs can be added after manager is stopped;
//! processing of current job queue will be cancelled if finish_jobs==false
KRNLIMP bool stop_job_manager(int core_id, bool finish_jobs);


//! creates virtual job manager (not restricted to single Core); returns manager ID
//! optional affinity mask restricts sheduling of jobs to masked Cores
KRNLIMP int create_virtual_job_manager(int stack_sz = 64 << 10, uint64_t affinity_mask = ~uint64_t(0), const char *thread_name = NULL,
  int thread_priority = DEFAULT_THREAD_PRIORITY,
  int idle_time_sec = DEFAULT_IDLE_TIME_SEC); // Note: 0 disables quit by idle timeout

//! sends stop signal to virtual job manager
//! no jobs can be added after manager is stopped;
//! processing of current job queue will be cancelled if finish_jobs==false
KRNLIMP void destroy_virtual_job_manager(int vmgr_id, bool finish_jobs);


//! add job to job manager on specified Core or by virtual manager ID; returns false on error
KRNLIMP bool add_job(int core_or_vmgr_id, IJob *job, bool prepend = false, bool need_release = true);

//! removes all queued jobs except for current; optionally calls release_done_jobs()
KRNLIMP void reset_job_queue(int core_or_vmgr_id, bool auto_release_jobs = true);

//! removes tagged queued jobs except for current; optionally calls release_done_jobs()
KRNLIMP void remove_jobs_by_tag(int core_or_vmgr_id, unsigned tag, bool auto_release_jobs = true);

//! returns true if job manager has active jobs
KRNLIMP bool is_job_manager_busy(int core_or_vmgr_id);

//! returns true if job is found in manager's main queue
KRNLIMP bool is_job_running(int core_or_vmgr_id, IJob *job);

//! returns true if job is found in manager's done queue
KRNLIMP bool is_job_done(int core_or_vmgr_id, IJob *job);

//! returns thread id which belongs to specified job manager; use get_current_thread_id() to compare
KRNLIMP int64_t get_job_manager_thread_id(int vmgr_id);


//! returns true when current thread is executing in context of any job manager
KRNLIMP bool is_in_job();

//! returns true if job manager is to be cancelled soon
KRNLIMP bool is_exit_requested(int core_or_vmgr_id);

//! releases all done jobs, must be called from main thread
//! this routine should be called regularly to release resources allocated by finished jobs
KRNLIMP void release_done_jobs();
} // namespace cpujobs

#include <supp/dag_undef_COREIMP.h>
