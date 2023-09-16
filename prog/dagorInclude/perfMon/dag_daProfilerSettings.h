//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <stdint.h>

class DataBlock;

namespace da_profiler
{
void set_profiling_settings(const DataBlock &);
uint32_t find_profiler_mode(const char *m);
const char *profiler_mode_name(uint32_t mode);
uint32_t profiler_modes_count();
const char *profiler_mode_index_name(uint32_t index);
uint32_t profiler_mode_index(uint32_t index); // return mode
} // namespace da_profiler
