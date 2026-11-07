//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#if !(_TARGET_C1 | _TARGET_C2 | RIPPER)
#error : must not be included for non-PS4 and non-PS5 target!
#endif

#include <drv/3d/dag_driver.h>

struct SceHmdFieldOfView;
struct SceHmdReprojectionTrackerState;

namespace d3d
{
//! returns current state of VSYNC
bool get_vsync_enabled();
//! enables or disables strong VSYNC (flips only on VBLANK); returns true on success
bool enable_vsync(bool enable);

namespace ps
{
// direct access to gpu heaps
void *alloc_mem_block(bool onion, uint32_t size, uint32_t alignment);
void free_mem_block(void *addr);

// Debug:
// to catch specific point of frame in debugger. id is an arbitrary value
void debug_point(int id = 0);
void set_verbose_mode(bool ena);          // enable verbose debug messages (auto-disabled on any submit)
void set_fill_gpu_blocks(bool ena);       // fill freed/uninitialized gpu memory with a recognizable pattern
void submit_and_stall(bool locked_state); // force gpu pipeline flush
void submit_background();
void freeze_frame(); // repeat current frame on present
} // namespace ps
}; // namespace d3d
