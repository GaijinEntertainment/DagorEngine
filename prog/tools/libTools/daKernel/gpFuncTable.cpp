// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <perfMon/gpuProfiler.h>

static bool init() { return false; }
static void shutdown() {}
static uint32_t insert_time_stamp(uint64_t *) { return 0; }
static uint32_t insert_time_stamp_unsafe(uint64_t *) { return 0; }
static uint64_t ticks_per_second() { return 1; }
static void finish_queries() {}
static int get_profile_cpu_ticks_reference(int64_t &c, int64_t &g)
{
  c = g = 1;
  return 0;
}
static uint32_t flip(uint64_t *) { return 0; }
static void begin_event(const char *) {}
static void end_event() {}
static void stop_ds(DrawStatSingle &) {}
static void start_ds(DrawStatSingle &) {}

struct GpFuncTable
{
  gpu_profiler::FuncTable tbl;
  GpFuncTable()
  {
#define SET_ENTRY(NM) this->tbl.NM = &::NM
    SET_ENTRY(init);
    SET_ENTRY(shutdown);
    SET_ENTRY(insert_time_stamp);
    SET_ENTRY(insert_time_stamp_unsafe);
    SET_ENTRY(ticks_per_second);
    SET_ENTRY(finish_queries);
    SET_ENTRY(get_profile_cpu_ticks_reference);
    SET_ENTRY(flip);
    SET_ENTRY(begin_event);
    SET_ENTRY(end_event);
    SET_ENTRY(stop_ds);
    SET_ENTRY(start_ds);
#undef SET_ENTRY
  }
};

static GpFuncTable gpf;

void install_gpu_profile_func_table(const gpu_profiler::FuncTable &_tbl) { ::gpf.tbl = _tbl; }

namespace gpu_profiler
{
bool init() { return ::gpf.tbl.init(); }
void shutdown() { return ::gpf.tbl.shutdown(); }
uint32_t insert_time_stamp(uint64_t *t) { return ::gpf.tbl.insert_time_stamp(t); }
uint32_t insert_time_stamp_unsafe(uint64_t *t) { return ::gpf.tbl.insert_time_stamp_unsafe(t); }
uint64_t ticks_per_second() { return ::gpf.tbl.ticks_per_second(); }
void finish_queries() { return ::gpf.tbl.finish_queries(); }
int get_profile_cpu_ticks_reference(int64_t &c, int64_t &g) { return ::gpf.tbl.get_profile_cpu_ticks_reference(c, g); }
uint32_t flip(uint64_t *r) { return ::gpf.tbl.flip(r); }
void begin_event(const char *n) { return ::gpf.tbl.begin_event(n); }
void end_event() { return ::gpf.tbl.end_event(); }
void stop_ds(DrawStatSingle &ds) { return ::gpf.tbl.stop_ds(ds); }
void start_ds(DrawStatSingle &ds) { return ::gpf.tbl.start_ds(ds); }
} // namespace gpu_profiler
