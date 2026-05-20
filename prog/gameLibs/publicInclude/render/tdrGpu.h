//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

extern void dump_tdr_settings();
extern void dump_memory_statistics();
extern unsigned dump_proc_memory(); // Return number of avail commit mem in MBs
extern void dump_gpu_memory();
extern void dump_gpu_freq();
extern void dump_gpu_temperature();

extern void dump_periodic_gpu_info(); // GPU temp/mem + system commit (adaptive rate)
