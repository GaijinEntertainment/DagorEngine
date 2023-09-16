#include <debug/dag_logSys.h>
#include <bindQuirrelEx/bindQuirrelEx.h>
#include <sqModules/sqModules.h>
#include <util/dag_delayedAction.h>
#include <util/dag_simpleString.h>
#include <perfMon/dag_cpuFreq.h>

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

  bool isFuncEqual(Sqrat::Function &func)
  {
    SQInteger cmpRes;
    HSQOBJECT a = cb.GetFunc(), b = func.GetFunc();
    return sq_direct_cmp(func.GetVM(), &a, &b, &cmpRes) && cmpRes == 0;
  }
};
static Tab<InterceptorRec> interceptors;

static debug_log_callback_t orig_debug_log = nullptr;
static int on_debug_log(int lev_tag, const char *fmt, const void *arg, int anum, const char *ctx_file, int ctx_line)
{
  if (lev_tag == LOGLEVEL_ERR && fmt)
  {
    String str;
    str.vprintf(0, fmt, (const DagorSafeArg *)arg, anum);
    bool found = false;
    for (auto &r : interceptors)
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
          found = true;
          break;
        }
    if (found)
      return 1;
  }

  if (orig_debug_log)
    return orig_debug_log(lev_tag, fmt, arg, anum, ctx_file, ctx_line);
  return 1;
}

static void register_callback(Sqrat::Array tags, Sqrat::Function func, bool is_mon)
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

static void register_callback_int_sq(Sqrat::Array tags, Sqrat::Function func) { register_callback(tags, func, false); }

static void register_callback_mon_sq(Sqrat::Array tags, Sqrat::Function func) { register_callback(tags, func, true); }

static void unregister_callback_sq(Sqrat::Function func)
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

void setup_logerr_interceptor() { orig_debug_log = debug_set_log_callback(on_debug_log); }
void clear_logerr_interceptors(HSQUIRRELVM vm) { unregister_all_callback_sq(vm); }
void term_logerr_interceptor() { clear_and_shrink(interceptors); }

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
    .SquirrelFunc("clear_logerr_interceptors", unregister_all_callback_sq, 1, ".");
}

} // namespace bindquirrel
