#include <perfMon/gpuProfiler.h>
#include <3d/dag_drv3d.h>
#include <perfMon/dag_graphStat.h>
#include <3d/dag_drv3dCmd.h>
#include <perfMon/dag_perfTimer.h>
#include <atomic>
#include <EASTL/unique_ptr.h>
#include <osApiWrappers/dag_spinlock.h>

namespace gpu_profiler
{
void stop_ds(DrawStatSingle &ds) { Stat3D::stopStatSingle(ds); }
void start_ds(DrawStatSingle &ds) { Stat3D::startStatSingle(ds); }
#if LOCK_AROUND_GPU_COMMAND
static OSSpinlock gpu_lock;
struct GpuExternalAccessScopeGuard
{
  GpuExternalAccessScopeGuard() { gpu_lock.lock(); }
  ~GpuExternalAccessScopeGuard() { gpu_lock.unlock(); }
};
#define GPU_GUARD GpuExternalAccessScopeGuard guard
#else
// we don't lock around gpu command as our profiler is profiling gpu events only from main thread
// if that would change, we need enable command locking
struct GpuExternalAccessScopeGuard
{};
#define GPU_GUARD
#endif
static std::atomic<bool> inited(false);
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
  std::atomic<uint32_t> queryCount{0};
  uint32_t queryStart{0};
  void *syncQuery{nullptr};
  uint64_t gpuFreq{1};
};

static eastl::unique_ptr<QueriesStorage> gpuStorage;
bool init()
{
  if (inited.load(std::memory_order_relaxed))
    return true;
  G_STATIC_ASSERT(GPU_MAX_QUERIES_SHIFT <= 16);
  GPU_GUARD;
  if (inited.load())
    return true;
  uint64_t gpuFreq = 0;
  d3d::driver_command(D3V3D_COMMAND_TIMESTAMPFREQ, &gpuFreq, 0, nullptr);
  if (!gpuFreq)
  {
    logwarn("gpu profiling is not possible, timestamps are not supported");
    return false; // timestamps are unsupported
  }
  inited = true;

  gpuStorage = eastl::make_unique<QueriesStorage>();
  gpuStorage->gpuFreq = gpuFreq;
  for (uint32_t i = 0; i < GPU_MAX_QUERIES; ++i)
  {
    d3d::driver_command(D3V3D_COMMAND_TIMESTAMPISSUE, &gpuStorage->queries[i], 0, 0);
    gpuStorage->queryResultsInternal[i] = ~0ULL;
    gpuStorage->queryResults[i] = &gpuStorage->queryResultsInternal[i];
  }
  d3d::driver_command(D3V3D_COMMAND_TIMESTAMPISSUE, &gpuStorage->syncQuery, 0, 0);
  return true;
}

void shutdown()
{
  GPU_GUARD;
  if (!inited.load())
    return;
  for (uint32_t i = 0; i < GPU_MAX_QUERIES; ++i)
  {
    d3d::driver_command(DRV3D_COMMAND_RELEASE_QUERY, &gpuStorage->queries[i], 0, 0);
  }
  d3d::driver_command(DRV3D_COMMAND_RELEASE_QUERY, &gpuStorage->syncQuery, 0, 0);
  gpuStorage.reset();
  inited = false;
}

static uint32_t insert_time_stamp_base()
{
  const uint32_t id = gpuStorage->queryCount.fetch_add(1) % GPU_MAX_QUERIES; // todo: we'd better return
  const uint32_t index = id % GPU_MAX_QUERIES;
  GPU_GUARD;
  d3d::driver_command(D3V3D_COMMAND_TIMESTAMPISSUE, &gpuStorage->queries[index], 0, 0);
  return id;
}

bool is_on() { return inited.load(std::memory_order_relaxed); }
uint32_t insert_time_stamp_unsafe(uint64_t *r)
{
  const uint32_t id = insert_time_stamp_base();
  gpuStorage->queryResults[id % GPU_MAX_QUERIES] = r;
  return id;
}
uint32_t insert_time_stamp(uint64_t *r) { return is_on() ? insert_time_stamp_unsafe(r) : 0; }

uint32_t insert_time_stamp()
{
  if (!inited.load(std::memory_order_relaxed))
    return ~0u;
  const uint32_t id = insert_time_stamp_base();
  const uint32_t index = id % GPU_MAX_QUERIES;
  gpuStorage->queryResults[index] = &gpuStorage->queryResultsInternal[index];
  return id;
}

uint64_t get_time_stamp(uint32_t id)
{
  // return *gpuStorage->queryResults[id];
  return is_on() ? gpuStorage->queryResultsInternal[id % GPU_MAX_QUERIES] : ~0ULL;
}

uint64_t ticks_per_second() { return gpuStorage ? gpuStorage->gpuFreq : 1; }

int get_cpu_ticks_reference(int64_t *out_cpu, int64_t *out_gpu)
{
  return d3d::driver_command(D3V3D_COMMAND_TIMECLOCKCALIBRATION, out_cpu, out_gpu, NULL);
}

int get_profile_cpu_ticks_reference(int64_t &out_cpu, int64_t &out_gpu) // that all has to be done inside d3d abstraction layers!
{
  int type = DRV3D_CPU_FREQ_TYPE_UNKNOWN;
  const int ret = d3d::driver_command(D3V3D_COMMAND_TIMECLOCKCALIBRATION, &out_cpu, &out_gpu, &type);
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
  const uint32_t count = gpuStorage->queryCount.load();
  if (uint32_t(count - gpuStorage->queryStart) >= GPU_MAX_QUERIES)
    logerr("gpu queries overrun, we have issued %d queries, which is more than %d buffer size",
      uint32_t(count - gpuStorage->queryStart), GPU_MAX_QUERIES);
  const uint32_t modQid = count % GPU_MAX_QUERIES;
  uint32_t id = gpuStorage->queryStart % GPU_MAX_QUERIES, processed = 0;
  for (; id != modQid; id = (id + 1) % GPU_MAX_QUERIES, ++processed)
  {
    if (!d3d::driver_command(D3V3D_COMMAND_TIMESTAMPGET, gpuStorage->queries[id], gpuStorage->queryResults[id], 0))
      break;
  }
  gpuStorage->queryStart += processed;
}
void finish_queries()
{
  if (!is_on())
    return;
  GPU_GUARD; // has to follow insert_time_stamp
  finish_queries_internal();
}
uint32_t flip(uint64_t *gpu_start)
{
  if (!is_on())
    return 0;
  d3d::driver_command(DRV3D_COMMAND_ACQUIRE_OWNERSHIP, NULL, NULL, NULL);
  const uint32_t ts = gpu_start ? insert_time_stamp(gpu_start) : insert_time_stamp(); // another spinlock is inside insert_time_stamp
  d3d::driver_command(DRV3D_COMMAND_RELEASE_OWNERSHIP, NULL, NULL, NULL);
  GPU_GUARD; // has to follow insert_time_stamp
  finish_queries_internal();

  bool disjoint = false;
  d3d::driver_command(D3V3D_COMMAND_TIMESTAMPFREQ, &gpuStorage->gpuFreq, &disjoint, nullptr);
  return ts;
}
uint32_t flip() { return flip(NULL); }
void begin_event(const char *name) { d3d::beginEvent(name); }
void end_event() { d3d::endEvent(); }
} // namespace gpu_profiler
