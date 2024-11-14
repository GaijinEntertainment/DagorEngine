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

static struct DelayedActionsContext
{
  Tab<DelayedRecord> delayed_actions, delayed_actions_buffered;
  WinCritSec delayedCrit;
  int quotaUsec = 40000;

  DelayedActionsContext() { delayed_actions.reserve(8); }
  ~DelayedActionsContext()
  {
    for (int i = 0; i < delayed_actions.size(); ++i)
      delayed_actions[i].destroy();
    clear_and_shrink(delayed_actions);
    for (int i = 0; i < delayed_actions_buffered.size(); ++i)
      delayed_actions_buffered[i].destroy();
    clear_and_shrink(delayed_actions_buffered);
  }

  void add(DelayedAction *action, delayed_callback cb = NULL, void *cb_arg = NULL, const char *debug_name = NULL)
  {
    if (!action && !cb)
      return;
    WinAutoLock lock(delayedCrit);
    delayed_actions.push_back(DelayedRecord(action, cb, cb_arg, debug_name ? debug_name : "DelayedAction"));
  }

  void addBuffered(DelayedAction *action, delayed_callback cb = NULL, void *cb_arg = NULL, const char *debug_name = NULL)
  {
    if (!action && !cb)
      return;
    WinAutoLock lock(delayedCrit);
    delayed_actions_buffered.push_back(DelayedRecord(action, cb, cb_arg, debug_name ? debug_name : "DelayedAction"));
  }

  bool remove(DelayedAction *action)
  {
    bool ret;
    {
      WinAutoLock lock(delayedCrit);
      ret = erase_item_by_value(delayed_actions, DelayedRecord(action, NULL, NULL, NULL));
    }
    if (ret)
      delete action;
    return ret;
  }

  void move_buffered_actions()
  {
    append_items(delayed_actions, delayed_actions_buffered.size(), delayed_actions_buffered.data());
    delayed_actions_buffered.clear();
  }
  void perform()
  {
    delayedCrit.lock();
    if (delayed_actions.empty())
    {
      move_buffered_actions();
      delayedCrit.unlock();
      return;
    }
    DA_PROFILE_EVENT("perform_delayed_actions");
    int i = 0, actionsLeft = 0, cnt = 0;
    auto getNextAction = [&, this]() {
      DelayedRecord act;
      for (; i < delayed_actions.size(); ++i)
        if (delayed_actions[i].precondition())
        {
          act = delayed_actions[i];
          erase_items(delayed_actions, i, 1);
          actionsLeft = delayed_actions.size();
          break;
        }
      return act;
    };
    DelayedRecord action = getNextAction();
    if (!action.isValid())
    {
      move_buffered_actions();
      delayedCrit.unlock();
      return;
    }
    delayedCrit.unlock();

    int64_t t_start = ref_time_ticks_qpc();
    do
    {
      action.run();
      action.destroy();
      cnt++;
      if (DAGOR_UNLIKELY(get_time_usec_qpc(t_start) > quotaUsec))
      {
        logwarn("stop perform_delayed_actions() after %d actions performed for %d usec; quota is %d usec;"
                " actions left in list: %d normal and %d buffered",
          cnt, get_time_usec_qpc(t_start), quotaUsec, actionsLeft, delayed_actions_buffered.size());
        // Note: we _want_ to break here regadless of debugger presense to avoid altering program behaviour
        break;
      }
      WinAutoLock lock(delayedCrit);
      action = getNextAction();
    } while (action.isValid());

    WinAutoLock lock(delayedCrit);
    move_buffered_actions();
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
};
static Tab<RegularActionRecord> regular_actions;

void register_regular_action_to_idle_cycle(delayed_callback cb, void *cb_arg)
{
  WinAutoLock lock(da_ctx.delayedCrit);
  for (int i = 0; i < regular_actions.size(); i++)
    if (regular_actions[i].cb == cb && regular_actions[i].cbArg == cb_arg)
      return;
  RegularActionRecord &r = regular_actions.push_back();
  r.cb = cb;
  r.cbArg = cb_arg;
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
void perform_regular_actions_for_idle_cycle()
{
  decltype(regular_actions) localActions;
  {
    WinAutoLock lock(da_ctx.delayedCrit);
    if (regular_actions.empty())
      return;
    localActions.swap(regular_actions);
  }

  for (auto &act : localActions)
    act.cb(act.cbArg);

  WinAutoLock lock(da_ctx.delayedCrit);
  if (regular_actions.empty())
    regular_actions.swap(localActions); // swap it back to preserve memory
  else                                  // we allow addition within delayed action but do not allow removal
  {
    for (auto &act : regular_actions) // append newly added
      localActions.push_back(RegularActionRecord{act.cb, act.cbArg});
    regular_actions.swap(localActions);
  }
}

#define EXPORT_PULL dll_pull_baseutil_delayedActions
#include <supp/exportPull.h>
