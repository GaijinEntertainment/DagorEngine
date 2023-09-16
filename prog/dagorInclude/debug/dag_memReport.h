//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

namespace memreport
{
//! dumps mem usage stats (for system and GPU memory, conditionally) to log if time_interval_msec passsed from last report
void dump_memory_usage_report(int time_interval_msec = 10000, bool sysmem = true, bool gpumem = true);

//! renders mem usage stats on screen (using StdGuiRender and current font)
//! (x0,y0) sets text frame anchor
//! (positive=left/top of rect from left/top of screen, negative=right/bottom of rect from right/bottom of screen)
void on_screen_memory_usage_report(int x0, int y0, bool sysmem = true, bool gpumem = true);

//! setup system memory reference (to show delta in mem reports)
void set_sysmem_reference();
//! resetup system memory reference (to hide delta in mem reports)
void reset_sysmem_reference();
} // namespace memreport
