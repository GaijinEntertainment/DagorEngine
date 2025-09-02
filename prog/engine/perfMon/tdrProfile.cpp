// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <perfMon/dag_statDrv.h>
#include <drv/3d/dag_commands.h>
#include <debug/dag_debug.h>

#if TIME_PROFILER_ENABLED && TDR_PROFILE

TdrProfile::TdrProfile(const char *in_name) : q1(NULL), name(in_name) { d3d::driver_command(Drv3dCommand::TIMESTAMPISSUE, &q1); }

TdrProfile::~TdrProfile()
{
  void *q2 = NULL;
  d3d::driver_command(Drv3dCommand::TIMESTAMPISSUE, &q2);

  int64_t t1, t2;
  while (!d3d::driver_command(Drv3dCommand::TIMESTAMPGET, q1, &t1)) {}

  while (!d3d::driver_command(Drv3dCommand::TIMESTAMPGET, q2, &t2)) {}

  static int64_t intervalFreq = 0;
  if (!intervalFreq)
    d3d::driver_command(Drv3dCommand::TIMESTAMPFREQ, &intervalFreq);

  float timeMs = double(t2 - t1) / (double(intervalFreq) / 1000.0);
  debug("TdrProfile '%s' = %.03fms%s", name, timeMs, timeMs > 100 ? " !!!" : "");

  d3d::driver_command(Drv3dCommand::RELEASE_QUERY, &q1);
  d3d::driver_command(Drv3dCommand::RELEASE_QUERY, &q2);
}

#endif // #if TIME_PROFILE_ENABLED
