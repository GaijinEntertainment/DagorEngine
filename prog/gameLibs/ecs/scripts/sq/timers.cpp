#include "timers.h"

#include <sqrat.h>
#include <generic/dag_tab.h>
#include <memory/dag_framemem.h>

#include <ecs/core/entityManager.h>
#include <ecs/core/entitySystem.h>
#include <daECS/core/updateStage.h>
#include <daECS/core/entityComponent.h>
#include <ecs/scripts/sqEntity.h>

#include <EASTL/string.h>
#include <bindQuirrelEx/bindQuirrelEx.h>
#include <ska_hash_map/flat_hash_map2.hpp>

ECS_REGISTER_EVENT_NS(ecs::sq, Timer);

namespace ecs
{

namespace sq
{

struct BaseTimer
{
  float interval;
  float timeout;
  bool repeat;
  bool useRealDt;
};

struct CallbackTimer : BaseTimer
{
  Sqrat::Function func;
};

struct EcsTimer : BaseTimer
{
  Sqrat::Object data;
  EcsTimer()
  {
    interval = timeout = 0;
    repeat = false;
  }
  EcsTimer(const Sqrat::Object &data_, float dt, bool repeat_) : data(data_)
  {
    interval = timeout = dt;
    repeat = repeat_;
  }
};

static Tab<CallbackTimer> callback_timers;
struct EidStringPairHash
{
  typedef ska::power_of_two_hash_policy hash_policy;
  size_t operator()(const eastl::pair<EntityId, eastl::string> &x) const
  {
    return (size_t)((eastl::hash<eastl::string>()(x.second) * 16777619) ^ (entity_id_t)x.first);
  }
};
static ska::flat_hash_map<eastl::pair<EntityId, eastl::string>, EcsTimer, EidStringPairHash> ecs_timers;


static int find_callback_timer(const Sqrat::Object &handler)
{
  if (handler.GetType() != OT_CLOSURE && handler.GetType() != OT_NATIVECLOSURE)
    return -1;

  HSQUIRRELVM vm = handler.GetVM();
  HSQOBJECT o = handler.GetObject();
  for (int i = 0, n = callback_timers.size(); i < n; ++i)
  {
    HSQOBJECT fh = callback_timers[i].func.GetFunc();

    SQInteger cmpRes = 0;
    if (::sq_direct_cmp(vm, &fh, &o, &cmpRes) && cmpRes == 0)
      return i;
  }
  return -1;
}

static Sqrat::Object set_callback_timer_impl(Sqrat::Object handler, float interval, bool repeat, bool use_real_dt)
{
  int curIdx = find_callback_timer(handler);
  CallbackTimer *t = nullptr;
  if (curIdx < 0)
    t = &callback_timers.push_back();
  else
    t = &callback_timers[curIdx];

  HSQUIRRELVM vm = handler.GetVM();
  t->func = Sqrat::Function(vm, Sqrat::Object(vm), handler.GetObject());
  t->interval = interval;
  t->repeat = repeat;
  t->useRealDt = use_real_dt;

  t->timeout = interval;

  return handler;
}

static Sqrat::Object set_callback_timer(Sqrat::Object handler, float interval, bool repeat)
{
  return set_callback_timer_impl(handler, interval, repeat, false /*use_real_dt*/);
}

static Sqrat::Object set_callback_timer_rt(Sqrat::Object handler, float interval, bool repeat)
{
  return set_callback_timer_impl(handler, interval, repeat, true /*use_real_dt*/);
}


static void clear_callback_timer(Sqrat::Object handler)
{
  int curIdx = find_callback_timer(handler);
  if (curIdx >= 0)
    erase_items(callback_timers, curIdx, 1);
}


static SQInteger set_timer(HSQUIRRELVM vm)
{
  Sqrat::Var<Sqrat::Table> params(vm, 2);

  Sqrat::Object eid = params.value.GetSlot("eid");
  Sqrat::Object id = params.value.GetSlot("id");
  Sqrat::Object data = params.value.GetSlot("data");
  Sqrat::Object interval = params.value.GetSlot("interval");
  Sqrat::Object repeat = params.value.GetSlot("repeat");

  if (eid.GetType() != OT_INTEGER)
    return sq_throwerror(vm, "Required parameter 'eid' is missing or is of wrong type");
  if (id.GetType() != OT_STRING)
    return sq_throwerror(vm, "Required parameter 'id' is missing or is not a string");
  if (!(interval.GetType() & SQOBJECT_NUMERIC))
    return sq_throwerror(vm, "Required parameter 'interval' is missing or is not numeric");
  if (repeat.GetType() != OT_BOOL && !(repeat.GetType() & SQOBJECT_NUMERIC))
    return sq_throwerror(vm, "Required parameter 'repeat' is missing or is not a boolean");

  auto key = eastl::make_pair(eid.Cast<EntityId>(), id.Cast<eastl::string>());
  auto it = ecs_timers.find(key);
  if (it != ecs_timers.end())
    return sq_throwerror(vm, "Timer already exists");
  ecs_timers[key] = EcsTimer(data, interval.Cast<float>(), repeat.Cast<bool>());
  return 0;
}


static SQInteger clear_timer(HSQUIRRELVM vm)
{
  Sqrat::Var<Sqrat::Table> params(vm, 2);
  Sqrat::Object eid = params.value.GetSlot("eid");
  Sqrat::Object id = params.value.GetSlot("id");

  if (eid.GetType() != OT_INTEGER)
    return sq_throwerror(vm, "Required parameter 'eid' is missing or is of wrong type");
  if (id.GetType() != OT_STRING)
    return sq_throwerror(vm, "Required parameter 'id' is missing or is not a string");

  auto key = eastl::make_pair(eid.Cast<EntityId>(), id.Cast<eastl::string>());
  auto it = ecs_timers.find(key);
  if (it != ecs_timers.end())
  {
    ecs_timers.erase(it);
    sq_pushbool(vm, true);
  }
  else
    sq_pushbool(vm, false);
  return 1;
}


void bind_timers(Sqrat::Table &exports)
{
  ///@module ecs
  exports.Func("set_callback_timer", set_callback_timer)
    .Func("set_callback_timer_rt", set_callback_timer_rt)
    .Func("clear_callback_timer", clear_callback_timer)
    .SquirrelFunc("set_timer", set_timer, 2, ".t")
    .SquirrelFunc("clear_timer", clear_timer, 2, ".t");
}


void shutdown_timers(HSQUIRRELVM /*vm*/)
{
  clear_and_shrink(callback_timers);
  ecs_timers.clear();
}


void update_timers(float dt, float rt_dt)
{
  eastl::vector<eastl::tuple<ecs::EntityId, eastl::string, Sqrat::Object, float>, framemem_allocator> notifications;

  for (auto it = ecs_timers.begin(); it != ecs_timers.end();)
  {
    it->second.timeout -= dt;

    if (it->second.timeout > 0)
      ++it;
    else
    {
      notifications.push_back(eastl::make_tuple(it->first.first, it->first.second, it->second.data, it->second.timeout));
      if (it->second.repeat)
      {
        it->second.timeout = it->second.interval;
        ++it;
      }
      else
        it = ecs_timers.erase(it);
    }
  }

  for (auto it = notifications.begin(); it != notifications.end(); ++it)
    g_entity_mgr->sendEvent(eastl::get<0>(*it), ecs::sq::Timer(eastl::get<1>(*it), eastl::get<2>(*it), eastl::get<3>(*it)));

  Tab<Sqrat::Function> functionsToCall(framemem_ptr());
  for (int i = callback_timers.size() - 1; i >= 0; --i)
  {
    CallbackTimer &t = callback_timers[i];
    t.timeout -= t.useRealDt ? rt_dt : dt;
    if (t.timeout > 0)
      continue;

    functionsToCall.push_back(t.func);
    if (t.repeat)
      t.timeout = t.interval;
    else
      erase_items(callback_timers, i, 1);
  }
  for (auto &func : functionsToCall)
    func();
}


} // namespace sq

} // namespace ecs
