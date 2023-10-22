#include <startup/dag_restart.h>
#include <generic/dag_tab.h>
#include <util/dag_globDef.h>
#include <debug/dag_debug.h>
#include <perfMon/dag_cpuFreq.h>
#include <util/dag_loadingProgress.h>

#define LOGLEVEL_DEBUG _MAKE4C('RSRT')

#if 0
#define VERBOSE_DEBUG debug
#else
#define VERBOSE_DEBUG(...)
#endif

RestartProc::RestartProc(int pr) { priority = pr; }

RestartProc::~RestartProc() {}

SRestartProc::SRestartProc(int f, int pr) : RestartProc(pr)
{
  flags = f;
  inited = 0;
}

void SRestartProc::destroy()
{
  if (inited)
  {
    inited = 0;
    shutdown();
  }
}

void SRestartProc::startupf(int f)
{
  if (inited)
  {
    VERBOSE_DEBUG("  * %s already initialized, skipping", procname());
    return;
  }
  if (!(f & flags))
  {
    VERBOSE_DEBUG("  * %s misses startup flags", procname());
    return;
  }
  inited = -1;
  startup();
  inited = 1;
  debug("  * %s initialized", procname());
}

void SRestartProc::shutdownf(int f)
{
  if (!inited)
  {
    VERBOSE_DEBUG("  * %s already uninitialized, skipping", procname());
    return;
  }
  if (inited < 0)
  {
    VERBOSE_DEBUG("  * %s in construction/destruction phase. skipping", procname());
    return;
  }
  if (!(f & flags))
  {
    VERBOSE_DEBUG("  * %s misses shutdown flags", procname());
    return;
  }
  inited = -1;
  shutdown();
  inited = 0;
  debug("  * %s uninitialized", procname());
}


static Tab<RestartProc *> rproc(inimem_ptr());

void add_restart_proc(RestartProc *pr)
{
  if (!pr)
    return;
  //  debug("add_rp %s %d",pr->procname(),pr->priority);

  remove_restart_proc(pr);
  if (rproc.size() <= 0)
  {
    rproc.push_back(pr);
    //    debug("  first 0");
    return;
  }
  if (pr->priority > rproc[0]->priority)
  {
    insert_items(rproc, 0, 1, &pr);
    //    debug("  high 0");
    return;
  }
  for (int i = 1; i < rproc.size(); ++i)
    if (rproc[i - 1]->priority >= pr->priority && pr->priority > rproc[i]->priority)
    {
      insert_items(rproc, i, 1, &pr);
      //    debug("  added %d",i);
      return;
    }
  rproc.push_back(pr);
  //  debug("  low %d",rproc.size()-1);
}

int remove_restart_proc(RestartProc *pr)
{
  if (!pr)
    return 0;
  for (int i = 0; i < rproc.size(); ++i)
  {
    if (rproc[i] == pr)
    {
      erase_items(rproc, i, 1);
      return 1;
    }
  }
  return 0;
}

int del_restart_proc(RestartProc *pr)
{
  if (!remove_restart_proc(pr))
    return 0;
  pr->destroy();
  return 1;
}

int find_restart_proc(RestartProc *pr)
{
  if (!pr)
    return 0;
  for (int i = 0; i < rproc.size(); ++i)
    if (rproc[i] == pr)
      return 1;
  return 0;
}


static int rflg = 0;

void set_restart_flags(int f) { rflg |= f; }

int is_restart_required(int f) { return rflg & f; }

void startup_game(int f, void (*on_before_proc)(const char *), void (*on_after_proc)(const char *))
{
  rflg &= ~RESTART_GO;

  debug("@ start up, flags=%08X", f & ~RESTART_GO);
  int t0 = get_time_msec();

  for (int i = 0; i < rproc.size(); ++i)
  {

    if (on_before_proc)
      (*on_before_proc)(rproc[i]->procname());

    int t1 = get_time_msec();
    loading_progress_point();
    VERBOSE_DEBUG("* starting %s", rproc[i]->procname());
    rproc[i]->startupf(f);
    VERBOSE_DEBUG("* started %s", rproc[i]->procname());

    int t = get_time_msec() - t1;
    if (t != 0)
      debug("@ started %s in %d ms", rproc[i]->procname(), t);

    if (on_after_proc)
      (*on_after_proc)(rproc[i]->procname());
  }

  debug("@ start up in %d ms", get_time_msec() - t0);
  (void)t0;

  rflg &= ~(f | RESTART_GO);
  // reset_cur_time();
  loading_progress_point();
}

void shutdown_game(int f)
{
  bool erase = (f & RESTART_ERASE_RPROC) != 0;
  debug("@ shut down, flags=%08X", f & ~RESTART_GO);
  int t0 = get_time_msec();

  for (int i = rproc.size() - 1; i >= 0; --i)
  {
    int t1 = get_time_msec();
    VERBOSE_DEBUG("* shutting down %s", rproc[i]->procname());
    rproc[i]->shutdownf(f);
    VERBOSE_DEBUG("* shut down %s", rproc[i]->procname());
    int t = get_time_msec() - t1;
    if (t != 0)
      debug("@ shut down %s in %d ms", rproc[i]->procname(), t);
    if (erase)
      erase_items(rproc, i, 1);
  }
  debug("@ shutdown in %d ms", get_time_msec() - t0);
  (void)t0;
}


#define EXPORT_PULL dll_pull_baseutil_restart
#include <supp/exportPull.h>
