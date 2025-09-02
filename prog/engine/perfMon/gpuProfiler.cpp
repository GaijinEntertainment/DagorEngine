// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <perfMon/gpuProfiler.h>
#include <drv/3d/dag_driver.h>
#include <perfMon/dag_graphStat.h>
#include <drv/3d/dag_lock.h>
#include <perfMon/dag_perfTimer.h>
#include <EASTL/unique_ptr.h>
#include <osApiWrappers/dag_spinlock.h>

namespace gpu_profiler
{
void stop_ds(DrawStatSingle &ds) { Stat3D::stopStatSingle(ds); }
void start_ds(DrawStatSingle &ds) { Stat3D::startStatSingle(ds); }

static bool initialized(false);
// Lets take max for all drivers from microprofile.cpp
enum
{
  GPU_MAX_QUERIES_SHIFT = 16,
  GPU_MAX_QUERIES = 1 << GPU_MAX_QUERIES_SHIFT
}; // has to be less than 65537, and has to be pow-of-2!
struct QueriesStorage
{
  void *queries[GPU_MAX_QUERIES]{0};
  uint64_t *queryResults[GPU_MAX_QUERIES]{0};
  uint64_t queryResultsInternal[GPU_MAX_QUERIES]{0};
  uint32_t queryCount{0};
  uint32_t queryStart{0};
  void *syncQuery{nullptr};
  uint64_t gpuFreq{1};
};

static eastl::unique_ptr<QueriesStorage> gpuStorage;
bool init()
{
  if (initialized)
    return true;
  G_STATIC_ASSERT(GPU_MAX_QUERIES_SHIFT <= 16);
  uint64_t gpuFreq = 0;
  d3d::driver_command(Drv3dCommand::TIMESTAMPFREQ, &gpuFreq);
  if (!gpuFreq)
  {
    logwarn("gpu profiling is not possible, timestamps are not supported");
    return false; // timestamps are unsupported
  }
  initialized = true;

  gpuStorage = eastl::make_unique<QueriesStorage>();
  gpuStorage->gpuFreq = gpuFreq;
  for (uint32_t i = 0; i < GPU_MAX_QUERIES; ++i)
  {
    d3d::driver_command(Drv3dCommand::TIMESTAMPISSUE, &gpuStorage->queries[i]);
    gpuStorage->queryResultsInternal[i] = ~0ULL;
    gpuStorage->queryResults[i] = &gpuStorage->queryResultsInternal[i];
  }
  d3d::driver_command(Drv3dCommand::TIMESTAMPISSUE, &gpuStorage->syncQuery);
  return true;
}

void resubmit_pending_timestamps()
{
  uint32_t id = gpuStorage->queryStart % GPU_MAX_QUERIES;
  const uint32_t modQid = gpuStorage->queryCount % GPU_MAX_QUERIES;
  for (; id != modQid; id = (id + 1) % GPU_MAX_QUERIES)
  {
    d3d::driver_command(Drv3dCommand::TIMESTAMPISSUE, &gpuStorage->queries[id]);
  }
}

void after_reset(bool)
{
  if (!initialized)
    return;
  resubmit_pending_timestamps();
}

void shutdown()
{
  if (!initialized)
    return;
  for (uint32_t i = 0; i < GPU_MAX_QUERIES; ++i)
  {
    d3d::driver_command(Drv3dCommand::RELEASE_QUERY, &gpuStorage->queries[i]);
  }
  d3d::driver_command(Drv3dCommand::RELEASE_QUERY, &gpuStorage->syncQuery);
  gpuStorage.reset();
  initialized = false;
}

static uint32_t insert_time_stamp_base()
{
  const uint32_t count = gpuStorage->queryCount;
  const uint32_t start = gpuStorage->queryStart;

  if (count - start == GPU_MAX_QUERIES - 1)
    return UINT32_MAX;

  gpuStorage->queryCount++;

  const uint32_t index = count % GPU_MAX_QUERIES;
  d3d::driver_command(Drv3dCommand::TIMESTAMPISSUE, &gpuStorage->queries[index]);
  return count;
}

bool is_on() { return initialized; }
uint32_t insert_time_stamp_unsafe(uint64_t *r)
{
  const uint32_t id = insert_time_stamp_base();
  if (id == UINT32_MAX)
    return id;

  gpuStorage->queryResults[id % GPU_MAX_QUERIES] = r;
  return id;
}
uint32_t insert_time_stamp(uint64_t *r) { return is_on() ? insert_time_stamp_unsafe(r) : 0; }

uint32_t insert_time_stamp()
{
  if (!initialized)
    return ~0u;
  const uint32_t id = insert_time_stamp_base();
  if (id == UINT32_MAX)
    return id;

  const uint32_t index = id % GPU_MAX_QUERIES;
  gpuStorage->queryResults[index] = &gpuStorage->queryResultsInternal[index];
  return id;
}

uint64_t get_time_stamp(uint32_t id)
{
  if (!is_on() || id == UINT32_MAX)
    return ~0ULL;

  return gpuStorage->queryResultsInternal[id % GPU_MAX_QUERIES];
}

uint64_t ticks_per_second() { return gpuStorage ? gpuStorage->gpuFreq : 1; }

int get_cpu_ticks_reference(int64_t *out_cpu, int64_t *out_gpu)
{
  return d3d::driver_command(Drv3dCommand::TIMECLOCKCALIBRATION, out_cpu, out_gpu);
}

int get_profile_cpu_ticks_reference(int64_t &out_cpu, int64_t &out_gpu) // that all has to be done inside d3d abstraction layers!
{
  int type = DRV3D_CPU_FREQ_TYPE_UNKNOWN;
  const int ret = d3d::driver_command(Drv3dCommand::TIMECLOCKCALIBRATION, &out_cpu, &out_gpu, &type);
  const int64_t curProfileTicks = profile_ref_ticks();
  // we implicitly assume that DRV3D_CPU_FREQ_TYPE_QPC == DRV3D_CPU_FREQ_TYPE_REF on windows;
  const int64_t srcRefTicks = ref_time_ticks();
  const double srcFreq = ref_ticks_frequency();
  if (!ret || !out_cpu || !out_gpu || type == DRV3D_CPU_FREQ_TYPE_UNKNOWN)
    return 0;
  if (type == DRV3D_CPU_FREQ_TYPE_PROFILE)
    return 1; // already in our reference!
  out_cpu = int64_t(curProfileTicks + double(out_cpu - srcRefTicks) * (profile_ticks_frequency() / srcFreq));
  return 1;
}

static void finish_queries_internal()
{
  const uint32_t count = gpuStorage->queryCount;
  const uint32_t start = gpuStorage->queryStart;

  const uint32_t modQid = count % GPU_MAX_QUERIES;
  uint32_t id = start % GPU_MAX_QUERIES, processed = 0;
  for (; id != modQid; id = (id + 1) % GPU_MAX_QUERIES, ++processed)
  {
    if (!d3d::driver_command(Drv3dCommand::TIMESTAMPGET, gpuStorage->queries[id], gpuStorage->queryResults[id]))
      break;
  }

  gpuStorage->queryStart += processed;
}
void finish_queries()
{
  if (!is_on())
    return;
  finish_queries_internal();
}
uint32_t flip(uint64_t *gpu_start)
{
  if (!is_on())
    return 0;

  const uint32_t ts = [=] {
    d3d::GpuAutoLock gpuLock;
    return gpu_start ? insert_time_stamp(gpu_start) : insert_time_stamp(); // another spinlock is inside insert_time_stamp
  }();
  finish_queries_internal();

  bool disjoint = false;
  d3d::driver_command(Drv3dCommand::TIMESTAMPFREQ, &gpuStorage->gpuFreq, &disjoint);
  return ts;
}
uint32_t flip() { return flip(NULL); }
void begin_event(const char *name) { d3d::beginEvent(name); }
void end_event() { d3d::endEvent(); }
} // namespace gpu_profiler

#include <drv/3d/dag_resetDevice.h>
REGISTER_D3D_AFTER_RESET_FUNC(gpu_profiler::after_reset);