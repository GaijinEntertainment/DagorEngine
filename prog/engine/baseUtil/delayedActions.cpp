// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_delayedAction.h>
#include <generic/dag_tab.h>
#include <perfMon/dag_cpuFreq.h>
#include <debug/dag_debug.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_lockProfiler.h>
#include <util/dag_lag.h>

#undef add_delayed_action
#undef add_delayed_action_buffered
#undef add_delayed_callback
#undef add_delayed_callback_buffered

struct ScopedPerfLag
{
#if ACTION_DEBUG_NAMES
  const char *name;
  uint64_t ref;
  ScopedPerfLag(const char *n) : name(n), ref(profile_ref_ticks()) {}
  ~ScopedPerfLag()
  {
    int64_t t = profile_ref_ticks() - ref;
    const int64_t freq = profile_ticks_frequency();
    if (t * 10000 > freq) // 100usec
    {
      const char *perfTagName = name;
#if TIME_PROFILER_ENABLED
#if _TARGET_PC_WIN || _TARGET_XBOX
      if (const char *pSlash = name ? strrchr(name, '\\') : nullptr)
#else
      if (const char *pSlash = name ? strrchr(name, '/') : nullptr)
#endif
        perfTagName = pSlash + 1;
#else
      G_UNUSED(perfTagName);
#endif
      DA_PROFILE_TAG(delayedActionName, perfTagName);
      if (dgs_lag_handler && t * 10 > freq)       // 100msec
        dgs_lag_handler(name, (t * 1000) / freq); // time in msec
    }
  }
#else
  ScopedPerfLag(const char *) {}
#endif
};

struct DelayedRecord
{
  DelayedAction *action = nullptr;
  delayed_callback cb = nullptr;
  void *cbArg;
#if ACTION_DEBUG_NAMES
  const char *debugName;
#else
  static constexpr inline const char *debugName = nullptr;
#endif
  DelayedRecord() = default;
  DelayedRecord(DelayedAction *a, delayed_callback c, void *cb_arg, const char *debug_name) :
    action(a),
    cb(c),
    cbArg(cb_arg)
#if ACTION_DEBUG_NAMES
    ,
    debugName(debug_name)
#endif
  {
    G_UNUSED(debug_name);
    G_ASSERT(a || c);
  }
  void run()
  {
    DA_PROFILE_EVENT("delayed_action");
    ScopedPerfLag timer(debugName);
    if (action)
      action->performAction();
    else
      (*cb)(cbArg);
  }
  bool operator==(const DelayedRecord &rhs) const { return action == rhs.action; }
  void destroy()
  {
    if (action)
      delete action;
    action = nullptr;
    cb = nullptr;
  }
  bool precondition() const { return action ? action->precondition() : true; }
  bool isValid() const { return action || cb; }
};

#if _TARGET_PC && (DAGOR_DBGLEVEL > 0 || DAGOR_FORCE_LOGS)
// ACTION_DEBUG_NAMES_LOC embeds a newline+tab, which would split the line and hide the site from a grep
static const char *one_line_name(const char *src, char *buf, size_t buf_size)
{
  if (!src)
    return "?";
  size_t i = 0;
  for (; src[i] && i + 1 < buf_size; ++i)
    buf[i] = (src[i] == '\n' || src[i] == '\t') ? ' ' : src[i];
  buf[i] = '\0';
  return buf;
}

static void warn_dropped(const Tab<DelayedRecord> &list, const char *list_name)
{
  char name[192];
  for (const DelayedRecord &rec : list)
    if (rec.isValid()) // constraint: exit() can run inside a pass
      logwarn("delayed %s <%s> in %s list dropped, never performed", rec.action ? "action" : "callback",
        one_line_name(rec.debugName, name, sizeof(name)), list_name);
}

// once teardown started no drain reads the lists again, so free the record instead of queueing it into a dead one
static void drop_posted_at_teardown(DelayedAction *action, const char *debug_name)
{
  char name[192];
  logwarn("delayed %s <%s> posted after teardown started, dropped", action ? "action" : "callback",
    one_line_name(debug_name, name, sizeof(name)));
  delete action;
}
#else
static void warn_dropped(const Tab<DelayedRecord> &, const char *) {}
static void drop_posted_at_teardown(DelayedAction *action, const char *) { delete action; }
#endif

static struct DelayedActionsContext
{
  Tab<DelayedRecord> delayed_actions, delayed_actions_buffered;
  WinCritSec delayedCrit;
  int quotaUsec = 40000;
  uint32_t postCount = 0;   //< bumped on every post, so flush() can tell a fresh record from a precondition-blocked one
  bool tearingDown = false; //< set once in the destructor and never cleared
  int runningPasses = 0;    //< run() can reenter perform(); outermost compacts

  DelayedActionsContext() { delayed_actions.reserve(8); }
  ~DelayedActionsContext()
  {
    // detach under the lock, since a back thread can still post while we tear down; destroy() deletes user
    // actions, so it runs outside the lock, the same rule perform() follows for run() and destroy()
    Tab<DelayedRecord> actions, buffered;
    {
      WinAutoLock lock(delayedCrit);
      tearingDown = true;
      actions.swap(delayed_actions);
      buffered.swap(delayed_actions_buffered);
    }

    // records are dropped here, never run: a callback that never ran leaks whatever it was meant to free
    warn_dropped(actions, "main");
    warn_dropped(buffered, "buffered");

    for (DelayedRecord &rec : actions)
      rec.destroy();
    for (DelayedRecord &rec : buffered)
      rec.destroy();
  }

  // false once teardown started, and then the record is ours to free: no drain will ever read it
  bool post(Tab<DelayedRecord> &list, DelayedAction *action, delayed_callback cb, void *cb_arg, const char *debug_name)
  {
    WinAutoLock lock(delayedCrit);
    if (DAGOR_UNLIKELY(tearingDown))
      return false;
    postCount++;
    list.push_back(DelayedRecord(action, cb, cb_arg, debug_name ? debug_name : "DelayedAction"));
    return true;
  }

  void add(DelayedAction *action, delayed_callback cb = NULL, void *cb_arg = NULL, const char *debug_name = NULL)
  {
    if (!action && !cb)
      return;
    if (!post(delayed_actions, action, cb, cb_arg, debug_name))
      drop_posted_at_teardown(action, debug_name);
  }

  void addBuffered(DelayedAction *action, delayed_callback cb = NULL, void *cb_arg = NULL, const char *debug_name = NULL)
  {
    if (!action && !cb)
      return;
    if (!post(delayed_actions_buffered, action, cb, cb_arg, debug_name))
      drop_posted_at_teardown(action, debug_name);
  }

  bool remove(DelayedAction *action)
  {
    {
      WinAutoLock lock(delayedCrit);
      auto it = eastl::find(delayed_actions.begin(), delayed_actions.end(), DelayedRecord(action, NULL, NULL, NULL));
      if (it == delayed_actions.end())
        return false;
      if (runningPasses && it != delayed_actions.end() - 1)
        *it = {}; // keep the indices of a running pass
      else
        delayed_actions.erase(it);
    }
    delete action;
    return true;
  }

  void move_buffered_actions()
  {
    append_items(delayed_actions, delayed_actions_buffered.size(), delayed_actions_buffered.data());
    delayed_actions_buffered.clear();
  }
  int queuedCount() const
  {
    return eastl::count_if(delayed_actions.begin(), delayed_actions.end(), [](const DelayedRecord &rec) { return rec.isValid(); });
  }
  // returns how many actions ran; 0 means none could and only buffered records were promoted, which is what flush()
  // stops on. a positive count can still be a pass the quota truncated, leaving runnable records queued
  int perform()
  {
    delayedCrit.lock();
    if (delayed_actions.empty())
    {
      move_buffered_actions();
      delayedCrit.unlock();
      return 0;
    }
    DA_PROFILE_EVENT("perform_delayed_actions");
    runningPasses++;
    int cnt = 0;
    bool quotaHit = false;
    int64_t t_start = ref_time_ticks_qpc();
    for (int i = 0; i < delayed_actions.size() && !quotaHit; ++i)
    {
      if (!delayed_actions[i].isValid() || !delayed_actions[i].precondition())
        continue;
      DelayedRecord action = eastl::exchange(delayed_actions[i], {});
      delayedCrit.unlock();
      action.run();
      action.destroy();
      cnt++;
      // Note: we _want_ to break here regardless of debugger presence to avoid altering program behaviour
      quotaHit = get_time_usec_qpc(t_start) > quotaUsec;
      delayedCrit.lock();
    }
    if (--runningPasses == 0)
      delayed_actions.erase(
        eastl::remove_if(delayed_actions.begin(), delayed_actions.end(), [](const DelayedRecord &rec) { return !rec.isValid(); }),
        delayed_actions.end());
    const int actionsLeft = quotaHit ? queuedCount() : 0, bufferedLeft = delayed_actions_buffered.size();
    move_buffered_actions();
    delayedCrit.unlock();
    if (DAGOR_UNLIKELY(quotaHit))
      logwarn("stop perform_delayed_actions() after %d actions performed for %d usec; quota is %d usec;"
              " actions left in list: %d normal and %d buffered",
        cnt, get_time_usec_qpc(t_start), quotaUsec, actionsLeft, bufferedLeft);
    return cnt;
  }

  void flush()
  {
    // perform() can promote buffered records and still return 0 without running them, so promote up front;
    // a pass that ran nothing with nothing posted meanwhile leaves only precondition-blocked records
    constexpr int maxPasses = 8;
    int pass = 0;
    for (; pass < maxPasses; ++pass)
    {
      uint32_t postedBefore;
      {
        WinAutoLock lock(delayedCrit);
        move_buffered_actions();
        postedBefore = postCount;
      }

      const int done = perform();

      WinAutoLock lock(delayedCrit);
      if (!done && postCount == postedBefore)
        break;
    }

    // both exits can leave records queued: precondition-blocked ones on the natural break, and whatever a
    // quota-truncated pass did not reach at the cap; the caller tears down without them having run
    int left;
    {
      WinAutoLock lock(delayedCrit);
      left = queuedCount() + delayed_actions_buffered.size();
    }
    if (left)
      logwarn("flush_delayed_actions() left %d action(s) queued (%s)", left,
        pass == maxPasses ? "pass cap reached" : "blocked on precondition");
  }

} da_ctx;

void add_delayed_action(DelayedAction *action, const char *debug_name, const char *) { da_ctx.add(action, NULL, NULL, debug_name); }

void add_delayed_action_buffered(DelayedAction *action, const char *debug_name, const char *)
{
  da_ctx.addBuffered(action, NULL, NULL, debug_name);
}

bool remove_delayed_action(DelayedAction *action) { return da_ctx.remove(action); }

void add_delayed_callback(delayed_callback cb, void *cb_arg, const char *debug_name, const char *)
{
  da_ctx.add(NULL, cb, cb_arg, debug_name);
}

void add_delayed_callback_buffered(delayed_callback cb, void *cb_arg, const char *debug_name, const char *)
{
  da_ctx.addBuffered(NULL, cb, cb_arg, debug_name);
}

/* you can't implement remove_delayed_callback() because pair cb+arg doesn not identify it uniquely */

void perform_delayed_actions() { da_ctx.perform(); }

void flush_delayed_actions() { da_ctx.flush(); }

void set_delayed_action_max_quota(int quota_usec) { da_ctx.quotaUsec = quota_usec; }

void execute_delayed_action_on_main_thread(DelayedAction *action, bool always_in_order, int wait_quant_msec)
{
  struct SyncDoWaitAction : public DelayedAction
  {
    volatile int &done;
    DelayedAction *action;
    SyncDoWaitAction(DelayedAction *a, volatile int &d) : done(d), action(a) {}
    virtual void performAction()
    {
      if (action)
      {
        action->performAction();
        delete action;
      }
      interlocked_release_store(done, 1);
    }

  private:
    SyncDoWaitAction &operator=(const SyncDoWaitAction &) { return *this; }
  };

  volatile int syncOpsDone = 0;
  if (is_main_thread())
  {
    if (always_in_order)
    {
      add_delayed_action(new SyncDoWaitAction(action, syncOpsDone));
      while (!syncOpsDone)
        perform_delayed_actions();
    }
    else if (action)
    {
      action->performAction();
      delete action;
    }
    return;
  }

  add_delayed_action(new SyncDoWaitAction(action, syncOpsDone));
  ScopeLockProfiler<da_profiler::DescRpc> lp;
  G_UNUSED(lp);
  while (!interlocked_acquire_load(syncOpsDone)) // fixme:this is actually wait on value
    sleep_msec(wait_quant_msec);
}

struct RegularActionRecord
{
  delayed_callback cb;
  void *cbArg;
  bool performOnWorkCycle;
};
static Tab<RegularActionRecord> regular_actions;

void register_regular_action_to_idle_cycle(delayed_callback cb, void *cb_arg, bool perform_on_work_cycle)
{
  WinAutoLock lock(da_ctx.delayedCrit);
  for (int i = 0; i < regular_actions.size(); i++)
    if (regular_actions[i].cb == cb && regular_actions[i].cbArg == cb_arg)
    {
      G_ASSERT(regular_actions[i].performOnWorkCycle == perform_on_work_cycle);
      return;
    }
  RegularActionRecord &r = regular_actions.push_back();
  r.cb = cb;
  r.cbArg = cb_arg;
  r.performOnWorkCycle = perform_on_work_cycle;
}
void unregister_regular_action_to_idle_cycle(delayed_callback cb, void *cb_arg)
{
  WinAutoLock lock(da_ctx.delayedCrit);
  for (int i = 0; i < regular_actions.size(); i++)
    if (regular_actions[i].cb == cb && regular_actions[i].cbArg == cb_arg)
    {
      erase_items(regular_actions, i, 1);
      return;
    }
}
void perform_regular_actions_for_idle_cycle(bool is_work_cycle)
{
  decltype(regular_actions) localActions;
  {
    WinAutoLock lock(da_ctx.delayedCrit);
    if (regular_actions.empty())
      return;
    localActions.swap(regular_actions);
  }

  for (auto &act : localActions)
    if (!is_work_cycle || act.performOnWorkCycle)
      act.cb(act.cbArg);

  WinAutoLock lock(da_ctx.delayedCrit);
  if (regular_actions.empty())
    regular_actions.swap(localActions); // swap it back to preserve memory
  else                                  // we allow addition within delayed action but do not allow removal
  {
    for (auto &act : regular_actions) // append newly added
      localActions.push_back(act);
    regular_actions.swap(localActions);
  }
}

#define EXPORT_PULL dll_pull_baseutil_delayedActions
#include <supp/exportPull.h>
