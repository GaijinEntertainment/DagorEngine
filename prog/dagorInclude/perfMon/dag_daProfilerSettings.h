//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>

class DataBlock;

namespace da_profiler
{
// returns current GPU core clock in MHz, or 0 if the platform can't report it
typedef int (*gpu_clock_getter_t)();

void set_profiling_settings(const DataBlock &);
void set_resolve_symbols(bool enabled);
void set_gpu_clock_getter(gpu_clock_getter_t getter);
uint32_t find_profiler_mode(const char *m);
const char *profiler_mode_name(uint32_t mode);
uint32_t profiler_modes_count();
const char *profiler_mode_index_name(uint32_t index);
uint32_t profiler_mode_index(uint32_t index); // return mode
} // namespace da_profiler
