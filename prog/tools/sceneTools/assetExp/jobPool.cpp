// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "jobSharedMem.h"
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_miscApi.h>
#include <perfMon/dag_cpuFreq.h>
#include <debug/dag_debug.h>
#include <stdio.h>
#include <atomic>

#include <libTools/util/atomicPrintf.h>
AtomicPrintfMutex AtomicPrintfMutex::inst;

void DabuildJobSharedMem::JobCtx::setup(unsigned tc, const char *prof)
{
  G_STATIC_ASSERT(sizeof(DabuildJobSharedMem::JobCtx) == 1024);

  targetCode = tc;
  memset((char *)profileName, 0, sizeof(profileName));
  if (prof)
    strncpy((char *)profileName, prof, 32);
}
void DabuildJobSharedMem::changeCtxState(JobCtx &ctx, int state)
{
  if (ctx.state >= 4)
    return;
  std::atomic_thread_fence(std::memory_order_seq_cst);
  ctx.state = state;
  interlocked_increment(respGen);
}

DabuildJobPool::Ctx *DabuildJobPool::getAvailableJobId(int timeout_msec)
{
  int te = get_time_msec() + timeout_msec;
  for (;;) // infinite cycle with explicit break
  {
    int inoperable_jobs = 0;
    for (int k = 0; k < jobs; k++)
    {
      switch (m->jobCtx[cJobIdx].state)
      {
        case 4:
          ATOMIC_PRINTF("ERR: job j%02d become inoperable\n", cJobIdx);
          logerr("job j%02d become inoperable", cJobIdx);
          std::atomic_thread_fence(std::memory_order_seq_cst);
          m->jobCtx[cJobIdx].state = 5;
          interlocked_increment(m->respGen);
          inoperable_jobs++;
          break;
        case 5: inoperable_jobs++; break;
        case 3:
        case 0:
          Ctx *ctx = &m->jobCtx[cJobIdx];
          cJobIdx = (cJobIdx + 1) % jobs;
          return ctx;
      }
      cJobIdx = (cJobIdx + 1) % jobs;
    }
    if (!timeout_msec || inoperable_jobs == jobs)
      return NULL;

    int gen = m->respGen;
    if (gen == respGen)
      sleep_msec(100);
    respGen = gen;

    if (get_time_msec() > te)
      return NULL;
  }
  return NULL;
}

bool DabuildJobPool::waitAllJobsDone(int timeout_msec, Ctx **out_done)
{
  int te = get_time_msec() + timeout_msec;
  static int jobs_in_flight = -1;
  for (;;) // infinite cycle with explicit break
  {
    int working = 0, gen = m->respGen;
    if (gen == respGen)
      sleep_msec(100);
    respGen = gen;

    for (int i = 0; i < jobs; i++)
      switch (m->jobCtx[i].state)
      {
        case 4:
          ATOMIC_PRINTF("ERR: job j%02d become inoperable\n", i);
          logerr("job j%02d become inoperable", i);
          [[fallthrough]];
        case 3:
          m->jobCtx[i].state = (m->jobCtx[i].state == 3) ? 0 : 5;
          if (out_done)
          {
            *out_done = &m->jobCtx[i];
            return false;
          }
        case 0:
        case 5: // inoperable
          break;

        default: working++; break;
      }
    if (jobs_in_flight != working)
    {
      jobs_in_flight = working;
      debug("%s: %d jobs in progress", __FUNCTION__, working);
      debug_flush(false);
      fflush(stdout);
    }
    if (!working)
      return true;

    if (get_time_msec() > te)
    {
      if (out_done)
        *out_done = NULL;
      return false;
    }
  }
  return false;
}

void DabuildJobPool::startJob(Ctx *ctx, int cmd)
{
  ctx->cmd = cmd;
  std::atomic_thread_fence(std::memory_order_seq_cst);
  ctx->state = 1;
  interlocked_increment(m->cmdGen);
}

bool DabuildJobPool::startAllJobs(int broadcast_cmd, unsigned target_code, const char *profile)
{
  int busy_cnt = 0;
  for (int i = 0; i < jobs; i++)
    switch (m->jobCtx[i].state)
    {
      case 3:
      case 0:
        m->jobCtx[i].cmd = broadcast_cmd;
        m->jobCtx[i].setup(target_code, profile);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        m->jobCtx[i].state = 1;
        interlocked_increment(m->cmdGen);
        break;
      case 4: // inoperable
      case 5: break;
      default: busy_cnt++;
    }
  return busy_cnt == 0;
}
