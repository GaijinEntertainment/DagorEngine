//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_preprocessor.h>
#include <perfMon/dag_drawStat.h>

#ifndef TIME_PROFILER_ENABLED
#if _TARGET_PC || (DAGOR_DBGLEVEL != 0)
#define TIME_PROFILER_ENABLED 1
#else
#define TIME_PROFILER_ENABLED 0
#endif
#endif

#if TIME_PROFILER_ENABLED
#define DA_PROFILER_ENABLED 1
#endif

#include <perfMon/dag_daProfiler.h>

class TimeIntervalNode;

// slow legacy dump
struct TimeIntervalInfo
{
  double tMin = 0.0, tMax = 0.0, tSpike = 0.0, tCurrent = 0.0;             // cpu
  double tGpuMin = 0.0, tGpuMax = 0.0, tGpuSpike = 0.0, tGpuCurrent = 0.0; // gpu - only valid from main thread
  double unknownTimeMsec = 0.0;                                            // cpu
  unsigned lifetimeCalls = 0, lifetimeFrames = 0;
  unsigned perFrameCalls = 0, skippedFrames = 0;
  int16_t depth = 0;
  bool gpuValid = false;
  DrawStatSingle stat;
};

#if TIME_PROFILER_ENABLED

#define TIME_PROFILE_THREAD(Name)  DA_PROFILE_THREAD(Name)
#define TIME_PROFILE_AUTO_THREAD() DA_PROFILE_THREAD(nullptr)

#define TIME_PROFILE(LabelName) DA_PROFILE_EVENT(#LabelName)

#define TIME_PROFILE_NAME(LabelName, Name) DA_PROFILE_NAMED_EVENT(Name)

#define TIME_D3D_PROFILE(LabelName) DA_PROFILE_GPU_EVENT(#LabelName)

#define TIME_D3D_PROFILE_NAME(LabelName, Name) DA_PROFILE_NAMED_GPU_EVENT(Name)

#if DAGOR_DBGLEVEL != 0

#define TIME_PROFILER_TICK() DA_PROFILE_TICK()
#else
#define TIME_PROFILER_TICK() DA_PROFILE_TICK()
#endif

#define TIME_PROFILER_TICK_END()

#if DAGOR_DBGLEVEL != 0
#define TIME_PROFILER_SHUTDOWN() DA_PROFILE_SHUTDOWN();
#else
#define TIME_PROFILER_SHUTDOWN() DA_PROFILE_SHUTDOWN();
#endif

#else

#define TIME_PROFILE(LabelName)
#define TIME_PROFILE_NAME(LabelName, Name) G_UNUSED(Name);
#define TIME_PROFILE_THREAD(Name)          G_UNUSED(Name);
#define TIME_PROFILE_AUTO_THREAD()
#define TIME_D3D_PROFILE(LabelName)
#define TIME_D3D_PROFILE_NAME(LabelName, Name) G_UNUSED(Name);
#define TIME_PROFILER_TICK()
#define TIME_PROFILER_TICK_END()
#define TIME_PROFILER_SHUTDOWN()
#define TIME_PROFILER_EXIT_THREAD()

#endif

// Time profile which works only in dev builds
#if DAGOR_DBGLEVEL > 0
#define TIME_PROFILE_WAIT_DEV(LabelName)          DA_PROFILE_WAIT(LabelName)
#define TIME_PROFILE_DEV(LabelName)               TIME_PROFILE(LabelName)
#define TIME_PROFILE_NAME_DEV                     TIME_PROFILE_NAME
#define TIME_D3D_PROFILE_DEV                      TIME_D3D_PROFILE
#define TIME_PROFILE_UNIQUE_DEV                   DA_PROFILE_UNIQUE
#define TIME_PROFILE_UNIQUE_EVENT_DEV             DA_PROFILE_UNIQUE_EVENT
#define TIME_PROFILE_UNIQUE_EVENT_NAMED_DEV(Name) DA_PROFILE_UNIQUE_EVENT_NAMED(Name)
#define TIME_PROFILE_UNIQUE_EVENT_DESC_DEV(Desc)  DA_PROFILE_UNIQUE_EVENT_DESC(Desc)
#else
#define TIME_PROFILE_WAIT_DEV(LabelName)
#define TIME_PROFILE_DEV(LabelName)
#define TIME_PROFILE_NAME_DEV(...)
#define TIME_D3D_PROFILE_DEV(LabelName)
#define TIME_PROFILE_UNIQUE_DEV
#define TIME_PROFILE_UNIQUE_EVENT_DEV(...)
#define TIME_PROFILE_UNIQUE_EVENT_NAMED_DEV(Name)
#define TIME_PROFILE_UNIQUE_EVENT_DESC_DEV(Desc)
#endif

#if TDR_PROFILE
#undef TIME_D3D_PROFILE
#define TIME_D3D_PROFILE(LabelName) TdrProfile DAG_CONCAT(TdrProfile, LabelName)(#LabelName);

struct TdrProfile
{
  const char *name;
  void *q1;

  TdrProfile(const char *in_name);
  ~TdrProfile();
};
#endif

#include <supp/dag_undef_KRNLIMP.h>
