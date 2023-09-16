//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

//! this function (if set) called just before new frame rendering starts
extern void (*dwc_hook_before_frame)();

//! this function (if set) called just after frame rendering finishes
extern void (*dwc_hook_after_frame)();

//! this function (if set) called just before new frame rendering;
//! frame will be started only is hook returns true;
//! function may perform useful work when usec_to_next_act > 0 and new frame cannot be started yet
extern bool (*dwc_can_draw_next_frame)(int frame_no, int usec_to_next_act);

//! this function (if set) called just dagor_work_cycle() during inInternalWinLoop=1
extern void (*dwc_hook_inside_internal_winloop)();

extern bool dwc_alloc_perform_delayed_actions;
extern bool dwc_alloc_perform_delayed_actions_in_internal_winloop;
