#include <perfMon/dag_statDrv.h>
#include <3d/dag_drv3d.h>
#include <debug/dag_debug.h>

#if TIME_PROFILER_ENABLED && TDR_PROFILE

TdrProfile::TdrProfile(const char *in_name) : q1(NULL), name(in_name) { d3d::driver_command(D3V3D_COMMAND_TIMESTAMPISSUE, &q1, 0, 0); }

TdrProfile::~TdrProfile()
{
  void *q2 = NULL;
  d3d::driver_command(D3V3D_COMMAND_TIMESTAMPISSUE, &q2, 0, 0);

  int64_t t1, t2;
  while (!d3d::driver_command(D3V3D_COMMAND_TIMESTAMPGET, q1, &t1, 0)) {}

  while (!d3d::driver_command(D3V3D_COMMAND_TIMESTAMPGET, q2, &t2, 0)) {}

  static int64_t intervalFreq = 0;
  if (!intervalFreq)
    d3d::driver_command(D3V3D_COMMAND_TIMESTAMPFREQ, &intervalFreq, 0, 0);

  float timeMs = double(t2 - t1) / (double(intervalFreq) / 1000.0);
  debug("TdrProfile '%s' = %.03fms%s", name, timeMs, timeMs > 100 ? " !!!" : "");

  d3d::driver_command(DRV3D_COMMAND_RELEASE_QUERY, &q1, 0, 0);
  d3d::driver_command(DRV3D_COMMAND_RELEASE_QUERY, &q2, 0, 0);
}

#endif // #if TIME_PROFILE_ENABLED
