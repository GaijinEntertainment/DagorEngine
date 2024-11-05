//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/functional.h>

namespace memoryreport
{

struct VRamState
{
  int64_t used_device_local{-1};
  int64_t used_shared{-1};
};

using TVRamStateCallback = eastl::function<void(VRamState &vram_state)>;

#if DAGOR_DBGLEVEL > 0 && !DAGOR_THREAD_SANITIZER
void init();

void start_report();
void stop_report();

void register_vram_state_callback(const TVRamStateCallback &callback);

int32_t get_device_vram_used_kb();
int32_t get_shared_vram_used_kb();

#else
inline void init(){};

inline void start_report(){};
inline void stop_report(){};

inline void register_vram_state_callback(const TVRamStateCallback & /*callback*/){};

inline int32_t get_device_vram_used_kb() { return -1; }
inline int32_t get_shared_vram_used_kb() { return -1; }
#endif

} // namespace memoryreport
