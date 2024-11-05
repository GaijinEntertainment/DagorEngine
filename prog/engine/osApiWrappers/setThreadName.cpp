// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_progGlobals.h>
#include <osApiWrappers/setProgGlobals.h>

void (*hook_set_thread_name)(const char *name) = NULL;

void win32_set_thread_name(const char *name)
{
  if (hook_set_thread_name)
    hook_set_thread_name(name);
}

#define EXPORT_PULL dll_pull_osapiwrappers_setThreadName
#include <supp/exportPull.h>
