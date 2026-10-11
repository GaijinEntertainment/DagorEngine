// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <startup/dag_purgeLogs.h>
#include <debug/dag_debug.h>
#include <debug/dag_logSys.h>
#include <generic/dag_tab.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_threads.h>
#include <perfMon/dag_cpuFreq.h>
#include <util/dag_globDef.h>
#include <util/dag_stlqsort.h>
#include <stdio.h>
#include <time.h>

namespace
{
struct PurgeLogsCfg
{
  int maxAgeDays, maxCount, maxFileSizeMB, timeBudgetSec;
  bool dryRun, logVerbose;
};
} // namespace

template <typename IsTerminating>
static void purge_obsolete_logs(const PurgeLogsCfg &cfg, const IsTerminating &is_terminating)
{
  const int refms = get_time_msec();
  const char *logDir = get_log_directory();
  if (!*logDir)
    return;
  const time_t curTime = time(nullptr);
  const time_t eraseTime = curTime - time_t(cfg.maxAgeDays) * 24 * 60 * 60;
  const time_t deadline = curTime + cfg.timeBudgetSec;
  const int64_t maxFileSize = int64_t(cfg.maxFileSizeMB) << 20;
  int nfound = 0, nremoved = 0;
  char tmpPath[DAGOR_MAX_PATH];
  auto out_of_time = [&] { return is_terminating() || (cfg.timeBudgetSec > 0 && time(nullptr) > deadline); };
  auto removef = [&](const alefind_t &ff, const char *why) {
    SNPRINTF(tmpPath, sizeof(tmpPath), "%s%s", logDir, ff.name);
    if (cfg.logVerbose)
      debug("purge_obsolete_logs: %sremove '%s' of size=%dK: %s", cfg.dryRun ? "would " : "", tmpPath, ff.size >> 10, why);
    if (cfg.dryRun || dd_erase(tmpPath))
      ++nremoved;
  };

  Tab<alefind_t> kept;
  SNPRINTF(tmpPath, sizeof(tmpPath), "%s*", logDir);
  for (const alefind_t &ff : dd_find_iterator(tmpPath, DA_FILE))
  {
    ++nfound;
    if (cfg.maxFileSizeMB > 0 && ff.size > maxFileSize)
      removef(ff, "too big");
    else if (cfg.maxAgeDays > 0 && ff.mtime >= 0 && ff.mtime <= eraseTime)
      removef(ff, "too old");
    else
      kept.push_back(ff);
    if (out_of_time())
      break;
  }

  const int over = int(kept.size()) - cfg.maxCount;
  if (cfg.maxCount > 0 && over > 0 && !out_of_time())
  {
    stlsort::sort(kept.begin(), kept.end(), [](const alefind_t &a, const alefind_t &b) { return a.mtime < b.mtime; });
    for (int i = 0; i < over && !out_of_time(); ++i)
      removef(kept[i], "over count");
  }

  debug("purge_obsolete_logs: %s %d/%d files in '%s' at %d ms%s", cfg.dryRun ? "would remove" : "removed", nremoved, nfound, logDir,
    get_time_msec() - refms, out_of_time() ? ", out of time" : "");
}

void start_purge_logs_thread(const DataBlock &cfg, bool def_enabled)
{
  if (!cfg.getBool("enabled", def_enabled))
    return;
  PurgeLogsCfg c;
  c.maxAgeDays = cfg.getInt("maxAgeDays", 30);
  c.maxCount = cfg.getInt("maxCount", 100);
  c.maxFileSizeMB = cfg.getInt("maxFileSizeMB", 1024);
  c.timeBudgetSec = cfg.getInt("timeBudgetSec", 120);
  c.dryRun = cfg.getBool("dryRun", DAGOR_DBGLEVEL > 0);
  c.logVerbose = cfg.getBool("logVerbose", false);
  execute_in_new_thread([c](auto is_terminating) { purge_obsolete_logs(c, is_terminating); }, "PurgeLogThread",
    DaThread::DEFAULT_STACK_SZ, cpujobs::DEFAULT_THREAD_PRIORITY + cpujobs::THREAD_PRIORITY_LOWER_STEP);
}
