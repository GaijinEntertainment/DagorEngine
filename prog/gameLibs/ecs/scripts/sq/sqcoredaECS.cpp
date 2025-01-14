// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/scripts/scripts.h>
#include <generic/dag_tab.h>
#include <util/dag_string.h>
#include <daECS/core/coreEvents.h>
#include <ecs/core/entityManager.h>
#include <daECS/core/template.h>
#include <daECS/core/internal/eventsDb.h>
#include <ecs/core/attributeEx.h>
#include <math/dag_e3dColor.h>
#include <ecs/scripts/sqBindEvent.h>
#include "squirrelEvent.h"
#include <ecs/scripts/sqEntity.h>
#include <EASTL/vector.h>
#include <EASTL/algorithm.h>
#include <EASTL/vector_map.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/internal/fixed_pool.h> // aligned_buffer
#include <EASTL/fixed_string.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <util/dag_hash.h>
#include <sqrat.h>
#include <perfMon/dag_statDrv.h>
#include <bindQuirrelEx/bindQuirrelEx.h>
#include <sqCrossCall/sqCrossCall.h>
#include <sqModules/sqModules.h>
#include <util/dag_string.h>
#include <memory/dag_framemem.h>
#include <ecs/scripts/sqAttrMap.h>
#include <daECS/core/internal/performQuery.h>
#include <daECS/core/sharedComponent.h>
#include <daECS/scene/scene.h>
#include <osApiWrappers/dag_critSec.h>
#include <util/dag_strUtil.h>
#include "queryExpression.h"
#include "timers.h"

EA_DISABLE_VC_WARNING(4505) // unreferenced local function has been removed

#define ON_UPDATE_EVENT_NAME "onUpdate"

ECS_REGISTER_EVENT(EventScriptBeforeReload);
ECS_REGISTER_EVENT(EventScriptReloaded);
ECS_REGISTER_EVENT(CmdRequireUIScripts);

static eastl::vector_map<ecs::component_type_t, SqPushCB> native_component_bindings;
void sq_register_native_component(ecs::component_type_t type, SqPushCB cb)
{
  native_component_bindings.emplace(type, cb).first->second = cb;
}

static eastl::vector_map<HSQUIRRELVM, WinCritSec *> vm_critical_sections;

void sqvm_enter_critical_section(HSQUIRRELVM vm, WinCritSec *sect)
{
  G_ASSERT(vm_critical_sections.find(vm) == vm_critical_sections.end());
  vm_critical_sections.emplace(vm, sect).first->second = sect;
}

void sqvm_leave_critical_section(HSQUIRRELVM vm)
{
  G_ASSERT(vm_critical_sections.find(vm) != vm_critical_sections.end());
  vm_critical_sections.erase(vm);
}

struct ScopedLockVM
{
  WinCritSec *critSect = nullptr;

  ScopedLockVM(HSQUIRRELVM vm)
  {
    auto sectIt = vm_critical_sections.find(vm);
    if (sectIt != vm_critical_sections.end())
      critSect = sectIt->second;

    if (critSect && !critSect->tryLock())
    {
#if DAGOR_DBGLEVEL > 0
      logerr("Script VM is running in a separate thread! Lock the main thread!");
#else
      LOGERR_ONCE("Script VM is running in a separate thread! Lock the main thread!");
#endif
      critSect->lock();
    }
  }

  ~ScopedLockVM()
  {
    if (critSect)
      critSect->unlock();
  }
};

namespace eastl
{

template <>
struct hash<Sqrat::Function>
{
  size_t operator()(const Sqrat::Function &f) const { return eastl::hash<SQInstance *>()(f.GetFunc()._unVal.pInstance); }
};

template <>
struct equal_to<tagSQObject>
{
  bool operator()(const tagSQObject &a, const tagSQObject &b) const
  {
    return a._type == b._type && a._unVal.pUserPointer == b._unVal.pUserPointer;
  }
};

template <>
struct equal_to<Sqrat::Function>
{
  bool operator()(const Sqrat::Function &a, const Sqrat::Function &b) const
  {
    return eastl::equal_to<tagSQObject>()(a.GetFunc(), b.GetFunc()) && eastl::equal_to<tagSQObject>()(a.GetEnv(), b.GetEnv());
  }
};

} // namespace eastl


static ska::flat_hash_map<ecs::event_type_t, ecs::sq::push_instance_fn_t, ska::power_of_two_std_hash<ecs::event_type_t>>
  eventTypeNative2Sq;
static ska::flat_hash_set<ecs::event_type_t, ska::power_of_two_std_hash<ecs::event_type_t>> squirrelEvents;

namespace ecs
{
namespace sq
{
void register_native_event_impl(ecs::event_type_t evtt, push_instance_fn_t pushf) { eventTypeNative2Sq.emplace(evtt, pushf); }
} // namespace sq
} // namespace ecs

static inline ecs::sq::push_instance_fn_t get_event_script_push_inst_fn(ecs::event_type_t evt)
{
  if (squirrelEvents.count(evt))
    return (ecs::sq::push_instance_fn_t)&Sqrat::ClassType<ecs::SchemelessEvent>::PushNativeInstance;
  auto it = eventTypeNative2Sq.find(evt);
  return (it != eventTypeNative2Sq.end()) ? it->second : nullptr;
}

namespace ecs
{

bool ecs_is_in_init_phase = true;

} // namespace ecs

namespace Sqrat
{

void PushVar(HSQUIRRELVM vm, ecs::Event *value)
{
  if (ecs::sq::push_instance_fn_t pushfn = get_event_script_push_inst_fn(value->getType()))
    pushfn(vm, value);
  else
  {
    G_ASSERTF(0, "No script class for event type 0x%X | %s", value->getType(), value->getName());
    sq_pushnull(vm);
  }
}

} // namespace Sqrat

using namespace ecs;

namespace ecs
{

enum CompAccess
{
  COMP_ACCESS_READONLY,
  COMP_ACCESS_READWRITE,
  COMP_ACCESS_INSPECT
};
static void push_comp_val(HSQUIRRELVM vm, const char *comp_name, EntityComponentRef comp, CompAccess access);
static bool pop_component_val(HSQUIRRELVM vm, SQInteger idx, EntityComponentRef &comp, const char *comp_name, String &err_msg);
static bool pop_comp_val(HSQUIRRELVM vm, SQInteger idx, ChildComponent &comp, const char *comp_name, ecs::component_type_t type,
  String &err_msg);

template <typename ObjClass, typename ArrClass, bool readonly>
static SQInteger comp_obj_get_all(HSQUIRRELVM vm, const ecs::Object &obj);

template <typename ObjClass, typename ArrClass, bool readonly>
static SQInteger comp_arr_get_all(HSQUIRRELVM vm, const ecs::Array &arr);

template <typename ArrClass, bool readonly>
static SQInteger comp_list_get_all(HSQUIRRELVM vm, const ArrClass &arr);

class ObjectRO : public Object
{};
class ArrayRO : public Array
{};
#define DECL_LIST_TYPE(lt, t) \
  class lt##RO : public lt    \
  {};
ECS_DECL_LIST_TYPES
#undef DECL_LIST_TYPE

struct VmExtraData
{
  Sqrat::Table compTbl;
  dag::Vector<Sqrat::Object> pendingCreationCbs;
  uint16_t createSystems : 1;
  uint16_t createFactories : 1;
  uint16_t id : 14;
  static inline uint16_t nextId = 0;
  uint16_t numPendingCreationCbs = 0;
  VmExtraData()
  {
    createSystems = createFactories = 1;
    id = 0;
  }

  struct PendingCreationCbRef
  {
    uint16_t id;
    int vmedSlotId = -1;
    int cbSlotId = -1;
    PendingCreationCbRef(uint16_t id_, int vi, int ci) : id(id_), vmedSlotId(vi), cbSlotId(ci) {}
    PendingCreationCbRef(PendingCreationCbRef &&other)
    {
      memcpy(this, &other, sizeof(other));
      other.vmedSlotId = -1;
    }
    PendingCreationCbRef(const PendingCreationCbRef &) { G_FAST_ASSERT(0); } //-V730 Shall not be called
    ~PendingCreationCbRef();
    eastl::pair<HSQUIRRELVM, VmExtraData> *isValid() const;
    void invoke(ecs::EntityId) const;
  };

  PendingCreationCbRef addPendingCreationCb(uint16_t id_, int vm_slot_id, Sqrat::Object &&cb)
  {
    int i = pendingCreationCbs.size();
    pendingCreationCbs.emplace_back(eastl::move(cb));
    numPendingCreationCbs++;
    return PendingCreationCbRef{id_, vm_slot_id, i};
  }
};
static eastl::vector_map<HSQUIRRELVM, VmExtraData, eastl::less<HSQUIRRELVM>, EASTLAllocatorType,
  eastl::fixed_vector<eastl::pair<HSQUIRRELVM, VmExtraData>, 2>>
  vm_extra_data;

eastl::pair<HSQUIRRELVM, VmExtraData> *VmExtraData::PendingCreationCbRef::isValid() const
{
  auto &vmed = static_cast<decltype(vm_extra_data)::base_type &>(vm_extra_data);
  if (vmedSlotId < vmed.size() && vmed[vmedSlotId].second.id == id && cbSlotId < vmed[vmedSlotId].second.pendingCreationCbs.size())
    return &vmed[vmedSlotId];
  return nullptr;
}

void VmExtraData::PendingCreationCbRef::invoke(ecs::EntityId ceid) const
{
  if (auto vmed = isValid())
  {
    G_ASSERT(vmed->first == vmed->second.pendingCreationCbs[cbSlotId].GetVM());
    Sqrat::Function cb(vmed->first, Sqrat::Object(vmed->first), vmed->second.pendingCreationCbs[cbSlotId]);
    cb(ceid);
  }
}

VmExtraData::PendingCreationCbRef::~PendingCreationCbRef()
{
  if (auto vmed = isValid())
  {
    G_ASSERT(vmed->first == vmed->second.pendingCreationCbs[cbSlotId].GetVM());
    if (--vmed->second.numPendingCreationCbs == 0)
      clear_and_shrink(vmed->second.pendingCreationCbs);
    else
      vmed->second.pendingCreationCbs[cbSlotId].Release();
  }
}

static inline bool is_vm_registered(HSQUIRRELVM vm, const char *errmsg, eastl::pair<HSQUIRRELVM, VmExtraData> **pit = nullptr)
{
  auto it = vm_extra_data.find(vm);
  if (DAGOR_LIKELY(it != vm_extra_data.end()))
  {
    if (pit)
      *pit = &*it;
    return true;
  }
  else
  {
    sq_throwerror(vm, errmsg);
    return false;
  }
}

static constexpr int COMPS_TABLE_MAX_NODES_COUNT = 16;

struct ScriptCompDesc
{
  Sqrat::Object key;
  Sqrat::Object defVal;
  bool script = false, native = false;
  bool isScriptComponent() const { return script; }
  bool isNativeComponent() const { return native; }

  const char *getCompName() const { return !key.IsNull() ? sq_objtostring(&const_cast<Sqrat::Object &>(key).GetObject()) : ""; }
};

struct CompListTypeInfo
{
  ecs::component_type_t type = 0;
  ecs::type_index_t typeId = ecs::INVALID_COMPONENT_TYPE_INDEX;
  uint16_t size = 0;
};
struct CompListDescHolder
{
  Tab<ecs::ComponentDesc> esCompsRw, esCompsRo, esCompsRq, esCompsNo;
  Tab<ScriptCompDesc> componentsRw, componentsRo, componentsRq, componentsNo;
  Tab<CompListTypeInfo> componentTypeInfo;
  ecs::component_index_t lastComponentsCount = 0;
  bool componentTypeInfoResolved = false;
  CompListDescHolder(IMemAlloc *m = tmpmem_ptr()) :
    esCompsRw(m),
    esCompsRo(m),
    esCompsRq(m),
    esCompsNo(m),
    componentsRw(m),
    componentsRo(m),
    componentsRq(m),
    componentsNo(m),
    componentTypeInfo(m)
  {}
};


struct ScriptEsData : CompListDescHolder
{
  mutable Sqrat::Object esName; // can't be null
  Sqrat::Function onUpdate;
  ska::flat_hash_map<ecs::event_type_t, Sqrat::Function, ska::power_of_two_std_hash<ecs::event_type_t>> onEvent;
  mutable Sqrat::Object tags, tracked, before, after;

  float updateInterval = 0;
  float updateTimeout = 0;
  float dtError = -1.f; // negative to avoid calling update until first timeout expires
  uint32_t maxInterval = 0, minInterval = 0;
  uint32_t tick = 0;
#if TIME_PROFILER_ENABLED
  mutable uint32_t dapToken = 0;
  uint32_t getDapToken() const
  {
    if (DAGOR_UNLIKELY(!dapToken))
      dapToken = ::da_profiler::add_copy_description(nullptr, /*line*/ 0, /*flags*/ 0, getESName()); // todo: add module name?
    return dapToken;
  }
#else
  uint32_t getDapToken() const { return 0; }
#endif

  const char *getESName() const { return sq_objtostring(&esName.GetObject()); }
#if DAECS_EXTENSIVE_CHECKS
  const char *debugGetESName() const { return getESName(); }
#else
  constexpr const char *debugGetESName() const { return nullptr; }
#endif
  ScriptEsData(Sqrat::Object &&es_name) : esName(eastl::move(es_name)) {}

  HSQUIRRELVM getVM() const { return esName.GetVM(); }
  const char *getTags() const { return sq_objtostring(&tags.GetObject()); }
  const char *getTracked() const { return sq_objtostring(&tracked.GetObject()); }
  const char *getBefore() const { return sq_objtostring(&before.GetObject()); }
  const char *getAfter() const { return sq_objtostring(&after.GetObject()); }

  void resetTimer() { dtError = -1.f; }

  bool isFilterSegmentEmpty() const { return maxInterval == minInterval; }
  bool isTimerNotExpired() const { return dtError < 0.f; }
  bool isNoFrameLag() const { return dtError < updateInterval; }

  float getTimeDelta() const { return isNoFrameLag() ? (updateInterval + dtError) : dtError; }
};

#define TIME_PROFILE_SQ_ES(LabelName, esd) DA_PROFILE_EVENT_DESC(esd->getDapToken());

static inline void dump_sq_callstack_in_log(HSQUIRRELVM vm)
{
  if (SQ_SUCCEEDED(sqstd_formatcallstackstring(vm)))
  {
    const char *cs = nullptr;
    G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, -1, &cs)));
    G_ASSERT(cs);
    debug("%s", cs);
    sq_pop(vm, 1);
  }
}

static void dump_sq_table_in_log(const Sqrat::Table &tbl) // cur impl assumes that keys are strings
{
  char tmp[128];
  static const char dot3[] = "...";
  Sqrat::Object::iterator it;
  while (tbl.Next(it))
  {
    HSQOBJECT k = it.getKey();
    G_UNUSED(k);
    HSQOBJECT v = it.getValue();
    G_UNUSED(v);
    switch (sq_type(v))
    {
      case OT_INTEGER: snprintf(tmp, sizeof(tmp), "%" PRId64, v._unVal.nInteger); break;
      case OT_FLOAT: snprintf(tmp, sizeof(tmp), "%f", v._unVal.fFloat); break;
      case OT_STRING:
        if ((size_t)snprintf(tmp, sizeof(tmp), "%s", sq_objtostring(&v)) >= sizeof(tmp))
          memcpy(tmp + sizeof(tmp) - sizeof(dot3), dot3, sizeof(dot3));
        break;
      case OT_BOOL: snprintf(tmp, sizeof(tmp), "%s", v._unVal.nInteger ? "true" : "false"); break;
      default: snprintf(tmp, sizeof(tmp), "%p", v._unVal.pTable); break;
    }
    debug("\t%s -> %s (sqt %#x)", sq_objtostring(&k), tmp, sq_type(v));
  }
}

class SqBindingHelper
{
public:
  template <bool readonly>
  static void put_components_to_table(const QueryView &qv, uint32_t idInChunk, uint32_t start, uint32_t count,
    Tab<ScriptCompDesc> &script_desc, dag::ConstSpan<CompListTypeInfo> type_info, Sqrat::Table &comp_tbl)
  {
    G_ASSERT(count == script_desc.size());
    HSQUIRRELVM vm = comp_tbl.GetVM();

    for (uint32_t ic = 0; ic < count; ++ic)
    {
      auto &desc = script_desc[ic];
      uint8_t *__restrict untypedData = (uint8_t *__restrict)qv.getComponentUntypedData(ic + start);
      if (!untypedData)
      {
        comp_tbl.SetValue(desc.key, desc.defVal);
        continue;
      }
      const CompListTypeInfo typeInfo = type_info[ic + start];
      untypedData += idInChunk * typeInfo.size;
      if (desc.isNativeComponent()) // || typeType == OT_NULL for generics
      {
        int prevTop = sq_gettop(vm);
        (void)prevTop;
        sq_pushobject(vm, comp_tbl.GetObject());
        sq_pushobject(vm, desc.key.GetObject());
        push_comp_val(vm, sq_objtostring(&desc.key.GetObject()),
          ecs::EntityComponentRef(untypedData, typeInfo.type, typeInfo.typeId, ecs::INVALID_COMPONENT_INDEX),
          readonly ? COMP_ACCESS_READONLY : COMP_ACCESS_READWRITE);
        G_VERIFY(SQ_SUCCEEDED(sq_rawset(vm, -3)));
        sq_pop(vm, 1); // compTbl
      }
      else if (desc.isScriptComponent())
      {
        // store scripted component
        Sqrat::Object *o = (Sqrat::Object *)untypedData;
        comp_tbl.SetValue(desc.key, *o);
      }
      else
      {
        G_ASSERTF(0, "Unexpected component type field type, nor native, neither script");
      }
    }
  }

  static void apply_rw_components_from_table(const QueryView &qv, uint32_t idInChunk, uint32_t start, uint32_t count,
    Tab<ScriptCompDesc> &script_desc, dag::ConstSpan<CompListTypeInfo> type_info, Sqrat::Table &comp_tbl)
  {
    G_ASSERT(count == script_desc.size());
    if (!count)
      return;
    HSQUIRRELVM vm = comp_tbl.GetVM();

    String errMsg(framemem_ptr());
    for (uint32_t ic = 0; ic < count; ++ic)
    {
      auto &desc = script_desc[ic];
      if (!desc.isNativeComponent()) // we can't apply anything except predefined types
        continue;
      uint8_t *__restrict untypedData = (uint8_t *__restrict)qv.getComponentUntypedData(ic + start);
      if (!untypedData)
        continue;
      const CompListTypeInfo typeInfo = type_info[ic + start];
      untypedData += idInChunk * typeInfo.size;

      sq_pushobject(vm, comp_tbl.GetObject());
      sq_pushobject(vm, desc.key.GetObject());
      if (SQ_FAILED(sq_rawget_noerr(vm, -2)))
      {
        sq_pop(vm, 1);
      }
      else
      {
        SQObjectType typeType = sq_gettype(vm, -1);
        if (typeType != OT_INSTANCE ||
            (typeInfo.type != ecs::ComponentTypeInfo<ecs::Object>::type && typeInfo.type != ecs::ComponentTypeInfo<ecs::Array>::type))
        {
          ecs::EntityComponentRef ref(untypedData, typeInfo.type, typeInfo.typeId, ecs::INVALID_COMPONENT_INDEX);
          if (!pop_component_val(vm, -1, ref, sq_objtostring(&desc.key.GetObject()), errMsg))
          {
            logerr("internal error on component <%s> apply: %s", desc.getCompName(), errMsg.str());
            dump_sq_callstack_in_log(vm);
          }
        }
        sq_pop(vm, 2);
      }
    }
  }

  static void sq_es_on_event(const Event &evt, const QueryView &__restrict components_in);
  static void sq_es_on_update(const UpdateStageInfo &info, const QueryView &__restrict components);
  static void sq_es_on_update_empty(const UpdateStageInfo &info, const QueryView &__restrict components);

  template <typename Array>
  static ecs::EntityComponentRef get_entity_component_ref(const Array &arr, size_t i)
  {
    using ItemType = typename Array::value_type;

    void *rawData = const_cast<void *>((void *)(&arr[i]));
    return EntityComponentRef(rawData, ecs::ComponentTypeInfo<ItemType>::type, arr.getItemTypeId(), INVALID_COMPONENT_INDEX);
  }
};

template <>
ecs::EntityComponentRef SqBindingHelper::get_entity_component_ref(const ecs::Array &arr, size_t i)
{
  return arr[i].getEntityComponentRef();
}

template <>
ecs::EntityComponentRef SqBindingHelper::get_entity_component_ref(const ecs::ArrayRO &arr, size_t i)
{
  return arr[i].getEntityComponentRef();
}

template <typename Fun>
inline void call_event_func(const Event &evt, Fun fun)
{
  fun(const_cast<Event *>(&evt)); // static cast to avoid extra instance copy
}

static bool load_component_desc(const Sqrat::Array &comps, Tab<ScriptCompDesc> &out_script_comps,
  Tab<ecs::ComponentDesc> &out_ecs_comps, String &err_msg);
enum FilterResult
{
  FILTER_SKIP,
  FILTER_PROCESS
};
enum CallbackResult
{
  CB_CONTINUE,
  CB_STOP
};
struct NoFilter
{
  constexpr FilterResult operator()(ecs::EntityId, int) const { return FILTER_PROCESS; }
};

#if DAECS_EXTENSIVE_CHECKS
class CompTableSizeGuard
{
  const Sqrat::Table &compTbl;
  int oldCompCount;
  const char *esName;

public:
  CompTableSizeGuard(const Sqrat::Table &tbl, const char *es_name) : compTbl(tbl), oldCompCount((int)compTbl.Length()), esName(es_name)
  {}
  ~CompTableSizeGuard()
  {
    if (oldCompCount != compTbl.Length())
    {
      logerr("Comps table has been altered within ES/Query '%s': %d -> %d comps", esName, oldCompCount, compTbl.Length());
      dump_sq_table_in_log(compTbl);
      dump_sq_callstack_in_log(compTbl.GetVM());
    }
  }
};

template <typename... Ts>
struct CompInstancesGuardHelper;
template <>
struct CompInstancesGuardHelper<>
{
  static void collectNumInstances(...) {}
  static void compareNumInstances(...) {}
};
template <typename First, typename... Rest>
struct CompInstancesGuardHelper<First, Rest...>
{
  static inline void collectNumInstances(void **class_data, int *num_instances, HSQUIRRELVM vm, int i = 0)
  {
    Sqrat::ClassData<First> *cd = Sqrat::ClassType<First>::getClassData(vm);
    num_instances[i] = cd->instances->size();
    class_data[i] = cd;
    CompInstancesGuardHelper<Rest...>::collectNumInstances(class_data, num_instances, vm, i + 1);
  }
  static inline void compareNumInstances(const void *const *class_data, const int *prev_instances, const char *es_name,
    const ecs::Event *evt, int i = 0)
  {
    auto cd = (const Sqrat::ClassData<First> *)class_data[i];
    if (prev_instances[i] != cd->instances->size())
      logerr("Quirrel instances leak detected (%d->%d) of class '%s' while handling %s in ES '%s'.\n"
             "Perhaps you have captured components or components table. Consider adding .getAll() call.",
        prev_instances[i], cd->instances->size(), cd->staticData->className.c_str(), evt ? evt->getName() : "(update)", es_name);
    CompInstancesGuardHelper<Rest...>::compareNumInstances(class_data, prev_instances, es_name, evt, i + 1);
  }
};
template <typename First, typename... Rest>
class CompInstancesGuard
{
  const ecs::Event *evt;
  const char *esName;
  void *classData[1 + sizeof...(Rest)];
  int numInstances[1 + sizeof...(Rest)];

public:
  CompInstancesGuard(HSQUIRRELVM vm, const char *es_name, const ecs::Event *ev = NULL) : esName(es_name), evt(ev)
  {
    CompInstancesGuardHelper<First, Rest...>::collectNumInstances(classData, numInstances, vm);
  }
  ~CompInstancesGuard() { CompInstancesGuardHelper<First, Rest...>::compareNumInstances(classData, numInstances, esName, evt); }
};

#define COMP_TABLE_SIZE_GUARD(g, ...) CompTableSizeGuard g(__VA_ARGS__)
#define COMP_INSTANCES_GUARD(g, ...)                                                                                              \
  CompInstancesGuard<ecs::Object, ecs::ObjectRO, ecs::Array, ecs::ArrayRO, /* Intentionally only some list types to avoid slowing \
                                                                              down sq calls too much */                           \
    ecs::StringList, ecs::StringListRO, ecs::IntList, ecs::IntListRO, ecs::FloatList, ecs::FloatListRO>                           \
  g(__VA_ARGS__)
#else
#define COMP_TABLE_SIZE_GUARD(g, ...) ((void)0)
#define COMP_INSTANCES_GUARD(g, ...)  ((void)0)
#endif

static SQInteger get_hash_comp(const char *name)
{
  logerr("comp %s not found", name ? name : "<n/a>");
  return 0;
}

static inline Sqrat::Table comps_create_table(HSQUIRRELVM vm, int icap = 0)
{
  if (icap < 2)
    return Sqrat::Table(vm);
  sq_newtableex(vm, icap);
  Sqrat::Var<Sqrat::Object> tv(vm, -1);
  sq_poptop(vm);
  return tv.value;
}

static inline Sqrat::Table comps_create_table(HSQUIRRELVM vm, const ecs::QueryView &comps)
{
  return comps_create_table(vm, comps.getRwCount() + comps.getRoCount() - /*eid*/ 1);
}

template <typename F>
static void exec_with_cached_comps_table(HSQUIRRELVM vm, const QueryView &qv, F cb)
{
  auto &shCompTable = vm_extra_data[vm].compTbl;
  bool useCachedTbl = !shCompTable.IsNull() && int(qv.getRwCount() + qv.getRoCount() - /*eid*/ 1) <= COMPS_TABLE_MAX_NODES_COUNT;
  Sqrat::Table compTbl = useCachedTbl ? eastl::move(shCompTable) : comps_create_table(vm, qv);

  cb(compTbl);

  if (useCachedTbl)
  {
    sq_pushobject(vm, compTbl.GetObject());
    G_VERIFY(SQ_SUCCEEDED(sq_clear(vm, -1, /*freemem*/ SQFalse)));
    sq_poptop(vm);

    shCompTable = eastl::move(compTbl); // Return back
  }
}

template <typename T, typename Filter = NoFilter>
static void ecs_scripts_call(Sqrat::Table &compTbl, CompListDescHolder &compListDesc, const ecs::QueryView &components, T cb,
  const char *debug_es_name = nullptr, Filter filter_in = NoFilter())
{
  G_UNUSED(debug_es_name);
  if (!components.getEntitiesCount())
    return;
  G_ASSERTF(components.getRwCount() == compListDesc.componentsRw.size(), "RW components count mismatch %d vs %d",
    components.getRwCount(), compListDesc.componentsRw.size());
  G_ASSERTF(components.getRoCount() == compListDesc.componentsRo.size() + 1, "RO components count mismatch %d vs %d",
    components.getRoCount(), compListDesc.componentsRo.size());
  auto &dataComponents = components.manager().getDataComponents();
  if (!compListDesc.componentTypeInfoResolved && dataComponents.size() > compListDesc.lastComponentsCount)
  {
    compListDesc.lastComponentsCount = dataComponents.size();
    compListDesc.componentTypeInfoResolved = true;
    compListDesc.componentTypeInfo.resize(components.getComponentsCount());
    const ecs::component_index_t *cindices = components.manager().queryComponents(components.getQueryId());
    for (size_t ei = compListDesc.componentTypeInfo.size(), i = 0; i < ei; ++i, ++cindices)
    {
      ecs::component_index_t cidx = *cindices;
      ecs::DataComponent dt = dataComponents.getComponentById(cidx);
      const ecs::type_index_t typeId = dt.componentType;
      compListDesc.componentTypeInfo[i].type = dt.componentTypeName;
      compListDesc.componentTypeInfo[i].typeId = typeId;
      if (typeId != ecs::INVALID_COMPONENT_TYPE_INDEX)
      {
        const ecs::ComponentType typeInfo = g_entity_mgr->getComponentTypes().getTypeInfo(typeId);
        compListDesc.componentTypeInfo[i].size = typeInfo.size;
      }
      else
      {
        compListDesc.componentTypeInfo[i].size = 0;
        compListDesc.componentTypeInfoResolved = false;
      }
    }
  }

  const uint32_t eidCompId = components.getRoStart() + components.getRoCount() - 1;
  G_ASSERT(components.getRoCount() && compListDesc.componentTypeInfo[eidCompId].type == ecs::ComponentTypeInfo<ecs::EntityId>::type);
  for (uint32_t i = components.begin(), ei = components.end(); i < ei; ++i)
  {
    ecs::EntityId eid = ((const ecs::EntityId *)components.getComponentUntypedData(eidCompId))[i];
    FilterResult filterRet = filter_in(eid, i);
    if (filterRet == FILTER_SKIP)
      continue;

    SqBindingHelper::put_components_to_table<false>(components, i, components.getRwStart(), components.getRwCount(),
      compListDesc.componentsRw, compListDesc.componentTypeInfo, compTbl);
    SqBindingHelper::put_components_to_table<true>(components, i, components.getRoStart(), components.getRoCount() - 1,
      compListDesc.componentsRo, compListDesc.componentTypeInfo, compTbl);

    CallbackResult cbRes;

    {
      COMP_TABLE_SIZE_GUARD(cszg, compTbl, debug_es_name);
      cbRes = cb(eid, compTbl);
    }

    SqBindingHelper::apply_rw_components_from_table(components, i, components.getRwStart(), components.getRwCount(),
      compListDesc.componentsRw, compListDesc.componentTypeInfo, compTbl);

    if (cbRes == CB_STOP)
      break;
  }
}


void register_pending_es()
{
  if (ecs_is_in_init_phase && g_entity_mgr) // if we send events during inital load, it is important to register all pending es
    g_entity_mgr->resetEsOrder();
}

struct EventDataGetter : ecs::Event
{
  EventDataGetter(HSQUIRRELVM vm, const SchemelessEvent &e) : ecs::Event(e.getType(), e.getLength(), e.getFlags())
  {
    comp_obj_get_all<ecs::ObjectRO, ecs::ArrayRO, /*RO*/ true>(vm, e.getData());
    HSQOBJECT hData;
    sq_getstackobj(vm, -1, &hData);
    sqData = Sqrat::Object(hData, vm);
    sq_pop(vm, 1);
  }

  Sqrat::Object sqData;
};


void SqBindingHelper::sq_es_on_event(const Event &evt, const QueryView &__restrict components)
{
  ScriptEsData *esData = (ScriptEsData *)components.getUserData();
  G_ASSERT_RETURN(esData, );
  G_ASSERT(!esData->onEvent.empty());

  HSQUIRRELVM vm = esData->getVM();

  ScopedLockVM lock(vm);

  auto it = esData->onEvent.find(evt.getType());
  if (DAGOR_UNLIKELY(it == esData->onEvent.end()))
  {
    LOGERR_ONCE("No function for event type %#x in scripted ES <%s>", evt.getType(), esData->getESName());
    return;
  }

  COMP_INSTANCES_GUARD(cig, vm, esData->debugGetESName(), &evt);

  {
    TIME_PROFILE_SQ_ES(sq_es_on_event, esData);
    Sqrat::Function &onEventFunc = it->second;

    exec_with_cached_comps_table(vm, components, [&](Sqrat::Table &compTbl) {
      if (components.getComponentsCount())
      {
        ecs_scripts_call(
          compTbl, *esData, components,
          [&evt, &onEventFunc](ecs::EntityId eid, Sqrat::Table &compTbl) {
            call_event_func(evt, [eid, &compTbl, &onEventFunc](ecs::Event *evt) {
              if (evt->getFlags() & EVFLG_SCHEMELESS)
              {
                if (evt->getFlags() & EVFLG_DESTROY)
                {
                  EventDataGetter getter(compTbl.GetVM(), *(const SchemelessEvent *)evt);
                  onEventFunc(&getter, eid, compTbl);
                }
                else
                  onEventFunc((SchemelessEvent *)evt, eid, compTbl);
              }
              else
                onEventFunc(evt, eid, compTbl);
            });
            return CB_CONTINUE;
          },
          esData->debugGetESName());
      }
      else // empty ES
      {
        call_event_func(evt, [&onEventFunc, &compTbl, vm](ecs::Event *evt) {
          if (evt->getFlags() & EVFLG_SCHEMELESS)
          {
            if ((evt->getFlags() & EVFLG_DESTROY))
            {
              EventDataGetter getter(vm, *(const SchemelessEvent *)evt);
              onEventFunc(&getter, ecs::EntityId(), compTbl);
            }
            else
              onEventFunc((SchemelessEvent *)evt, ecs::EntityId(), compTbl);
          }
          else
            onEventFunc(evt, ecs::EntityId(), compTbl);
        });
      }
    });
  }
}

static int current_tick = 0;
void squirrel_update_tick_es(const ecs::UpdateStageInfo &, const QueryView &__restrict)
{
  current_tick++;
} // this is to be called once per act
static ecs::EntitySystemDesc squirrel_update_tick_es_desc("squirrel_update_tick_es", ecs::EntitySystemOps(squirrel_update_tick_es),
  empty_span(), empty_span(), empty_span(), empty_span(), ecs::EventSetBuilder<>::build(), (1 << ecs::UpdateStageInfoAct::STAGE));

static inline void update_es_timer(ScriptEsData *es_data, const float dt)
{
  if (es_data->tick == current_tick)
    return;

  const float newUpdateTimeout = es_data->updateTimeout - dt;
  es_data->maxInterval = USHRT_MAX * (es_data->updateTimeout / es_data->updateInterval);
  es_data->minInterval = USHRT_MAX * (max(newUpdateTimeout, 0.f) / es_data->updateInterval);
  if (newUpdateTimeout > 0)
    es_data->updateTimeout = newUpdateTimeout;
  else // wrap
  {
    es_data->updateTimeout = es_data->updateInterval;
    es_data->dtError = fabsf(newUpdateTimeout);
    if (es_data->dtError >= es_data->updateInterval) // overflow?
      es_data->dtError = dt;
  }
  es_data->tick = current_tick;
}

void SqBindingHelper::sq_es_on_update(const UpdateStageInfo &info, const QueryView &__restrict components)
{
  ScriptEsData *esData = (ScriptEsData *)components.getUserData();
  G_ASSERT_RETURN(esData, );
  G_ASSERT(!esData->onUpdate.IsNull());
  G_ASSERTF(components.getComponentsCount() != 0, "Script ES '%s' shall be non-empty", esData->getESName());
  G_ASSERTF(info.stage == US_ACT, "Script ES update stage = %d", info.stage);

  Sqrat::Function &onUpdateFunc = esData->onUpdate;
  HSQUIRRELVM vm = onUpdateFunc.GetVM();
  ScopedLockVM lock(vm);

  const UpdateStageInfoAct &actInfo = *info.cast<ecs::UpdateStageInfoAct>();
  COMP_INSTANCES_GUARD(cig, vm, esData->debugGetESName());
  {
    update_es_timer(esData, actInfo.dt);

    if (esData->isFilterSegmentEmpty() || esData->isTimerNotExpired()) // empty segment or first interval timer is not expired yet
      return;

    const auto scriptCall = [&onUpdateFunc, esData](ecs::EntityId eid, Sqrat::Table &compTbl) {
      if (!onUpdateFunc.Execute(esData->getTimeDelta(), eid, compTbl))
      {
        LOGERR_ONCE("Script ES '%s' update call failed", esData->getESName());
#if DAECS_EXTENSIVE_CHECKS
        static bool shooted = false;
        if (!shooted)
        {
          dump_sq_callstack_in_log(compTbl.GetVM());
          shooted = true;
        }
#endif
      }
      return CB_CONTINUE;
    };

    TIME_PROFILE_SQ_ES(sq_es_on_update, esData);

    exec_with_cached_comps_table(vm, components, [&](Sqrat::Table &compTbl) {
      if (esData->isNoFrameLag())
      {
        ecs_scripts_call(compTbl, *esData, components, scriptCall, esData->debugGetESName(),
          [esData](ecs::EntityId eid, int) // 'esData' pointer is captured by value
          {
            const uint32_t seedByEid = EidHashFNV1a()(eid) & USHRT_MAX;
            return (esData->minInterval <= seedByEid && seedByEid < esData->maxInterval) ? FILTER_PROCESS : FILTER_SKIP;
          });
      }
      else // we can't use interval logic when frame delta is bigger then whole interval. Assume that this is relatively rare occasion
           // (lagged frames)
      {
        ecs_scripts_call(compTbl, *esData, components, scriptCall, esData->debugGetESName());

        esData->updateTimeout = esData->updateInterval;
        esData->resetTimer(); // wait for next timeout expire
      }
    });
  }
}


void SqBindingHelper::sq_es_on_update_empty(const UpdateStageInfo &info, const QueryView &__restrict components)
{
  ScriptEsData *esData = (ScriptEsData *)components.getUserData();
  G_ASSERT_RETURN(esData, );
  G_ASSERT(!esData->onUpdate.IsNull());
  G_ASSERTF(components.getComponentsCount() == 0, "Script ES '%s' shall be empty", esData->getESName());
  G_ASSERTF(info.stage == US_ACT, "Script ES update stage = %d", info.stage);

  Sqrat::Function &onUpdateFunc = esData->onUpdate;
  ScopedLockVM lock(onUpdateFunc.GetVM());

  const UpdateStageInfoAct &actInfo = *info.cast<ecs::UpdateStageInfoAct>();
  COMP_INSTANCES_GUARD(cig, onUpdateFunc.GetVM(), esData->debugGetESName());
  {
    update_es_timer(esData, actInfo.dt);

    if (esData->isTimerNotExpired())
      return;

    TIME_PROFILE_SQ_ES(sq_es_on_update_empty, esData);

    Sqrat::Table compTbl = Sqrat::Table(onUpdateFunc.GetVM());
    if (!onUpdateFunc.Execute(esData->getTimeDelta(), ecs::INVALID_ENTITY_ID, compTbl))
    {
      LOGERR_ONCE("Script ES '%s' update call failed", esData->getESName());
#if DAECS_EXTENSIVE_CHECKS
      static bool shooted = false;
      if (!shooted)
      {
        dump_sq_callstack_in_log(compTbl.GetVM());
        shooted = true;
      }
#endif
    }

    esData->resetTimer();
  }
}


static ecs::component_type_t get_component_type(uint32_t type_hash, const char *comp_name)
{
  ecs::component_index_t componentId = g_entity_mgr->getDataComponents().findComponentId(type_hash);
  if (componentId != ecs::INVALID_COMPONENT_INDEX)
    return g_entity_mgr->getDataComponents().getComponentById(componentId).componentTypeName;

  logerr("can't auto detect component <%s> type, as this component is not existent yet", comp_name);
  return ecs::ComponentTypeInfo<ecs::auto_type>::type;
}

static bool load_component_desc(const Sqrat::Array &comps, Tab<ScriptCompDesc> &out_script_comps,
  Tab<ecs::ComponentDesc> &out_ecs_comps, String &err_msg)
{
  out_script_comps.resize(0);
  out_ecs_comps.resize(0);

  if (comps.IsNull())
    return true;

  Tab<ScriptCompDesc> script_comps;
  Tab<ecs::ComponentDesc> ecs_comps;

  SQInteger nComp = ::max(SQInteger(0), comps.Length());
  script_comps.resize(nComp);
  ecs_comps.resize(nComp);

  for (int iComp = 0; iComp < nComp; ++iComp)
  {
    ScriptCompDesc &scriptCompDesc = script_comps[iComp];
    Sqrat::Object co = comps.GetSlot(iComp);
    if (co.GetType() == OT_STRING)
    {
      scriptCompDesc.key = co;
      scriptCompDesc.native = true;
      scriptCompDesc.script = false;
      // todo: check if script component native
      ecs::component_type_t componentType = ecs::ComponentTypeInfo<ecs::auto_type>::type;
      // debug("component = %s", sq_objtostring(&co.GetObject()));
      ecs_comps[iComp] = ecs::ComponentDesc(ECS_HASH_SLOW(scriptCompDesc.getCompName()), componentType, 0);
      continue;
    }
    else if (co.GetType() == OT_ARRAY)
    {
      Sqrat::Array arr = comps.GetSlot(iComp);
      G_ASSERT(arr.Length() >= 1);
      int ecsCompFlags = 0;
      scriptCompDesc.key = arr.GetSlot(SQInteger(0));
      if (scriptCompDesc.key.GetType() != OT_STRING)
      {
        err_msg.printf(0, "first comp element expected to be component name (string), instead got %X", scriptCompDesc.key.GetType());
        return false;
      }
      HashedConstString compName = ECS_HASH_SLOW(scriptCompDesc.getCompName());
      int arrLen = arr.Length();
      if (arrLen > 2)
      {
        scriptCompDesc.defVal = arr.GetSlot(SQInteger(2));
        ecsCompFlags = CDF_OPTIONAL;
      }
      if (arrLen > 1)
      {
        SQObjectType typeType = arr.GetSlot(1).GetType();
        if (typeType == OT_TABLE)
        {
          scriptCompDesc.native = false;
          scriptCompDesc.script = true;
          ecs_comps[iComp] = ecs::ComponentDesc(compName, compName.hash, ecsCompFlags);
        }
        else if (typeType == OT_INTEGER)
        {
          scriptCompDesc.native = true;
          scriptCompDesc.script = false;
          ecs_comps[iComp] = ecs::ComponentDesc(compName,
            arr.GetSlot(1).Cast<SQInteger>(), // predefined types
            ecsCompFlags);
        }
        else if (typeType == OT_CLASS || typeType == OT_NULL)
        {
          scriptCompDesc.native = true;
          scriptCompDesc.script = false;
          ecs::component_type_t componentType = get_component_type(compName.hash, compName.str);

          ecs_comps[iComp] = ecs::ComponentDesc(compName, componentType, ecsCompFlags);
        }
        else
        {
          err_msg.printf(0, "Unexpected component <%s> type field type %X", compName.str, typeType);
          return false;
        }
      }
      else // (arrLen == 1)
      {
        scriptCompDesc.native = true;
        ecs::component_type_t componentType = ecs::ComponentTypeInfo<ecs::auto_type>::type;
        // todo: check if script component native
        ecs_comps[iComp] = ecs::ComponentDesc(ECS_HASH_SLOW(scriptCompDesc.getCompName()), componentType, 0);
      }
    }
    else
    {
      err_msg.printf(0, "component must be string or array, instead got %X", co.GetType());
      return false;
    }

    // debug("ecs_comps[%d] = %s(0x%X)", iComp, compName.str, );
  }

  out_script_comps.swap(script_comps);
  out_ecs_comps.swap(ecs_comps);
  return true;
}


inline bool entity_system_is_squirrel(EntitySystemDesc *desc)
{
  return desc->isDynamic() && desc->getUserData() &&
         (desc->getOps().onUpdate == &SqBindingHelper::sq_es_on_update ||
           desc->getOps().onUpdate == &SqBindingHelper::sq_es_on_update_empty ||
           desc->getOps().onEvent == &SqBindingHelper::sq_es_on_event); // otherwise it is not ours
}

static void script_es_desc_deleter(ecs::EntitySystemDesc *desc)
{
  G_ASSERT_RETURN(entity_system_is_squirrel(desc), );
  ScriptEsData *esData = (ScriptEsData *)desc->getUserData();
  delete esData;
  desc->setUserData(nullptr);
}

template <typename AddEvent>
inline void get_event_mask(const char *es, HSQUIRRELVM vm, EventSet &evtMask, const Sqrat::Table &on_event, AddEvent add_event)
{
  G_UNUSED(es);
  Sqrat::RootTable rootT(vm);
  Sqrat::Object::iterator it;
  while (on_event.Next(it))
  {
    Sqrat::Object key(it.getKey(), vm);
    SQObjectType keyType = key.GetType();
    const char *evClassName = NULL;
    ecs::event_type_t evtClassType = 0;
    if (keyType == OT_CLASS)
    {
      const Sqrat::AbstractStaticClassData *ascd = Sqrat::AbstractStaticClassData::FromObject(&key.GetObject());
      if (ascd && ascd->className != "DasEvent") // skip base DasEvent instance, see SqDasEventClass
        evClassName = ascd->className.c_str();
      else
        evtClassType = key.RawGetSlotValue<int>("eventType", evtClassType); // DascriptEvent::eventType, see SqDasEventClass
    }
    else if (keyType == OT_STRING)
      evClassName = sq_objtostring(&key.GetObject());
    if (evClassName || evtClassType != 0)
    {
      if (evClassName && !strcmp(evClassName, ON_UPDATE_EVENT_NAME))
        continue;
      ecs::event_type_t evType = evtClassType != 0 ? evtClassType : ECS_HASH_SLOW(evClassName).hash;
      evtMask.insert(evType);
      add_event(evType, Sqrat::Function(vm, rootT, it.getValue()));
#if DAGOR_DBGLEVEL > 0
      if (!squirrelEvents.count(evType) && !eventTypeNative2Sq.count(evType))
        logerr("event <%s> used in <%s> is not regiresterd as Squirrel or Core event in squirrel", evClassName, es);
#endif
    }
    else
      G_ASSERT_LOG(0, "Invalid event id type %X", keyType);
  }
}

static SQInteger clear_vm_entity_systems(HSQUIRRELVM vm)
{
  debug("clear_vm_entity_systems");
  ecs::remove_if_systems([&vm](EntitySystemDesc *desc) {
    if (entity_system_is_squirrel(desc))
    {
      ScriptEsData *scriptData = (ScriptEsData *)desc->getUserData();
      if (scriptData->getVM() == vm)
      {
        desc->freeIfDynamic();
        return true;
      }
    }
    return false;
  });

  if (g_entity_mgr)
    g_entity_mgr->resetEsOrder();

  return 0;
}

static SQInteger register_entity_system(HSQUIRRELVM vm)
{
  HSQOBJECT hESName;
  const char *es_name = SQ_SUCCEEDED(sq_getstackobj(vm, 2, &hESName)) ? sq_objtostring(&hESName) : nullptr;

  DEBUG_CTX("[SQ ECS] register_entity_system(%s)", es_name);

  if (!ecs_is_in_init_phase)
    return sq_throwerror(vm, "ES can be registered only during initialization");

  if (!es_name || !*es_name)
    return sq_throwerror(vm, "Attempt to register ES with bad name");

  EntitySystemDesc *desc = ecs::find_if_systems([es_name](EntitySystemDesc *desc) { return strcmp(desc->name, es_name) == 0; });
  if (desc)
  {
    eastl::string errMsg(eastl::string::CtorSprintf(), "Duplicate ES <%s> registration attempt", es_name);
    return sq_throwerror(vm, errMsg.c_str());
  }

  //  Sqrat::Var<Sqrat::Object> update_func(vm, 3);
  Sqrat::Var<Sqrat::Table> on_event(vm, 3);

  Sqrat::Array comps_rw, comps_ro, comps_rq, comps_no;
  Sqrat::Table params;
  G_ASSERT(params.IsNull());

  Sqrat::Table compsDescTbl = Sqrat::Var<Sqrat::Table>(vm, 4).value;
  comps_rw = compsDescTbl.RawGetSlot("comps_rw");
  comps_ro = compsDescTbl.RawGetSlot("comps_ro");
  comps_rq = compsDescTbl.RawGetSlot("comps_rq");
  comps_no = compsDescTbl.RawGetSlot("comps_no");

  const bool isEmptyEs = comps_ro.IsNull() && comps_rw.IsNull() && comps_rq.IsNull() && comps_no.IsNull();

  if (sq_gettop(vm) >= 5)
    params = (Sqrat::Var<Sqrat::Table>(vm, 5)).value;

  Sqrat::Object update_func = on_event.value.IsNull() ? nullptr : on_event.value.RawGetSlot(ON_UPDATE_EVENT_NAME);

  float updateInterval = 0;
  const float minUpdateInterval = 0.1f;
  if (!update_func.IsNull())
  {
    if (!params.IsNull())
      updateInterval = params.RawGetSlotValue("updateInterval", 0.0f);
    if (updateInterval < minUpdateInterval)
    {
      if (!isEmptyEs)
      {
        eastl::string errMsg(eastl::string::CtorSprintf(),
          "Non-empty script ES %s has update function and must specify updateInterval parameter which is >= %f", es_name,
          minUpdateInterval);
        return sq_throwerror(vm, errMsg.c_str());
      }
      else
        updateInterval = minUpdateInterval; // doesn't matter actually
    }
  }

  auto esData = eastl::make_unique<ScriptEsData>(Sqrat::Object(hESName, vm));

  if (!params.IsNull())
  {
    auto getSqString = [&](Sqrat::Object &str, const char *slot) {
      Sqrat::Object tagsParamObj = params.RawGetSlot(slot);
      SQObjectType sqt = tagsParamObj.GetType();
      if (sqt == OT_STRING)
        str = tagsParamObj;
      else if (sqt != OT_NULL)
      {
        eastl::string errMsg(eastl::string::CtorSprintf(), "%s in ES <%s> registration must be string, instead got %#x", slot, es_name,
          sqt);
        (void)sq_throwerror(vm, errMsg.c_str());
        return false;
      }
      return true;
    };

    if (!getSqString(esData->tags, "tags") || !getSqString(esData->tracked, "track") || !getSqString(esData->before, "before") ||
        !getSqString(esData->after, "after"))
      return SQ_ERROR;
  }

  if (!update_func.IsNull())
  {
    esData->onUpdate = Sqrat::Function(vm, Sqrat::Object(vm), update_func);
    esData->updateTimeout = esData->updateInterval = updateInterval;
  }

  EventSet evtMask;
  get_event_mask(es_name, vm, evtMask, on_event.value,
    [&esData](event_type_t evType, Sqrat::Function &&f) { esData->onEvent[evType] = eastl::move(f); });

  String errMsg;
  if (!load_component_desc(comps_rw, esData->componentsRw, esData->esCompsRw, errMsg) ||
      !load_component_desc(comps_ro, esData->componentsRo, esData->esCompsRo, errMsg) ||
      !load_component_desc(comps_rq, esData->componentsRq, esData->esCompsRq, errMsg) ||
      !load_component_desc(comps_no, esData->componentsNo, esData->esCompsNo, errMsg))
  {
    return sq_throwerror(vm, errMsg);
  }

  if (esData->esCompsRo.size() + esData->esCompsRw.size() + esData->esCompsRq.size() + esData->esCompsNo.size() > 0)
    esData->esCompsRo.push_back(ecs::ComponentDesc(ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>(), 0));

  const auto sqEsOnUpdate = isEmptyEs ? &SqBindingHelper::sq_es_on_update_empty : &SqBindingHelper::sq_es_on_update;
  ecs::EntitySystemOps eops(!esData->onUpdate.IsNull() ? sqEsOnUpdate : NULL,
    !esData->onEvent.empty() ? &SqBindingHelper::sq_es_on_event : NULL);

  if (eops.empty())
    return sqstd_throwerrorf(vm, "Attempt to register no-op ES <%s>", es_name);

  // ctor puts system to the list
  new ecs::EntitySystemDesc(es_name, eops, make_span<const ecs::ComponentDesc>(esData->esCompsRw.data(), esData->esCompsRw.size()),
    make_span<const ecs::ComponentDesc>(esData->esCompsRo.data(), esData->esCompsRo.size()),
    make_span<const ecs::ComponentDesc>(esData->esCompsRq.data(), esData->esCompsRq.size()),
    make_span<const ecs::ComponentDesc>(esData->esCompsNo.data(), esData->esCompsNo.size()), eastl::move(evtMask),
    update_func.IsNull() ? 0 : (1 << US_ACT), esData->getTags(), esData->getTracked(), esData->getBefore(), esData->getAfter(),
    esData.get(),
    /*quant*/ 0,
    /*dynamic*/ true, script_es_desc_deleter);
  esData.release(); // ownership moved to desc

  return 0;
}


#define COMMON_LIST     \
  COMMON_COMP(Point2);  \
  COMMON_COMP(Point3);  \
  COMMON_COMP(Point4);  \
  COMMON_COMP(IPoint2); \
  COMMON_COMP(IPoint3); \
  COMMON_COMP(IPoint4); \
  COMMON_COMP(DPoint3); \
  COMMON_COMP(TMatrix); \
  COMMON_COMP(E3DCOLOR);

#define TYPE_NULL 0

template <typename TypeRW, typename TypeRO>
static inline void push_array_val(HSQUIRRELVM vm, const EntityComponentRef &comp, CompAccess access)
{
  if (access == COMP_ACCESS_READONLY)
    Sqrat::PushVar(vm, static_cast<const TypeRO *>(comp.getNullable<TypeRW>()));
  else
    Sqrat::PushVar(vm, comp.getNullable<TypeRW>());
}

static void push_comp_val(HSQUIRRELVM vm, const char *comp_name, const EntityComponentRef comp, CompAccess access)
{
  int prevTop = sq_gettop(vm);
  G_UNUSED(prevTop);

  switch (comp.getUserType())
  {
    case ecs::ComponentTypeInfo<ecs::string>::type:
    {
      auto &str = comp.get<ecs::string>();
      sq_pushstring(vm, str.c_str(), str.length());
    }
    break;
    case ecs::ComponentTypeInfo<ecs::EntityId>::type: sq_pushinteger(vm, (ecs::entity_id_t)comp.get<ecs::EntityId>()); break;
    case ecs::ComponentTypeInfo<int>::type: sq_pushinteger(vm, comp.get<int>()); break;
    case ecs::ComponentTypeInfo<uint8_t>::type: sq_pushinteger(vm, comp.get<uint8_t>()); break;
    case ecs::ComponentTypeInfo<uint16_t>::type: sq_pushinteger(vm, comp.get<uint16_t>()); break;
    case ecs::ComponentTypeInfo<int64_t>::type: sq_pushinteger(vm, comp.get<int64_t>()); break;
    case ecs::ComponentTypeInfo<uint64_t>::type: sq_pushinteger(vm, comp.get<uint64_t>()); break;
    case ecs::ComponentTypeInfo<float>::type: sq_pushfloat(vm, comp.get<float>()); break;
    case ecs::ComponentTypeInfo<bool>::type: sq_pushbool(vm, comp.get<bool>()); break;

    case ecs::ComponentTypeInfo<ecs::Object>::type:
    {
      if (access == COMP_ACCESS_READONLY)
        Sqrat::PushVar(vm, static_cast<const ecs::ObjectRO *>(comp.getNullable<ecs::Object>()));
      else
        Sqrat::PushVar(vm, comp.getNullable<ecs::Object>());
      break;
    }
    case ecs::ComponentTypeInfo<ecs::Array>::type:
    {
      push_array_val<ecs::Array, ecs::ArrayRO>(vm, comp, access);
      break;
    }

#define PUSH_LIST_VAL(T)                                                                      \
  case ecs::ComponentTypeInfo<T>::type:                                                       \
  {                                                                                           \
    push_array_val<T, T##RO>(vm, comp, access);                                               \
    break;                                                                                    \
  }                                                                                           \
  case ecs::ComponentTypeInfo<ecs::SharedComponent<T>>::type:                                 \
  {                                                                                           \
    if (access != COMP_ACCESS_READWRITE)                                                      \
    {                                                                                         \
      const ecs::SharedComponent<T> *sharedObj = comp.getNullable<ecs::SharedComponent<T>>(); \
      const T *obj = sharedObj ? sharedObj->get() : nullptr;                                  \
      Sqrat::PushVar(vm, static_cast<const T##RO *>(obj));                                    \
    }                                                                                         \
    else                                                                                      \
    {                                                                                         \
      logerr("Shared component %s is readonly list", comp_name);                              \
      sq_pushnull(vm);                                                                        \
    }                                                                                         \
    break;                                                                                    \
  }
#define DECL_LIST_TYPE(lt, t) PUSH_LIST_VAL(ecs::lt)
      ECS_DECL_LIST_TYPES
#undef DECL_LIST_TYPE
#undef PUSH_LIST_VAL

    case ecs::ComponentTypeInfo<ecs::SharedComponent<ecs::Object>>::type:
    {
      if (access != COMP_ACCESS_READWRITE)
      {
        const ecs::SharedComponent<ecs::Object> *sharedObj = comp.getNullable<ecs::SharedComponent<ecs::Object>>();
        const ecs::Object *obj = sharedObj ? sharedObj->get() : nullptr;
        Sqrat::PushVar(vm, static_cast<const ecs::ObjectRO *>(obj));
      }
      else
      {
        logerr("Shared component %s is readonly object", comp_name);
        sq_pushnull(vm);
      }
      break;
    }
    case ecs::ComponentTypeInfo<ecs::SharedComponent<ecs::Array>>::type:
    {
      if (access != COMP_ACCESS_READWRITE)
      {
        const ecs::SharedComponent<ecs::Array> *sharedArr = comp.getNullable<ecs::SharedComponent<ecs::Array>>();
        const ecs::Array *arr = sharedArr ? sharedArr->get() : nullptr;
        Sqrat::PushVar(vm, static_cast<const ecs::ArrayRO *>(arr));
      }
      else
      {
        logerr("Shared component %s is readonly array", comp_name);
        sq_pushnull(vm);
      }
      break;
    }
#define COMMON_COMP(type_name) \
  case ecs::ComponentTypeInfo<type_name>::type: Sqrat::Var<type_name>::push(vm, comp.get<type_name>()); break
      COMMON_LIST
#undef COMMON_COMP
    case TYPE_NULL:
    default:
      auto res = native_component_bindings.find(comp.getUserType());
      if (res != native_component_bindings.end())
      {
        res->second(vm, comp.getRawData());
        break;
      }
      if (const char *tname = g_entity_mgr->getComponentTypes().findTypeName(comp.getUserType()))
      {
        sq_pushstring(vm, tname, -1);
        break;
      }
      logerr("Unexpected component type %s 0x%X", comp_name, comp.getUserType()); // FIXME: replace with rawvalue
      G_UNUSED(comp_name);
      sq_pushnull(vm);
      break;
  }

  G_ASSERT(sq_gettop(vm) == prevTop + 1);
}


static ecs::component_type_t ecs_type_from_sq_obj(const Sqrat::Object &obj, const char *debug_name = nullptr);

template <typename TypeRW, typename TypeRO>
static bool pop_list_inst(HSQUIRRELVM vm, SQInteger idx, EntityComponentRef &comp)
{
  SQObjectType sqType = sq_gettype(vm, idx);
  if (sqType != OT_INSTANCE)
    return false;

  HSQOBJECT hObj;
  sq_getstackobj(vm, idx, &hObj);

  if (Sqrat::ClassType<TypeRW>::IsObjectOfClass(&hObj))
  {
    Sqrat::Var<const TypeRW *> arrVar(vm, idx);
    comp = *arrVar.value;
    return true;
  }
  else if (Sqrat::ClassType<TypeRO>::IsObjectOfClass(&hObj))
  {
    Sqrat::Var<const TypeRO *> arrVar(vm, idx);
    comp = *(static_cast<const TypeRW *>(arrVar.value));
    return true;
  }
  return false;
}

template <typename TypeRW, typename TypeRO>
static bool pop_list_val(HSQUIRRELVM vm, SQInteger idx, EntityComponentRef &comp, const char *comp_name)
{
  SQObjectType sqType = sq_gettype(vm, idx);
  if (sqType == OT_INSTANCE)
  {
    if (pop_list_inst<TypeRW, TypeRO>(vm, idx, comp))
      return true;
    return false;
  }
  else if (sqType == OT_ARRAY)
  {
    int prevTop = sq_gettop(vm);
    G_UNUSED(prevTop);

    TypeRW list;
    SQInteger len = sq_getsize(vm, idx);
    sq_push(vm, idx);

    bool statusOk = true;
    String errMsg;
    for (SQInteger i = 0; i < len && statusOk; ++i)
    {
      ecs::ChildComponent memberComp;
      sq_pushinteger(vm, i);
      G_VERIFY(SQ_SUCCEEDED(sq_rawget(vm, -2)));
      HSQOBJECT hValObj;
      G_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &hValObj)));
      bool succeeded = pop_comp_val(vm, -1, memberComp, comp_name, ecs::ComponentTypeInfo<typename TypeRW::value_type>::type, errMsg);
      if (succeeded)
        list.push_back(eastl::move(memberComp));
      else
      {
        statusOk = false;
        errMsg.printf(32, "Error reading List %s member %d", comp_name, i);
      }
      sq_pop(vm, 1);
    }

    sq_pop(vm, 1);

    if (statusOk)
      comp = eastl::move(list);

    G_ASSERT(prevTop == sq_gettop(vm));
    return statusOk;
  }
  return false;
}

static bool pop_component_val(HSQUIRRELVM vm, SQInteger idx, EntityComponentRef &comp, const char *comp_name, String &err_msg)
{
  auto sqGetErr = [&err_msg, comp_name, idx, vm](const char *expected_typename) {
    err_msg.printf(0, "Failed to pop component <%s> (sq type %s expected, got %#08X instead)", comp_name, expected_typename,
      sq_gettype(vm, idx));
    return false;
  };
  switch (comp.getUserType())
  {
    case ecs::ComponentTypeInfo<ecs::string>::type:
    {
      const char *s = NULL;
      if (SQ_FAILED(sq_getstring(vm, idx, &s)))
        return sqGetErr("string");
      comp = ecs::string(s);
      return true;
    }

    case ecs::ComponentTypeInfo<ecs::EntityId>::type:
    {
      SQInteger val = 0;
      if (SQ_FAILED(sq_getinteger(vm, idx, &val)))
      {
        Sqrat::Var<const ecs::EntityId *> eidVal(vm, idx);
        if (eidVal.value)
        {
          comp = *eidVal.value;
          return true;
        }
        return sqGetErr("eid(int|instance)");
      }
      comp = ecs::EntityId(val);

      return true;
    }
    case ecs::ComponentTypeInfo<uint8_t>::type:
    {
      SQInteger val = 0;
      if (SQ_FAILED(sq_getinteger(vm, idx, &val)))
        return sqGetErr("integer");
      comp = uint8_t(val);
      return true;
    }
    case ecs::ComponentTypeInfo<uint16_t>::type:
    {
      SQInteger val = 0;
      if (SQ_FAILED(sq_getinteger(vm, idx, &val)))
        return sqGetErr("integer");
      comp = uint16_t(val);
      return true;
    }
    case ecs::ComponentTypeInfo<int>::type:
    {
      SQInteger val = 0;
      if (SQ_FAILED(sq_getinteger(vm, idx, &val)))
        return sqGetErr("integer");
      comp = int(val);
      return true;
    }
    case ecs::ComponentTypeInfo<int64_t>::type:
    {
      SQInteger val = 0;
      if (SQ_FAILED(sq_getinteger(vm, idx, &val)))
        return sqGetErr("integer");
      comp = int64_t(val);
      return true;
    }
    case ecs::ComponentTypeInfo<uint64_t>::type:
    {
      SQInteger val = 0;
      if (SQ_FAILED(sq_getinteger(vm, idx, &val)))
        return sqGetErr("integer");
      comp = uint64_t(val);
      return true;
    }

    case ecs::ComponentTypeInfo<float>::type:
    {
      SQFloat val = 0;
      if (SQ_FAILED(sq_getfloat(vm, idx, &val)))
        return sqGetErr("float");
      comp = val;
      return true;
    }

    case ecs::ComponentTypeInfo<bool>::type:
    {
      SQBool val = 0;
      if (SQ_FAILED(sq_getbool(vm, idx, &val)))
        return sqGetErr("bool");
      comp = bool(val);
      return true;
    }

    case ecs::ComponentTypeInfo<ecs::Object>::type:
    {
      SQObjectType sqType = sq_gettype(vm, idx);
      if (sqType == OT_INSTANCE)
      {
        HSQOBJECT hObj;
        sq_getstackobj(vm, idx, &hObj);
        if (Sqrat::ClassType<ecs::Object>::IsObjectOfClass(&hObj))
        {
          Sqrat::Var<const ecs::Object *> objVar(vm, idx);
          comp = *objVar.value;
          return true;
        }
        else if (Sqrat::ClassType<ecs::ObjectRO>::IsObjectOfClass(&hObj))
        {
          Sqrat::Var<const ecs::ObjectRO *> objVar(vm, idx);
          comp = *(static_cast<const ecs::Object *>(objVar.value));
          return true;
        }
        else
          return sqGetErr("ecs::Object instance");
      }
      else if (sqType == OT_TABLE)
      {
        int prevTop = sq_gettop(vm);
        G_UNUSED(prevTop);

        ecs::Object obj;
        sq_push(vm, idx);
        sq_pushnull(vm);
        String memberErrMsg;
        bool statusOk = true;
        while (SQ_SUCCEEDED(sq_next(vm, -2)) && statusOk)
        {
          const char *mname = nullptr;
          if (SQ_FAILED(sq_getstring(vm, -2, &mname)))
            continue;
          ChildComponent memberComp;
          HSQOBJECT hValObj;
          G_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &hValObj)));
          if (sq_type(hValObj) == OT_NULL)
            ; // skip nulls as if it never existed
          else if (pop_comp_val(vm, -1, memberComp, mname, ecs_type_from_sq_obj(Sqrat::Object(hValObj, vm), mname), memberErrMsg))
            obj.addMember(mname, eastl::move(memberComp));
          else
          {
            err_msg.printf(64, "Error reading Object %s member %s: %s", comp_name, mname, memberErrMsg.c_str());
            statusOk = false;
          }
          sq_pop(vm, 2); // pops key and value, keep iterator on top
        }
        sq_pop(vm, 2); // pops the iterator and table

        comp = eastl::move(obj);

        G_ASSERT(prevTop == sq_gettop(vm));
        return statusOk;
      }
      else
        return sqGetErr("ecs::Object instance");
      return false;
    }

#define DECL_LIST_TYPE(lt, t)                                          \
  case ecs::ComponentTypeInfo<lt>::type:                               \
  {                                                                    \
    if (!pop_list_val<ecs::lt, ecs::lt##RO>(vm, idx, comp, comp_name)) \
      return false;                                                    \
    return true;                                                       \
  }
      ECS_DECL_LIST_TYPES
#undef DECL_LIST_TYPE

    case ecs::ComponentTypeInfo<ecs::Array>::type:
    {
      SQObjectType sqType = sq_gettype(vm, idx);
      if (sqType == OT_INSTANCE)
      {
        HSQOBJECT hObj;
        sq_getstackobj(vm, idx, &hObj);
        if (Sqrat::ClassType<ecs::Array>::IsObjectOfClass(&hObj))
        {
          Sqrat::Var<const ecs::Array *> arrVar(vm, idx);
          comp = *arrVar.value;
          return true;
        }
        else if (Sqrat::ClassType<ecs::ArrayRO>::IsObjectOfClass(&hObj))
        {
          Sqrat::Var<const ecs::ArrayRO *> arrVar(vm, idx);
          comp = *(static_cast<const ecs::Array *>(arrVar.value));
          return true;
        }
        else
        {
          err_msg = "Expected ecs::Array instance";
          return false;
        }
      }
      else if (sqType == OT_ARRAY)
      {
        int prevTop = sq_gettop(vm);
        G_UNUSED(prevTop);

        ecs::Array arr;
        SQInteger len = sq_getsize(vm, idx);
        sq_push(vm, idx);

        bool statusOk = true;
        String errMsg;
        for (SQInteger i = 0; i < len && statusOk; ++i)
        {
          ecs::ChildComponent memberComp;
          sq_pushinteger(vm, i);
          G_VERIFY(SQ_SUCCEEDED(sq_rawget(vm, -2)));
          HSQOBJECT hValObj;
          G_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &hValObj)));
          bool succeeded =
            pop_comp_val(vm, -1, memberComp, comp_name, ecs_type_from_sq_obj(Sqrat::Object(hValObj, vm), comp_name), errMsg);
          if (succeeded)
            arr.push_back(eastl::move(memberComp));
          else
          {
            statusOk = false;
            errMsg.printf(32, "Error reading Array %s member %d", comp_name, i);
          }
          sq_pop(vm, 1); // array item value
        }

        sq_pop(vm, 1); // pop array

        if (statusOk)
          comp = eastl::move(arr);

        G_ASSERT(prevTop == sq_gettop(vm));
        return statusOk;
      }
      else
        return sqGetErr("ecs::Array instance");
    }

#define COMMON_COMP(type_name)                   \
  case ecs::ComponentTypeInfo<type_name>::type:  \
    do                                           \
    {                                            \
      Sqrat::Var<type_name *> val(vm, idx);      \
      if (!val.value)                            \
        return sqGetErr(#type_name " instance"); \
      comp = *val.value;                         \
      return true;                               \
    } while (false)
      COMMON_LIST
#undef COMMON_COMP

    case TYPE_NULL:
    default: err_msg.printf(32, "Unexpected component type %#08X %s", comp.getUserType(), comp_name); return false;
  }
}

static bool pop_comp_val(HSQUIRRELVM vm, SQInteger idx, ChildComponent &comp, const char *comp_name, ecs::component_type_t type,
  String &err_msg)
{
  if (comp.getUserType() != type)
  {
    if (comp.getUserType() != 0)
    {
      err_msg.printf(128, "invalid type on component <%s> assignment. Assign 0x%X to 0x%X", comp_name, type, comp.getUserType());
      return false;
    }
    switch (type)
    {
      case ecs::ComponentTypeInfo<ecs::Array>::type: comp = ecs::Array(); break;
      case ecs::ComponentTypeInfo<ecs::Object>::type: comp = ecs::Object(); break;
#define DECL_LIST_TYPE(lt, t)                              \
  case ecs::ComponentTypeInfo<t>::type: comp = t(); break; \
  case ecs::ComponentTypeInfo<ecs::lt>::type: comp = lt(); break;
        ECS_DECL_LIST_TYPES
#undef DECL_LIST_TYPE
      default:
        if (g_entity_mgr->getComponentTypes().findTypeName(comp.getUserType()))
          return true;
        err_msg.printf(32, "Unexpected component type 0x%X %s", type, comp_name);
        return false;
    };
  }
  EntityComponentRef compRef = comp.getEntityComponentRef();
  return pop_component_val(vm, idx, compRef, comp_name, err_msg);
}

static inline ecs::Object sq_table_to_object(const Sqrat::Object &data_)
{
  ecs::Object data;
  if (!data_.IsNull())
  {
    HSQUIRRELVM vm = data_.GetVM();
    ChildComponent objComp = ecs::Object();
    sq_pushobject(vm, data_.GetObject());
    String err;
    EntityComponentRef objRef = objComp.getEntityComponentRef();
    if (pop_component_val(vm, -1, objRef, "SQEvent::schemeless_data", err))
      data = eastl::move(objComp.getRW<ecs::Object>());
    else
      logerr("can't convert payload to Object: <%s>", err.c_str());
    sq_pop(vm, 1);
  }
  return eastl::move(data);
}

sq::SQEvent::SQEvent(ecs::event_type_t tp, const Sqrat::Object &data_) :
  ecs::SchemelessEvent(tp, eastl::move(sq_table_to_object(data_)))
{
  G_STATIC_ASSERT(sizeof(SQEvent) == sizeof(SchemelessEvent));
}


static SQInteger obsolete_dbg_get_comp_val(HSQUIRRELVM vm)
{
  int top = sq_gettop(vm);
  G_ASSERT(top == 3 || top == 4);

  const char *comp_name = NULL;
  SQInteger eidInt = ECS_INVALID_ENTITY_ID_VAL;
  G_VERIFY(SQ_SUCCEEDED(sq_getinteger(vm, 2, &eidInt)));
  G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, 3, &comp_name)));

  ecs::EntityId eid((ecs::entity_id_t)eidInt);

  const EntityComponentRef comp = g_entity_mgr->getComponentRef(eid, ECS_HASH_SLOW(comp_name));
  if (!comp.isNull())
    push_comp_val(vm, comp_name, comp, COMP_ACCESS_READWRITE);
  else if (top == 3)
    sq_pushnull(vm);
  else
    sq_push(vm, 4);

  return 1;
}

// returns unsigned in value, like for ecs::component_t
static SQInteger calc_hash(HSQUIRRELVM vm)
{
  const char *str = nullptr;
  sq_getstring(vm, 2, &str);
  sq_pushinteger(vm, ECS_HASH_SLOW(str).hash); // uint
  return 1;
}

// returns signed int value, like hash of string in ecs::IntList
static SQInteger calc_hash_int(HSQUIRRELVM vm)
{
  const char *str = nullptr;
  sq_getstring(vm, 2, &str);
  sq_pushinteger(vm, (int)ECS_HASH_SLOW(str).hash); // int
  return 1;
}

static SQInteger _dbg_get_all_comps_inspect(HSQUIRRELVM vm)
{
  G_ASSERT(sq_gettop(vm) == 2);

  Sqrat::Var<EntityId> eid(vm, 2);

  sq_newtable(vm);

  for (ComponentsIterator it = g_entity_mgr->getComponentsIterator(eid.value); it; ++it)
  {
    ComponentsIterator::Ret comp = *it;
    if (g_entity_mgr->getDataComponents().getComponentById(comp.second.getComponentId()).flags & DataComponent::IS_COPY)
      continue;
    sq_pushstring(vm, comp.first, strlen(comp.first));
    push_comp_val(vm, comp.first, comp.second, COMP_ACCESS_INSPECT);

    G_VERIFY(SQ_SUCCEEDED(sq_rawset(vm, -3)));
  }

  G_ASSERTF(sq_gettop(vm) == 3, "top = %d", sq_gettop(vm));

  return 1;
}

static SQInteger _dbg_get_comp_val_inspect(HSQUIRRELVM vm)
{
  int top = sq_gettop(vm);
  G_ASSERT(top == 3 || top == 4);

  const char *comp_name = NULL;
  SQInteger eidInt = ECS_INVALID_ENTITY_ID_VAL;
  G_VERIFY(SQ_SUCCEEDED(sq_getinteger(vm, 2, &eidInt)));
  G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, 3, &comp_name)));

  ecs::EntityId eid((ecs::entity_id_t)eidInt);

  const EntityComponentRef comp = g_entity_mgr->getComponentRef(eid, ECS_HASH_SLOW(comp_name));
  if (!comp.isNull())
    push_comp_val(vm, comp_name, comp, COMP_ACCESS_INSPECT);
  else if (top == 3)
    sq_pushnull(vm);
  else
    sq_push(vm, 4);

  return 1;
}


static uint32_t get_comp_type(ecs::EntityId eid, const char *comp_name)
{
  return g_entity_mgr->getComponentRef(eid, ECS_HASH_SLOW(comp_name)).getUserType();
}


static SQInteger obsolete_dbg_set_comp_val(HSQUIRRELVM vm)
{
  int top = sq_gettop(vm);
  (void)top;
  G_ASSERT(top == 4 || top == 5);

  bool typeProvided = (top == 5);

  const char *comp_name = nullptr;
  SQInteger eidInt = ECS_INVALID_ENTITY_ID_VAL;
  G_VERIFY(SQ_SUCCEEDED(sq_getinteger(vm, 2, &eidInt)));
  G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, 3, &comp_name)));

  ecs::EntityId eid((ecs::entity_id_t)eidInt);

  EntityComponentRef newComp =
    g_entity_mgr->getComponentRefRW(eid, g_entity_mgr->getDataComponents().findComponentId(ECS_HASH_SLOW(comp_name).hash));
  if (newComp.isNull())
  {
    if (g_entity_mgr->doesEntityExist(eid))
      logerr("component <%s> is missing in %d eid (%s)", comp_name, ecs::entity_id_t(eid), g_entity_mgr->getEntityTemplateName(eid));
    else
      debug("attempt to obsolete_dbg_set_comp_val (%s) to dead eid (%d)", comp_name, ecs::entity_id_t(eid));
    sq_pushnull(vm);
    return 1;
  }
  if (typeProvided)
  {
    ecs::component_type_t compType = Sqrat::Var<ecs::component_type_t>(vm, 4).value;
    if (compType != newComp.getUserType())
    {
      logerr("component <%s> in %d eid (%s) has 0x%X type (hinted 0x%X)", comp_name, ecs::entity_id_t(eid),
        g_entity_mgr->getEntityTemplateName(eid), newComp.getUserType(), compType);
    }
  }

  String errMsg;
  if (!pop_component_val(vm, typeProvided ? 5 : 4, newComp, comp_name, errMsg))
    return sq_throwerror(vm, errMsg);

  sq_pushbool(vm, SQTrue);
  return 1;
}


static ecs::component_type_t ecs_type_from_sq_obj(const Sqrat::Object &obj, const char *debug_name)
{
  SQObjectType tp = obj.GetType();

  switch (tp)
  {
    case OT_INTEGER:
      if (debug_name)
      {
        if (str_ends_with(debug_name, "eid"))
          return ecs::ComponentTypeInfo<ecs::EntityId>::type;
        else if (str_ends_with(debug_name, "_int64"))
          return ecs::ComponentTypeInfo<int64_t>::type;
      }
      return ecs::ComponentTypeInfo<int>::type;
    case OT_FLOAT: return ecs::ComponentTypeInfo<float>::type;
    case OT_BOOL: return ecs::ComponentTypeInfo<bool>::type;
    case OT_STRING: return ecs::ComponentTypeInfo<ecs::string>::type;
    case OT_TABLE: return ecs::ComponentTypeInfo<ecs::Object>::type;
    case OT_ARRAY: return ecs::ComponentTypeInfo<ecs::Array>::type;
    default: break;
  }

  if (tp == OT_INSTANCE)
  {
    HSQOBJECT hObj = obj.GetObject();
    Sqrat::AbstractStaticClassData *ascd = Sqrat::AbstractStaticClassData::FromObject(&hObj);
    G_ASSERT(ascd);

#define CHECK(tp)                                                      \
  if (ascd == Sqrat::ClassType<tp>::getStaticClassData().lock().get()) \
    return ecs::ComponentTypeInfo<tp>::type;
#define CHECK_RO(tp)                                                       \
  if (ascd == Sqrat::ClassType<tp##RO>::getStaticClassData().lock().get()) \
    return ecs::ComponentTypeInfo<tp>::type;

    CHECK(TMatrix)
    CHECK(Point2)
    CHECK(Point3)
    CHECK(Point4)
    CHECK(IPoint2)
    CHECK(IPoint3)
    CHECK(IPoint4)
    CHECK(E3DCOLOR)
    CHECK(ecs::Object)
    CHECK(ecs::Array)
    CHECK(ecs::EntityId)
    CHECK_RO(ecs::Object)
    CHECK_RO(ecs::Array)

#define DECL_LIST_TYPE(lt, t) \
  CHECK(ecs::lt)              \
  CHECK_RO(ecs::lt)
    ECS_DECL_LIST_TYPES
#undef DECL_LIST_TYPE

#undef CHECK
#undef CHECK_RO

    G_ASSERTF(0, "Unsupported script component class %s", ascd->className.c_str());
    return TYPE_NULL;
  }

  G_UNUSED(debug_name);
  char tmpb[32];
  SNPRINTF(tmpb, sizeof(tmpb), "%X", tp);
  G_ASSERTF(0, "Unsupported script component type %s for '%s'", tp == OT_NULL ? "<null>" : tmpb,
    (debug_name ? debug_name : "<unknown>"));
  return TYPE_NULL;
}

template <typename ComponentsContainer>
static inline SQRESULT sq_obj_to_comps(HSQUIRRELVM vm, const Sqrat::Var<Sqrat::Object> &comp_map_obj, ComponentsContainer &comp_map)
{
  String errMsg;
  bool paramsValid = true;
  Sqrat::Object::iterator it;
  while (comp_map_obj.value.Next(it))
  {
    ChildComponent comp;
    if (sq_type(it.getValue()) == OT_ARRAY)
    {
      Sqrat::Array rec(it.getValue(), vm);
      G_ASSERT(rec.Length() == 2);

      auto compType = rec.GetSlot(1).Cast<ecs::component_type_t>();
      int prevTop = sq_gettop(vm);
      G_UNUSED(prevTop);
      sq_pushobject(vm, rec.GetSlot(SQInteger(0)));
      if (!pop_comp_val(vm, -1, comp, it.getName(), compType, errMsg))
        paramsValid = false;
      sq_pop(vm, 1);
      G_ASSERT(sq_gettop(vm) == prevTop);
    }
    else
    {
      Sqrat::Object itval(it.getValue(), vm);
      if (itval.IsNull())
        continue; // skip nulls as if it never existed
      ecs::component_type_t compType = ecs_type_from_sq_obj(itval, it.getName());
      int prevTop = sq_gettop(vm);
      G_UNUSED(prevTop);
      sq_pushobject(vm, it.getValue());
      if (!pop_comp_val(vm, -1, comp, it.getName(), compType, errMsg))
        paramsValid = false;
      sq_pop(vm, 1);
      G_ASSERT(sq_gettop(vm) == prevTop);
    }
    const char *name = it.getName();
    HashedConstString hName;
    if (name && *name == '#')
      hName = HashedConstString{NULL, (component_t)strtol(name + 1, NULL, 16)};
    else
      hName = ECS_HASH_SLOW(name);
    comp_map[hName] = eastl::move(comp);
  }
  if (!paramsValid)
    return sq_throwerror(vm, errMsg);
  return SQ_OK;
}

SQRESULT sq_obj_to_comp_map(HSQUIRRELVM vm, const Sqrat::Var<Sqrat::Object> &comp_map_obj, ComponentsMap &comp_map)
{
  return sq_obj_to_comps(vm, comp_map_obj, comp_map);
}


SQInteger comp_map_to_sq_obj(HSQUIRRELVM vm, const ComponentsMap &comp_map)
{
  int top = sq_gettop(vm);
  (void)top;
  sq_newtable(vm);
  for (auto &comp : comp_map)
  {
    char buf[16];
    const char *compName = g_entity_mgr->getTemplateDB().getComponentName(comp.first);
    if (!compName)
    {
      snprintf(buf, sizeof(buf), "#%X", comp.first);
      compName = buf;
    }
    sq_pushstring(vm, compName, strlen(compName));
    sq_newarray(vm, 2);

    ecs::component_type_t type = comp.second.getUserType();

    {
      sq_pushinteger(vm, 0);
      switch (type)
      {
        case ecs::ComponentTypeInfo<ecs::Object>::type:
          comp_obj_get_all<ecs::Object, ecs::Array, false>(vm, comp.second.get<ecs::Object>());
          break;
        case ecs::ComponentTypeInfo<ecs::Array>::type:
          comp_arr_get_all<ecs::Object, ecs::Array, false>(vm, comp.second.get<ecs::Array>());
          break;

#define DECL_LIST_TYPE(lt, t) \
  case ecs::ComponentTypeInfo<ecs::lt>::type: comp_list_get_all<ecs::lt, false>(vm, comp.second.get<ecs::lt>()); break;
          ECS_DECL_LIST_TYPES
#undef DECL_LIST_TYPE

        default: push_comp_val(vm, compName, comp.second.getEntityComponentRef(), COMP_ACCESS_READWRITE); break;
      };
      G_VERIFY(SQ_SUCCEEDED(sq_rawset(vm, -3)));
    }

    {
      sq_pushinteger(vm, 1);
      sq_pushinteger(vm, type);
      G_VERIFY(SQ_SUCCEEDED(sq_rawset(vm, -3)));
    }

    G_VERIFY(SQ_SUCCEEDED(sq_rawset(vm, -3)));
  }
  G_ASSERT(sq_gettop(vm) == top + 1);
  return 1;
}

static SQInteger create_entity_impl(HSQUIRRELVM vm, bool sync)
{
  if (!Sqrat::check_signature<EntityManager *>(vm))
    return SQ_ERROR;

  decltype(vm_extra_data)::value_type *vmit = nullptr;
  if (DAGOR_UNLIKELY(!is_vm_registered(vm, "Attempt to create entity in unknown (shutdowned?) vm", &vmit)))
    return SQ_ERROR;

  register_pending_es();
  Sqrat::Var<EntityManager *> mgr(vm, 1);
  Sqrat::Var<const char *> tplName(vm, 2);
  Sqrat::Var<Sqrat::Object> compMapObj(vm, 3);
  Sqrat::Var<Sqrat::Object> callback(vm, 4);

  G_ASSERT(callback.value.GetType() == OT_NULL || callback.value.GetType() == OT_CLOSURE);

  ComponentsInitializer compMap;
  SQInteger res = sq_obj_to_comps(vm, compMapObj, compMap);

  if (SQ_FAILED(res))
    return res;

  ecs::EntityId eid = ecs::INVALID_ENTITY_ID;
  if (sync || callback.value.IsNull())
  {
    if (sync)
      eid = mgr.value->createEntitySync(tplName.value, eastl::move(compMap));
    else
      eid = mgr.value->createEntityAsync(tplName.value, eastl::move(compMap));
  }
  else
  {
    int vmedSlotId = eastl::distance(vm_extra_data.begin(), vmit);
    auto cbRef = vmit->second.addPendingCreationCb(vmit->second.id, vmedSlotId, eastl::move(callback.value));
    eid = mgr.value->createEntityAsync(tplName.value, eastl::move(compMap),
      [cbRef = eastl::move(cbRef)](EntityId ceid) { cbRef.invoke(ceid); });
  }
  sq_pushinteger(vm, (ecs::entity_id_t)eid); // Warning: this eid can'not be used for get<> until creation callback is called!

  return 1;
}

static SQInteger create_entity(HSQUIRRELVM vm) { return create_entity_impl(vm, /*sync*/ false); }
static SQInteger create_entity_sync(HSQUIRRELVM vm) { return create_entity_impl(vm, /*sync*/ true); }


static SQInteger recreate_entity(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<EntityManager *>(vm))
    return SQ_ERROR;

  decltype(vm_extra_data)::value_type *vmit = nullptr;
  if (DAGOR_UNLIKELY(!is_vm_registered(vm, "Attempt to re-create entity in unknown (shutdowned?) vm", &vmit)))
    return SQ_ERROR;

  register_pending_es();
  Sqrat::Var<EntityManager *> mgr(vm, 1);
  Sqrat::Var<EntityId> eid(vm, 2);
  Sqrat::Var<const char *> tplName(vm, 3);
  Sqrat::Var<Sqrat::Object> compMapObj(vm, 4);
  Sqrat::Var<Sqrat::Object> callback(vm, 5);

  G_ASSERT(callback.value.GetType() == OT_NULL || callback.value.GetType() == OT_CLOSURE);

  ComponentsInitializer compMap;
  SQInteger res = sq_obj_to_comps(vm, compMapObj, compMap);

  if (SQ_FAILED(res))
    return res;

  if (callback.value.IsNull())
    mgr.value->reCreateEntityFromAsync(eid.value, tplName.value, eastl::move(compMap), ComponentsMap());
  else
  {
    int vmedSlotId = eastl::distance(vm_extra_data.begin(), vmit);
    auto cbRef = vmit->second.addPendingCreationCb(vmit->second.id, vmedSlotId, eastl::move(callback.value));
    mgr.value->reCreateEntityFromAsync(eid.value, tplName.value, eastl::move(compMap), {},
      [cbRef = eastl::move(cbRef)](EntityId ceid) { cbRef.invoke(ceid); });
  }

  return 0;
}


#define GET_OBJ_OR_LEAVE(vm, n, const_)                           \
  if (!Sqrat::check_signature<const_ ObjClass *>(vm, n))          \
    return SQ_ERROR;                                              \
  Sqrat::Var<const_ ObjClass *> objVar(vm, n);                    \
  G_ASSERT_RETURN(objVar.value, sq_throwerror(vm, "Bad 'this'")); \
  auto &obj = *objVar.value;


template <typename ObjClass, typename ArrClass, bool readonly>
static SQInteger comp_obj_get_all(HSQUIRRELVM vm, const ecs::Object &obj)
{
  sq_newtable(vm);
  for (auto &it : obj)
  {
    sq_pushstring(vm, it.first.data(), it.first.length());
    ecs::component_type_t ctype = it.second.getUserType();
    switch (ctype)
    {
      case ecs::ComponentTypeInfo<ecs::Object>::type:
        comp_obj_get_all<ObjClass, ArrClass, readonly>(vm, it.second.get<ecs::Object>());
        break;
      case ecs::ComponentTypeInfo<ecs::Array>::type:
        comp_arr_get_all<ObjClass, ArrClass, readonly>(vm, it.second.get<ecs::Array>());
        break;
#define DECL_LIST_TYPE(lt, t) \
  case ecs::ComponentTypeInfo<ecs::lt>::type: comp_list_get_all<ecs::lt, readonly>(vm, it.second.get<ecs::lt>()); break;
        ECS_DECL_LIST_TYPES
#undef DECL_LIST_TYPE
      default:
        push_comp_val(vm, it.first.data(), it.second.getEntityComponentRef(), readonly ? COMP_ACCESS_READONLY : COMP_ACCESS_READWRITE);
        break;
    }
    G_VERIFY(SQ_SUCCEEDED(sq_rawset(vm, -3)));
  }
  return 1;
}

template <bool readonly>
static SQInteger comp_is_readonly(HSQUIRRELVM vm)
{
  sq_pushbool(vm, readonly);
  return 1;
}

template <typename ItemType>
static SQInteger comp_item_type(HSQUIRRELVM vm)
{
  sq_pushstring(vm, "", -1);
  return 1;
}
#define DECL_LIST_TYPE(lt, t)                 \
  template <>                                 \
  SQInteger comp_item_type<t>(HSQUIRRELVM vm) \
  {                                           \
    sq_pushstring(vm, #t, -1);                \
    return 1;                                 \
  }
ECS_DECL_LIST_TYPES
#undef DECL_LIST_TYPE

template <typename ObjClass, typename ArrClass, bool readonly>
static SQInteger comp_obj_get_all(HSQUIRRELVM vm)
{
  GET_OBJ_OR_LEAVE(vm, 1, const);
  return comp_obj_get_all<ObjClass, ArrClass, readonly>(vm, obj);
}

template <typename ObjClass, bool readonly>
static SQInteger comp_obj_get(HSQUIRRELVM vm)
{
  GET_OBJ_OR_LEAVE(vm, 1, const);
  const char *key;
  G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, 2, &key)));
  auto it = obj.find_as(ECS_HASH_SLOW(key));
  if (it == obj.end())
  {
    sq_pushnull(vm);
    return sq_throwobject(vm);
  }

  push_comp_val(vm, key, it->second.getEntityComponentRef(), readonly ? COMP_ACCESS_READONLY : COMP_ACCESS_READWRITE);
  return 1;
}

template <typename ObjClass>
static SQInteger comp_obj_set(HSQUIRRELVM vm)
{
  GET_OBJ_OR_LEAVE(vm, 1, );
  const char *key;
  G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, 2, &key)));
  auto it = obj.find_as(ECS_HASH_SLOW(key));
  ecs::component_type_t expectedCompType = TYPE_NULL;
  if (it != obj.end())
    expectedCompType = it->second.getUserType();
  else
  {
    Sqrat::Var<Sqrat::Object> sqo(vm, 3);
    expectedCompType = ecs_type_from_sq_obj(sqo.value, key);
    G_ASSERT_RETURN(expectedCompType != TYPE_NULL, sq_throwerror(vm, "Can't determine ECS type"));
  }

  String errMsg;
  if (it != obj.end())
  {
    if (!pop_comp_val(vm, 3, it->second, key, expectedCompType, errMsg))
      return sq_throwerror(vm, errMsg);
  }
  else
  {
    if (!pop_comp_val(vm, 3, obj.insert(ECS_HASH_SLOW(key)), key, expectedCompType, errMsg))
      return sq_throwerror(vm, errMsg);
  }

  return 0;
}

template <typename ObjClass>
static SQInteger comp_obj_erase(HSQUIRRELVM vm)
{
  GET_OBJ_OR_LEAVE(vm, 1, );
  const char *key;
  G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, 2, &key)));

  auto it = obj.find_as(ECS_HASH_SLOW(key));
  bool ok = (it != obj.end());
  if (ok)
    obj.erase(it);

  sq_pushbool(vm, ok);
  return 1;
}

template <typename ObjClass>
static SQInteger comp_obj_nexti(HSQUIRRELVM vm)
{
  GET_OBJ_OR_LEAVE(vm, 1, const);

  if (sq_gettype(vm, 2) == OT_NULL) // initial iteration
  {
    auto it = obj.begin();
    if (it == obj.end())
      return 0;
    sq_pushstring(vm, it->first.c_str(), it->first.size());
  }
  else
  {
    const char *key;
    G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, 2, &key)));
    auto it = obj.find_as(ECS_HASH_SLOW(key));
    ++it;
    if (it == obj.end())
      return 0;
    sq_pushstring(vm, it->first.c_str(), it->first.size());
  }
  return 1;
}


#define GET_ARR_OR_LEAVE(vm, n, const_)                           \
  if (!Sqrat::check_signature<const_ ArrClass *>(vm, n))          \
    return SQ_ERROR;                                              \
  Sqrat::Var<const_ ArrClass *> objVar(vm, n);                    \
  G_ASSERT_RETURN(objVar.value, sq_throwerror(vm, "Bad 'this'")); \
  auto &arr = *objVar.value;

template <typename ArrClass, bool readonly>
static SQInteger comp_arr_get(HSQUIRRELVM vm)
{
  GET_ARR_OR_LEAVE(vm, 1, const);
  if (!(sq_gettype(vm, 2) & SQOBJECT_NUMERIC))
  {
    sq_pushnull(vm);
    return sq_throwobject(vm);
  }

  SQInteger i;
  sq_getinteger(vm, 2, &i);
  if (i < 0 || (unsigned)i >= arr.size())
  {
    sq_pushnull(vm);
    return sq_throwobject(vm);
  }
  push_comp_val(vm, nullptr, SqBindingHelper::get_entity_component_ref(arr, i),
    readonly ? COMP_ACCESS_READONLY : COMP_ACCESS_READWRITE);
  return 1;
}

template <typename ArrClass>
static SQInteger comp_arr_set(HSQUIRRELVM vm)
{
  GET_ARR_OR_LEAVE(vm, 1, );
  SQInteger i = Sqrat::Var<SQInteger>(vm, 2).value;
  if (i < 0 || (unsigned)i >= arr.size())
  {
    String errMsg(32, "Index %d out of bounds %d", (int)i, (int)arr.size());
    return sq_throwerror(vm, errMsg);
  }
  String errMsg;
  ecs::ChildComponent &comp = arr[(int)i];
  if (!pop_comp_val(vm, 3, comp, nullptr, comp.getUserType(), errMsg))
    return sq_throwerror(vm, errMsg);
  return 0;
}

template <typename ArrClass>
static SQInteger comp_list_set(HSQUIRRELVM vm)
{
  GET_ARR_OR_LEAVE(vm, 1, );
  SQInteger i = Sqrat::Var<SQInteger>(vm, 2).value;
  if (i < 0 || (unsigned)i >= arr.size())
  {
    String errMsg(32, "Index %d out of bounds %d", (int)i, (int)arr.size());
    return sq_throwerror(vm, errMsg);
  }
  String errMsg;
  EntityComponentRef compRef = SqBindingHelper::get_entity_component_ref(arr, i);
  return pop_component_val(vm, 3, compRef, nullptr, errMsg) ? 0 : sq_throwerror(vm, errMsg);
}

template <typename ArrClass>
static SQInteger comp_arr_nexti(HSQUIRRELVM vm)
{
  GET_ARR_OR_LEAVE(vm, 1, const);
  Sqrat::Object prevIdx = Sqrat::Var<Sqrat::Object>(vm, 2).value;
  if (prevIdx.IsNull()) // initial iteration
  {
    if (arr.empty())
      sq_pushnull(vm);
    else
      sq_pushinteger(vm, 0);
  }
  else
  {
    SQInteger nextI = prevIdx.Cast<SQInteger>() + 1;
    if (nextI < arr.size())
      sq_pushinteger(vm, nextI);
    else
      sq_pushnull(vm);
  }
  return 1;
}

template <typename T>
static SQInteger comp_t_len(const T *ptr)
{
  return ptr->size();
}

template <typename ArrClass>
static void comp_arr_clear(ArrClass *arr)
{
  arr->clear();
}

template <typename ArrClass>
static SQInteger comp_arr_append(HSQUIRRELVM vm)
{
  GET_ARR_OR_LEAVE(vm, 1, );

  ecs::component_type_t expectedCompType = 0;
  if (sq_gettop(vm) > 2)
    expectedCompType = Sqrat::Var<ecs::component_type_t>(vm, 3).value;
  else
  {
    Sqrat::Var<Sqrat::Object> sqo(vm, 2);
    expectedCompType = ecs_type_from_sq_obj(sqo.value);
  }

  ecs::ChildComponent comp;
  String errMsg;
  if (!pop_comp_val(vm, 2, comp, nullptr, expectedCompType, errMsg))
    return sq_throwerror(vm, errMsg);

  arr.push_back(eastl::move(comp));

  sq_settop(vm, 1);
  return 1;
}

template <typename ArrClass>
static SQInteger comp_arr_insert(HSQUIRRELVM vm)
{
  GET_ARR_OR_LEAVE(vm, 1, );
  SQInteger i = Sqrat::Var<SQInteger>(vm, 2).value;

  ecs::component_type_t expectedCompType = 0;
  if (sq_gettop(vm) > 3)
    expectedCompType = Sqrat::Var<ecs::component_type_t>(vm, 4).value;
  else
  {
    Sqrat::Var<Sqrat::Object> sqo(vm, 3);
    expectedCompType = ecs_type_from_sq_obj(sqo.value);
  }

  ecs::ChildComponent comp;
  String errMsg;
  if (!pop_comp_val(vm, 3, comp, nullptr, expectedCompType, errMsg))
    return sq_throwerror(vm, errMsg);

  if (i < 0 || i > arr.size())
  {
    String errMsg(32, "Index %d out of bounds %d", (int)i, (int)arr.size());
    return sq_throwerror(vm, errMsg);
  }
  arr.insert(arr.begin() + i, eastl::move(comp));

  sq_settop(vm, 1);
  return 1;
}

static SQInteger pop_child_component(HSQUIRRELVM vm, ecs::ChildComponent &comp)
{
  ecs::component_type_t expectedCompType = 0;
  if (sq_gettop(vm) > 2)
    expectedCompType = Sqrat::Var<ecs::component_type_t>(vm, 3).value;
  else
  {
    Sqrat::Var<Sqrat::Object> sqo(vm, 2);
    expectedCompType = ecs_type_from_sq_obj(sqo.value);
  }

  String errMsg;
  if (!pop_comp_val(vm, 2, comp, nullptr, expectedCompType, errMsg))
    return sq_throwerror(vm, errMsg);

  return 1;
}

template <typename ArrClass>
static SQInteger comp_arr_indexof(HSQUIRRELVM vm)
{
  GET_ARR_OR_LEAVE(vm, 1, const);

  ecs::ChildComponent comp;
  if (SQ_FAILED(pop_child_component(vm, comp)))
    return SQ_ERROR;

  for (const ChildComponent &elem : arr)
    if (elem == comp)
    {
      sq_pushinteger(vm, eastl::distance(arr.begin(), &elem));
      return 1;
    }

  return 0;
}

template <typename ArrClass, typename ItemType>
static SQInteger comp_list_indexof(HSQUIRRELVM vm)
{
  GET_ARR_OR_LEAVE(vm, 1, const);

  ecs::ChildComponent comp;
  if (SQ_FAILED(pop_child_component(vm, comp)))
    return SQ_ERROR;

  if (!comp.is<ItemType>())
    return sq_throwerror(vm, "Type mismatch!");

  const ItemType &val = comp.get<ItemType>();

  for (const ItemType &elem : arr)
    if (elem == val)
    {
      sq_pushinteger(vm, eastl::distance(arr.begin(), &elem));
      return 1;
    }

  return 0;
}


template <typename ArrClass>
static SQInteger comp_arr_remove(HSQUIRRELVM vm)
{
  GET_ARR_OR_LEAVE(vm, 1, );

  SQInteger idx;
  G_VERIFY(SQ_SUCCEEDED(sq_getinteger(vm, 2, &idx)));

  if (idx < 0 || idx >= arr.size())
    return sq_throwerror(vm, "Invalid index");

  arr.erase(arr.begin() + idx);
  return 0;
}


template <typename ArrClass>
static SQInteger comp_arr_pop(HSQUIRRELVM vm)
{
  GET_ARR_OR_LEAVE(vm, 1, );

  if (arr.empty())
    return sq_throwerror(vm, "Array is empty");

  arr.pop_back();
  return 0;
}

template <typename ObjClass, typename ArrClass, bool readonly>
static SQInteger comp_arr_get_all(HSQUIRRELVM vm, const ecs::Array &arr)
{
  auto len = arr.size();
  sq_newarray(vm, len);
  for (int i = 0; i < len; ++i)
  {
    sq_pushinteger(vm, i);
    ecs::component_type_t type = arr[i].getUserType();
    switch (type)
    {
      case ecs::ComponentTypeInfo<ecs::Object>::type:
        comp_obj_get_all<ObjClass, ArrClass, readonly>(vm, arr[i].get<ecs::Object>());
        break;
      case ecs::ComponentTypeInfo<ecs::Array>::type:
        comp_arr_get_all<ObjClass, ArrClass, readonly>(vm, arr[i].get<ecs::Array>());
        break;
#define DECL_LIST_TYPE(lt, t) \
  case ecs::ComponentTypeInfo<ecs::lt>::type: comp_list_get_all<ecs::lt, readonly>(vm, arr[i].get<ecs::lt>()); break;
        ECS_DECL_LIST_TYPES
#undef DECL_LIST_TYPE
      default:
        push_comp_val(vm, "n/a", arr[i].getEntityComponentRef(), readonly ? COMP_ACCESS_READONLY : COMP_ACCESS_READWRITE);
        break;
    }
    G_VERIFY(SQ_SUCCEEDED(sq_rawset(vm, -3)));
  }
  return 1;
}

template <typename ArrClass, bool readonly>
static SQInteger comp_list_get_all(HSQUIRRELVM vm, const ArrClass &arr)
{
  auto len = arr.size();
  sq_newarray(vm, len);
  for (int i = 0; i < len; ++i)
  {
    sq_pushinteger(vm, i);
    push_comp_val(vm, "n/a", SqBindingHelper::get_entity_component_ref(arr, i),
      readonly ? COMP_ACCESS_READONLY : COMP_ACCESS_READWRITE);
    G_VERIFY(SQ_SUCCEEDED(sq_rawset(vm, -3)));
  }
  return 1;
}

template <typename ObjClass, typename ArrClass, bool readonly>
static SQInteger comp_arr_get_all(HSQUIRRELVM vm)
{
  GET_ARR_OR_LEAVE(vm, 1, const);
  return comp_arr_get_all<ObjClass, ArrClass, readonly>(vm, arr);
}

template <typename ArrClass, bool readonly>
static SQInteger comp_list_get_all(HSQUIRRELVM vm)
{
  GET_ARR_OR_LEAVE(vm, 1, const);
  return comp_list_get_all<ArrClass, readonly>(vm, arr);
}

// NB: edit same define in aotEcsEvents.h
// NB: list without ecs::Object!
#define DAS_EVENT_ECS_CONT_LIST_TYPES      \
  DECL_LIST_TYPE(IntList, "IntList")       \
  DECL_LIST_TYPE(FloatList, "FloatList")   \
  DECL_LIST_TYPE(Point3List, "Point3List") \
  DECL_LIST_TYPE(Point4List, "Point4List")
// NB: add new types carefully, every new type has performance cost

namespace sq
{
SQInteger push_event_field(HSQUIRRELVM vm, ecs::Event *event, ecs::EventsDB::event_id_t event_id, int idx)
{
  const ecs::EventsDB &eventsDB = g_entity_mgr->getEventsDb();
  const ecs::component_type_t fieldType = eventsDB.getFieldType(event_id, idx);
  switch (fieldType)
  {
    case ecs::ComponentTypeInfo<ecs::string>::type:
      sq_pushstring(vm, eventsDB.getEventFieldValue<const char *>(*event, event_id, idx), -1);
      break;
    case ecs::ComponentTypeInfo<ecs::EntityId>::type:
      sq_pushinteger(vm, (ecs::entity_id_t)eventsDB.getEventFieldValue<const ecs::EntityId>(*event, event_id, idx));
      break;
    case ecs::ComponentTypeInfo<int>::type: sq_pushinteger(vm, eventsDB.getEventFieldValue<int>(*event, event_id, idx)); break;
    case ecs::ComponentTypeInfo<int8_t>::type: sq_pushinteger(vm, eventsDB.getEventFieldValue<int8_t>(*event, event_id, idx)); break;
    case ecs::ComponentTypeInfo<uint8_t>::type: sq_pushinteger(vm, eventsDB.getEventFieldValue<uint8_t>(*event, event_id, idx)); break;
    case ecs::ComponentTypeInfo<int16_t>::type: sq_pushinteger(vm, eventsDB.getEventFieldValue<int16_t>(*event, event_id, idx)); break;
    case ecs::ComponentTypeInfo<uint16_t>::type:
      sq_pushinteger(vm, eventsDB.getEventFieldValue<uint16_t>(*event, event_id, idx));
      break;
    case ecs::ComponentTypeInfo<uint32_t>::type:
      sq_pushinteger(vm, eventsDB.getEventFieldValue<uint32_t>(*event, event_id, idx));
      break;
    case ecs::ComponentTypeInfo<int64_t>::type: sq_pushinteger(vm, eventsDB.getEventFieldValue<int64_t>(*event, event_id, idx)); break;
    case ecs::ComponentTypeInfo<uint64_t>::type:
      sq_pushinteger(vm, eventsDB.getEventFieldValue<uint64_t>(*event, event_id, idx));
      break;
    case ecs::ComponentTypeInfo<float>::type: sq_pushfloat(vm, eventsDB.getEventFieldValue<float>(*event, event_id, idx)); break;
    case ecs::ComponentTypeInfo<bool>::type: sq_pushbool(vm, eventsDB.getEventFieldValue<bool>(*event, event_id, idx)); break;

#define COMMON_COMP(TypeName)                                                                     \
  case ecs::ComponentTypeInfo<TypeName>::type:                                                    \
    Sqrat::Var<TypeName>::push(vm, eventsDB.getEventFieldValue<TypeName>(*event, event_id, idx)); \
    break
      COMMON_LIST
#undef COMMON_COMP

    case ecs::ComponentTypeInfo<ecs::Object>::type:
    {
      ecs::Object **field = (ecs::Object **)eventsDB.getEventFieldRawValue(*event, event_id, idx);
      if (field)
        Sqrat::PushVar(vm, *field);
      else
        sq_pushnull(vm);
      break;
    }

#define PUSH_LIST_VAL(T)                                                                                 \
  case ecs::ComponentTypeInfo<T>::type:                                                                  \
  {                                                                                                      \
    T **list = (T **)eventsDB.getEventFieldRawValue(*event, event_id, idx);                              \
    if (list && *list)                                                                                   \
      push_array_val<T, T##RO>(vm, (*list)->getEntityComponentRef(g_entity_mgr), COMP_ACCESS_READWRITE); \
    else                                                                                                 \
      sq_pushnull(vm);                                                                                   \
    break;                                                                                               \
  }

#define DECL_LIST_TYPE(lt, t) PUSH_LIST_VAL(ecs::lt)
      DAS_EVENT_ECS_CONT_LIST_TYPES
#undef DECL_LIST_TYPE
#undef PUSH_LIST_VAL

    case TYPE_NULL:
    default:
      auto res = native_component_bindings.find(fieldType);
      if (res != native_component_bindings.end())
      {
        res->second(vm, eventsDB.getEventFieldRawValue(*event, event_id, idx));
        break;
      }
      logerr("Unexpected field %s <0X%X> of event %s <0X%X>", eventsDB.getFieldName(event_id, idx), fieldType,
        eventsDB.getEventName(event_id), event->getType());
      sq_pushnull(vm);
      break;
  }
  return 1;
}

bool pop_event_field(ecs::event_type_t event_type, Sqrat::Object slot, const ecs::string &field_name, ecs::component_type_t type,
  uint8_t offset, char *evtData)
{
  switch (type)
  {
    case ecs::ComponentTypeInfo<ecs::string>::type:
    {
      auto val = slot.Cast<eastl::string>();
      char **res = (char **)(evtData + offset);
      *res = str_dup(val.c_str(), strmem);
      break;
    }
    case ecs::ComponentTypeInfo<int>::type:
    case ecs::ComponentTypeInfo<ecs::EntityId>::type: *(int *)(evtData + offset) = slot.Cast<int32_t>(); break;
    case ecs::ComponentTypeInfo<int8_t>::type: *(int8_t *)(evtData + offset) = slot.Cast<int8_t>(); break;
    case ecs::ComponentTypeInfo<uint8_t>::type: *(uint8_t *)(evtData + offset) = slot.Cast<uint8_t>(); break;
    case ecs::ComponentTypeInfo<int16_t>::type: *(int16_t *)(evtData + offset) = slot.Cast<int16_t>(); break;
    case ecs::ComponentTypeInfo<uint16_t>::type: *(uint16_t *)(evtData + offset) = slot.Cast<uint16_t>(); break;
    case ecs::ComponentTypeInfo<uint32_t>::type: *(uint32_t *)(evtData + offset) = slot.Cast<uint32_t>(); break;
    case ecs::ComponentTypeInfo<int64_t>::type: *(int64_t *)(evtData + offset) = slot.Cast<int64_t>(); break;
    case ecs::ComponentTypeInfo<uint64_t>::type: *(uint64_t *)(evtData + offset) = slot.Cast<uint64_t>(); break;
    case ecs::ComponentTypeInfo<float>::type: *(float *)(evtData + offset) = slot.Cast<float>(); break;
    case ecs::ComponentTypeInfo<bool>::type: *(bool *)(evtData + offset) = slot.Cast<bool>(); break;

#define COMMON_COMP(TypeName) \
  case ecs::ComponentTypeInfo<TypeName>::type: *(TypeName *)(evtData + offset) = slot.Cast<TypeName>(); break
      COMMON_LIST
#undef COMMON_COMP

    case ecs::ComponentTypeInfo<ecs::Object>::type:
      if (slot.GetType() == OT_INSTANCE)
      {
        auto hObj = slot.GetObject();
        if (Sqrat::ClassType<ecs::Object>::IsObjectOfClass(&hObj))
        {
          auto objVar = slot.GetVar<ecs::Object *>();
          *(ecs::Object **)(evtData + offset) = new ecs::Object(*objVar.value);
          return true;
        }
        else if (Sqrat::ClassType<ecs::ObjectRO>::IsObjectOfClass(&hObj))
        {
          auto objVar = slot.GetVar<ecs::ObjectRO *>();
          *(ecs::Object **)(evtData + offset) = new ecs::Object(*static_cast<ecs::Object *>(objVar.value));
          return true;
        }
        logerr("Unexpected field %s <0X%X> of event %s <0X%X>", field_name.c_str(), type,
          g_entity_mgr->getEventsDb().findEventName(event_type), event_type);
        return false;
      }
      // TODO:
      // if (slot.GetType() == OT_TABLE)
      else
        logerr("Unexpected field %s <0X%X> of event %s <0X%X>", field_name.c_str(), type,
          g_entity_mgr->getEventsDb().findEventName(event_type), event_type);
      return false;

#define POP_LIST_VAL(T)                                                                 \
  case ecs::ComponentTypeInfo<T>::type:                                                 \
    if (slot.GetType() == OT_INSTANCE)                                                  \
    {                                                                                   \
      auto hObj = slot.GetObject();                                                     \
      if (Sqrat::ClassType<T>::IsObjectOfClass(&hObj))                                  \
      {                                                                                 \
        auto objVar = slot.GetVar<T *>();                                               \
        *(T **)(evtData + offset) = new T(*objVar.value);                               \
        return true;                                                                    \
      }                                                                                 \
      else if (Sqrat::ClassType<T##RO>::IsObjectOfClass(&hObj))                         \
      {                                                                                 \
        auto objVar = slot.GetVar<T##RO *>();                                           \
        *(T **)(evtData + offset) = new T(*static_cast<T *>(objVar.value));             \
        return true;                                                                    \
      }                                                                                 \
      logerr("Unexpected field %s <0X%X> of event %s <0X%X>", field_name.c_str(), type, \
        g_entity_mgr->getEventsDb().findEventName(event_type), event_type);             \
      return false;                                                                     \
    }                                                                                   \
    else                                                                                \
      logerr("Unexpected field %s <0X%X> of event %s <0X%X>", field_name.c_str(), type, \
        g_entity_mgr->getEventsDb().findEventName(event_type), event_type);             \
    return false;

#define DECL_LIST_TYPE(lt, t) POP_LIST_VAL(ecs::lt)
      DAS_EVENT_ECS_CONT_LIST_TYPES
#undef DECL_LIST_TYPE
#undef POP_LIST_VAL

    default:
    {
      logerr("Unexpected field %s <0X%X> of event %s <0X%X>", field_name.c_str(), type,
        g_entity_mgr->getEventsDb().findEventName(event_type), event_type);
      return false;
    }
  }
  return true;
}
} // namespace sq

} // namespace ecs

namespace bind_dascript
{
uint8_t get_dasevent_routing(ecs::event_type_t type);
extern const uint8_t DASEVENT_NATIVE_ROUTING;
extern const uint8_t DASEVENT_NO_ROUTING;
} // namespace bind_dascript

static void emgr_dispatch_event(ecs::EntityManager *mgr, ecs::EntityId eid, ecs::Event &evt)
{
#if DAGOR_DBGLEVEL > 0
  if (!squirrelEvents.count(evt.getType()) && !eventTypeNative2Sq.count(evt.getType()))
    logerr("send event <%s> is not regiresterd as Squirrel event", evt.getName());

  const uint8_t routing = bind_dascript::get_dasevent_routing(evt.getType());
  if (routing != bind_dascript::DASEVENT_NATIVE_ROUTING && routing != bind_dascript::DASEVENT_NO_ROUTING)
    logerr("send event <%s> is probably net event and should be send using sendNetEvent/broadcastNetEvent.", evt.getName());
#endif
  register_pending_es();
  if (DAGOR_LIKELY(ecs::is_schemeless_event(evt)))
    mgr->dispatchEvent(eid, eastl::move((ecs::SchemelessEvent &)evt));
  else
    mgr->dispatchEvent(eid, evt);
}

static SQInteger emgr_dispatch_event(HSQUIRRELVM vm, bool bcast)
{
  if (!Sqrat::check_signature<EntityManager *>(vm, 1))
    return SQ_ERROR;
  if (!Sqrat::check_signature<ecs::Event *>(vm, bcast ? 2 : 3))
    return SQ_ERROR;
  if (DAGOR_UNLIKELY(!is_vm_registered(vm, "Attempt to send|bcast event in unknown (shutdowned?) vm")))
    return SQ_ERROR;
  Sqrat::Var<EntityManager *> mgr(vm, 1);
  SQInteger eid = ecs::ECS_INVALID_ENTITY_ID_VAL;
  if (!bcast)
  {
    sq_getinteger(vm, 2, &eid);
    if (eid == ecs::ECS_INVALID_ENTITY_ID_VAL)
      return 0;
  }
  Sqrat::Var<ecs::Event *> evt(vm, bcast ? 2 : 3);
  emgr_dispatch_event(mgr.value, ecs::EntityId(ecs::entity_id_t(eid)), *evt.value);
  return 0;
}
static SQInteger emgr_send_event(HSQUIRRELVM vm) { return emgr_dispatch_event(vm, /*bcast*/ false); }
static SQInteger emgr_bcast_event(HSQUIRRELVM vm) { return emgr_dispatch_event(vm, /*bcast*/ true); }

static const ecs::TemplateDB *emgr_get_tempate_db(ecs::EntityManager *mgr) { return &mgr->getTemplateDB(); }
static const ecs::EventsDB *emgr_get_events_db(ecs::EntityManager *mgr) { return &mgr->getEventsDb(); }

static ecs::component_type_t emgr_get_component_type(ecs::EntityManager *mgr, const char *component_name)
{
  ecs::component_index_t cidx = mgr->getDataComponents().findComponentId(ECS_HASH_SLOW(component_name).hash);
  if (cidx == ecs::INVALID_COMPONENT_INDEX)
    return 0;
  ecs::DataComponent component = mgr->getDataComponents().getComponentById(cidx);
#if DAECS_EXTENSIVE_CHECKS
  if (strcmp(component_name, mgr->getDataComponents().getComponentNameById(cidx)) != 0)
    logerr("component name hash collision <%s> with <%s>", component_name, mgr->getDataComponents().getComponentNameById(cidx));
#endif
  return component.componentTypeName;
}
static ecs::component_type_t get_semantic_type(const char *type_name) { return ECS_HASH_SLOW(type_name).hash; }

static const char *get_component_name_by_idx(ecs::component_index_t cidx)
{
  return g_entity_mgr ? g_entity_mgr->getDataComponents().getComponentNameById(cidx) : nullptr;
}

static const char *get_component_name_by_hash(ecs::component_t name_hash)
{
  return g_entity_mgr ? g_entity_mgr->getDataComponents().findComponentName(name_hash) : nullptr;
}

static const char *emgr_get_component_type_name(ecs::EntityManager *mgr, const char *component_name)
{
  ecs::component_index_t cidx = mgr->getDataComponents().findComponentId(ECS_HASH_SLOW(component_name).hash);
  if (cidx == ecs::INVALID_COMPONENT_INDEX)
    return nullptr;
  ecs::DataComponent component = mgr->getDataComponents().getComponentById(cidx);
#if DAECS_EXTENSIVE_CHECKS
  if (strcmp(component_name, mgr->getDataComponents().getComponentNameById(cidx)) != 0)
    logerr("component name hash collision <%s> with <%s>", component_name, mgr->getDataComponents().getComponentNameById(cidx));
#endif
  return mgr->getComponentTypes().getTypeNameById(component.componentType);
}

static const char *emgr_get_type_name(ecs::EntityManager *mgr, ecs::component_type_t name)
{
  return mgr->getComponentTypes().findTypeName(name);
}

static const ecs::Template *tpldb_get_template_by_name(const ecs::TemplateDB *db, const char *name)
{
  return db->getTemplateByName(name);
}
static uint32_t tpldb_get_size(const ecs::TemplateDB *db) { return db->size(); }

static const DataBlock *tpldb_get_template_meta_info(const ecs::TemplateDB *db, const char *templ_name)
{
  return db->info().getTemplateMetaInfo(templ_name);
}
static const DataBlock *tpldb_get_component_meta_info(const ecs::TemplateDB *db, const char *comp_name)
{
  return db->info().getComponentMetaInfo(comp_name);
}
static bool tpldb_has_component_meta_info(const ecs::TemplateDB *db, const char *templ_name, const char *comp_name)
{
  return db->info().hasComponentMetaInfo(templ_name, comp_name);
}


static SQInteger evtdb_get_event_field(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<ecs::Event *>(vm, 2))
    return SQ_ERROR;
  Sqrat::Var<ecs::Event *> evt(vm, 2);
  Sqrat::Var<int> eventId(vm, 3);
  Sqrat::Var<int> index(vm, 4);
  return ecs::sq::push_event_field(vm, evt.value, eventId.value, index.value);
}

static SQInteger create_template(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<EntityManager *>(vm))
    return SQ_ERROR;

  Sqrat::Var<EntityManager *> mgr(vm, 1);
  Sqrat::Var<const char *> tplName(vm, 2);
  Sqrat::Var<Sqrat::Object> compMapObj(vm, 3);

  Sqrat::Var<bool> isSingletone(vm, 4);

  const SQChar *extends = nullptr; //== probably list of extends?
  SQInteger sz = 0;
  if (sq_gettop(vm) >= 5)
    sq_getstringandsize(vm, 5, &extends, &sz);

  ComponentsMap compMap;
  SQInteger res = sq_obj_to_comp_map(vm, compMapObj, compMap);

  if (SQ_FAILED(res))
    return res;
  ecs::Template tmpl = Template(tplName.value, eastl::move(compMap), Template::component_set{}, Template::component_set{},
    Template::component_set{}, isSingletone.value);
  dag::ConstSpan<const char *> extendsList(&extends, 1);
  auto result = mgr.value->addTemplate(eastl::move(tmpl), extends ? &extendsList : nullptr);
  sq_pushinteger(vm, result);
  return 1;
}

static int template_get_num_parents(const ecs::Template *templ) { return templ->getParents().size(); }

static const ecs::Template *template_get_parent(const ecs::Template *templ, int index)
{
  if (index < 0 || index >= templ->getParents().size())
    return nullptr;
  const auto templ_id = templ->getParents()[index];
  return g_entity_mgr->getTemplateDB().getTemplateById(templ_id);
}

static SQInteger template_get_components_names(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<ecs::Template *>(vm))
    return SQ_ERROR;

  const ecs::Template *pTemplate = Sqrat::Var<const ecs::Template *>(vm, 1).value;
  if (!pTemplate)
    return SQ_ERROR;

  eastl::vector<eastl::string> names;
  const auto &db = g_entity_mgr->getTemplateDB();
  const auto &comps = pTemplate->getComponentsMap();
  for (const auto &c : comps)
  {
    const char *cname = db.getComponentName(c.first);
    if (cname)
      names.push_back(cname);
  }

  int idx = 0;
  Sqrat::Array res(vm, names.size());
  for (const auto &name : names)
    res.SetValue(SQInteger(idx++), name.c_str());

  Sqrat::Var<Sqrat::Array>::push(vm, res);
  return 1;
}

static SQInteger template_get_tags(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<ecs::Template *>(vm))
    return SQ_ERROR;

  const ecs::Template *pTemplate = Sqrat::Var<const ecs::Template *>(vm, 1).value;
  if (!pTemplate)
    return SQ_ERROR;

  eastl::vector<eastl::string> tags;

  const auto &db = g_entity_mgr->getTemplateDB();
  const ecs::component_type_t tagHash = ECS_HASH("ecs::Tag").hash;
  HashedKeySet<ecs::component_t> added;

  const auto &dbdata = g_entity_mgr->getTemplateDB().data();
  dbdata.iterate_template_parents(*pTemplate, [&](const ecs::Template &tmpl) {
    const auto &comps = tmpl.getComponentsMap();
    for (const auto &c : comps)
    {
      auto tp = db.getComponentType(c.first);
      if (tp == tagHash && !added.has_key(c.first))
      {
        added.insert(c.first);
        const char *compName = db.getComponentName(c.first);
        if (compName)
          tags.push_back(compName);
      }
    }
  });

  eastl::sort(tags.begin(), tags.end());

  int idx = 0;
  Sqrat::Array res(vm, tags.size());
  for (const auto &tag : tags)
    res.SetValue(SQInteger(idx++), tag.c_str());

  Sqrat::Var<Sqrat::Array>::push(vm, res);
  return 1;
}

static SQInteger template_get_comp_val(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<ecs::Template *>(vm))
    return SQ_ERROR;

  ecs::Template *tpl = Sqrat::Var<ecs::Template *>(vm, 1).value;
  G_ASSERT_RETURN(tpl, sq_throwerror(vm, "Bad 'this'"));
  Sqrat::Var<const char *> name(vm, 2);
  const ChildComponent &comp = tpl->getComponent(ECS_HASH_SLOW(name.value), g_entity_mgr->getTemplateDB().data());
  push_comp_val(vm, name.value, comp.getEntityComponentRef(), COMP_ACCESS_READONLY);
  return 1;
}

static SQInteger template_get_comp_val_nullable(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<ecs::Template *>(vm))
    return SQ_ERROR;

  ecs::Template *tpl = Sqrat::Var<ecs::Template *>(vm, 1).value;
  G_ASSERT_RETURN(tpl, sq_throwerror(vm, "Bad 'this'"));
  Sqrat::Var<const char *> name(vm, 2);
  const ChildComponent &comp = tpl->getComponent(ECS_HASH_SLOW(name.value), g_entity_mgr->getTemplateDB().data());
  if (comp.isNull())
    sq_pushnull(vm);
  else
    push_comp_val(vm, name.value, comp.getEntityComponentRef(), COMP_ACCESS_READONLY);
  return 1;
}

static bool template_has_comp(const ecs::Template *tpl, const char *name)
{
  return tpl->hasComponent(ECS_HASH_SLOW(name), g_entity_mgr->getTemplateDB().data());
}

static uint32_t get_comp_flags(ecs::EntityId eid, const char *comp)
{
  if (const char *templ_name = g_entity_mgr->getEntityTemplateName(eid))
    if (auto *templ = g_entity_mgr->getTemplateDB().getTemplateByName(templ_name))
      return templ->getRegExpInheritedFlags(ECS_HASH_SLOW(comp), g_entity_mgr->getTemplateDB().data());
  return 0;
}

class SqQuery
{
public:
  SqQuery(const Sqrat::Object &qname, const Sqrat::Object &comps_desc, const char *filter_string);
  ~SqQuery()
  {
    G_ASSERT_RETURN(g_entity_mgr, );
    g_entity_mgr->destroyQuery(qid);
  }

  template <int args_start>
  static SQInteger perform(HSQUIRRELVM vm);
  static SQInteger script_ctor(HSQUIRRELVM vm);

  const char *debugGetQueryName() const
  {
#if DAECS_EXTENSIVE_CHECKS
    return g_entity_mgr->getQueryName(qid);
#else
    return nullptr;
#endif
  }

private:
  bool parseFilter();

private:
  QueryId qid;
  CompListDescHolder compListDesc;

  eastl::string filterString;
  ExpressionTree filterExpr;
  bool filterParsed = false;
};


SqQuery::SqQuery(const Sqrat::Object &qname, const Sqrat::Object &comps_desc, const char *filter_string)
{
  const char *sqname;
  if (DAGOR_LIKELY(qname.GetType() == OT_STRING))
    sqname = qname.GetVar<const SQChar *>().value; // Note: string ref is held by calling code
  else
  {
    sqname = "script_func";
    logerr("Unexpected type for SqQuery's name (first arg) - expected string, got %d", (int)qname.GetType());
    dump_sq_callstack_in_log(comps_desc.GetVM());
  }

  Sqrat::Array compsRw = comps_desc.RawGetSlot("comps_rw");
  Sqrat::Array compsRo = comps_desc.RawGetSlot("comps_ro");
  Sqrat::Array compsRq = comps_desc.RawGetSlot("comps_rq");
  Sqrat::Array compsNo = comps_desc.RawGetSlot("comps_no");

  String errMsg;
  if (!load_component_desc(compsRw, compListDesc.componentsRw, compListDesc.esCompsRw, errMsg) ||
      !load_component_desc(compsRo, compListDesc.componentsRo, compListDesc.esCompsRo, errMsg) ||
      !load_component_desc(compsRq, compListDesc.componentsRq, compListDesc.esCompsRq, errMsg) ||
      !load_component_desc(compsNo, compListDesc.componentsNo, compListDesc.esCompsNo, errMsg))
  {
    logerr("SqQuery '%s' components description error: %s", sqname, errMsg.c_str());
    dump_sq_callstack_in_log(comps_desc.GetVM());
  }

  if (
    compListDesc.esCompsRo.size() + compListDesc.esCompsRw.size() + compListDesc.esCompsRq.size() + compListDesc.esCompsNo.size() > 0)
    compListDesc.esCompsRo.push_back(ecs::ComponentDesc(ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>(), 0));
  // else
  //   return 0;

  qid = g_entity_mgr->createQuery(
    NamedQueryDesc(sqname, make_span((const ecs::ComponentDesc *)compListDesc.esCompsRw.data(), compListDesc.esCompsRw.size()),
      make_span((const ecs::ComponentDesc *)compListDesc.esCompsRo.data(), compListDesc.esCompsRo.size()),
      make_span((const ecs::ComponentDesc *)compListDesc.esCompsRq.data(), compListDesc.esCompsRq.size()),
      make_span((const ecs::ComponentDesc *)compListDesc.esCompsNo.data(), compListDesc.esCompsNo.size())));

  if (filter_string)
    filterString = filter_string;

  filterParsed = false;
}

bool SqQuery::parseFilter()
{
  filterParsed = parse_query_filter(filterExpr, filterString.c_str(),
    make_span((const ecs::ComponentDesc *)compListDesc.esCompsRw.data(), compListDesc.esCompsRw.size()),
    make_span((const ecs::ComponentDesc *)compListDesc.esCompsRo.data(), compListDesc.esCompsRo.size()));
  return filterParsed;
}


template <int args_start>
SQInteger SqQuery::perform(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<SqQuery *>(vm))
    return SQ_ERROR;

  Sqrat::Var<SqQuery *> varSelf(vm, 1);

  int cbIdx = args_start, filterIdx = args_start + 1;
  bool isSingleEntity = false;
  ecs::EntityId singleEid = ecs::INVALID_ENTITY_ID;
  if (sq_gettype(vm, args_start) == OT_INTEGER)
  {
    isSingleEntity = true;
    ++cbIdx;
    ++filterIdx;
    SQInteger eidVal;
    G_VERIFY(SQ_SUCCEEDED(sq_getinteger(vm, args_start, &eidVal)));
    singleEid = ecs::EntityId(entity_id_t(eidVal));
  }

  HSQOBJECT hFuncObj;
  sq_getstackobj(vm, cbIdx, &hFuncObj);
  if (sq_type(hFuncObj) != OT_CLOSURE && sq_type(hFuncObj) != OT_NATIVECLOSURE)
    return sq_throwerror(vm, "Expected closure for query callback function");

  const char *filterStr = nullptr;
  if (sq_gettop(vm) >= filterIdx)
    if (sq_gettype(vm, filterIdx) != OT_NULL)
      G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, filterIdx, &filterStr)));

  struct QueryData
  {
    Sqrat::Function func;
    Sqrat::Table compTbl;
    ExpressionTree *filterToUse;
    Sqrat::Object scriptRes;
  } qd = {Sqrat::Function(vm, Sqrat::Object(vm), hFuncObj), comps_create_table(vm), nullptr};

  SqQuery &sqQuery = *varSelf.value;

  ExpressionTree localFilterExpr;
  if (filterStr)
  {
    auto compsRw = make_span((const ecs::ComponentDesc *)sqQuery.compListDesc.esCompsRw.data(), sqQuery.compListDesc.esCompsRw.size());
    auto compsRo = make_span((const ecs::ComponentDesc *)sqQuery.compListDesc.esCompsRo.data(), sqQuery.compListDesc.esCompsRo.size());
    bool fltOk = parse_query_filter(localFilterExpr, filterStr, compsRw, compsRo);
    if (!fltOk)
      return sq_throwerror(vm, "Failed to parse filter");
    qd.filterToUse = &localFilterExpr;
  }
  else
  {
    if (!sqQuery.filterParsed && !sqQuery.filterString.empty())
    {
      if (!sqQuery.parseFilter())
        return sq_throwerror(vm, "Failed to parse filter");
    }
    if (sqQuery.filterParsed)
      qd.filterToUse = &sqQuery.filterExpr;
  }

  auto queryCb = [&qd, &sqQuery](const ecs::QueryView &__restrict qv) {
    if (!qd.scriptRes.IsNull())
      return ecs::QueryCbResult::Stop;

    auto cb = [&qd](ecs::EntityId eid, Sqrat::Table &compTbl) -> CallbackResult {
      bool success = qd.func.Evaluate(eid, compTbl, qd.scriptRes);
      G_UNUSED(success);
#if DAECS_EXTENSIVE_CHECKS
      if (!success)
      {
        static bool shooted = false;
        if (!shooted)
        {
          dump_sq_callstack_in_log(compTbl.GetVM());
          logerr("Script ES query call failed");
          shooted = true;
        }
      }
#endif
      return qd.scriptRes.IsNull() ? CB_CONTINUE : CB_STOP;
    };

    if (qd.filterToUse && qd.filterToUse->nodes.size())
    {
      PerformExpressionTree tree(qd.filterToUse->nodes.data(), qd.filterToUse->nodes.size(), &qv);
      ecs_scripts_call(qd.compTbl, sqQuery.compListDesc, qv, cb, sqQuery.debugGetQueryName(),
        [&tree](const ecs::EntityId, int id) { return tree.performExpression(id) ? FILTER_PROCESS : FILTER_SKIP; });
    }
    else
      ecs_scripts_call(qd.compTbl, sqQuery.compListDesc, qv, cb, sqQuery.debugGetQueryName());
    return ecs::QueryCbResult::Continue;
  };

  if (isSingleEntity)
    ecs::perform_query(g_entity_mgr, singleEid, sqQuery.qid, queryCb);
  else
    ecs::perform_query(g_entity_mgr, sqQuery.qid, queryCb);

  sq_pushobject(vm, qd.scriptRes.GetObject());

  return 1;
}


SQInteger SqQuery::script_ctor(HSQUIRRELVM vm)
{
  if (!ecs::ecs_is_in_init_phase)
    return sq_throwerror(vm, "Queries can be created only during initialization");

  Sqrat::Var<Sqrat::Object> name(vm, 2);
  Sqrat::Var<Sqrat::Object> compsDesc(vm, 3);
  const char *filterString = nullptr;
  if (sq_gettop(vm) > 3)
    sq_getstring(vm, 4, &filterString);

  SqQuery *q = new SqQuery(name.value, compsDesc.value, filterString);
  Sqrat::ClassType<SqQuery>::SetManagedInstance(vm, 1, q);
  return 0;
}


static SQInteger readonly_method_error(HSQUIRRELVM vm) { return sq_throwerror(vm, "Cannot mutate readonly Object/Array"); }

// (const char *name, [int cast_type]) or (obsolete) (const char *name, [bool has_payload], [int cast_type])
static SQInteger register_sq_event(HSQUIRRELVM vm)
{
  int nArgs = sq_gettop(vm);
  Sqrat::Var<const char *> name(vm, 2);
  SQInteger castType = 0;
  if (nArgs > 2)
  {
    sq_getinteger(vm, (nArgs == 3) ? 3 : 4, &castType); // ignore obsolete has_payload (if 3 args passed)
    castType &= EVFLG_CASTMASK;
  }
  event_type_t hashName = ECS_HASH_SLOW(name.value).hash;
  if (g_entity_mgr)
  {
    debug("registered %ssq_event <%s|0x%X>",
      (castType & EVCAST_UNICAST) ? "unicast " : ((castType & EVCAST_BROADCAST) ? "broadcast " : ""), name.value, hashName);
    g_entity_mgr->getEventsDbMutable().registerEvent(hashName, sizeof(SchemelessEvent),
      ecs::EVFLG_DESTROY | ecs::EVFLG_SCHEMELESS | int(castType), name.value, &SchemelessEvent::destroy, &SchemelessEvent::move_out);
  }
  squirrelEvents.insert(hashName);
  sq_pushinteger(vm, hashName);
  return 1;
}

static SQInteger modify_es_list(HSQUIRRELVM vm)
{
  bool wasInInitPhase = ecs_is_in_init_phase;
  if (!wasInInitPhase)
    start_es_loading();

  sq_pushnull(vm); // 'this' for call
  SQRESULT res = sq_call(vm, 1, SQFalse, SQTrue);

  if (!wasInInitPhase)
    end_es_loading();

  return SQ_SUCCEEDED(res) ? 0 : -1;
}

static SQInteger get_schemeless_payload(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<ecs::SchemelessEvent *>(vm))
    return SQ_ERROR;

  Sqrat::Var<ecs::SchemelessEvent *> evt(vm, 1);
  ecs::Object &data = const_cast<ecs::Object &>(evt.value->getData());
  Sqrat::PushVar<ecs::Object &>(vm, data);
  return 1;
}

static SQInteger get_null(HSQUIRRELVM) { return 0; }

template <typename CT, typename T, typename ItemType, bool readonly = false>
static void bind_list_class(HSQUIRRELVM vm, Sqrat::Table &tbl, const char *type_name)
{
  CT sqListClass(vm, type_name);
  ///@class ecs/BaseList
  /**
    **This is not real class. Just all List classes has this methods!**

    Real classes are following:
      * CompIntList(RO)
      * CompUInt16List(RO)
      * CompStringList(RO)
      * CompEidList(RO)
      * CompFloatList(RO)
      * CompPoint2List(RO)
      * CompPoint3List(RO)
      * CompPoint4List(RO)
      * CompIPoint2List(RO)
      * CompIPoint3List(RO)
      * CompBoolList(RO)
      * CompTMatrixList(RO)
      * CompColorList(RO)
      * CompInt64List(RO)
      * CompUInt64List(RO)
  */
  sqListClass //
    .Ctor()
    .SquirrelFunc("_get", comp_arr_get<T, readonly>, 2, "x.")
    .SquirrelFunc("_nexti", comp_arr_nexti<T>, 2, "xi|o")
    .GlobalFunc("len", comp_t_len<T>)
    .SquirrelFunc("indexof", comp_list_indexof<T, ItemType>, -2, "x.i")
    .SquirrelFunc("getAll", comp_list_get_all<T, readonly>, 1, "x")
    .SquirrelFunc("isReadOnly", comp_is_readonly<readonly>, 1, "x")
    .SquirrelFunc("listType", comp_item_type<ItemType>, 1, "x")
    /**/;

  if (readonly)
    ///@class ecs/ListRO
    ///@extends ecs.BaseList
    sqListClass //
      .SquirrelFunc("_set", readonly_method_error, 3, "xi.")
      .SquirrelFunc("append", readonly_method_error, -2, "x.i")
      .SquirrelFunc("insert", readonly_method_error, -3, "xi.i")
      .SquirrelFunc("remove", readonly_method_error, 2, "xi")
      .SquirrelFunc("pop", readonly_method_error, 1, "x")
      .SquirrelFunc("clear", readonly_method_error, 1, "x")
      /**/;
  else
    ///@class ecs/List
    ///@extends ecs.BaseList
    sqListClass //
      .SquirrelFunc("_set", comp_list_set<T>, 3, "xi.")
      .SquirrelFunc("append", comp_arr_append<T>, -2, "x.i")
      .SquirrelFunc("insert", comp_arr_insert<T>, -3, "xi.i")
      .SquirrelFunc("remove", comp_arr_remove<T>, 2, "xi")
      .SquirrelFunc("pop", comp_arr_pop<T>, 1, "x")
      .GlobalFunc("clear", comp_arr_clear<T>)
      /**/;

  tbl.Bind(type_name, sqListClass);
}

template <typename TypeRW, typename TypeRO, typename ItemType>
static void bind_list_type(HSQUIRRELVM vm, Sqrat::Table &tbl, const char *type_name)
{
  using TS = eastl::fixed_string<char, 256>;
  bind_list_class<Sqrat::Class<TypeRO>, TypeRO, ItemType, /*ro*/ true>(vm, tbl, TS(TS::CtorSprintf(), "%sRO", type_name).c_str());
  bind_list_class<Sqrat::DerivedClass<TypeRW, TypeRO>, TypeRW, ItemType>(vm, tbl, type_name);
}


static SQInteger sqevent_ctor(HSQUIRRELVM vm)
{
  using namespace ecs::sq;

  Sqrat::Var<Sqrat::Object> name(vm, 2);
  SQEvent *inst = nullptr;
  if (sq_gettop(vm) == 2)
    inst = new SQEvent(name.value);
  else
  {
    Sqrat::Var<Sqrat::Object> data(vm, 3);
    inst = new SQEvent(name.value, data.value);
  }
  Sqrat::ClassType<SQEvent>::SetManagedInstance(vm, 1, inst);
  return 0;
}


static SQInteger eid_ctor(HSQUIRRELVM vm)
{
  SQInteger eidVal = ecs::ECS_INVALID_ENTITY_ID_VAL;
  if (sq_gettop(vm) > 1)
    sq_getinteger(vm, 2, &eidVal);
  Sqrat::ClassType<ecs::EntityId>::SetManagedInstance(vm, 1, new ecs::EntityId(eidVal));
  return 0;
}


void ecs_register_sq_binding(SqModules *module_mgr, bool create_systems, bool create_factories)
{
  HSQUIRRELVM vm = module_mgr->getVM();
  auto ins = vm_extra_data.insert(vm);
  VmExtraData &vmExtra = ins.first->second;
  vmExtra.createSystems = create_systems;
  vmExtra.createFactories = create_factories;
  if (ins.second)
  {
    vmExtra.id = vmExtra.nextId++;
    vmExtra.compTbl = comps_create_table(vm, COMPS_TABLE_MAX_NODES_COUNT);
  }

  Sqrat::Table tblEcs(vm);
  ///@module ecs
  tblEcs.SetValue("COMP_FLAG_REPLICATED", (const SQInteger)ecs::FLAG_REPLICATED);
  tblEcs.SetValue("COMP_FLAG_CHANGE_EVENT", (const SQInteger)ecs::FLAG_CHANGE_EVENT);
  tblEcs.SetValue("INVALID_ENTITY_ID", ECS_INVALID_ENTITY_ID_VAL);
  tblEcs.SetValue("EVCAST_UNICAST", EVCAST_UNICAST);
  tblEcs.SetValue("EVCAST_BROADCAST", EVCAST_BROADCAST);

  {
    ///@class Event
    Sqrat::Class<ecs::Event, Sqrat::NoConstructor<ecs::Event>> sqEvent(vm, "Event");
    sqEvent.Func("getType", &ecs::Event::getType);
    /* binding of 'getName' is intentionally not implemented as might be null in some situations (i.e. networked events) */
    Sqrat::Object sqEventCls(sqEvent.GetObject(), vm);
    ///@resetscope
    tblEcs.Bind("Event", sqEventCls);
  }
  ecs::sq::EventsBind<
#define ECS_DECL_CORE_EVENT(x, ...) x,
    ECS_DECL_ALL_CORE_EVENTS
#undef ECS_DECL_CORE_EVENT
      ecs::sq::Timer,
    EventScriptReloaded, ecs::EventOnLocalSceneEntitiesCreated>::bind(vm, tblEcs);
  {
    ///@class ecs/SchemelessEvent
    ///@extends Event
    Sqrat::DerivedClass<ecs::SchemelessEvent, ecs::Event> schemelessEvent(vm, "SchemelessEvent");
    schemelessEvent.Ctor<ecs::event_type_t>().SquirrelProp("data", get_null).SquirrelProp("payload", get_schemeless_payload);
    Sqrat::Object schemelessEventCls(schemelessEvent.GetObject(), vm);
    ///@resetscope
    tblEcs.Bind("SchemelessEvent", schemelessEventCls);
  }
  {
    ///@class ecs/SQEvent
    ///@extends ecs.SchemelessEvent
    Sqrat::DerivedClass<ecs::sq::SQEvent, ecs::SchemelessEvent> sqEvent(vm, "SQEvent");
    sqEvent.SquirrelCtor(sqevent_ctor, -2, "x s|i .");
    Sqrat::Object sqEventCls(sqEvent.GetObject(), vm);
    ///@resetscope
    tblEcs.Bind("SQEvent", sqEventCls);
  }

  ///@class ecs/TemplateDB
  Sqrat::Class<TemplateDB, Sqrat::NoConstructor<TemplateDB>> sqTemplateDB(vm, "TemplateDB");
  sqTemplateDB //
    .GlobalFunc("getTemplateByName", &tpldb_get_template_by_name)
    .GlobalFunc("size", &tpldb_get_size)
    .GlobalFunc("getTemplateMetaInfo", &tpldb_get_template_meta_info)
    .GlobalFunc("getComponentMetaInfo", &tpldb_get_component_meta_info)
    .GlobalFunc("hasComponentMetaInfo", &tpldb_has_component_meta_info)
    /**/;

  ///@class ecs/EventsDB
  Sqrat::Class<EventsDB, Sqrat::NoConstructor<EventsDB>> sqEventsDB(vm, "EventsDB");
  sqEventsDB //
    .Func("findEvent", &EventsDB::findEvent)
    .Func("hasEventScheme", &EventsDB::hasEventScheme)
    .Func("getFieldsCount", &EventsDB::getFieldsCount)
    .Func("findFieldIndex", &EventsDB::findFieldIndex)
    .Func("getFieldName", &EventsDB::getFieldName)
    .Func("getFieldType", &EventsDB::getFieldType)
    .Func("getFieldOffset", &EventsDB::getFieldOffset)
    .SquirrelFunc("getEventFieldValue", evtdb_get_event_field, 4, ".xii")
    /**/;
  ///@class ecs/Template
  Sqrat::Class<Template, Sqrat::NoConstructor<Template>> sqTemplate(vm, "Template");
  sqTemplate //
    .Func("getName", &Template::getName)
    //.Func("getBase", &Template::getBase)
    .SquirrelFunc("getCompVal", template_get_comp_val, 2, "xs")
    .SquirrelFunc("getCompValNullable", template_get_comp_val_nullable, 2, "xs")
    .GlobalFunc("hasComponent", &template_has_comp)
    .GlobalFunc("getNumParentTemplates", template_get_num_parents)
    .GlobalFunc("getParentTemplate", template_get_parent)
    .SquirrelFunc("getComponentsNames", template_get_components_names, 1, "x")
    .SquirrelFunc("getTags", template_get_tags, 1, "x")
    /**/;


  typedef bool (EntityManager::*removeCompFunc)(EntityId, const char *);

  ///@class ecs/EntityManager
  Sqrat::Class<EntityManager, Sqrat::NoConstructor<EntityManager>> sqEntityManager(vm, "EntityManager");
  sqEntityManager //
    .Func("destroyEntity", (bool(EntityManager::*)(const ecs::EntityId &)) & EntityManager::destroyEntity)
    .Func("doesEntityExist", &EntityManager::doesEntityExist)
    .Func("getNumEntities", &EntityManager::getNumEntities)

    //.Func("getNumComponents", &EntityManager::getNumComponents)

    .SquirrelFunc("sendEvent", emgr_send_event, 3, "xix")
    .SquirrelFunc("broadcastEvent", emgr_bcast_event, 2, "xx")

    //.Func("getEntityTemplate", &EntityManager::getEntityTemplate)
    .Func("getEntityTemplateName", &EntityManager::getEntityTemplateName)
    .Func("getEntityFutureTemplateName", &EntityManager::getEntityFutureTemplateName)
    .GlobalFunc("getTemplateDB", emgr_get_tempate_db) // FIXME: sqrat binding is not supports returning references from member
                                                      // functions
    .GlobalFunc("getEventsDB", emgr_get_events_db)
    .SquirrelFunc("createEntity", create_entity, -3, "xstc|o")
    .SquirrelFunc("createEntitySync", create_entity_sync, -3, "xst|o")
    .SquirrelFunc("reCreateEntityFrom", recreate_entity, -4, "xistc|o")
    .SquirrelFunc("createTemplate", create_template, -3, "xstssis")
    .GlobalFunc("getComponentType", emgr_get_component_type)
    .GlobalFunc("getComponentTypeName", emgr_get_component_type_name)
    .GlobalFunc("getTypeName", emgr_get_type_name)
    /**/;

  Sqrat::Class<UpdateStageInfoAct, Sqrat::NoConstructor<UpdateStageInfoAct>> sqUpdateStageInfoAct(vm, "UpdateStageInfoAct");
  sqUpdateStageInfoAct //
    .Var("dt", &UpdateStageInfoAct::dt)
    .Var("curTime", &UpdateStageInfoAct::curTime)
    /**/;
  ///@module ecs
  ///@class CompObjectRO
  Sqrat::Class<ecs::ObjectRO> sqCompObjectRO(vm, "CompObjectRO");
  sqCompObjectRO //
    .SquirrelFunc("isReadOnly", comp_is_readonly<true>, 1, "x")
    .SquirrelFunc("getAll", comp_obj_get_all<ecs::ObjectRO, ecs::ArrayRO, /*RO*/ true>, 1, "x")
    .SquirrelFunc("_get", comp_obj_get<ecs::ObjectRO, true>, 2, "xs")
    .SquirrelFunc("_set", readonly_method_error, 3, "xs.")
    .SquirrelFunc("_newslot", readonly_method_error, 3, "xs.")
    .SquirrelFunc("_nexti", comp_obj_nexti<ecs::ObjectRO>, 2, "xs|o")
    .SquirrelFunc("remove", readonly_method_error, 2, "xs")
    .GlobalFunc("len", comp_t_len<ecs::ObjectRO>)
    /**/;

  ///@class CompObject
  ///@extends ecs.CompObjectRO
  Sqrat::DerivedClass<ecs::Object, ecs::ObjectRO> sqCompObject(vm, "CompObject");
  sqCompObject //
    .SquirrelFunc("isReadOnly", comp_is_readonly<false>, 1, "x")
    .SquirrelFunc("getAll", comp_obj_get_all<ecs::Object, ecs::Array, false>, 1, "x")
    .SquirrelFunc("_get", comp_obj_get<ecs::Object, false>, 2, "xs")
    .SquirrelFunc("_set", comp_obj_set<ecs::Object>, 3, "xs.")
    .SquirrelFunc("_newslot", comp_obj_set<ecs::Object>, 3, "xs.")
    .SquirrelFunc("_nexti", comp_obj_nexti<ecs::Object>, 2, "xs|o")
    .SquirrelFunc("remove", comp_obj_erase<ecs::Object>, 2, "xs")
    .GlobalFunc("len", comp_t_len<ecs::Object>)
    /**/;

  ///@class CompArrayRO
  Sqrat::Class<ecs::ArrayRO> sqCompArrayRO(vm, "CompArrayRO");
  sqCompArrayRO //
    .SquirrelFunc("isReadOnly", comp_is_readonly<true>, 1, "x")
    .SquirrelFunc("getAll", comp_arr_get_all<ecs::ObjectRO, ecs::ArrayRO, true>, 1, "x")
    .SquirrelFunc("_get", comp_arr_get<ecs::ArrayRO, true>, 2, "xi")
    .SquirrelFunc("_set", readonly_method_error, 3, "xi.")
    .SquirrelFunc("_nexti", comp_arr_nexti<ecs::ArrayRO>, 2, "xi|o")
    .GlobalFunc("len", comp_t_len<ecs::ArrayRO>)
    .SquirrelFunc("indexof", comp_arr_indexof<ecs::ArrayRO>, -2, "x.i")
    .SquirrelFunc("append", readonly_method_error, -2, "x.i")
    .SquirrelFunc("insert", readonly_method_error, -3, "xi.i")
    .SquirrelFunc("remove", readonly_method_error, 2, "xi")
    .SquirrelFunc("pop", readonly_method_error, 1, "x")
    /**/;

  ///@class CompArray
  ///@extends ecs.CompArrayRO
  Sqrat::DerivedClass<ecs::Array, ecs::ArrayRO> sqCompArray(vm, "CompArray");
  sqCompArray //
    .SquirrelFunc("isReadOnly", comp_is_readonly<false>, 1, "x")
    .SquirrelFunc("getAll", comp_arr_get_all<ecs::Object, ecs::Array, false>, 1, "x")
    .SquirrelFunc("_get", comp_arr_get<ecs::Array, false>, 2, "x.")
    .SquirrelFunc("_set", comp_arr_set<ecs::Array>, 3, "xi.")
    .SquirrelFunc("_nexti", comp_arr_nexti<ecs::Array>, 2, "xi|o")
    .GlobalFunc("len", comp_t_len<ecs::Array>)
    .SquirrelFunc("indexof", comp_arr_indexof<ecs::Array>, -2, "x.i")
    .SquirrelFunc("append", comp_arr_append<ecs::Array>, -2, "x.i")
    .SquirrelFunc("insert", comp_arr_insert<ecs::Array>, -3, "xi.i")
    .SquirrelFunc("remove", comp_arr_remove<ecs::Array>, 2, "xi")
    .SquirrelFunc("pop", comp_arr_pop<ecs::Array>, 1, "x")
    .GlobalFunc("clear", comp_arr_clear<ecs::Array>)
    /**/;

  ///@class ecs/SqQuery
  Sqrat::Class<SqQuery, Sqrat::NoCopy<SqQuery>> classSqQuery(vm, "SqQuery");
  classSqQuery //
    .SquirrelCtor(SqQuery::script_ctor, -3, "xsts")
    .SquirrelFunc("perform", &SqQuery::perform<2>, -2, "x i|c c|s|o s|o")
    .SquirrelFunc("_call", &SqQuery::perform<3>, -3, "x . i|c c|s|o s|o")
    /**/;

  ///@class ecs/EntityId
  Sqrat::Class<ecs::EntityId> sqEntityId(vm, "EntityId");
  sqEntityId.SquirrelCtor(eid_ctor, -1, "xi");

  Sqrat::DerivedClass<EventDataGetter, ecs::Event, Sqrat::NoConstructor<EventDataGetter>> sqEventGetterProxy(vm, "EventDataGetter");
  ///@skipline
  sqEventGetterProxy.Var("data", &EventDataGetter::sqData);
  ///@module ecs
  tblEcs //
    .Func("get_component_name_by_idx", get_component_name_by_idx)
    .Func("get_component_name_by_hash", get_component_name_by_hash)
    .SquirrelFunc("register_sq_event", register_sq_event, -2, ".sb|ii|o")
    /**/;

  tblEcs.SquirrelFunc("_dbg_get_all_comps_inspect", _dbg_get_all_comps_inspect, 2, ".i");
  tblEcs.SquirrelFunc("_dbg_get_comp_val_inspect", _dbg_get_comp_val_inspect, -3, ".is");
  tblEcs.SquirrelFunc("obsolete_dbg_get_comp_val", obsolete_dbg_get_comp_val, -3, ".is");
  tblEcs.Func("get_comp_type", get_comp_type);
  tblEcs.SquirrelFunc("obsolete_dbg_set_comp_val", obsolete_dbg_set_comp_val, -3, ".is");
  tblEcs.Func("get_comp_flags", get_comp_flags); // stub
  tblEcs.Func("get_semantic_type", get_semantic_type);
  tblEcs.SquirrelFunc("calc_hash", calc_hash, 2, ".s");
  tblEcs.SquirrelFunc("calc_hash_int", calc_hash_int, 2, ".s");

#define COMP(n, type_name) tblEcs.SetValue(#n, SQInteger(ecs::ComponentTypeInfo<type_name>::type));
  tblEcs.SetValue("TYPE_NULL", 0);
  ///@const TYPE_STRING
  COMP(TYPE_STRING, ecs::string)
  ///@const TYPE_INT8
  COMP(TYPE_INT8, int8_t)
  ///@const TYPE_UINT8
  COMP(TYPE_UINT8, uint8_t)
  ///@const TYPE_INT16
  COMP(TYPE_INT16, int16_t)
  ///@const TYPE_UINT16
  COMP(TYPE_UINT16, uint16_t)
  ///@const TYPE_INT
  COMP(TYPE_INT, int)
  ///@const TYPE_UINT
  COMP(TYPE_UINT, uint32_t)
  ///@const TYPE_INT64
  COMP(TYPE_INT64, int64_t)
  ///@const TYPE_UINT64
  COMP(TYPE_UINT64, uint64_t)
  ///@const TYPE_FLOAT
  COMP(TYPE_FLOAT, float)
  ///@const TYPE_POINT2
  COMP(TYPE_POINT2, Point2)
  ///@const TYPE_POINT3
  COMP(TYPE_POINT3, Point3)
  ///@const TYPE_POINT4
  COMP(TYPE_POINT4, Point4)
  ///@const TYPE_IPOINT2
  COMP(TYPE_IPOINT2, IPoint2)
  ///@const TYPE_IPOINT3
  COMP(TYPE_IPOINT3, IPoint3)
  ///@const TYPE_IPOINT4
  COMP(TYPE_IPOINT4, IPoint4)
  ///@const TYPE_DPOINT3
  COMP(TYPE_DPOINT3, DPoint3)
  ///@const TYPE_BOOL
  COMP(TYPE_BOOL, bool)
  ///@const TYPE_MATRIX
  COMP(TYPE_MATRIX, TMatrix)
  ///@const TYPE_EID
  COMP(TYPE_EID, EntityId)
  ///@const TYPE_COLOR
  COMP(TYPE_COLOR, E3DCOLOR)
  ///@const TYPE_OBJECT
  COMP(TYPE_OBJECT, ecs::Object)
  ///@const TYPE_ARRAY
  COMP(TYPE_ARRAY, ecs::Array)
  ///@const TYPE_SHARED_OBJECT
  COMP(TYPE_SHARED_OBJECT, ecs::SharedComponent<ecs::Object>)
  ///@const TYPE_SHARED_ARRAY
  COMP(TYPE_SHARED_ARRAY, ecs::SharedComponent<ecs::Array>)
  ///@const TYPE_INT_LIST
  COMP(TYPE_INT_LIST, ecs::IntList)
  ///@const TYPE_UINT16_LIST
  COMP(TYPE_UINT16_LIST, ecs::UInt16List)
  ///@const TYPE_STRING_LIST
  COMP(TYPE_STRING_LIST, ecs::StringList)
  ///@const TYPE_EID_LIST
  COMP(TYPE_EID_LIST, ecs::EidList)
  ///@const TYPE_FLOAT_LIST
  COMP(TYPE_FLOAT_LIST, ecs::FloatList)
  ///@const TYPE_POINT2_LIST
  COMP(TYPE_POINT2_LIST, ecs::Point2List)
  ///@const TYPE_POINT3_LIST
  COMP(TYPE_POINT3_LIST, ecs::Point3List)
  ///@const TYPE_POINT4_LIST
  COMP(TYPE_POINT4_LIST, ecs::Point4List)
  ///@const TYPE_IPOINT2_LIST
  COMP(TYPE_IPOINT2_LIST, ecs::IPoint2List)
  ///@const TYPE_IPOINT3_LIST
  COMP(TYPE_IPOINT3_LIST, ecs::IPoint3List)
  ///@const TYPE_IPOINT4_LIST
  COMP(TYPE_IPOINT4_LIST, ecs::IPoint4List)
  ///@const TYPE_BOOL_LIST
  COMP(TYPE_BOOL_LIST, ecs::BoolList)
  ///@const TYPE_TMATRIX_LIST
  COMP(TYPE_TMATRIX_LIST, ecs::TMatrixList)
  ///@const TYPE_COLOR_LIST
  COMP(TYPE_COLOR_LIST, ecs::ColorList)
  ///@const TYPE_INT64_LIST
  COMP(TYPE_INT64_LIST, ecs::Int64List)
  ///@const TYPE_UINT64_LIST
  COMP(TYPE_UINT64_LIST, ecs::UInt64List)
  ///@const TYPE_TAG
  COMP(TYPE_TAG, ecs::Tag)
  ///@const TYPE_AUTO
  COMP(TYPE_AUTO, ecs::auto_type)
#undef COMP

#define DECL_LIST_TYPE(lt, t) bind_list_type<ecs::lt, ecs::lt##RO, t>(vm, tblEcs, "Comp" #lt);
  ECS_DECL_LIST_TYPES
#undef DECL_LIST_TYPE

  tblEcs.Bind("CompObject", sqCompObject);
  tblEcs.Bind("CompObjectRO", sqCompObjectRO);
  tblEcs.Bind("CompArrayRO", sqCompArrayRO);
  tblEcs.Bind("CompArray", sqCompArray);
  tblEcs.Bind("SqQuery", classSqQuery);
  tblEcs.Bind("EntityId", sqEntityId);

  if (create_systems)
  {
    tblEcs.SquirrelFunc("clear_vm_entity_systems", clear_vm_entity_systems, 1);
    tblEcs.SquirrelFunc("register_entity_system", register_entity_system, -4, ". s t t t|o");
    tblEcs.SquirrelFunc("modify_es_list", modify_es_list, 2, ".c");
    tblEcs.Func("start_es_loading", start_es_loading);
    tblEcs.Func("end_es_loading", end_es_loading);
  }

  G_ASSERT(g_entity_mgr);
  tblEcs.SetValue("g_entity_mgr", g_entity_mgr.get());
  ///@ftype x
  ///@fvalue instance of EntityManager class
  if (create_systems || create_factories)
    ecs::sq::bind_timers(tblEcs);

  module_mgr->addNativeModule("ecs", tblEcs);
}

void shutdown_ecs_sq_script(HSQUIRRELVM vm)
{
  G_ASSERT(vm);
  debug("shutdown_ecs_sq_script called");

  auto itExtra = vm_extra_data.find(vm);
  G_ASSERT_RETURN(itExtra != vm_extra_data.end(), );
  VmExtraData &extra = itExtra->second;

  if (extra.createSystems)
  {
    bool removed = false;
    remove_if_systems([&vm, &removed](EntitySystemDesc *system) {
      if (!entity_system_is_squirrel(system))
        return false;
      ScriptEsData *esData = (ScriptEsData *)system->getUserData();
      if (esData->getVM() != vm)
        return false;
      system->freeIfDynamic();
      removed = true;
      return true;
    });
    if (removed && g_entity_mgr)
      g_entity_mgr->resetEsOrder();
    ecs::sq::shutdown_timers(vm);
  }

  vm_extra_data.erase(itExtra);
}


void update_ecs_sq_timers(float dt, float rt_dt) { ecs::sq::update_timers(dt, rt_dt); }

void start_es_loading() { ecs::ecs_is_in_init_phase = true; }

void end_es_loading()
{
  ecs::ecs_is_in_init_phase = false;
  if (g_entity_mgr)
    g_entity_mgr->resetEsOrder();
}

EA_RESTORE_VC_WARNING()
