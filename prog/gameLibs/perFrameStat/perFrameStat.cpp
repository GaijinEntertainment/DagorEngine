// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <perFrameStat/perFrameStat.h>
#include <render/gpuWatchMs.h>

#include <generic/dag_tab.h>
#include <workCycle/dag_workCyclePerf.h>
#include <osApiWrappers/dag_files.h>
#include <perfMon/dag_memoryReport.h>
#include <startup/dag_globalSettings.h>

#include <EASTL/unique_ptr.h>
#include <EASTL/string.h>
#include <EASTL/fixed_vector.h>

namespace benchmark_perframe_stat
{
namespace gpu_time
{
constexpr static int GPU_TIMESTAMP_LATENCY = 5;
eastl::fixed_vector<GPUWatchMs, GPU_TIMESTAMP_LATENCY> gpu_timings{};

struct GPUWatchValidator
{
  ~GPUWatchValidator() { G_ASSERTF(gpu_timings.empty(), "benchmark_perframe_stat: GPU timings not empty on shutdown"); }
} validator;

void init()
{
  GPUWatchMs::init_freq();
  gpu_timings.resize(GPU_TIMESTAMP_LATENCY);
}

bool initialized() { return !gpu_timings.empty(); }

void close() { gpu_timings.clear(); }

// guard it from multiple calls, used only by add_last_frame
static int get_last()
{
  if (!initialized())
    return 0;

  static size_t gpuTimingsIdx = 0;
  uint64_t gpuTimeUs = 0;
  gpu_timings[gpuTimingsIdx].stop();
  gpuTimingsIdx = (gpuTimingsIdx + 1) % gpu_timings.size();
  if (!gpu_timings[gpuTimingsIdx].read(gpuTimeUs, 1000000)) // -V1051
  {
    // don't trust anyone, clear it yourself on failure
    gpuTimeUs = 0;
  }
  gpu_timings[gpuTimingsIdx].start();

  return static_cast<int>(gpuTimeUs);
}

} // namespace gpu_time

static uint32_t prev_frame_id = 0;
static Tab<PerFrameData> perframe_stat_buffer(inimem_ptr());

void init(unsigned frames_limit)
{
  if (workcycleperf::record_mode == workcycleperf::RecordMode::Disabled)
  {
    debug("benchmark_perframe_stat: record mode is disabled. CPU only cycle and workcycle times will be recorded.");
    workcycleperf::record_mode = workcycleperf::RecordMode::All;
  }
  else if (workcycleperf::record_mode != workcycleperf::RecordMode::All)
    debug("benchmark_perframe_stat: current mode is %d, not all metrics will be recorded",
      static_cast<int>(workcycleperf::record_mode));

  if (perframe_stat_buffer.capacity())
    debug("benchmark_perframe_stat: buffer was already initialized with capacity %zu, this is suboptimal",
      perframe_stat_buffer.capacity());

  perframe_stat_buffer.reserve(static_cast<size_t>(frames_limit));
}

bool initialized() { return perframe_stat_buffer.capacity() > 0; }

void reset()
{
  if (workcycleperf::record_mode == workcycleperf::RecordMode::Disabled)
  {
    logerr("benchmark_perframe_stat: nothing to reset because record mode is disabled");
    return;
  }

  perframe_stat_buffer.clear();
}

void add_last_frame()
{
  const auto frameId = dagor_get_global_frame_id();
  // call once per frame
  if (DAGOR_UNLIKELY(prev_frame_id >= frameId))
  {
    logerr("benchmark_perframe_stat: attempt to add frame stat for incorrect frame id: %u, previous frame id: %u", frameId,
      prev_frame_id);
    return;
  }

  const size_t capacity = perframe_stat_buffer.capacity();
  if (DAGOR_UNLIKELY(perframe_stat_buffer.size() >= capacity))
    logwarn("benchmark_perframe_stat: buffer is full and will be increased, which may lead to incorrect results. Increase the "
            "buffer size on next run. The current limit is %zu.",
      capacity);

  perframe_stat_buffer.push_back({workcycleperf::last_workcycle_time_usec, workcycleperf::last_cpu_only_cycle_time_usec,
    memoryreport::get_device_vram_used_kb(), gpu_time::get_last()});
  prev_frame_id = frameId;
}

static void default_dump_to_file(const char *fname)
{
  eastl::unique_ptr<void, DagorFileCloser> f(df_open(fname, DF_WRITE | DF_CREATE | DF_IGNORE_MISSING));
  if (!f)
  {
    logerr("benchmark_perframe_stat: failed to open file %s", fname);
    return;
  }

  const char *header = nullptr;
  const bool dumpGpuTime = gpu_time::initialized();
  if (dumpGpuTime)
    header = "workcycle_time_msec,cpu_only_cycle_time_msec,gpu_time_msec,gpu_memory_usage_kb\n";
  else
    header = "workcycle_time_msec,cpu_only_cycle_time_msec,gpu_memory_usage_kb\n";
  df_printf(f.get(), header);

  for (const PerFrameData &fd : perframe_stat_buffer)
  {
    if (dumpGpuTime)
      df_printf(f.get(), "%g,%g,%g,%d\n", fd.workcycleTimeUsec / 1000.f, fd.cpuOnlyCycleTimeUsec / 1000.f, fd.gpuTimeUsec / 1000.f,
        fd.gpuMemoryUsageKb);
    else
      df_printf(f.get(), "%g,%g,%d\n", fd.workcycleTimeUsec / 1000.f, fd.cpuOnlyCycleTimeUsec / 1000.f, fd.gpuMemoryUsageKb);
  }
}

static void (*dump_to_file_fn)(const char *) = default_dump_to_file;

void dump_to_file(const char *fname)
{
  if (workcycleperf::record_mode == workcycleperf::RecordMode::Disabled)
  {
    logerr("benchmark_perframe_stat: nothing to dump because record mode is disabled");
    return;
  }

  dump_to_file_fn(fname);
}

void set_dump_to_file(void (*dump_function)(const char *fname))
{
  dump_to_file_fn = dump_function ? dump_function : default_dump_to_file;
}

dag::ConstSpan<PerFrameData> get_per_frame_data() { return perframe_stat_buffer; }
} // namespace benchmark_perframe_stat
