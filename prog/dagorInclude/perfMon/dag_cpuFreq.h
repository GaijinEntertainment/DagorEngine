//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#define DAGOR_ALLOW_FAST_UNSAFE_TIMERS 0 // define it to 1 locally when going to profile fine-grained code on proper Intel CPU

#include <util/dag_stdint.h>

#include <supp/dag_define_COREIMP.h>

#ifdef __cplusplus
extern "C"
{

  // initialize timers
  KRNLIMP void measure_cpu_freq(bool force_lowres_timer = false);
#endif

  // returns ticks frequency, ticks * 1000000 / ref_ticks_frequency - usec
  KRNLIMP int64_t ref_ticks_frequency(void);

  // returns reference time label (in ticks!)
  KRNLIMP int64_t ref_time_ticks(void);
  // returns reference time label relative to base reference time label
  KRNLIMP int64_t rel_ref_time_ticks(int64_t ref, int time_usec);
  // convert ref time delta to nsecs
  KRNLIMP int64_t ref_time_delta_to_nsec(int64_t ref);
  // convert ref time delta to usecs
  KRNLIMP int64_t ref_time_delta_to_usec(int64_t ref);
  // returns time passed since reference time label, in nanoseconds
  KRNLIMP int64_t get_time_nsec(int64_t ref);
  // returns time passed since reference time label, in microseconds
  KRNLIMP int get_time_usec(int64_t ref);
  // returns time passed since initialization time in milliseconds
  KRNLIMP int get_time_msec(void);

// aliases for legacy code
#define ref_time_ticks_qpc ref_time_ticks
#define get_time_usec_qpc  get_time_usec
#define get_time_msec_qpc  get_time_msec

#if _TARGET_PC_LINUX
  // converts milliseconds passed since initialization to wallclock time
  KRNLIMP int64_t time_msec_to_localtime(int64_t t);
#endif

#ifdef __cplusplus
}
#endif

#include <supp/dag_undef_COREIMP.h>
