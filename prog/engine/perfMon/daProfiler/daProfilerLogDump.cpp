// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_files.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_zlibIo.h>
#include "daProfilerInternal.h"
#include "daProfilePlatform.h"
#include "stl/daProfilerString.h"
#include "daProfilerDumpServer.h"
#include "dumpLayer.h"
#include <time.h>

namespace da_profiler
{

class LogDumpServer final : public ProfilerDumpClient
{
public:
  struct EventStats
  {
    uint64_t minTicks = ~0ull, maxTicks = 0, totalTicks = 0;
    uint64_t numEvents = 0;

    void update(uint64_t dt)
    {
      numEvents++;
      totalTicks += dt;
      minTicks = min(minTicks, dt);
      maxTicks = max(maxTicks, dt);
    }
  };

  string eventName;
  LogDumpServer(const char *name) : eventName(name) {}
  ~LogDumpServer() { close(); }
  void close() {}
  // ProfilerDumpClient
  const char *getDumpClientName() const override { return "LogDump"; }
  DumpProcessing process(const Dump &dump) override
  {
    EventStats gpuStats, cpuStats;
    dump.gpuEvents.forEach([&, this](const GpuEventData &e) {
      uint32_t color;
      const char *name = get_description(e.description, color);
      if (!name || e.end == ~0ull)
        return;
      if (strcmp(name, eventName.c_str()) != 0)
        return;
      gpuStats.update(e.end - e.start);
    });

    for (auto &th : dump.threads)
      th.events.forEach([&, this](const EventData &e) {
        uint32_t color;
        const char *name = get_description(e.description, color);
        if (!name || e.end == ~0ull)
          return;
        if (strcmp(name, eventName.c_str()) != 0)
          return;
        cpuStats.update(e.end - e.start);
      });

    auto gpuFreq = gpu_frequency();
    auto cpuFreq = cpu_frequency();

    double gs = 1.0e6 / gpuFreq;
    report_debug("GPU event %s: min=%.3f avg=%.3f max=%.3f us", eventName.c_str(), gpuStats.minTicks * gs,
      gpuStats.totalTicks * gs / max<uint64_t>(gpuStats.numEvents, 1), gpuStats.maxTicks * gs);
    double cs = 1.0e6 / cpuFreq;
    report_debug("CPU event %s: min=%.3f avg=%.3f max=%.3f us", eventName.c_str(), cpuStats.minTicks * cs,
      cpuStats.totalTicks * cs / max<uint64_t>(cpuStats.numEvents, 1), cpuStats.maxTicks * cs);
    return DumpProcessing::Continue;
  }
  DumpProcessing updateDumpClient(const UserSettings &) override { return DumpProcessing::Continue; }
  int priority() const override { return 1; }
  void removeDumpClient() override { delete this; }
};

bool stop_log_dump_server(const char *name) { return remove_dump_client_by_name(name ? name : "null name"); }
bool start_log_dump_server(const char *name)
{
  add_dump_client(new LogDumpServer(name));
  return true;
}

}; // namespace da_profiler

#define EXPORT_PULL dll_pull_perfMon_daProfilerLogDump
#include <supp/exportPull.h>
