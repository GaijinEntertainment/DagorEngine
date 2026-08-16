// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <quirrel/ecsComputed/ecsComputed_api.h>

#include <generic/dag_tab.h>
#include <generic/dag_span.h>
#include <util/dag_string.h>
#include <debug/dag_debug.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/component.h>
#include <daECS/core/internal/performQuery.h>
#include <ecs/scripts/scripts.h>
#include <ecs/scripts/sq/compDesc.h>
#include <ecs/scripts/sq/queryExpression.h>
#include <quirrel/frp/dag_frp.h>
#include <quirrel/sqStackChecker.h>
#include <sqmodules/sqmodules.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_miscApi.h>
#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector_map.h>
#include <sqrat.h>
#include <sqstdaux.h>


namespace
{

struct CompColumn
{
  Sqrat::Object key;    // component name; also the table key when there are several
  Sqrat::Object defVal; // optional components read as this when absent
  bool boxed = false;   // engine-only type that can not be mirrored; reads as defVal
};

class SqEcsComputed;

// Component indices in the tree are positions in the query desc, so it must be
// built against the same QueryView the values are read from.
struct RowFilter
{
  PerformExpressionTree tree;
  const bool active;

  RowFilter(ExpressionTree &expr, const ecs::QueryView &qv) :
    tree(expr.nodes.data(), expr.nodes.size(), &qv), active(!expr.nodes.empty())
  {}
  bool operator()(uint32_t row) { return !active || tree.performExpression((int)row); }
};

class EcsComputedSource final : public sqfrp::INativeComputedSource
{
public:
  HSQUIRRELVM vm = nullptr;
  sqfrp::ObservablesGraph *graph = nullptr;
  sqfrp::NodeId nodeId;
  SqEcsComputed *owner = nullptr; // cleared by whichever of the two dies first

  eastl::string esName, trackedCsv, tagsStr;
  uint32_t loadEpoch = 0;
  ecs::QueryId qid;

  Tab<CompColumn> columns;
  // QueryCompTypes defaults to tmpmem; this one lives as long as the mirror
  ecs::sq::QueryCompTypes columnTypes{midmem_ptr()};
  Tab<ecs::ComponentDesc> descRo, descRq, descNo;
  Sqrat::Object defVal;

  eastl::string filterSrc;   // kept: the parser writes into it in place
  ExpressionTree filterExpr; // empty when there is no filter

  // map mode value: one table, rewritten in place, so the node holds this very object
  Sqrat::Object mapTable;
  // non-map mode: the value of the previous pull; pushed again when the raw data
  // still matches it, so the graph's identity check skips the deep compare
  Sqrat::Object lastVal;

  // Raw copies of the mirrored values, written at event time on the entity
  // manager owner thread; a pull only turns them into script values.
  Tab<ecs::ChildComponent> rawVals; // non-map: the values of the current match
  ecs::EntityId rawEid;             // non-map: the entity rawVals mirror
  bool matched = false;
  bool sqValid = false; // lastVal is built from the current rawVals
  bool mapMode = false;

  struct RawRow
  {
    Tab<ecs::ChildComponent> vals;
    bool sqDirty = false; // already listed in dirtyEids
  };
  eastl::vector_map<ecs::entity_id_t, RawRow> rawMap;
  Tab<ecs::entity_id_t> dirtyEids; // rows the next pull syncs into mapTable
  bool needFullResync = false;     // dirtyEids overflowed: reconcile the whole table

  // bounds what piles up in a mirror that nobody pulls; a pull past the cap
  // reconciles the whole table against the raw cache
  static constexpr int MAX_DIRTY_EIDS = 64;

  // All that happens on an ECS change: copy the changed rows and flag the node.
  // No script, nothing that can re-enter the VM from inside an event dispatch.
  void onEcsEvent(const ecs::Event &evt, const ecs::QueryView &qv);

  // read the entities that existed before the mirror; later changes come as events
  void seed();

  // the script handle died; the ES desc still owns this object
  void detach()
  {
    owner = nullptr;
    graph = nullptr;
    nodeId = sqfrp::NodeId{};
  }

  void releaseScriptRefs()
  {
    defVal.Release();
    mapTable.Release();
    lastVal.Release();
    for (CompColumn &c : columns)
    {
      c.key.Release();
      c.defVal.Release();
    }
  }

  sqfrp::NativePullResult pull(HSQUIRRELVM) override;

private:
  bool typesChecked = false;
  void resolveTypes(const ecs::QueryView &qv)
  {
    if (columnTypes.update(qv, qv.getRoStart(), columns.size()) && !typesChecked)
      checkColumnTypes(qv.manager());
  }
  void checkColumnTypes(const ecs::EntityManager &mgr);
  ecs::EntityId rowEid(const ecs::QueryView &qv, uint32_t row) const;
  void copyRow(const ecs::QueryView &qv, uint32_t row, Tab<ecs::ChildComponent> &out);
  void markRowDirty(ecs::entity_id_t eid);
  bool syncMapRow(ecs::entity_id_t eid);
  bool resyncMapTable();
  bool applyRowsMap(const ecs::QueryView &qv, bool lost);
  bool applyRowsSingle(const ecs::QueryView &qv, bool lost);
  void adoptMatch(const ecs::QueryView *exclude);
  void pushCompValue(const Tab<ecs::ChildComponent> &vals, int ci);
  void pushRowValue(const Tab<ecs::ChildComponent> &vals);
  bool compUnchanged(const Tab<ecs::ChildComponent> &vals, int ci, const HSQOBJECT &slot);
  bool rowUnchanged(const Tab<ecs::ChildComponent> &vals, const HSQOBJECT &old_row);
};

static volatile uint32_t g_events = 0, g_recalcs = 0, g_copied_rows = 0;
static volatile int g_dead_handles = 0; // handles collected since the last sweep

void EcsComputedSource::onEcsEvent(const ecs::Event &evt, const ecs::QueryView &qv)
{
  // the raw cache and the node state are unlocked: this must not race a graph pull
  G_ASSERTF(!qv.manager().isConstrainedMTMode(), "ecs.computed '%s' signalled in constrained MT mode", esName.c_str());
  interlocked_increment(g_events);
  resolveTypes(qv);
  const ecs::event_type_t et = evt.getType();
  const bool lost = et == ecs::EventEntityDestroyed::staticType() || et == ecs::EventComponentsDisappear::staticType();
  const bool changed = mapMode ? applyRowsMap(qv, lost) : applyRowsSingle(qv, lost);
  if (changed && graph && nodeId.isValid())
    graph->invalidateNativeComputed(nodeId);
}

void EcsComputedSource::checkColumnTypes(const ecs::EntityManager &mgr)
{
  typesChecked = true;
  for (int i = 0; i < columns.size(); ++i)
    if (mgr.getComponentTypes().getTypeInfo(columnTypes[i].typeId).flags & ecs::COMPONENT_TYPE_BOXED)
    {
      columns[i].boxed = true;
      logerr("ecs.computed '%s': component <%s> is a boxed engine type and can not be mirrored, it reads as the default value",
        esName.c_str(), sq_objtostring(&columns[i].key.GetObject()));
    }
}

void EcsComputedSource::copyRow(const ecs::QueryView &qv, uint32_t row, Tab<ecs::ChildComponent> &out)
{
  interlocked_increment(g_copied_rows);
  out.resize(columns.size());
  for (int i = 0; i < columns.size(); ++i)
  {
    const ecs::sq::CompTypeInfo &ti = columnTypes[i];
    const uint8_t *data = columns[i].boxed ? nullptr : ecs::sq::comp_row_data(qv, qv.getRoStart() + i, row, ti);
    // a null (absent optional or boxed) entry reads as the column default
    out[i] = data ? ecs::ChildComponent(ti.type, data, ecs::ChildComponent::CopyType::Deep) : ecs::ChildComponent();
  }
}

void EcsComputedSource::markRowDirty(ecs::entity_id_t eid)
{
  if (needFullResync)
    return;
  if (dirtyEids.size() >= MAX_DIRTY_EIDS)
  {
    needFullResync = true;
    dirtyEids.clear(); // keep the capacity: a mirror that overflowed once will again
    return;
  }
  dirtyEids.push_back(eid);
}

bool EcsComputedSource::applyRowsMap(const ecs::QueryView &qv, bool lost)
{
  RowFilter passes(filterExpr, qv);
  bool changed = false;
  for (uint32_t i = qv.begin(), e = qv.end(); i < e; ++i)
  {
    const ecs::entity_id_t eid = (ecs::entity_id_t)rowEid(qv, i);
    if (!lost && passes(i))
    {
      RawRow &row = rawMap[eid];
      copyRow(qv, i, row.vals);
      if (!row.sqDirty)
      {
        row.sqDirty = true;
        markRowDirty(eid);
      }
      changed = true;
    }
    else if (auto it = rawMap.find(eid); it != rawMap.end())
    {
      rawMap.erase(it);
      markRowDirty(eid);
      changed = true;
    }
  }
  return changed;
}

bool EcsComputedSource::applyRowsSingle(const ecs::QueryView &qv, bool lost)
{
  RowFilter passes(filterExpr, qv);
  bool changed = false;
  bool lostMatch = false;
  for (uint32_t i = qv.begin(), e = qv.end(); i < e; ++i)
  {
    const bool rowMatches = !lost && passes(i);
    if (matched && rowEid(qv, i) == rawEid)
    {
      if (rowMatches)
      {
        copyRow(qv, i, rawVals);
        changed = true;
      }
      else
        lostMatch = true;
    }
    else if (!matched && rowMatches)
    {
      matched = true;
      rawEid = rowEid(qv, i);
      copyRow(qv, i, rawVals);
      changed = true;
    }
  }
  if (lostMatch)
  {
    matched = false;
    // a destroy dispatches while the row still queries as alive, so exclude it
    adoptMatch(lost ? &qv : nullptr);
    changed = true;
  }
  if (changed)
    sqValid = false;
  return changed;
}

void EcsComputedSource::adoptMatch(const ecs::QueryView *exclude)
{
  if (!g_entity_mgr)
    return;
  ecs::perform_query(g_entity_mgr.get(), qid, ecs::stoppable_query_cb_t([this, exclude](const ecs::QueryView &qv) {
    resolveTypes(qv);
    RowFilter passes(filterExpr, qv);
    for (uint32_t i = qv.begin(), e = qv.end(); i < e; ++i)
    {
      if (!passes(i))
        continue;
      const ecs::EntityId eid = rowEid(qv, i);
      bool excluded = false;
      if (exclude)
        for (uint32_t j = exclude->begin(), je = exclude->end(); !excluded && j < je; ++j)
          excluded = rowEid(*exclude, j) == eid;
      if (excluded)
        continue;
      matched = true;
      rawEid = eid;
      copyRow(qv, i, rawVals);
      return ecs::QueryCbResult::Stop;
    }
    return ecs::QueryCbResult::Continue;
  }));
}

void EcsComputedSource::seed()
{
  if (!g_entity_mgr)
    return;
  if (!mapMode)
  {
    adoptMatch(nullptr);
    return;
  }
  ecs::perform_query(g_entity_mgr.get(), qid, ecs::query_cb_t([this](const ecs::QueryView &qv) {
    resolveTypes(qv);
    applyRowsMap(qv, /*lost*/ false);
  }));
}


ecs::EntityId EcsComputedSource::rowEid(const ecs::QueryView &qv, uint32_t row) const
{
  // the eid column is appended after the value columns, see mk_ecs_computed
  const ecs::EntityId *eids = (const ecs::EntityId *)qv.getComponentUntypedData(qv.getRoStart() + columns.size());
  return eids ? eids[row] : ecs::INVALID_ENTITY_ID;
}

void EcsComputedSource::pushCompValue(const Tab<ecs::ChildComponent> &vals, int ci)
{
  CompColumn &c = columns[ci];
  const ecs::ChildComponent &raw = vals[ci];
  if (raw.isNull())
  {
    sq_pushobject(vm, c.defVal.GetObject()); // absent optional component; null object pushes null
    return;
  }
  // deep-copies complex types: the value outlives the raw cache entry
  const ecs::EntityComponentRef ref = raw.getEntityComponentRef();
  ecs::sq::push_comp_val_copy(vm, sq_objtostring(&c.key.GetObject()), ref.getRawData(), ref.getUserType(), ref.getTypeId());
}

void EcsComputedSource::pushRowValue(const Tab<ecs::ChildComponent> &vals)
{
  if (columns.size() == 1)
  {
    pushCompValue(vals, 0);
    return;
  }
  sq_newtableex(vm, columns.size());
  for (int i = 0; i < columns.size(); ++i)
  {
    sq_pushobject(vm, columns[i].key.GetObject());
    pushCompValue(vals, i);
    G_VERIFY(SQ_SUCCEEDED(sq_rawset(vm, -3)));
  }
}

bool EcsComputedSource::compUnchanged(const Tab<ecs::ChildComponent> &vals, int ci, const HSQOBJECT &slot)
{
  const ecs::ChildComponent &raw = vals[ci];
  if (raw.isNull()) // an absent optional is mirrored as the defVal object itself
    return sq_fast_equal_by_value_deep(&slot, &columns[ci].defVal.GetObject(), 1);
  const ecs::EntityComponentRef ref = raw.getEntityComponentRef();
  return ecs::sq::comp_val_equal(vm, slot, ref.getRawData(), ref.getUserType(), ref.getTypeId());
}

// Compares the raw row against its script copy in old_row without building
// anything: no table, no deep copies. A false negative only costs a rebuild
// of this one row.
bool EcsComputedSource::rowUnchanged(const Tab<ecs::ChildComponent> &vals, const HSQOBJECT &old_row)
{
  if (columns.size() == 1)
    return compUnchanged(vals, 0, old_row);
  if (sq_type(old_row) != OT_TABLE)
    return false;
  sq_pushobject(vm, old_row);
  bool eq = sq_getsize(vm, -1) == (SQInteger)columns.size();
  for (int i = 0; eq && i < columns.size(); ++i)
  {
    sq_pushobject(vm, columns[i].key.GetObject());
    if (SQ_FAILED(sq_rawget(vm, -2)))
    {
      eq = false;
      break;
    }
    HSQOBJECT slot;
    G_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &slot)));
    sq_poptop(vm); // old_row still holds it
    eq = compUnchanged(vals, i, slot);
  }
  sq_poptop(vm);
  return eq;
}

// mapTable is at the stack top; update or delete one slot from the raw cache
bool EcsComputedSource::syncMapRow(ecs::entity_id_t eid)
{
  auto it = rawMap.find(eid);
  if (it == rawMap.end())
  {
    sq_pushinteger(vm, (SQInteger)eid);
    // one slot is left in every case: the deleted value, null for an absent slot, or the key on failure
    bool removed = false;
    if (SQ_SUCCEEDED(sq_rawdeleteslot(vm, -2, SQTrue)))
      removed = sq_gettype(vm, -1) != OT_NULL;
    sq_poptop(vm);
    return removed;
  }
  it->second.sqDirty = false;
  sq_pushinteger(vm, (SQInteger)eid);
  if (SQ_SUCCEEDED(sq_rawget(vm, -2)))
  {
    HSQOBJECT oldRow;
    G_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &oldRow)));
    // the old row stays on the stack so it cannot be collected mid-compare
    const bool same = rowUnchanged(it->second.vals, oldRow);
    sq_poptop(vm);
    if (same)
      return false;
  }
  sq_pushinteger(vm, (SQInteger)eid);
  pushRowValue(it->second.vals);
  G_VERIFY(SQ_SUCCEEDED(sq_rawset(vm, -3)));
  return true;
}

// dirtyEids overflowed: reconcile the whole table (at the stack top) against
// the raw cache
bool EcsComputedSource::resyncMapTable()
{
  // collect the dead keys first: sq_next can not survive a delete
  Tab<SQInteger> deadKeys(tmpmem);
  sq_pushnull(vm);
  while (SQ_SUCCEEDED(sq_next(vm, -2)))
  {
    SQInteger key = 0;
    G_VERIFY(SQ_SUCCEEDED(sq_getinteger(vm, -2, &key)));
    if (rawMap.find((ecs::entity_id_t)key) == rawMap.end())
      deadKeys.push_back(key);
    sq_pop(vm, 2);
  }
  sq_poptop(vm);
  bool changed = false;
  for (SQInteger key : deadKeys)
  {
    sq_pushinteger(vm, key);
    if (SQ_SUCCEEDED(sq_rawdeleteslot(vm, -2, SQTrue)))
      changed = true; // an enumerated key always held a value
    sq_poptop(vm);
  }
  for (auto &kv : rawMap)
    changed = syncMapRow(kv.first) || changed;
  return changed;
}

sqfrp::NativePullResult EcsComputedSource::pull(HSQUIRRELVM)
{
  // enforce the api contract: off the owner thread a pull may run only while
  // daECS defers the mirroring events out of constrained MT mode
  G_ASSERTF(!g_entity_mgr || g_entity_mgr->isConstrainedMTMode() || g_entity_mgr->getOwnerThreadId() == get_current_thread_id(),
    "ecs.computed '%s' pulled off the owner thread outside constrained MT mode", esName.c_str());
  if (!mapMode)
  {
    if (!sqValid)
    {
      sqValid = true;
      if (!matched)
        lastVal = defVal;
      // an unchanged row keeps the previous object, so the graph's identity
      // check skips both the deep compare and the value rebuild
      else if (!rowUnchanged(rawVals, lastVal.GetObject()))
      {
        interlocked_increment(g_recalcs);
        pushRowValue(rawVals);
        HSQOBJECT h;
        G_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &h)));
        lastVal = Sqrat::Object(h, vm);
        sq_poptop(vm);
      }
    }
    sq_pushobject(vm, lastVal.GetObject());
    return sqfrp::NativePullResult::CompareWithPrev;
  }

  // A row is often dirty without its value changing: a filter component write,
  // or a recreate that kept the mirrored comps. Compare first: an identical row
  // must not wake the consumers of the map, and must not cost a table either.
  bool changed = false;
  sq_pushobject(vm, mapTable.GetObject());
  if (needFullResync || !dirtyEids.empty())
  {
    interlocked_increment(g_recalcs);
    if (needFullResync)
      changed = resyncMapTable();
    else
      for (ecs::entity_id_t eid : dirtyEids)
        changed = syncMapRow(eid) || changed;
    needFullResync = false;
    dirtyEids.clear();
  }
  // the map is one table edited in place: the graph can not see the change itself
  return changed ? sqfrp::NativePullResult::Changed : sqfrp::NativePullResult::CompareWithPrev;
}


// Script-owned handle. The source is owned by the dynamic EntitySystemDesc, which
// outlives the handle until the next sweep; detaching is enough to make it inert.
class SqEcsComputed final : public sqfrp::ComputedHandle
{
public:
  EcsComputedSource *source = nullptr;

  SqEcsComputed(sqfrp::NodeId id_, sqfrp::ObservablesGraph *g, EcsComputedSource *s) : sqfrp::ComputedHandle(id_, g), source(s) {}

  ~SqEcsComputed() override
  {
    if (source)
    {
      source->detach();
      source = nullptr;
      // unregistering the ES waits for the sweep: this can run at a GC point
      // inside an ECS event dispatch
      interlocked_increment(g_dead_handles);
    }
  }
};


void ecs_computed_es_event(const ecs::Event &evt, const ecs::QueryView &qv)
{
  if (EcsComputedSource *src = (EcsComputedSource *)qv.getUserData())
    src->onEcsEvent(evt, qv);
}

void ecs_computed_es_desc_deleter(ecs::EntitySystemDesc *desc)
{
  EcsComputedSource *src = (EcsComputedSource *)desc->getUserData();
  desc->setUserData(nullptr);
  if (!src)
    return;
  if (src->owner) // the handle outlives the source: stop it dereferencing this
  {
    src->owner->source = nullptr;
    src->owner = nullptr;
  }
  // the node outlives the source too, and holds a pointer to it
  if (src->graph && src->nodeId.isValid())
    src->graph->detachNativeComputed(src->nodeId);
  // released here, not in the handle dtor, which can run during VM teardown
  src->releaseScriptRefs();
  if (src->qid && g_entity_mgr)
    g_entity_mgr->destroyQuery(src->qid);
  delete src;
}


// Removes the systems whose handle died, at a point where that is safe.
// Does not reorder the systems: sweep() does that itself, and during es-loading
// the order is rebuilt at the end anyway.
int remove_dead_systems()
{
  if (interlocked_acquire_load(g_dead_handles) == 0)
    return 0;
  interlocked_release_store(g_dead_handles, 0);
  int removed = 0;
  ecs::remove_if_systems([&removed](ecs::EntitySystemDesc *desc) {
    if (desc->getOps().onEvent != &ecs_computed_es_event)
      return false;
    EcsComputedSource *src = (EcsComputedSource *)desc->getUserData();
    if (!src || src->owner)
      return false;
    desc->freeIfDynamic();
    ++removed;
    return true;
  });
  return removed;
}


static const int vm_state_key_dummy = _MAKE4C('ECSC');
static const SQUserPointer vm_state_key = (SQUserPointer)&vm_state_key_dummy;

struct VmState
{
  eastl::string defaultTags;
  uint32_t loadEpoch = 0;
};

static VmState *get_vm_state(HSQUIRRELVM vm)
{
  SqStackChecker chk(vm);
  sq_pushregistrytable(vm);
  sq_pushuserpointer(vm, vm_state_key);
  SQUserPointer ptr = nullptr;
  if (SQ_SUCCEEDED(sq_rawget(vm, -2)))
  {
    sq_getuserpointer(vm, -1, &ptr);
    sq_poptop(vm);
  }
  sq_poptop(vm);
  return (VmState *)ptr;
}

static VmState *create_vm_state(HSQUIRRELVM vm)
{
  VmState *state = new VmState();
  SqStackChecker chk(vm);
  sq_pushregistrytable(vm);
  sq_pushuserpointer(vm, vm_state_key);
  sq_pushuserpointer(vm, state);
  G_VERIFY(SQ_SUCCEEDED(sq_rawset(vm, -3)));
  sq_poptop(vm);
  return state;
}

static void release_vm_state(HSQUIRRELVM vm)
{
  VmState *state = get_vm_state(vm);
  if (!state)
    return;
  SqStackChecker chk(vm);
  sq_pushregistrytable(vm);
  sq_pushuserpointer(vm, vm_state_key);
  G_VERIFY(SQ_SUCCEEDED(sq_rawdeleteslot(vm, -2, SQFalse)));
  sq_poptop(vm);
  delete state;
}

volatile uint32_t g_computed_counter = 0;

void on_module_unload(HSQUIRRELVM vm, bool is_closing)
{
  if (is_closing)
    return; // shutdown_vm does that path
  if (VmState *state = get_vm_state(vm))
    ++state->loadEpoch;
}


// out_tracked collects the names for the ES tracked set; list_name is for errors
bool parse_names(const Sqrat::Object &list, const char *list_name, Tab<ecs::ComponentDesc> &out, Tab<CompColumn> *out_cols,
  eastl::string *out_tracked, String &err)
{
  if (list.IsNull())
    return true;
  if (list.GetType() != OT_ARRAY)
  {
    err.printf(0, "ecs.computed: '%s' must be an array or null, got %s", list_name, sq_objtypestr(list.GetType()));
    return false;
  }
  const String label(0, "ecs.computed: '%s'", list_name);
  Sqrat::Array arr(list);
  for (SQInteger i = 0, n = arr.Length(); i < n; ++i)
  {
    ecs::sq::ParsedComp comp;
    if (!ecs::sq::parse_comp_entry(arr.GetSlot(i), (int)i, label.c_str(), comp, err))
      return false;
    if (comp.typeSlot == ecs::sq::CompTypeSlot::SCRIPT_COMP)
    {
      err.printf(0, "%s entry #%d is a script component; only native components can be mirrored", label.c_str(), (int)i);
      return false;
    }
    const ecs::HashedConstString compName = ECS_HASH_SLOW(comp.getName());
    // a query desc keeps the type it was built with and auto never resolves later,
    // so an untyped value component only works if it is registered already
    const ecs::component_type_t type =
      comp.typeSlot == ecs::sq::CompTypeSlot::EXPLICIT ? comp.type : ecs::sq::registered_comp_type(compName.hash);
    out.push_back(ecs::ComponentDesc(compName, type, comp.hasDefVal ? ecs::CDF_OPTIONAL : 0));
    if (out_tracked)
    {
      if (!out_tracked->empty())
        *out_tracked += ",";
      *out_tracked += compName.str;
    }
    if (out_cols)
    {
      CompColumn col;
      col.key = comp.key;
      col.defVal = comp.defVal;
      out_cols->push_back(col);
    }
  }
  return true;
}


SQInteger mk_ecs_computed_impl(HSQUIRRELVM vm, bool map_mode)
{
  if (!g_entity_mgr)
    return sq_throwerror(vm, "ecs.computed: no entity manager");
  // outside the loading phase every mirror would cost a whole-game resetEsOrder
  if (!is_es_loading())
    return sq_throwerror(vm,
      "ecs.computed: mirrors can be created only during the es-loading phase, like ecs queries and entity systems");
  if (g_entity_mgr->isConstrainedMTMode())
    return sq_throwerror(vm, "ecs.computed: mirrors can not be created in constrained MT mode");
  VmState *vmState = get_vm_state(vm);
  if (!vmState)
    return sq_throwerror(vm, "ecs.computed: module is not bound to this VM");
  sqfrp::ObservablesGraph *graph = sqfrp::ObservablesGraph::get_from_vm(vm);
  if (!graph)
    return sq_throwerror(vm, "ecs.computed: no frp graph in this VM (frp module must be bound first)");
  Sqrat::ClassData<SqEcsComputed> *classData = Sqrat::ClassType<SqEcsComputed>::getClassData(vm);
  if (!classData)
    return sq_throwerror(vm, "ecs.computed: EcsComputed class is not registered in this VM");

  Sqrat::Table params = Sqrat::Var<Sqrat::Table>(vm, 2).value;
  eastl::unique_ptr<EcsComputedSource> src = eastl::make_unique<EcsComputedSource>();
  src->vm = vm;
  src->graph = graph;
  src->loadEpoch = vmState->loadEpoch;
  src->mapMode = map_mode;
  src->defVal = params.RawGetSlot("defVal");
  if (map_mode && !src->defVal.IsNull())
    return sq_throwerror(vm, "ecs.computed: 'defVal' makes no sense for an eid map: no matches is an empty table");

  // all listed components are tracked: the pull rebuilds the whole value and does
  // not care which one changed. Filter components too, for a different reason: a
  // change that flips the filter is the only report of a row entering or leaving
  String err;
  if (!parse_names(params.RawGetSlot("comps"), "comps", src->descRo, &src->columns, &src->trackedCsv, err) ||
      !parse_names(params.RawGetSlot("comps_rq"), "comps_rq", src->descRq, nullptr, nullptr, err) ||
      !parse_names(params.RawGetSlot("comps_no"), "comps_no", src->descNo, nullptr, nullptr, err))
    return sq_throwerror(vm, err);
  if (src->columns.empty())
    return sq_throwerror(vm, "ecs.computed: 'comps' must list at least one component");

  // eid goes right after the value columns (rowEid indexes it there), filter
  // components after it, so the value column indices stay put
  src->descRo.push_back(ecs::ComponentDesc(ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>(), 0));
  const int filterCompsStart = src->descRo.size();
  if (!parse_names(params.RawGetSlot("comps_filter"), "comps_filter", src->descRo, nullptr, &src->trackedCsv, err))
    return sq_throwerror(vm, err);
  for (int i = filterCompsStart; i < src->descRo.size(); ++i)
    for (int j = 0; j < filterCompsStart; ++j)
      if (src->descRo[i].name == src->descRo[j].name)
      {
        err.printf(0,
          "ecs.computed: 'comps_filter' entry #%d repeats a component from 'comps'; "
          "a filter can name mirrored components directly",
          i - filterCompsStart);
        return sq_throwerror(vm, err);
      }

  Sqrat::Object nameObj = params.RawGetSlot("name");
  if (nameObj.GetType() == OT_STRING)
    src->esName = sq_objtostring(&nameObj.GetObject());
  else if (!nameObj.IsNull())
  {
    err.printf(0, "ecs.computed: 'name' must be a string or null, got %s", sq_objtypestr(nameObj.GetType()));
    return sq_throwerror(vm, err);
  }
  const bool explicitName = !src->esName.empty();
  if (!explicitName)
    src->esName.sprintf("ecs_computed_%s_%u", sq_objtostring(&src->columns[0].key.GetObject()),
      interlocked_increment(g_computed_counter));

  Sqrat::Object tagsObj = params.RawGetSlot("tags");
  if (tagsObj.GetType() == OT_STRING)
    src->tagsStr = sq_objtostring(&tagsObj.GetObject());
  else if (!tagsObj.IsNull())
  {
    err.printf(0, "ecs.computed: 'tags' must be a string or null, got %s", sq_objtypestr(tagsObj.GetType()));
    return sq_throwerror(vm, err);
  }
  else
    src->tagsStr = vmState->defaultTags;

  auto roSpan = make_span((const ecs::ComponentDesc *)src->descRo.data(), src->descRo.size());
  auto rqSpan = make_span((const ecs::ComponentDesc *)src->descRq.data(), src->descRq.size());
  auto noSpan = make_span((const ecs::ComponentDesc *)src->descNo.data(), src->descNo.size());

  // compiled against the final ro list: the tree addresses components by their
  // position in it, and bakes in their types
  Sqrat::Object filterObj = params.RawGetSlot("filter");
  if (filterObj.GetType() == OT_STRING)
  {
    src->filterSrc = sq_objtostring(&filterObj.GetObject());
    // like an ecs query filter string, an empty one means no filter
    if (!src->filterSrc.empty() && !parse_query_filter(src->filterExpr, src->filterSrc.c_str(), {}, roSpan))
      return sq_throwerror(vm, "ecs.computed: failed to parse the 'filter' expression, see the log for details");
  }
  else if (!filterObj.IsNull())
  {
    err.printf(0, "ecs.computed: 'filter' must be a string or null, got %s", sq_objtypestr(filterObj.GetType()));
    return sq_throwerror(vm, err);
  }

  // an ES name is a global key in daECS; a generated one carries a counter and can not clash.
  // Done after all parameter checks: a replaced mirror must not be lost to a bad new one
  if (explicitName)
  {
    ecs::EntitySystemDesc *prev =
      ecs::find_if_systems([&src](ecs::EntitySystemDesc *d) { return strcmp(d->name, src->esName.c_str()) == 0; });
    EcsComputedSource *prevSrc =
      prev && prev->getOps().onEvent == &ecs_computed_es_event ? (EcsComputedSource *)prev->getUserData() : nullptr;
    if (prev && !prevSrc)
      return sqstd_throwerrorf(vm, "ecs.computed: 'name' <%s> is already taken by an entity system", src->esName.c_str());
    if (prevSrc && prevSrc->vm != vm)
      return sqstd_throwerrorf(vm, "ecs.computed: 'name' <%s> is already taken by a mirror of another VM", src->esName.c_str());
    if (prevSrc && prevSrc->loadEpoch == src->loadEpoch)
      return sqstd_throwerrorf(vm, "ecs.computed: 'name' <%s> is used by another mirror of the same script load, names must be unique",
        src->esName.c_str());
    if (prevSrc)
    {
      // the previous load's mirror; the dead-handle sweep would only come next frame,
      // and daECS drops one of two same-named systems when it sorts
      ecs::remove_if_systems([prev](ecs::EntitySystemDesc *d) {
        if (d != prev)
          return false;
        d->freeIfDynamic(); //-V522
        return true;
      });
    }
  }

  const eastl::string queryName = src->esName + "_query";
  src->qid = g_entity_mgr->createQuery(ecs::NamedQueryDesc(queryName.c_str(), {}, roSpan, rqSpan, noSpan));
  if (!src->qid)
    return sq_throwerror(vm, "ecs.computed: failed to create the ECS query");

  if (src->mapMode)
    src->mapTable = Sqrat::Table(vm);
  // entities that exist already produce no events: read them once, here on the
  // owner thread with no concurrent writes
  src->seed();
  src->nodeId = graph->createNativeComputed(src.get());
  SqEcsComputed *handle = new SqEcsComputed(src->nodeId, graph, src.get());
  src->owner = handle;

  // owns src from here on: the desc deleter frees it
  new ecs::EntitySystemDesc(src->esName.c_str(), ecs::EntitySystemOps(nullptr, &ecs_computed_es_event), {}, roSpan, rqSpan, noSpan,
    // no EventComponentsAppear: EventEntityRecreated already fires for every ES matching
    // the new archetype, and daECS logerrs when an ES subscribes to both
    ecs::EventSetBuilder<ecs::EventEntityCreated, ecs::EventEntityRecreated, ecs::EventEntityDestroyed, ecs::EventComponentsDisappear,
      ecs::EventComponentChanged>::build(),
    /*stage mask*/ 0, /*tags*/ src->tagsStr.empty() ? nullptr : src->tagsStr.c_str(),
    /*tracked*/ src->trackedCsv.c_str(), /*before*/ nullptr, /*after*/ nullptr, src.get(), /*quant*/ 0, /*dynamic*/ true,
    &ecs_computed_es_desc_deleter);
  src.release();

  // no reorder needed here: end_es_loading rebuilds the order for the whole
  // phase, and that covers the removal below too
  remove_dead_systems();

  sq_pushobject(vm, classData->classObj);
  G_VERIFY(SQ_SUCCEEDED(sq_createinstance(vm, -1)));
  sq_remove(vm, -2);
  Sqrat::ClassType<SqEcsComputed>::SetManagedInstance(vm, -1, handle);
  return 1;
}

SQInteger mk_ecs_computed(HSQUIRRELVM vm) { return mk_ecs_computed_impl(vm, /*map_mode*/ false); }
SQInteger mk_ecs_computed_eid_map(HSQUIRRELVM vm) { return mk_ecs_computed_impl(vm, /*map_mode*/ true); }

} // namespace


namespace ecscomputed
{

void bind_module(SqModules *module_mgr, const char *default_es_tags)
{
  HSQUIRRELVM vm = module_mgr->getVM();
  G_ASSERTF_RETURN(Sqrat::ClassType<sqfrp::ComputedHandle>::hasClassData(vm), ,
    "frp classes must be bound to this VM before ecs.computed");

  Sqrat::DerivedClass<SqEcsComputed, sqfrp::ComputedHandle, Sqrat::NoConstructor<SqEcsComputed>> cls(vm, "EcsComputed");

  VmState *state = get_vm_state(vm);
  if (!state)
    state = create_vm_state(vm);
  state->defaultTags = default_es_tags ? default_es_tags : "";
  module_mgr->addModuleUnloadCallback(&on_module_unload);

  Sqrat::Table exports(vm);
  exports.Bind("EcsComputed", cls);
  exports.SquirrelFunc("mkEcsComputed", mk_ecs_computed, 2, ".t");
  exports.SquirrelFunc("mkEcsComputedEidMap", mk_ecs_computed_eid_map, 2, ".t");
  module_mgr->addNativeModule("ecs.computed", exports);
}


void shutdown_vm(HSQUIRRELVM vm)
{
  release_vm_state(vm);
  bool removed = false;
  ecs::remove_if_systems([vm, &removed](ecs::EntitySystemDesc *desc) {
    if (desc->getOps().onEvent != &ecs_computed_es_event)
      return false;
    EcsComputedSource *src = (EcsComputedSource *)desc->getUserData();
    if (!src || src->vm != vm)
      return false;
    desc->freeIfDynamic();
    removed = true;
    return true;
  });
  if (removed && g_entity_mgr)
    g_entity_mgr->resetEsOrder();
}

int sweep()
{
  if (!g_entity_mgr || g_entity_mgr->isConstrainedMTMode())
    return 0; // dead systems are inert, retrying next frame is fine
  const int removed = remove_dead_systems();
  if (removed)
    g_entity_mgr->resetEsOrder();
  return removed;
}

void get_stats(unsigned &out_events, unsigned &out_recalcs, unsigned &out_copied_rows)
{
  out_events = interlocked_acquire_load(g_events);
  out_recalcs = interlocked_acquire_load(g_recalcs);
  out_copied_rows = interlocked_acquire_load(g_copied_rows);
}

void reset_stats()
{
  interlocked_release_store(g_events, 0u);
  interlocked_release_store(g_recalcs, 0u);
  interlocked_release_store(g_copied_rows, 0u);
}

} // namespace ecscomputed
