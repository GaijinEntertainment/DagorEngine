// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <perfMon/dag_drawStat.h>
namespace gpu_profiler
{
bool init();
void shutdown();
uint32_t insert_time_stamp(uint64_t *result);
uint64_t ticks_per_second();
bool is_on();
void finish_queries();
int get_profile_cpu_ticks_reference(int64_t &out_cpu, int64_t &out_gpu); // cpu timestamp is in
                                                                         // profile_ref_ticks/profile_ticks_frequency domain
uint32_t flip(uint64_t *result);
void begin_event(const char *name);
void end_event();
void stop_ds(DrawStatSingle &ds);
void start_ds(DrawStatSingle &ds);
void begin_gpu_stats();
void end_gpu_stats(uint64_t *result);

bool get_gpu_thread_name(char *buf, const size_t max_len);
} // namespace gpu_profiler