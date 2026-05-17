// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <quirrel/asyncRuntime/dagorAsyncRuntime.h>

#include <quirrel/sqasync_helpers.h>
#include <sqasync.h>
#include <sqmodules/sqmodules.h>
#include <sqrat.h>
#include <squirrel.h>

#include <EASTL/vector.h>
#include <debug/dag_assert.h>
#include <debug/dag_log.h>
#include <osApiWrappers/dag_spinlock.h>
#include <perfMon/dag_cpuFreq.h>
#include <util/dag_globDef.h>


namespace sqasync_dagor
{

namespace
{

struct TimerEntry
{
  sqasync::VmCallback fn;
  void *user;
  int fire_msec;
};


struct ResolveCtx
{
  sqasync_helpers::PromiseHolder holder;
  explicit ResolveCtx(HSQUIRRELVM vm) : holder(vm) {}
};


// Posted from sqasync::pump() (nextFrame) and AsyncRuntimeScope::pump() (delay).
void resolve_with_null_cb(void *user)
{
  ResolveCtx *ctx = (ResolveCtx *)user;
  ctx->holder.resolveNull();
  delete ctx;
}


// Create a paired heap ResolveCtx with a fresh Promise. On success: leaves
// the Promise instance on the stack (so the native can return 1) and returns
// the heap ctx. On failure: nothing left on the stack, returns nullptr.
ResolveCtx *make_pending_resolve_ctx(HSQUIRRELVM vm)
{
  auto *ctx = new ResolveCtx(vm);
  if (sq_isnull(ctx->holder.instance()))
  {
    delete ctx;
    return nullptr;
  }
  sq_pushobject(vm, ctx->holder.instance());
  return ctx;
}

} // anonymous namespace


struct AsyncRuntimeScope::Impl
{
  HSQUIRRELVM vm = nullptr;
  OSSpinlock lock;
  eastl::vector<TimerEntry> timers;
  bool shut_down = false;

  // Drain pending entries under lock and latch shut_down. Idempotent: a
  // second call returns an empty vector. Caller decides what to do with the
  // drained entries (post to inbox / log + drop).
  eastl::vector<TimerEntry> drainPending()
  {
    eastl::vector<TimerEntry> all;
    OSSpinlockScopedLock lk(lock);
    if (shut_down)
      return all;
    shut_down = true;
    all.swap(timers);
    return all;
  }
};


// Native binding for `async.delay(seconds): Promise`.
// Stack on entry: 1=this, 2=seconds, -1=scope userpointer (free var).
static SQInteger async_delay_native(HSQUIRRELVM vm)
{
  SQUserPointer scopeUp = nullptr;
  if (SQ_FAILED(sq_getuserpointer(vm, -1, &scopeUp)) || !scopeUp)
    return sq_throwerror(vm, "async.delay: runtime not bound");
  AsyncRuntimeScope *scope = static_cast<AsyncRuntimeScope *>(scopeUp);

  SQFloat seconds = 0;
  if (SQ_FAILED(sq_getfloat(vm, 2, &seconds)))
  {
    SQInteger asInt = 0;
    if (SQ_SUCCEEDED(sq_getinteger(vm, 2, &asInt)))
      seconds = (SQFloat)asInt;
    else
      return sq_throwerror(vm, "async.delay: 'seconds' must be a number");
  }

  ResolveCtx *ctx = make_pending_resolve_ctx(vm);
  if (!ctx)
    return SQ_ERROR;

  if (SQ_FAILED(scope->postDelayed((double)seconds, &resolve_with_null_cb, ctx)))
  {
    sq_pop(vm, 1); // drop the instance we pushed for return
    delete ctx;
    return sq_throwerror(vm, "async.delay: runtime is shutting down");
  }

  return 1; // promise instance left on stack
}


// Native binding for `async.nextFrame(): Promise`.
// Resolves on the next sqasync::pump() invocation.
static SQInteger async_next_frame_native(HSQUIRRELVM vm)
{
  ResolveCtx *ctx = make_pending_resolve_ctx(vm);
  if (!ctx)
    return SQ_ERROR;

  if (SQ_FAILED(sqasync::post_on_vm_thread(vm, &resolve_with_null_cb, ctx)))
  {
    sq_pop(vm, 1); // drop the instance we pushed for return
    delete ctx;
    return sq_throwerror(vm, "async.nextFrame: runtime is shutting down");
  }

  return 1; // promise instance left on stack
}


// Builds the "async" module table. Leaves the table on the stack on success.
static bool push_async_module_table(HSQUIRRELVM vm, AsyncRuntimeScope &scope)
{
  sq_newtable(vm);
  const SQInteger tableIdx = sq_gettop(vm);

  // Promise class slot.
  sq_pushstring(vm, "Promise", -1);
  if (SQ_FAILED(sqasync::push_promise_class(vm)))
  {
    sq_settop(vm, tableIdx - 1); // drop key + table
    return false;
  }
  sq_newslot(vm, tableIdx, SQFalse);

  // delay(seconds) native, bound with scope pointer as a free var.
  sq_pushstring(vm, "delay", -1);
  sq_pushuserpointer(vm, &scope);
  sq_newclosure(vm, async_delay_native, 1);
  sq_setnativeclosurename(vm, -1, "delay");
  G_VERIFY(SQ_SUCCEEDED(sq_setparamscheck(vm, 2, ".n")));
  sq_newslot(vm, tableIdx, SQFalse);

  // nextFrame() native -- resolves on the next pump.
  sq_pushstring(vm, "nextFrame", -1);
  sq_newclosure(vm, async_next_frame_native, 0);
  sq_setnativeclosurename(vm, -1, "nextFrame");
  G_VERIFY(SQ_SUCCEEDED(sq_setparamscheck(vm, 1, ".")));
  sq_newslot(vm, tableIdx, SQFalse);

  return true;
}


AsyncRuntimeScope::AsyncRuntimeScope(SqModules *module_mgr) : impl(new Impl)
{
  HSQUIRRELVM vm = module_mgr->getVM();
  impl->vm = vm;

  if (SQ_FAILED(sqasync::bind(vm)))
  {
    G_ASSERTF(0, "sqasync::bind failed (already bound on this VM?)");
    return;
  }

  if (!push_async_module_table(vm, *this))
  {
    G_ASSERTF(0, "asyncRuntime: cannot build 'async' module table");
    return;
  }

  HSQOBJECT moduleHandle;
  sq_resetobject(&moduleHandle);
  sq_getstackobj(vm, -1, &moduleHandle);

  module_mgr->addNativeModule("async", Sqrat::Object(moduleHandle, vm));

  sq_pop(vm, 1); // drop the table; addNativeModule kept its own ref
}


AsyncRuntimeScope::~AsyncRuntimeScope()
{
  auto pending = impl->drainPending();
  if (!pending.empty())
    logwarn("[asyncRuntime] AsyncRuntimeScope destroyed without shutdown(); %d pending timer ctx(s) leaked", (int)pending.size());
  delete impl;
}


void AsyncRuntimeScope::shutdown()
{
  // Caller ordering contract: this runs while the VM is still alive AND while
  // the sqasync inbox is still open (before sq_close), so the timer callbacks
  // land in the inbox and run during sqasync's own drain at sq_close.
  auto pending = impl->drainPending();
  for (const auto &e : pending)
  {
    if (SQ_FAILED(sqasync::post_on_vm_thread(impl->vm, e.fn, e.user)))
      e.fn(e.user); // inbox already closed; direct-fire so the heap ctx is freed
  }
}


SQRESULT AsyncRuntimeScope::postDelayed(double delay_sec, sqasync::VmCallback fn, void *user)
{
  const int delay_msec = (delay_sec > 0.0) ? (int)(delay_sec * 1000.0) : 0;
  const int fire = get_time_msec() + delay_msec;
  OSSpinlockScopedLock lk(impl->lock);
  if (impl->shut_down)
    return SQ_ERROR;
  impl->timers.push_back({fn, user, fire});
  return SQ_OK;
}


SQInteger AsyncRuntimeScope::pump(SQInteger max_steps)
{
  const int now = get_time_msec();

  // On the VM thread by contract, so post_on_vm_thread (no lock) is the right primitive.
  eastl::vector<TimerEntry> expired;
  {
    OSSpinlockScopedLock lk(impl->lock);
    auto &t = impl->timers;
    for (size_t i = 0; i < t.size(); /*manual*/)
    {
      if (t[i].fire_msec - now <= 0)
      {
        expired.push_back(t[i]);
        t[i] = t.back();
        t.pop_back();
      }
      else
        ++i;
    }
  }

  for (const auto &e : expired)
  {
    if (SQ_FAILED(sqasync::post_on_vm_thread(impl->vm, e.fn, e.user)))
    {
      // Inbox closed: fire directly so the callback can free its heap ctx.
      // Callbacks must tolerate a closed runtime.
      e.fn(e.user);
    }
  }

  return sqasync::pump(impl->vm, max_steps);
}


bool AsyncRuntimeScope::hasPendingTimers() const
{
  OSSpinlockScopedLock lk(impl->lock);
  return !impl->timers.empty();
}

} // namespace sqasync_dagor
