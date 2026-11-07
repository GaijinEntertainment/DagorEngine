// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_logSys.h>
#include <bindQuirrelEx/bindQuirrelEx.h>
#include <sqmodules/sqmodules.h>
#include <util/dag_delayedAction.h>
#include <util/dag_simpleString.h>
#include <perfMon/dag_cpuFreq.h>
#include <generic/dag_tab.h>
#include <util/dag_string.h>


namespace bindquirrel
{

struct InterceptorRec
{
  Tab<SimpleString> tags;
  Sqrat::Function cb;
  bool isMonitor = false;
  uint8_t gen = 0;

  void reset()
  {
    clear_and_shrink(tags);
    cb.Release();
    ++gen;
  }
  bool isFreeSlot() const { return cb.GetVM() == nullptr; }

  bool isFuncEqual(const Sqrat::Function &func)
  {
    HSQOBJECT a = cb.GetFunc(), b = func.GetFunc();
    return sq_obj_is_equal(func.GetVM(), &a, &b);
  }
};
static Tab<InterceptorRec> interceptors;

static LogHandlerId interceptor_handler_id = LogHandlerId::Invalid;
static void on_debug_log(void *, LogMessage &msg)
{
  if (!msg.fmt)
    return;
  const bool shownByOtherHandler = msg.flags & LOGMSG_SHOWN_TO_USER;
  String str;
  str.vprintf(0, msg.fmt, (const DagorSafeArg *)msg.arg, msg.anum);
  for (auto &r : interceptors)
  {
    if (!r.isMonitor && shownByOtherHandler)
      continue;
    for (auto &s : r.tags)
      if (strstr(str, s))
      {
        uint8_t gen = r.gen;
        int i = &r - interceptors.data();
        int ti = &s - r.tags.data();
        delayed_call([logstr = String(str), i, ti, gen, t = get_time_msec()]() {
          InterceptorRec *ir = safe_at(interceptors, i); // Note: interceptors might be cleared already (hence at())
          if (ir && ir->gen == gen)
            ir->cb(ir->tags[ti].str(), logstr.str(), t);
        });
        if (r.isMonitor)
          continue;
        msg.flags |= LOGMSG_SHOWN_TO_USER;
        break;
      }
  }
}

static void register_callback(Sqrat::Array tags, const Sqrat::Function &func, bool is_mon)
{
  int freeIdx = -1;
  for (auto &r : interceptors)
  {
    if (freeIdx < 0 && r.isFreeSlot())
      freeIdx = &r - interceptors.data();
    if (r.isFuncEqual(func))
    {
      r.tags.resize(tags.Length());
      for (int i = 0; i < r.tags.size(); i++)
        r.tags[i] = tags.GetValue<const char *>(i);
      return;
    }
  }

  InterceptorRec &r = (freeIdx >= 0) ? interceptors[freeIdx] : interceptors.push_back();
  r.cb = func;
  r.tags.resize(tags.Length());
  r.isMonitor = is_mon;
  for (int i = 0; i < r.tags.size(); i++)
    r.tags[i] = tags.GetValue<const char *>(i);
}

static void register_callback_int_sq(Sqrat::Array tags, const Sqrat::Function &func) { register_callback(tags, func, false); }

static void register_callback_mon_sq(Sqrat::Array tags, const Sqrat::Function &func) { register_callback(tags, func, true); }

static void unregister_callback_sq(const Sqrat::Function &func)
{
  for (auto &intr : interceptors)
    if (intr.isFuncEqual(func))
      intr.reset();
}

static SQInteger unregister_all_callback_sq(HSQUIRRELVM vm)
{
  for (auto &intr : interceptors)
    if (intr.cb.GetVM() == vm)
      intr.reset();
  return 0;
}

void setup_logerr_interceptor()
{
  if (interceptor_handler_id == LogHandlerId::Invalid)
    interceptor_handler_id = debug_add_log_handler(&on_debug_log, nullptr, {.levelMask = LOGMASK_ERR, .order = LOGORDER_SHOW_SCRIPT});
}
void clear_logerr_interceptors(HSQUIRRELVM vm) { unregister_all_callback_sq(vm); }
void term_logerr_interceptor()
{
  debug_remove_log_handler(interceptor_handler_id);
  interceptor_handler_id = LogHandlerId::Invalid;
  clear_and_shrink(interceptors);
}

void logerr_interceptor_bind_api(Sqrat::Table &nsTbl)
{
  nsTbl
    // register_logerr_interceptor(tags, callback) tags - ["sometag","tag2"], callback - function(tag, logstring, timestamp){})
    .Func("register_logerr_interceptor", register_callback_int_sq)
    // register_logerr_monitor(tags, callback) tags - ["sometag","tag2"], callback - function(tag, logstring, timestamp){})
    .Func("register_logerr_monitor", register_callback_mon_sq)
    // unregister_logerr_interceptor(callback)
    .Func("unregister_logerr_interceptor", unregister_callback_sq)
    // clear_logerr_interceptors()
    .SquirrelFuncDeclString(unregister_all_callback_sq, "clear_logerr_interceptors(): null")
    /**/;
}

} // namespace bindquirrel
