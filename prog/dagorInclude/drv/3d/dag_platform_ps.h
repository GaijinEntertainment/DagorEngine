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
VDECL get_program_vdecl(PROGRAM);

//! returns current state of VSYNC
bool get_vsync_enabled();
//! enables or disables strong VSYNC (flips only on VBLANK); returns true on success
bool enable_vsync(bool enable);

namespace ps
{
// Texture creation
Texture *create_raw_tex();
void update_raw_tex(Texture *tex, void *gnm_texture);

// direct access to gpu heaps
void *alloc_mem_block(bool onion, uint32_t size, uint32_t alignment);
void free_mem_block(void *addr);

// Debug:
//! record frame to /hostapp/captured (only for specially compiled driver)
//-1=stop  0=query state >0=count  returns true if in progress
bool record_frame(int control);

// to catch specific point of frame in debugger. id is an arbitrary value
void debug_point(int id = 0);
void set_verbose_mode(bool ena);          // enable verbose debug messages (auto-disabled on any submit)
void submit_and_stall(bool locked_state); // force gpu pipeline flush
void submit_background();
void freeze_frame(); // repeat current frame on present

// returns Gnm::Texture pointer
void *get_raw_texture_descriptor(Texture *tex);
} // namespace ps
}; // namespace d3d
