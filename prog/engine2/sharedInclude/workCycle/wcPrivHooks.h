
#ifndef __DAGOR_WCPRIVHOOKS_H
#define __DAGOR_WCPRIVHOOKS_H
#pragma once


//! this function (if set) called just before new frame rendering starts
//! NOTE: this function is specially for threadSampler (if installed)
extern void (*dwc_hook_ts_before_frame)();

extern void (*dwc_hook_fps_log)(int);

extern void (*dwc_hook_memory_report)(void);

#endif
