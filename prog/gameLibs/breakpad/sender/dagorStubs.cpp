// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <perfMon/dag_cpuFreq.h>
#include <util/dag_delayedAction.h>
#include <osApiWrappers/dag_basePath.h>
#include <stdlib.h>

// for lockProfiler

void execute_delayed_action_on_main_thread(DelayedAction *, bool, int) {}

const char *dd_get_named_mount_path(const char *, int) { return nullptr; }
const char *dd_resolve_named_mount_in_path(const char *fpath, const char *&mnt_path)
{
  mnt_path = "";
  return fpath;
}
