#pragma once
#include <util/dag_stdint.h>
struct DrawStatSingle;
namespace gpu_profiler
{
bool init() { return false; }
void shutdown() {}
uint32_t insert_time_stamp() { return 0; }
uint32_t insert_time_stamp(uint64_t *) { return 0; }
uint32_t insert_time_stamp_unsafe(uint64_t *) { return 0; }
uint64_t get_time_stamp(uint32_t) { return 0; }
uint64_t ticks_per_second() { return 1; }
bool is_on() { return false; }
void finish_queries() {}
int get_cpu_ticks_reference(int64_t *, int64_t *) { return 0; }
int get_profile_cpu_ticks_reference(int64_t &out_cpu, int64_t &out_gpu)
{
  out_cpu = out_gpu = 1;
  return 0;
}
uint32_t flip() { return 0; }
uint32_t flip(uint64_t *) { return 0; }
void begin_event(const char *) {}
void end_event() {}
void stop_ds(DrawStatSingle &) {}
void start_ds(DrawStatSingle &) {}
} // namespace gpu_profiler
