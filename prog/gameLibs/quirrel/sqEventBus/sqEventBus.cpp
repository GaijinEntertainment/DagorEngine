// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <quirrel/sqEventBus/sqEventBus.h>
#include <quirrel/sqCrossCall/sqCrossCall.h>
#include <EASTL/hash_map.h>
#include <EASTL/hash_set.h>
#include <memory/dag_framemem.h>
#include <quirrel/quirrel_json/jsoncpp.h>
#include <quirrel/sqStackChecker.h>
#include <util/dag_string.h>
#include <util/dag_finally.h>
#include <osApiWrappers/dag_miscApi.h>
#include <generic/dag_tab.h>
#include <quirrel/sqModules/sqModules.h>
#include <squirrel.h>
#include <sqrat/sqratFunction.h>
#include <sqext.h>
#include <dag/dag_vector.h>
#include <osApiWrappers/dag_rwLock.h>
#include <osApiWrappers/dag_spinlock.h>
#include <osApiWrappers/dag_atomic.h>


namespace sqeventbus
{

static constexpr SQInteger NO_PAYLOAD = 999999;

NativeEventHandler g_native_event_handler = nullptr;

struct EvHandler
{
  Sqrat::Function func;
  bool oneHit = false;
  bool listenAll = false;
  eastl::hash_set<eastl::string> allowedSources;

  EvHandler(Sqrat::Function f, bool oh) : func(f), oneHit(oh) {}
  EvHandler() {}

  bool isSourceAccepted(const char *source_id)
  {
    if (listenAll)
      return true;
    return allowedSources.find_as(source_id) != allowedSources.end();
  }
};

using HandlersMap = eastl::hash_multimap<eastl::string, EvHandler>;

struct QueuedEvent
{
  Sqrat::Function handler;
  Json::Value value;
  int timestamp;
};

struct BoundVm
{
  ProcessingMode processingMode;
  HandlersMap handlers;
  dag::Vector<QueuedEvent> queue, queueCurrent;
  OSReadWriteLock handlersLock;
  OSSpinlock queueLock;
};

static eastl::hash_map<HSQUIRRELVM, BoundVm> vms;
static OSReentrantReadWriteLock vmsLock;
static volatile int is_in_send = 0;


static void process_events_impl(HSQUIRRELVM vm, BoundVm &vmData);


static bool collect_handlers(BoundVm &vm_data, const char *evt_name, const char *source_id, Tab<Sqrat::Function> &handlers_out)
{
  G_ASSERT(handlers_out.empty());
  ScopedLockWriteTemplate guard(vm_data.handlersLock);

  auto handlersRange = vm_data.handlers.equal_range(evt_name);
  for (auto itHandler = handlersRange.first; itHandler != handlersRange.second;)
  {
    if (!itHandler->second.isSourceAccepted(source_id))
    {
      ++itHandler;
      continue;
    }

    handlers_out.push_back(itHandler->second.func);
    if (itHandler->second.oneHit)
      itHandler = vm_data.handlers.erase(itHandler);
    else
      ++itHandler;
  }

  return !handlers_out.empty();
}


static bool has_listeners_in_vm(BoundVm &vm_data, const char *evt_name, const char *source_id)
{
  ScopedLockReadTemplate guard(vm_data.handlersLock);
  auto handlersRange = vm_data.handlers.equal_range(evt_name);
  for (auto itHandler = handlersRange.first; itHandler != handlersRange.second;)
  {
    if (itHandler->second.isSourceAccepted(source_id))
      return true;
  }
  return false;
}


static SQInteger has_listeners_internal(HSQUIRRELVM vm_from, const char *evt_name, bool foreign_only, const char *source_id)
{
  ScopedLockReadTemplate guard(vmsLock);
  for (auto itVm = vms.begin(); itVm != vms.end(); ++itVm)
  {
    HSQUIRRELVM vm_to = itVm->first;
    if (foreign_only && vm_to == vm_from)
      continue;
    BoundVm &target = itVm->second;
    if (has_listeners_in_vm(target, evt_name, source_id))
    {
      sq_pushbool(vm_from, SQTrue);
      return 1;
    }
  }
  sq_pushbool(vm_from, SQFalse);
  return 1;
}


static SQRESULT throw_send_error(HSQUIRRELVM vm_from, const char *err_msg, const Sqrat::Object &payload)
{
  Sqrat::Table errInfo(vm_from);
  errInfo.SetValue("errMsg", err_msg);
  errInfo.SetValue("payload", payload);
  sq_pushobject(vm_from, errInfo.GetObject());
  return sq_throwobject(vm_from);
}


static SQInteger send_internal(HSQUIRRELVM vm_from, const char *evt_name, SQInteger payload_idx, bool foreign_only,
  const char *source_id)
{
  SQInteger handledCount = 0;
  int prevTopFrom = sq_gettop(vm_from);
  G_UNUSED(prevTopFrom);
  String repushErrMsg;

  interlocked_increment(is_in_send);
  FINALLY([] { interlocked_decrement(is_in_send); });

  Tab<Sqrat::Function> handlersCopy(framemem_ptr());
  Json::Value msg;
  bool msgConverted = false;

  ScopedLockReadTemplate guard(vmsLock);

  auto thisVm = vms.find(vm_from);
  if (thisVm != vms.end() && thisVm->second.processingMode == ProcessingMode::IMMEDIATE)
    process_events_impl(vm_from, thisVm->second);

  for (auto itVm = vms.begin(); itVm != vms.end(); ++itVm)
  {
    HSQUIRRELVM vm_to = itVm->first;
    if (foreign_only && vm_to == vm_from)
      continue;

    BoundVm &target = itVm->second;

    if (!collect_handlers(target, evt_name, source_id, handlersCopy))
      continue;

    if (!msgConverted && (vm_from != vm_to || target.processingMode == ProcessingMode::MANUAL_PUMP))
    {
      Sqrat::Object msgObj(vm_from);

      if (payload_idx != NO_PAYLOAD)
      {
        HSQOBJECT hMsg;
        sq_getstackobj(vm_from, payload_idx, &hMsg);
        msgObj = Sqrat::Object(hMsg, vm_from);
      }

      msg = quirrel_to_jsoncpp(msgObj);
      msgConverted = true;
    }

    if (vm_to == vm_from && target.processingMode == ProcessingMode::IMMEDIATE)
    {
      SqStackChecker stackCheckTo(vm_to);
      Sqrat::Object msgDst(vm_to);

      if (payload_idx != NO_PAYLOAD)
      {
        HSQOBJECT hMsg;
        sq_getstackobj(vm_from, payload_idx, &hMsg);
        msgDst = Sqrat::Object(hMsg, vm_from);
      }

      for (Sqrat::Function &handler : handlersCopy)
      {
        if (!handler.Execute(msgDst))
        {
          SQFunctionInfo fi;
          sq_ext_getfuncinfo(handler.GetFunc(), &fi);
          String errMsg(0, "Subscriber %s call failed, target VM = %p | %p", fi.name, handler.GetVM(), vm_to);
          Sqrat::Object msgSrc;
          if (payload_idx != NO_PAYLOAD)
            msgSrc = Sqrat::Var<Sqrat::Object>(vm_from, payload_idx).value;
          return throw_send_error(vm_from, errMsg.c_str(), msgSrc);
        }
        ++handledCount;
      }
    }
    else
    {
      for (Sqrat::Function &handler : handlersCopy)
      {
        QueuedEvent qevt;
        qevt.handler = handler;
        qevt.value = msg;
        qevt.timestamp = get_time_msec();
        {
          OSSpinlockScopedLock queueGuard(target.queueLock);
          target.queue.emplace_back(eastl::move(qevt));
        }
        ++handledCount;
      }
    }

    handlersCopy.clear();
  }

  if (thisVm != vms.end() && thisVm->second.processingMode == ProcessingMode::IMMEDIATE)
    process_events_impl(vm_from, thisVm->second);

  sq_pushinteger(vm_from, handledCount);
  return 1;
}


void send_event(const char *event_name, const Sqrat::Object &data, const char *source_id)
{
  G_ASSERT_RETURN(source_id, );
  if (g_native_event_handler)
  {
    Json::Value json = quirrel_to_jsoncpp(data);
    g_native_event_handler(event_name, json, source_id, /*immediate*/ false);
  }

  if (data.GetVM() == nullptr)
    return;

  HSQUIRRELVM vm = data.GetVM();
  SqStackChecker stackCheck(vm);
  sq_pushobject(vm, data.GetObject());
  SQInteger ret = send_internal(vm, event_name, sq_gettop(vm), false, source_id);
  G_UNUSED(ret);
  G_ASSERTF(SQ_SUCCEEDED(ret), "eventbus send failed for '%s'", event_name);
  stackCheck.restore();
}


void send_event(const char *event_name, const Json::Value &data, const char *source_id)
{
  interlocked_increment(is_in_send);
  FINALLY([] { interlocked_decrement(is_in_send); });

  if (g_native_event_handler)
    g_native_event_handler(event_name, data, source_id, /*immediate*/ false);

  ScopedLockReadTemplate guard(vmsLock);
  for (auto &v : vms)
  {
    BoundVm &target = v.second;

    Tab<Sqrat::Function> handlersCopy(framemem_ptr());
    if (!collect_handlers(target, event_name, source_id, handlersCopy))
      continue;

    for (Sqrat::Function &handler : handlersCopy)
    {
      QueuedEvent qevt;
      qevt.handler = handler;
      qevt.value = data;
      qevt.timestamp = get_time_msec();
      {
        OSSpinlockScopedLock queueGuard(target.queueLock);
        target.queue.emplace_back(eastl::move(qevt));
      }
    }
  }
}

void send_event(const char *event_name, const char *source_id) { send_event(event_name, Json::Value(), source_id); }

bool has_listeners(const char *event_name, const char *source_id)
{
  ScopedLockReadTemplate guard(vmsLock);
  for (auto &v : vms)
  {
    BoundVm &target = v.second;
    if (has_listeners_in_vm(target, event_name, source_id))
      return true;
  }
  return false;
}


static SQInteger sq_send_impl(HSQUIRRELVM vm_from, bool foreign_only)
{
  bool hasPayload = sq_gettop(vm_from) > 3;

  const char *eventName = nullptr;
  sq_getstring(vm_from, 2, &eventName);
  const char *vmId = nullptr;
  G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm_from, hasPayload ? 4 : 3, &vmId)));
  SQInteger payloadIdx = hasPayload ? 3 : NO_PAYLOAD;
  return send_internal(vm_from, eventName, payloadIdx, foreign_only, vmId);
}

static SQInteger send(HSQUIRRELVM vm_from) { return sq_send_impl(vm_from, false); }

static SQInteger send_foreign(HSQUIRRELVM vm_from) { return sq_send_impl(vm_from, true); }


static SQInteger has_listeners(HSQUIRRELVM vm_from)
{
  const char *eventName = nullptr;
  sq_getstring(vm_from, 2, &eventName);
  const char *vmId = nullptr;
  G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm_from, 3, &vmId)));
  return has_listeners_internal(vm_from, eventName, false, vmId);
}


static SQInteger has_foreign_listeners(HSQUIRRELVM vm_from)
{
  const char *eventName = nullptr;
  sq_getstring(vm_from, 2, &eventName);
  const char *vmId = nullptr;
  G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm_from, 3, &vmId)));
  return has_listeners_internal(vm_from, eventName, true, vmId);
}


static void process_events_impl(HSQUIRRELVM vm, BoundVm &vmData)
{
  SqStackChecker stackCheck(vm);
  auto hook = sq_set_sq_call_hook(vm, nullptr); // remove the hook temprarily to avoid recursion

  {
    OSSpinlockScopedLock queueGuard(vmData.queueLock);
    vmData.queueCurrent.swap(vmData.queue);
  }

  for (QueuedEvent &qevt : vmData.queueCurrent)
  {
    if (!qevt.handler.GetVM())
      continue;
    auto handler = eastl::move(qevt.handler); // to avoid recursion
    if (!handler.Execute(jsoncpp_to_quirrel(vm, qevt.value)))
    {
      SQFunctionInfo fi;
      sq_ext_getfuncinfo(qevt.handler.GetFunc(), &fi);
      logerr("Subscriber %s call failed, target VM = %p", fi.name, qevt.handler.GetVM());
    }
  }

  vmData.queueCurrent.clear();
  sq_set_sq_call_hook(vm, hook);
}


void process_events(HSQUIRRELVM vm)
{
  ScopedLockReadTemplate guard(vmsLock);

  auto itVm = vms.find(vm);
  G_ASSERTF_RETURN(itVm != vms.end(), , "VM is not bound");

  BoundVm &vmData = itVm->second;
  G_ASSERT_RETURN(vmData.processingMode == ProcessingMode::MANUAL_PUMP, );

  process_events_impl(vm, vmData);
}


static SQInteger subscribe_impl(HSQUIRRELVM vm, bool onehit)
{
  const char *evtName = nullptr;
  HSQOBJECT funcObj;
  sq_getstring(vm, 2, &evtName);
  sq_getstackobj(vm, 3, &funcObj);

  Sqrat::Function f(vm, Sqrat::Object(vm), funcObj);

  ScopedLockReadTemplate vmGuard(vmsLock);
  auto &boundVm = vms[vm];

  ScopedLockWriteTemplate handlerGuard(boundVm.handlersLock);
  HandlersMap &handlers = boundVm.handlers;

  // prevent duplicate subscribe
  auto itRange = handlers.equal_range(evtName);
  for (auto itHandler = itRange.first; itHandler != itRange.second; ++itHandler)
  {
    if (itHandler->second.func.IsEqual(f))
    {
      sq_pushbool(vm, false);
      return 1;
    }
  }

  auto resIt = handlers.insert(eastl::make_pair(eastl::string(evtName), EvHandler(f, onehit)));
  G_ASSERT(resIt != handlers.end());

  // setup filters
  EvHandler &eh = resIt->second;
  if (sq_gettop(vm) < 4)
    eh.listenAll = true;
  else
  {
    eh.listenAll = false;
    SQObjectType fltType = sq_gettype(vm, 4);
    if (fltType == OT_STRING)
    {
      const char *s = nullptr;
      sq_getstring(vm, 4, &s);
      if (strcmp(s, "*") == 0)
        eh.listenAll = true;
      else
        eh.allowedSources.emplace(s);
    }
    else if (fltType == OT_ARRAY)
    {
      Sqrat::Var<Sqrat::Array> fltArr(vm, 4);
      SQInteger len = fltArr.value.Length();
      if (len < 1)
        return sq_throwerror(vm, "Empty filters array");
      for (SQInteger i = 0; i < len; ++i)
      {
        Sqrat::Object item = fltArr.value.GetSlot(i);
        if (item.GetType() != OT_STRING)
          return sq_throwerror(vm, "Source name must be a string");
        else
          eh.allowedSources.emplace(item.GetVar<const char *>().value);
      }
    }
    else
      G_ASSERTF(0, "Unexpected filter object type %X", fltType);
  }

  sq_pushbool(vm, true);
  return 1;
}


static SQInteger subscribe(HSQUIRRELVM vm) { return subscribe_impl(vm, false); }


static SQInteger subscribe_onehit(HSQUIRRELVM vm) { return subscribe_impl(vm, true); }


static SQInteger unsubscribe(HSQUIRRELVM vm)
{
  ScopedLockReadTemplate vmGuard(vmsLock);
  auto itVm = vms.find(vm);
  if (itVm == vms.end())
  {
    sq_pushbool(vm, false);
    return 1;
  }

  const char *eventName = nullptr;
  G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, 2, &eventName)));

  HSQOBJECT hFunc;
  sq_getstackobj(vm, 3, &hFunc);
  Sqrat::Function func(vm, Sqrat::Object(vm), hFunc);

  ScopedLockWriteTemplate handlerGuard(itVm->second.handlersLock);
  HandlersMap &handlers = itVm->second.handlers;
  auto itRange = handlers.equal_range(eventName);

  for (auto itHandler = itRange.first; itHandler != itRange.second; ++itHandler)
  {
    if (itHandler->second.func.IsEqual(func))
    {
      handlers.erase(itHandler);
      sq_pushbool(vm, true);
      return 1;
    }
  }

  sq_pushbool(vm, false);
  return 1;
}


static void sq_call_hook(HSQUIRRELVM vm)
{
  ScopedLockReadTemplate guard(vmsLock);

  auto itVm = vms.find(vm);
  G_ASSERTF_RETURN(itVm != vms.end(), , "VM is not bound");

  BoundVm &vmData = itVm->second;
  process_events_impl(vm, vmData);
}


void bind(SqModules *module_mgr, const char *vm_id, ProcessingMode mode)
{
  HSQUIRRELVM vm = module_mgr->getVM();

  ScopedLockWriteTemplate guard(vmsLock);
  G_ASSERT(vms.find(vm) == vms.end());
  BoundVm &vmData = vms[vm];
  vmData.processingMode = mode;

  if (mode == ProcessingMode::IMMEDIATE)
    G_VERIFY(sq_set_sq_call_hook(vm, sq_call_hook) == nullptr);

  sq_pushstring(vm, vm_id, -1);
  Sqrat::Object sqVmId = Sqrat::Var<Sqrat::Object>(vm, -1).value;
  sq_pop(vm, 1);

  Sqrat::Table api(vm);

  api
    .SquirrelFunc("subscribe", subscribe, -3, ".sc a|s") // alias for backward compatibility
    .SquirrelFunc("subscribe_onehit", subscribe_onehit, -3, ".sc a|s")
    .SquirrelFunc("unsubscribe", unsubscribe, 3, ".sc")
    .SquirrelFunc("send", send, -2, ".s.", nullptr, 1, &sqVmId)
    .SquirrelFunc("send_foreign", send_foreign, -2, ".s.", nullptr, 1, &sqVmId)
    .SquirrelFunc("has_listeners", has_listeners, 2, ".s", nullptr, 1, &sqVmId)
    .SquirrelFunc("has_foreign_listeners", has_foreign_listeners, 2, ".s", nullptr, 1, &sqVmId)
    ///@module eventbus
    .SquirrelFunc("eventbus_subscribe", subscribe, -3, ".sc a|s")
    .SquirrelFunc("eventbus_subscribe_onehit", subscribe_onehit, -3, ".sc a|s")
    .SquirrelFunc("eventbus_unsubscribe", unsubscribe, 3, ".sc")
    .SquirrelFunc("eventbus_send", send, -2, ".s.", nullptr, 1, &sqVmId)
    .SquirrelFunc("eventbus_send_foreign", send_foreign, -2, ".s.", nullptr, 1, &sqVmId)
    .SquirrelFunc("eventbus_has_listeners", has_listeners, 2, ".s", nullptr, 1, &sqVmId)
    .SquirrelFunc("eventbus_has_foreign_listeners", has_foreign_listeners, 2, ".s", nullptr, 1, &sqVmId)
    /**/;

  module_mgr->addNativeModule("eventbus", api);
}


void unbind(HSQUIRRELVM vm)
{
  ScopedLockWriteTemplate guard(vmsLock);
  G_ASSERT(!interlocked_acquire_load(is_in_send));
  auto hook = sq_set_sq_call_hook(vm, nullptr);
  G_UNUSED(hook);
  G_ASSERT(hook == sq_call_hook || hook == nullptr);
  vms.erase(vm);
}


void clear_on_reload(HSQUIRRELVM vm)
{
  ScopedLockReadTemplate guard(vmsLock);
  G_ASSERT(!interlocked_acquire_load(is_in_send));
  auto itVm = vms.find(vm);
  if (itVm != vms.end())
  {
    ScopedLockWriteTemplate handlerGuard(itVm->second.handlersLock);
    itVm->second.handlers.clear();
  }
}


void do_with_vm(const eastl::function<void(HSQUIRRELVM)> &callback)
{
  ScopedLockReadTemplate guard(vmsLock);
  // return any VM to allow creating script objects that will be passed to all VMs
  auto it = vms.begin();
  callback(it != vms.end() ? it->first : nullptr);
}


void set_native_event_handler(NativeEventHandler handler) { g_native_event_handler = handler; }


} // namespace sqeventbus
