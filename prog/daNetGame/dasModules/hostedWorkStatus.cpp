// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "main/hostedServerLauncher.h"
#include "main/workStatus.h"
#include <generic/dag_tab.h>
#include <osApiWrappers/dag_critSec.h>
#include <util/dag_string.h>

static WinCritSec g_work_status_cs;
static Tab<const char *> g_work_status_tab;

void __cdecl default_hosted_work_status_callback(bool set, const char *text)
{
  WinAutoLock lock(g_work_status_cs);
  if (set)
  {
    for (int i = 0; i < g_work_status_tab.size(); ++i)
      if (!g_work_status_tab[i])
      {
        g_work_status_tab[i] = text;
        return;
      }
    if (g_work_status_tab.size() >= 4)
      return;
    g_work_status_tab.push_back(text);
    return;
  }
  for (int i = 0; i < g_work_status_tab.size(); ++i)
    if (g_work_status_tab[i] == text)
    {
      g_work_status_tab[i] = nullptr;
      return;
    }
}

void for_each_hosted_work_status(void (*fn)(const char *s, void *ctx), void *ctx)
{
  String shared;
  int n = 0;
  {
    WinAutoLock lock(g_work_status_cs);
    n = g_work_status_tab.size();
  }
  for (int i = 0; i < n; ++i)
  {
    {
      WinAutoLock lock(g_work_status_cs);
      if (!g_work_status_tab[i])
        continue;
      shared = g_work_status_tab[i];
    }
    fn(shared.c_str(), ctx);
  }
}

namespace
{
struct WorkStatusHostInstall
{
  WorkStatusHostInstall() { workstatus::set_callback(&default_hosted_work_status_callback); }
} g_work_status_host_install;
} // namespace
