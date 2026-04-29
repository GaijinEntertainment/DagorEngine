// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <quirrel/sqEventBus/sqEventBus.h>
#include <quirrel/sqEventBus/sqEventBusEx.h>
#include <quirrel/sqCrossCall/sqCrossCall.h>
#include <generic/dag_relocatableFixedVector.h>
#include <EASTL/vector_multimap.h>
#include <EASTL/vector_set.h>
#include <EASTL/variant.h>
#include <EASTL/optional.h>
#include <util/dag_hash.h>
#include <util/dag_string.h>
#include <memory/dag_framemem.h>
#include <quirrel/quirrel_json/jsoncpp.h>
#include <quirrel/quirrel_json/rapidjson.h>
#include <quirrel/sqStackChecker.h>
#include <osApiWrappers/dag_miscApi.h>
#include <generic/dag_tab.h>
#include <sqmodules/sqmodules.h>
#include <squirrel.h>
#include <quirrel/quirrelHost/memtrace.h>
#include <sqrat/sqratFunction.h>
#include <sqext.h>
#include <dag/dag_vector.h>
#include <osApiWrappers/dag_rwLock.h>
#include <osApiWrappers/dag_rwSpinLock.h>
#include <osApiWrappers/dag_spinlock.h>
#include <osApiWrappers/dag_atomic.h>
#include <rapidjson/document.h>

// Squirrel internals
#include <squirrel/sqpcheader.h>
#include <squirrel/sqvm.h>

// Char used separate event name from source, e.g. "foo.bar|baz" -> eventName="foo.bar", source="bar"
#define EVENT_SOURCE_SEP_CHAR '|'

namespace sqeventbus
{
struct BoundVm;
struct QueuedEvent;
} // namespace sqeventbus
DAG_DECLARE_RELOCATABLE(sqeventbus::BoundVm);
DAG_DECLARE_RELOCATABLE(sqeventbus::QueuedEvent);

namespace sqeventbus
{
static constexpr SQInteger NO_PAYLOAD = 0;

NativeEventHandler g_native_event_handler = nullptr;

struct EvHandler
{
#if DAGOR_DBGLEVEL > 0 && _DEBUG_TAB_
  eastl::string debugEventName; // For debugging/hash collisions
#endif
  Sqrat::Function func;
  bool oneHit = false;
  eastl::vector_set<uint32_t> allowedSources; // if empty - accepts all

  EvHandler(Sqrat::Function &&f, bool oh) : func(eastl::move(f)), oneHit(oh) {}

  bool isSourceAccepted(uint32_t srchash) const { return allowedSources.empty() || allowedSources.count(srchash); }
};

// Note: rapidjson::Document's copy ctor is private (i.e. prohibited)
static inline JsonVariant copy_rjson(const rapidjson::Document &rjson)
{
  JsonVariant v{rapidjson::Document()};
  eastl::get<rapidjson::Document>(v).CopyFrom(rjson, eastl::get<rapidjson::Document>(v).GetAllocator());
  return v;
}

static Sqrat::Object jsonv_to_quirrel(HSQUIRRELVM vm, const JsonVariant &value, int memory_threshold_bytes = 0)
{
  const bool haveMemoryThreshold = memory_threshold_bytes > 0;
  const int previousMemoryTreshold = haveMemoryThreshold ? sqmemtrace::set_huge_alloc_threshold(memory_threshold_bytes) : -1;

  Sqrat::Object result = eastl::visit(
    [vm, &value](const auto &json) {
      using JsonType = eastl::remove_cvref_t<decltype(json)>;
      if constexpr (eastl::is_same_v<JsonType, Json::Value>)
        return jsoncpp_to_quirrel(vm, json);
      else if constexpr (eastl::is_same_v<JsonType, rapidjson::Document>)
        return rapidjson_to_quirrel(vm, json);
      else if constexpr (eastl::is_same_v<JsonType, intptr_t>)
        return jsonv_to_quirrel(vm, *(const JsonVariant *)((char *)&value + json));
      else if constexpr (eastl::is_same_v<JsonType, Sqrat::Object>)
        return json;

      G_ASSERTF(false, "Unexpected JsonVariant type %d", (int)value.index());
      return Sqrat::Object(vm);
    },
    value);

  if (haveMemoryThreshold)
  {
    sqmemtrace::set_huge_alloc_threshold(previousMemoryTreshold);
  }

  return result;
}


struct QueuedEvent
{
  Sqrat::Function handler;   // To consider: unionine/variant with `eventSource`?
  eastl::string eventSource; // 2 strings joined with EVENT_SOURCE_SEP_CHAR; not empty only if handler is null
  JsonVariant value;
  int memoryThresholdBytes = 0; // zero means no value

  static eastl::string buildEventSource(const char *evt, const char *src)
  {
    G_ASSERTF(!strchr(evt, EVENT_SOURCE_SEP_CHAR), "\"%s\" : '%c' characted is not allowed in event name", evt, EVENT_SOURCE_SEP_CHAR);
    if (!src)
      return evt;
    else
      return eastl::string(eastl::string::CtorSprintf{}, "%s%c%s", evt, EVENT_SOURCE_SEP_CHAR, src);
  }

  QueuedEvent(Sqrat::Function &&h, eastl::string &&es, JsonVariant &&jv) :
    handler(eastl::move(h)), eventSource(eastl::move(es)), value(eastl::move(jv))
  {}
};

struct BoundVm
{
  ProcessingMode processingMode;
  bool freezeWEvtTables;
  bool freezeSqTables;
  eastl::vector_multimap</*fnv1a-32 hash*/ uint32_t, EvHandler> handlers;
  dag::Vector<QueuedEvent> queue;
  SpinLockReadWriteLock handlersLock;
  OSSpinlock queueLock;
};

using BaseVmsArray = dag::RelocatableFixedVector<eastl::pair<HSQUIRRELVM, BoundVm>, 4, false>;
class VmsArray : public BaseVmsArray
{
public:
  BaseVmsArray::iterator find(HSQUIRRELVM v)
  {
    return eastl::find(begin(), end(), v, [](auto &p, auto vv) { return p.first == vv; });
  }
  eastl::pair<BaseVmsArray::iterator, bool> insert(HSQUIRRELVM v)
  {
    G_ASSERT(find(v) == end());
    return eastl::make_pair(&emplace_back(eastl::pair_first_construct, v), true);
  }
  void erase(HSQUIRRELVM v)
  {
    auto it = find(v);
    if (it != end())
      BaseVmsArray::erase(it);
  }
};
static VmsArray vms;
static OSReentrantReadWriteLock vmsLock;
static bool mtsafe_mode = false;
#if DAGOR_DBGLEVEL > 0
static volatile int is_in_send = 0;
struct ScopeDebugInSendGuard
{
  ScopeDebugInSendGuard() { interlocked_increment(is_in_send); }
  ~ScopeDebugInSendGuard() { interlocked_decrement(is_in_send); }
};
#else
struct ScopeDebugInSendGuard
{};
#endif

void set_mtsafe_mode(bool m) { interlocked_relaxed_store(mtsafe_mode, m); }

static void process_events_impl(HSQUIRRELVM vm, BoundVm &vmData);

template <typename F>
static inline bool collect_handlers_nolock(BoundVm &vm_data, const char *evt_name, const char *source_id, F ccb)
{
  uint32_t evhash = str_hash_fnv1a(evt_name);
  auto it = vm_data.handlers.find(evhash);
  if (it == vm_data.handlers.end())
    return false;
  uint32_t srchash = source_id ? str_hash_fnv1a(source_id) : str_hash_fnv1a("native");
  for (; it != vm_data.handlers.end() && it->first == evhash;)
  {
    if (!it->second.isSourceAccepted(srchash))
    {
      ++it;
      continue;
    }

    if (ccb(it->second))
      return true;

    if (!it->second.oneHit)
      ++it;
    else
      it = vm_data.handlers.erase(it);
  }
  return false;
}

static inline bool query_handlers(BoundVm &vm_data, const char *evt_name, const char *source_id)
{
  ScopedLockReadTemplate vmGuard(vm_data.handlersLock);
  return collect_handlers_nolock(vm_data, evt_name, source_id, [](auto &) { return true; });
}

static inline bool collect_handlers(BoundVm &vm_data, int8_t &isMainThreadCached, const char *event_name, const char *source_id,
  Tab<Sqrat::Function> &handlers_out)
{
  G_ASSERT(handlers_out.empty());
  ScopedLockWriteTemplate handlerGuard(vm_data.handlersLock); // To consider: use rw lock only if oneHit handlers exist
  if (collect_handlers_nolock(vm_data, event_name, source_id, [&](EvHandler &eh) {
        if (isMainThreadCached < 0)
          isMainThreadCached = is_main_thread();
        if (!isMainThreadCached)
          return true; // Called from non main thread -> stop iterating to use delayed eventSource code path
        if (!eh.oneHit)
          handlers_out.push_back(eh.func);
        else
          handlers_out.push_back(eastl::move(eh.func));
        return false; // keep iterating
      }))
    return true;
  return !handlers_out.empty();
}

static inline bool collect_handlers(BoundVm &vm_data, eastl::string &&eventSource, Tab<Sqrat::Function> &handlers_out)
{
  G_ASSERT(handlers_out.empty());
  const char *source = nullptr;
  // To consider: save flag about not having encoded event source to avoid this `strstr` call
  if (char *pSep = strchr(eventSource.data(), EVENT_SOURCE_SEP_CHAR))
  {
    *pSep = '\0';
    source = pSep + 1;
  }
  ScopedLockWriteTemplate handlerGuard(vm_data.handlersLock); // To consider: use rw lock only if oneHit handlers exist
  collect_handlers_nolock(vm_data, eventSource.c_str(), source, [&](EvHandler &eh) {
    if (!eh.oneHit)
      handlers_out.push_back(eh.func);
    else
      handlers_out.push_back(eastl::move(eh.func));
    return false; // keep iterating
  });
  return !handlers_out.empty();
}

static SQInteger has_listeners_internal(HSQUIRRELVM vm_from, const char *evt_name, bool foreign_only, const char *source_id)
{
  ScopedLockReadTemplate guard(vmsLock);
  for (auto itVm = vms.begin(); itVm != vms.end(); ++itVm)
  {
    HSQUIRRELVM vm_to = itVm->first;
    if (foreign_only && vm_to == vm_from)
      continue;
    if (query_handlers(itVm->second, evt_name, source_id))
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

static SQInteger send_internal(HSQUIRRELVM vm_from, const char *event_name, SQInteger payload_idx, bool foreign_only,
  const char *source_id)
{
  int prevTopFrom = sq_gettop(vm_from);
  G_UNUSED(prevTopFrom);
  String repushErrMsg;

  [[maybe_unused]] ScopeDebugInSendGuard inSendGuard;

  eastl::optional<rapidjson::Document> maybeMsg;

  ScopedLockReadTemplate guard(vmsLock);

  auto thisVm = vms.find(vm_from);
  if (thisVm != vms.end() && thisVm->second.processingMode == ProcessingMode::IMMEDIATE)
    process_events_impl(vm_from, thisVm->second);

  int8_t isMainThreadCached = -1;
  for (auto itVm = vms.begin(); itVm != vms.end(); ++itVm)
  {
    HSQUIRRELVM vm_to = itVm->first;
    if (foreign_only && vm_to == vm_from)
      continue;

    BoundVm &target = itVm->second;

    Tab<Sqrat::Function> handlersCopy(framemem_ptr());
    if (!collect_handlers(target, isMainThreadCached, event_name, source_id, handlersCopy))
      continue;

    Sqrat::Object msgDst(vm_to);
    if (payload_idx == NO_PAYLOAD)
      ;
    else if (vm_from == vm_to) // Send to the same vm -> just queue in table
      msgDst = Sqrat::Var<Sqrat::Object>(vm_to, payload_idx).value;
    else if (!interlocked_relaxed_load(sqeventbus::mtsafe_mode)) // Cross push !mtsafe mode
    {
      if (DAGOR_UNLIKELY(!is_main_thread())) // To consider: throw err?
      {
        sqstd_printcallstack(vm_from);
        logerr("Attempt to send squirrel event <%s> in !mtsafe mode. Was `set_mtsafe_mode(true)` call missed somewhere?", event_name);
      }
      int prevTopTo = sq_gettop(vm_to);
      if (DAGOR_UNLIKELY(!sq_cross_push(vm_from, payload_idx, vm_to, &repushErrMsg)))
      {
        sq_poptop(vm_to);
        String errMsg(0, "Cross-push failed, target VM = %p: %s", vm_to, repushErrMsg.c_str());
        return throw_send_error(vm_from, errMsg.c_str(), Sqrat::Var<Sqrat::Object>(vm_from, payload_idx).value);
      }
      msgDst = Sqrat::Var<Sqrat::Object>(vm_to, -1).value;
      sq_settop(vm_to, prevTopTo);
    }
    else if (!maybeMsg) // Cross push in mtsafe mode
    {
      Sqrat::Object msgObj(vm_from);
      msgObj = Sqrat::Var<Sqrat::Object>(vm_from, payload_idx).value;
      maybeMsg = quirrel_to_rapidjson(msgObj);
    }

#if DAGOR_DBGLEVEL > 0 && defined(_DEBUG_TAB_)
    // Logerr on non str table keys. TODO: do something about non-pure data types too
    if (payload_idx != NO_PAYLOAD && !maybeMsg)
      quirrel_to_jsoncpp(msgDst);
#endif

    if (vm_to != vm_from || target.processingMode != ProcessingMode::IMMEDIATE)
    {
      auto buildJv = [&]() { return maybeMsg ? copy_rjson(*maybeMsg) : JsonVariant{msgDst}; };
      OSSpinlockScopedLock queueGuard(target.queueLock);
      if (handlersCopy.empty())
        target.queue.emplace_back(Sqrat::Function{}, QueuedEvent::buildEventSource(event_name, source_id), buildJv());
      else
        for (auto &h : handlersCopy)
          target.queue.emplace_back(eastl::move(h), eastl::string{}, buildJv());
    }
    else
      for (Sqrat::Function &handler : handlersCopy)
        if (!handler.Execute(msgDst)) [[unlikely]]
        {
          SQFunctionInfo fi;
          sq_ext_getfuncinfo(handler.GetFunc(), &fi);
          String errMsg(0, "Subscriber %s call failed, target VM = %p", fi.name, vm_to);
          Sqrat::Object msgSrc;
          if (payload_idx != NO_PAYLOAD)
            msgSrc = Sqrat::Var<Sqrat::Object>(vm_from, payload_idx).value;
          return throw_send_error(vm_from, errMsg.c_str(), msgSrc);
        }
  } // end of vm iter

  if (thisVm != vms.end() && thisVm->second.processingMode == ProcessingMode::IMMEDIATE)
    process_events_impl(vm_from, thisVm->second);

  return 0; // null
}

void send_event_ex(const char *event_name, rapidjson::Document &&data, const EventParams &event_params)
{
  [[maybe_unused]] ScopeDebugInSendGuard inSendGuard;

  if (g_native_event_handler)
    g_native_event_handler(event_name, copy_rjson(data), event_params.sourceId, /*immediate*/ false);

  ScopedLockReadTemplate guard(vmsLock);

  int copyFromVmIdx = -1;
  int8_t isMainThreadCached = -1;
  for (auto &v : vms)
  {
    BoundVm &target = v.second;

    Tab<Sqrat::Function> handlersCopy(framemem_ptr());
    if (!collect_handlers(target, isMainThreadCached, event_name, event_params.sourceId, handlersCopy))
      continue;

    OSSpinlockScopedLock queueGuard(target.queueLock);

    auto pushQEvt = [&](Sqrat::Function &&f, eastl::string &&s, JsonVariant &&jv) {
      auto &qevt = target.queue.emplace_back(eastl::move(f), eastl::move(s), eastl::move(jv));
      qevt.memoryThresholdBytes = event_params.memoryThresholdBytes;
    };
    if (!handlersCopy.empty())
    {
      G_ASSERTF(is_main_thread(), "evt=%s vmi=%d", event_name, &v - vms.data()); // Guaranteed by `collect_handlers` impl.
      Sqrat::Object sqjs = rapidjson_to_quirrel(v.first, data);
      if (auto &hf = handlersCopy.front(); handlersCopy.size() == 1) // Fast move code path
        pushQEvt(eastl::move(hf), eastl::string{}, JsonVariant(eastl::in_place<decltype(sqjs)>, eastl::move(sqjs)));
      else
        for (auto &h : handlersCopy)
          pushQEvt(eastl::move(h), eastl::string{}, JsonVariant(eastl::in_place<decltype(sqjs)>, sqjs));
    }
    else // non main thread
    {
      eastl::string evSrc = QueuedEvent::buildEventSource(event_name, event_params.sourceId);
      if (copyFromVmIdx < 0) [[likely]] // Move on first call
      {
        copyFromVmIdx = (int(&v - vms.data()) << 24) | int(target.queue.size());
        JsonVariant jv{eastl::in_place<rapidjson::Document>, eastl::move(data)};
        pushQEvt(Sqrat::Function{}, eastl::move(evSrc), eastl::move(jv));
      }
      else // copy from previously moved on 2nd and further calls (should be rare code path)
      {
        int vmi = copyFromVmIdx >> 24;
        OSSpinlockScopedLock maybeLock2((vmi != (&v - vms.data())) ? &vms[vmi].second.queueLock : nullptr);
        const JsonVariant &otherJv = vms[vmi].second.queue[copyFromVmIdx & (~0u >> 8)].value;
        G_ASSERT(eastl::holds_alternative<rapidjson::Document>(otherJv));
        JsonVariant jv(eastl::in_place<rapidjson::Document>);
        auto &rd = eastl::get<rapidjson::Document>(jv);
        rd.CopyFrom(eastl::get<rapidjson::Document>(otherJv), rd.GetAllocator());
        pushQEvt(Sqrat::Function{}, eastl::move(evSrc), eastl::move(jv));
      }
    }
  }
}

static void send_event_jsoncpp(const char *event_name, Json::Value &data, const char *source_id, bool can_be_moved)
{
  [[maybe_unused]] ScopeDebugInSendGuard inSendGuard;

  if (g_native_event_handler)
    g_native_event_handler(event_name, data, source_id, /*immediate*/ false);

  BoundVm *copyFromVm = nullptr;
  int copyFromQidx = -1;
  int8_t isMainThreadCached = -1;
  ScopedLockReadTemplate guard(vmsLock);
  for (auto &v : vms)
  {
    BoundVm &target = v.second;

    Tab<Sqrat::Function> handlersCopy(framemem_ptr());
    if (!collect_handlers(target, isMainThreadCached, event_name, source_id, handlersCopy))
      continue;

    JsonVariant jv = [&]() -> JsonVariant {
      if (!can_be_moved || (data.isNull() && !copyFromVm)) // legacy slow copying path (or no data)
        return data;
      else if (!copyFromVm) // fast move path
      {
        copyFromVm = &target;
        OSSpinlockScopedLock queueGuard(target.queueLock);
        copyFromQidx = target.queue.size();
        return eastl::move(data);
      }
      else if (copyFromVm == &target) // copy from same vm
      {
        OSSpinlockScopedLock queueGuard(target.queueLock);
        char *base = (char *)&(target.queue.data() + target.queue.size())->value;
        char *copy = (char *)&target.queue[copyFromQidx].value;
        return intptr_t(copy - base); // Store relative offset to copy source
      }
      else // copy from different vm
      {
        G_ASSERT(copyFromVm && copyFromQidx >= 0);
        OSSpinlockScopedLock queueGuard(copyFromVm->queueLock);
        const JsonVariant &otherJv = copyFromVm->queue[copyFromQidx].value;
        G_ASSERT(eastl::holds_alternative<Json::Value>(otherJv));
        return JsonVariant{eastl::get<Json::Value>(otherJv)};
      }
    }();
    OSSpinlockScopedLock queueGuard(target.queueLock);
    if (handlersCopy.empty())
      target.queue.emplace_back(Sqrat::Function{}, QueuedEvent::buildEventSource(event_name, source_id), eastl::move(jv));
    else
      for (auto &h : handlersCopy)
      {
        if (&h == &handlersCopy.back())
          target.queue.emplace_back(eastl::move(h), eastl::string{}, eastl::move(jv));
        else
          target.queue.emplace_back(eastl::move(h), eastl::string{}, JsonVariant{jv});
      }
  }
}

void send_event(const char *event_name, const Json::Value &data, const char *source_id)
{
  send_event_jsoncpp(event_name, const_cast<Json::Value &>(data), source_id, /*can_be_moved*/ false);
}

void send_event(const char *event_name, Json::Value &&data, const char *source_id)
{
  send_event_jsoncpp(event_name, data, source_id, /*can_be_moved*/ true);
}

void send_event(const char *event_name, const char *source_id) { send_event(event_name, Json::Value(), source_id); }

void write_event_main_thread(const char *event_name, const eastl::fixed_function<sizeof(void *) * 8, void(Value &)> &wcb,
  const char *source_id)
{
  G_ASSERTF(is_main_thread(), "This function is for use in main thread only. Use write_event/send_event if threading desired");

  ScopedLockReadTemplate guard(vmsLock);
  bool nHandled = g_native_event_handler == nullptr;
  int8_t isMainThreadCached = 1; // 1 (i.e. not -1) since this fn is guaranteed to be called in main thread
  for (auto &v : vms)
  {
    BoundVm &target = v.second;

    Tab<Sqrat::Function> handlersCopy(framemem_ptr());
    if (!collect_handlers(target, isMainThreadCached, event_name, source_id, handlersCopy))
      continue;

    Value data(v.first); // One shared table per VM
    wcb(data);
    if (target.freezeWEvtTables)
      data.obj.FreezeSelf();

    if (!eastl::exchange(nHandled, true))
      g_native_event_handler(event_name, quirrel_to_jsoncpp(data.obj), source_id, /*immediate*/ false);

    OSSpinlockScopedLock queueGuard(target.queueLock);
    for (auto &h : handlersCopy)
    {
      if (&h == &handlersCopy.back())
        target.queue.emplace_back(eastl::move(h), eastl::string{}, eastl::move(data.obj));
      else
        target.queue.emplace_back(eastl::move(h), eastl::string{}, data.obj);
    }
  }
}


bool has_listeners(const char *event_name, const char *source_id)
{
  ScopedLockReadTemplate guard(vmsLock);
  for (auto &v : vms)
    if (query_handlers(v.second, event_name, source_id))
      return true;
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
  static constexpr int QCUR_CAP = 16;
  dag::RelocatableFixedVector<QueuedEvent, QCUR_CAP, true, framemem_allocator> queueCurrent;
  {
    OSSpinlockScopedLock queueGuard(vmData.queueLock);
    if (vmData.queue.empty())
      return;
    if (vmData.queue.size() > QCUR_CAP) [[unlikely]]
      queueCurrent.reserve(vmData.queue.size());
    for (auto &e : vmData.queue)
      queueCurrent.push_back(eastl::move(e));
    vmData.queue.clear();
  }

  SqStackChecker stackCheck(vm);

  // Remove the hook temporarily to avoid recursion
  auto hook = (vmData.processingMode == ProcessingMode::IMMEDIATE) ? sq_set_sq_call_hook(vm, nullptr) : nullptr;

  Tab<Sqrat::Function> handlersCopy(framemem_ptr());
  for (QueuedEvent &qevt : queueCurrent)
  {
    if (qevt.handler.IsNull()) [[unlikely]]
    {
      handlersCopy.clear();
      if (!collect_handlers(vmData, eastl::move(qevt.eventSource), handlersCopy))
        continue;
    }
    else
      G_ASSERT(qevt.eventSource.empty()); // Either non null handler or not empty eventSource, but not both

    Sqrat::Object valueObject = jsonv_to_quirrel(vm, qevt.value, qevt.memoryThresholdBytes);
    if (vmData.freezeSqTables)
      valueObject.FreezeSelf();

    auto execHandlers = [](const auto &handlers, const Sqrat::Object &o) {
      for (const Sqrat::Function &handler : handlers)
        if (!handler.Execute(o)) [[unlikely]]
        {
          SQFunctionInfo fi;
          sq_ext_getfuncinfo(handler.GetFunc(), &fi);
          logerr("Subscriber %s call failed, target VM = %p", fi.name, handler.GetVM());
        }
    };
    if (!qevt.handler.IsNull())
      execHandlers(make_span_const(&qevt.handler, 1), valueObject);
    else
      execHandlers(handlersCopy, valueObject);
  }

  if (hook)
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
  auto itv = vms.find(vm);
  G_ASSERT_RETURN(itv != vms.end(), 0); // bind isn't called?
  auto &boundVm = itv->second;

  ScopedLockWriteTemplate handlerGuard(boundVm.handlersLock);
  auto &handlers = boundVm.handlers;

  uint32_t evtNameHash = str_hash_fnv1a(evtName);
  auto itLb = handlers.lower_bound(evtNameHash);
  for (auto it = itLb; it != handlers.end() && it->first == evtNameHash; ++it)
  {
#if DAGOR_DBGLEVEL > 0 && _DEBUG_TAB_
    G_ASSERTF(it->second.debugEventName == evtName, "fnv1a-32 hash collision: hash('%s') == fvn1a('%s') == %#x",
      it->second.debugEventName.c_str(), evtName, evtNameHash);
#endif
    if (it->second.func.IsEqual(f))
    {
      sq_pushbool(vm, false);
      return 1;
    }
  }

  auto resIt = handlers.insert(itLb, eastl::make_pair(evtNameHash, EvHandler(eastl::move(f), onehit)));
  G_ASSERT(resIt != handlers.end());
  EvHandler &eh = resIt->second;
#if DAGOR_DBGLEVEL > 0 && _DEBUG_TAB_
  eh.debugEventName = evtName;
#endif

  // setup filters
  if (sq_gettop(vm) < 4)
    clear_and_shrink(eh.allowedSources); // listen all
  else
  {
    SQObjectType fltType = sq_gettype(vm, 4);
    if (fltType == OT_STRING)
    {
      const char *s = nullptr;
      sq_getstring(vm, 4, &s);
      if (strcmp(s, "*") == 0)
        clear_and_shrink(eh.allowedSources);
      else
        eh.allowedSources.emplace(str_hash_fnv1a(s));
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
          eh.allowedSources.emplace(str_hash_fnv1a(item.GetVar<const char *>().value));
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
  auto &handlers = itVm->second.handlers;
  auto itRange = handlers.equal_range(str_hash_fnv1a(eventName));

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
  if (itVm == vms.end() && vms.find(vm->_sharedstate->_root_vm._unVal.pThread) != vms.end())
    return; // ignore coroutines

  G_ASSERTF_RETURN(itVm != vms.end(), , "VM is not bound");

  BoundVm &vmData = itVm->second;
  process_events_impl(vm, vmData);
}

void flush_events()
{
  ScopedLockReadTemplate guard(vmsLock);
  for (auto &vm : vms)
    process_events_impl(vm.first, vm.second);
}

void bind(SqModules *module_mgr, const char *vm_id, ProcessingMode mode, bool freeze_wevt_tables, bool freeze_sq_tables)
{
  HSQUIRRELVM vm = module_mgr->getVM();

  ScopedLockWriteTemplate guard(vmsLock);
  auto ins = vms.insert(vm);
  BoundVm &vmData = ins.first->second;
  vmData.processingMode = mode;
  vmData.freezeWEvtTables = freeze_wevt_tables;
  vmData.freezeSqTables = freeze_sq_tables;

  if (mode == ProcessingMode::IMMEDIATE)
    G_VERIFY(sq_set_sq_call_hook(vm, sq_call_hook) == nullptr);

  sq_pushstring(vm, vm_id, -1);
  Sqrat::Object sqVmId = Sqrat::Var<Sqrat::Object>(vm, -1).value;
  sq_pop(vm, 1);

  Sqrat::Table api(vm);

  api
    .SquirrelFuncDeclString(subscribe, "subscribe(event: string, callback: function, [filter: array|string]): bool") // alias for
                                                                                                                     // backward
                                                                                                                     // compatibility
    .SquirrelFuncDeclString(subscribe_onehit, "subscribe_onehit(event: string, callback: function, [filter: array|string]): bool")
    .SquirrelFuncDeclString(unsubscribe, "unsubscribe(event: string, callback: function): bool")
    .SquirrelFuncDeclString(send, "send(event: string, [payload: any]): null", nullptr, 1, &sqVmId)
    .SquirrelFuncDeclString(send_foreign, "send_foreign(event: string, [payload: any]): null", nullptr, 1, &sqVmId)
    .SquirrelFuncDeclString(has_listeners, "has_listeners(event: string): bool", nullptr, 1, &sqVmId)
    .SquirrelFuncDeclString(has_foreign_listeners, "has_foreign_listeners(event: string): bool", nullptr, 1, &sqVmId)
    ///@module eventbus
    .SquirrelFuncDeclString(subscribe, "eventbus_subscribe(event: string, callback: function, [filter: array|string]): bool")
    .SquirrelFuncDeclString(subscribe_onehit,
      "eventbus_subscribe_onehit(event: string, callback: function, [filter: array|string]): bool")
    .SquirrelFuncDeclString(unsubscribe, "eventbus_unsubscribe(event: string, callback: function): bool")
    .SquirrelFuncDeclString(send, "eventbus_send(event: string, [payload: any]): null", nullptr, 1, &sqVmId)
    .SquirrelFuncDeclString(send_foreign, "eventbus_send_foreign(event: string, [payload: any]): null", nullptr, 1, &sqVmId)
    .SquirrelFuncDeclString(has_listeners, "eventbus_has_listeners(event: string): bool", nullptr, 1, &sqVmId)
    .SquirrelFuncDeclString(has_foreign_listeners, "eventbus_has_foreign_listeners(event: string): bool", nullptr, 1, &sqVmId)
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

void set_native_event_handler(NativeEventHandler handler) { g_native_event_handler = handler; }

} // namespace sqeventbus
