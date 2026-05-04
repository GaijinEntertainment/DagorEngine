// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <quirrel/frp/dag_frp.h>
#include <sqmodules/sqmodules.h>
#include <quirrel/sqStackChecker.h>
#include <sqext.h>

#include <math/dag_mathUtils.h>
#include <startup/dag_globalSettings.h>
#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_statDrv.h>
#include <perfMon/dag_perfTimer.h>
#include <sqstdaux.h>

#include <EASTL/algorithm.h>
#include <EASTL/string.h>
#include <EASTL/hash_map.h>
#include <EASTL/hash_set.h>
#include <dag/dag_vectorMap.h>

#include <squirrel/sqpcheader.h>
#include <squirrel/sqvm.h>
#include <squirrel/sqstate.h>
#include <squirrel/sqfuncproto.h>
#include <squirrel/sqclosure.h>
#include <squirrel/vartrace.h>
#include <bindQuirrelEx/sqratDagor.h>

namespace sqfrp
{
struct NodeVarTraces
{
#if SQ_VAR_TRACE_ENABLED
  dag::Vector<VarTrace> data;

  void init(uint32_t idx)
  {
    if (idx >= data.size())
      data.resize(idx + 1);
    data[idx] = VarTrace();
  }

  void save(uint32_t idx, const SQObject &val, HSQUIRRELVM vm)
  {
    if (idx < data.size())
      data[idx].saveStack(val, vm);
  }

  void print(uint32_t idx, char *buf, int size)
  {
    if (idx < data.size())
      data[idx].printStack(buf, size);
    else
      buf[0] = 0;
  }
#else
  void init(uint32_t) {}
  void save(uint32_t, const SQObject &, HSQUIRRELVM) {}
  void print(uint32_t, char *buf, int) { buf[0] = 0; }
#endif
};
} // namespace sqfrp

#define FRP_DEBUG_MODE 0

#if FRP_DEBUG_MODE
#define FRPDBG DEBUG_CTX
#else
#define FRPDBG(...)
#endif


#define MAX_ALLOWED_SUBSCRIBERS_QUEUE_LENGTH 200
#define MAX_SUBSCRIBERS_PER_TRIGGER          2000


namespace Sqrat
{

template <>
struct InstanceToString<sqfrp::WatchedHandle>
{
  static SQInteger Format(HSQUIRRELVM vm) { return sqfrp::WatchedHandle::_tostring(vm); }
};

template <>
struct InstanceToString<sqfrp::ComputedHandle>
{
  static SQInteger Format(HSQUIRRELVM vm) { return sqfrp::ComputedHandle::_tostring(vm); }
};

} // namespace Sqrat


namespace sqfrp
{


#if DAGOR_DBGLEVEL > 0
static bool use_deprecated_methods_warning = true;
#else
static bool use_deprecated_methods_warning = false;
#endif

static const int computed_initial_update_dummy = _MAKE4C('INIT');
static const SQUserPointer computed_initial_update = (SQUserPointer)&computed_initial_update_dummy;


static bool is_app_working()
{
  //== TODO: check if loading resources etc.
  return dgs_app_active;
}

static bool logerr_graph_error(HSQUIRRELVM vm, const char *err_msg);


void ScriptSourceInfo::init(HSQUIRRELVM vm)
{
  if (!vm)
  {
    initSourceFileName = "<no VM>";
    initSourceLine = -1;
    initFuncName = "<no VM>";
    return;
  }

  SQStackInfos si;
  for (int depth = 2; depth <= 3; ++depth)
  {
    // skip native calls in script->native->script chain (function like persist())
    if (SQ_SUCCEEDED(sq_stackinfos(vm, depth, &si)) && si.line >= 0)
    {
      initSourceFileName = si.source;
      initSourceLine = si.line;
      initFuncName = si.funcname;
      return;
    }
  }

  initSourceFileName = "<no stack>";
  initSourceLine = -1;
  initFuncName = "<no stack>";
}

void ScriptSourceInfo::fillInfo(Sqrat::Table &info) const
{
  Sqrat::Table source(info.GetVM());
  source.SetValue("file", initSourceFileName);
  source.SetValue("line", initSourceLine);
  source.SetValue("func", initFuncName);

  info.SetValue("source", source);
}


void ScriptSourceInfo::toString(String &s) const
{
  s.printf(0, "%s, %s:%d", initFuncName.c_str(), initSourceFileName.c_str(), initSourceLine);
}


// --------------------------------------------------------------------------
// ObservablesGraph
// --------------------------------------------------------------------------

static const int observable_graph_key_dummy = _MAKE4C('FRPG');
static const int *observable_graph_key = &observable_graph_key_dummy;

static dag::Vector<ObservablesGraph *> all_graphs;
static OSSpinlock all_graphs_lock;

dag::Vector<ObservablesGraph *> get_all_graphs()
{
  OSSpinlockScopedLock guard(all_graphs_lock);
  dag::Vector<ObservablesGraph *> copy = all_graphs;
  return copy;
}

ObservablesGraph::ObservablesGraph(HSQUIRRELVM v, const char *graph_name) :
  vm(v), graphName(graph_name), varTraces(eastl::make_unique<NodeVarTraces>())
{
  SqStackChecker chk(vm);

  sq_pushregistrytable(vm);
  sq_pushuserpointer(vm, (SQUserPointer)observable_graph_key);

  if (SQ_SUCCEEDED(sq_rawget(vm, -2)))
  {
    sq_pop(vm, 1);
    DAG_FATAL("Only one FRP graph per VM is allowed");
  }

  sq_pushuserpointer(vm, (SQUserPointer)observable_graph_key);
  sq_pushuserpointer(vm, this);
  G_VERIFY(SQ_SUCCEEDED(sq_rawset(vm, -3)));
  sq_pop(vm, 1);

  {
    OSSpinlockScopedLock guard(all_graphs_lock);
    all_graphs.push_back(this);
  }
}

ObservablesGraph::~ObservablesGraph()
{
  OSSpinlockScopedLock guard(all_graphs_lock);
  if (auto it = eastl::find(all_graphs.begin(), all_graphs.end(), this); it != all_graphs.end())
    all_graphs.erase_unsorted(it);
}

void ObservablesGraph::setName(const char *n)
{
  if (graphName != n)
    graphName = n;
}

ObservablesGraph *ObservablesGraph::get_from_vm(HSQUIRRELVM vm)
{
  SqStackChecker chk(vm);

  sq_pushregistrytable(vm);
  sq_pushuserpointer(vm, (SQUserPointer)observable_graph_key);
  G_VERIFY(SQ_SUCCEEDED(sq_rawget(vm, -2)));
  SQUserPointer ptr = nullptr;
  sq_getuserpointer(vm, -1, &ptr);
  sq_pop(vm, 2);
  return (ObservablesGraph *)ptr;
}


// --------------------------------------------------------------------------
// Slot management
// --------------------------------------------------------------------------

NodeId ObservablesGraph::allocSlot()
{
  uint32_t idx;
  if (!freeList.empty())
  {
    idx = freeList.back();
    freeList.pop_back();
  }
  else
  {
    idx = (uint32_t)slots.size();
    slots.push_back();
  }
  NodeSlot &s = slots[idx];
  s.flags = 0;
  s.alive = true;
  s.version = 0;
  s.nodeState = NodeState::CLEAN;
  s.numNoCheckSubscribers = 0;
  s.lastChangeCause = NodeId{};
  s.numDirtySources = 0;
  s.deferredTriggerStreak = 0;
  s.lastDeferredTriggerGen = 0;
  s.timeChangeReq = 0;
  s.timeChanged = 0;
  sq_resetobject(&s.value);
  sq_resetobject(&s.func);
  s.sources.clear();
  s.dependents.clear();
  s.watchers.clear();
  s.scriptSubscribers.clear();
  s.initInfo.reset();
  varTraces->init(idx);
  return NodeId{idx, s.generation};
}

void ObservablesGraph::releaseMutatorWhiteList(uint32_t slot_index)
{
  auto wlIt = mutatorWhiteLists.find(slot_index);
  if (wlIt != mutatorWhiteLists.end())
  {
    for (HSQOBJECT &obj : wlIt->second)
      pendingRelease.push_back(obj);
    mutatorWhiteLists.erase(wlIt);
  }
}

void ObservablesGraph::freeSlot(NodeId id)
{
  G_ASSERT(id.index < slots.size());
  NodeSlot &s = slots[id.index];
  G_ASSERT(s.alive && s.generation == id.generation);

  // Collect SQ objects for deferred release (sq_release called later by
  // flushPendingRelease). Direct release would risk: (1) during shutdown()
  // iteration, cascading freeSlot on not-yet-visited nodes skips their
  // onNodeGraphShutdown; (2) mutatorWhiteLists iterator invalidation if
  // re-entrant freeSlot erases a different key from the same vector_map.
  pendingRelease.push_back(s.value);
  pendingRelease.push_back(s.func);
  sq_resetobject(&s.value);
  sq_resetobject(&s.func);

  s.sources.clear();
  s.dependents.clear();
  s.watchers.clear();
  s.scriptSubscribers.clear();
  s.initInfo.reset();

  releaseMutatorWhiteList(id.index);

  s.alive = false;
  ++s.generation;
  freeList.push_back(id.index);

  deferredNotifyQueue.erase(id);
}

void ObservablesGraph::flushPendingRelease()
{
  // Process pending releases. sq_release may cascade and trigger more
  // freeSlot calls, which append to pendingRelease -- the loop picks
  // those up naturally. Pop from back for efficiency.
  while (!pendingRelease.empty())
  {
    HSQOBJECT obj = pendingRelease.back();
    pendingRelease.pop_back();
    sq_release(vm, &obj);
  }
}

// --------------------------------------------------------------------------
// Node creation
// --------------------------------------------------------------------------

NodeId ObservablesGraph::createWatched(HSQOBJECT initial_value)
{
  NodeId id = allocSlot();
  NodeSlot &s = node(id);

  s.value = initial_value;
  sq_addref(vm, &s.value);
  if (forceImmutable)
    s.value._flags |= SQOBJ_FLAG_IMMUTABLE;

  s.isDeferred = true;
  s.needImmediate = false;

  s.initInfo = eastl::make_unique<ScriptSourceInfo>();
  s.initInfo->init(vm);

  s.timeChangeReq = s.timeChanged = ::get_time_msec();

  varTraces->save(id.index, s.value, vm);

  notifyGraphChanged();
  return id;
}

NodeId ObservablesGraph::createComputed(HSQOBJECT func_obj, dag::Vector<SourceEntry> &&sources, bool pass_cur_val)
{
  NodeId id = allocSlot();
  NodeSlot &s = node(id);

  s.isComputed = true;
  s.isDeferred = true;
  s.needImmediate = false;
  s.funcAcceptsCurVal = pass_cur_val;

  s.func = func_obj;
  sq_addref(vm, &s.func);
  s.sources = eastl::move(sources);

  s.initInfo = eastl::make_unique<ScriptSourceInfo>();
  SQFunctionInfo fi;
  if (SQ_SUCCEEDED(sq_ext_getfuncinfo(func_obj, &fi)))
  {
    s.initInfo->initFuncName = fi.name;
    s.initInfo->initSourceFileName = fi.source;
    s.initInfo->initSourceLine = fi.line;
  }
  else
    s.initInfo->init(vm);

  s.timeChangeReq = s.timeChanged = ::get_time_msec();

  notifyGraphChanged();
  return id;
}


// --------------------------------------------------------------------------
// Value access
// --------------------------------------------------------------------------

static void replace_value(HSQUIRRELVM vm, NodeSlot &s, HSQOBJECT new_val, bool force_immutable)
{
  sq_release(vm, &s.value);
  s.value = new_val;
  sq_addref(vm, &s.value);
  if (force_immutable)
    s.value._flags |= SQOBJ_FLAG_IMMUTABLE;
}


Sqrat::Object ObservablesGraph::getValue(NodeId id)
{
  NodeSlot *s = resolve(id);
  if (!s)
    return Sqrat::Object();
  if (s->isComputed)
    ensureUpToDate(id);
  return Sqrat::Object(s->value, vm);
}


bool ObservablesGraph::setValue(NodeId id, const Sqrat::Object &new_val)
{
  NodeSlot *s = resolve(id);
  if (!s)
    return logerr_graph_error(vm, "Stale NodeId");

  s->timeChangeReq = ::get_time_msec();

  const HSQOBJECT &objOld = s->value;
  const HSQOBJECT &objNew = new_val.GetObject();
  if (!sq_obj_is_equal(vm, &objOld, &objNew))
  {
    replace_value(vm, *s, objNew, forceImmutable);

    s->timeChanged = s->timeChangeReq;
    ++s->version;
    markDependentsDirty(id);

    varTraces->save(id.index, s->value, vm);
    if (!triggerRoot(id))
      return false;
  }
  return true;
}


// --------------------------------------------------------------------------
// Lifecycle
// --------------------------------------------------------------------------

void ObservablesGraph::destroyNode(NodeId id)
{
  NodeSlot *s = resolve(id);
  if (!s)
    return;

  // Notify external watchers
  s->isIteratingWatchers = true;
  for (IStateWatcher *w : s->watchers)
    w->onObservableRelease(id);
  s->isIteratingWatchers = false;

  if (currentlyDispatchingSource == id)
    currentlyDispatchingSource = NodeId{};

  // Remove from dependent lists of sources
  if (s->isComputed)
  {
    bool wasImmediate = s->needImmediate;
    for (auto &src : s->sources)
      removeDependent(src.id, id);
    if (wasImmediate)
    {
      for (auto &src : s->sources)
        updateNeedImmediate(src.id);
    }
  }

  freeSlot(id);
  notifyGraphChanged();
  flushPendingRelease();
}

void ObservablesGraph::onNodeGraphShutdown(NodeId id, bool exiting, Tab<Sqrat::Object> &cleared_storage)
{
  NodeSlot *s = resolve(id);
  if (!s)
    return;

  // Clear script subscribers
  cleared_storage.reserve(cleared_storage.size() + s->scriptSubscribers.size());
  for (Sqrat::Function &f : s->scriptSubscribers)
    cleared_storage.push_back(Sqrat::Object(f.GetFunc(), vm));
  s->scriptSubscribers.clear();
  s->numNoCheckSubscribers = 0;

  releaseMutatorWhiteList(id.index);

  if (exiting)
  {
    // Move value/func to pendingRelease (released in phase 3 of shutdown).
    if (s->isComputed)
    {
      pendingRelease.push_back(s->func);
      sq_resetobject(&s->func);
    }
    pendingRelease.push_back(s->value);
    sq_resetobject(&s->value);
  }
}

void ObservablesGraph::shutdown(bool is_closing_vm)
{
  int t0 = get_time_msec();
  DEBUG_CTX("[FRP] '%s' graph shutdown (is_closing_vm = %d): %d nodes total", graphName.c_str(), is_closing_vm, (int)nodeCount());

  stubObservableClases.clear();

  // Shutdown in 3 phases. No sq_release in phases 1-2: releasing a script object
  // can trigger ~WatchedHandle -> destroyNode -> freeSlot on an unvisited slot,
  // causing phase 1 to skip it and leak its script closures.

  // Extract all script objects from nodes (no sq_release).
  Tab<Sqrat::Object> clearedSubscribers(framemem_ptr());
  for (uint32_t i = 0; i < slots.size(); ++i)
  {
    if (!slots[i].alive)
      continue;
    NodeId id{i, slots[i].generation};
    onNodeGraphShutdown(id, is_closing_vm, clearedSubscribers);
  }

  // Free or prune node slots (freeSlot defers sq_release to pendingRelease).
  clearedSubscribers.clear();
  for (uint32_t i = 0; i < slots.size(); ++i)
  {
    NodeSlot &s = slots[i];
    if (!s.alive)
      continue;
    if (s.isComputed || is_closing_vm)
    {
      freeSlot(NodeId{i, s.generation});
    }
    else
    {
      // Watched: keep alive with value so persist-cached handles remain valid.
      s.dependents.clear();
      s.watchers.clear();
    }
  }
  deferredNotifyQueue.clear();

  // Release all collected script objects
  flushPendingRelease();

  // Not required, but keep for the compatibility with the older implementation
  sq_collectgarbage(vm);

  DEBUG_CTX("[FRP] '%s' shutdown(closing=%d) finished in %d ms, %d nodes remaining", graphName.c_str(), is_closing_vm,
    get_time_msec() - t0, (int)nodeCount());
}


// --------------------------------------------------------------------------
// Subscriptions
// --------------------------------------------------------------------------

void ObservablesGraph::addDependent(NodeId source, NodeId dependent)
{
  NodeSlot *s = resolve(source);
  if (!s)
    return;
  if (eastl::find(s->dependents.begin(), s->dependents.end(), dependent) == s->dependents.end())
    s->dependents.push_back(dependent);
}

void ObservablesGraph::removeDependent(NodeId source, NodeId dependent)
{
  NodeSlot *s = resolve(source);
  if (!s)
    return;
  erase_item_by_value(s->dependents, dependent);
}

void ObservablesGraph::subscribeWatcher(NodeId id, IStateWatcher *watcher)
{
  FRPDBG("[FRP] subscribeWatcher(node=%d, watcher=%p)", id.index, watcher);
  NodeSlot *s = resolve(id);
  if (!s)
    return;
  G_ASSERT(!s->isIteratingWatchers);
  G_ASSERT(watcher);
  if (watcher && find_value_idx(s->watchers, watcher) < 0)
  {
    s->watchers.push_back(watcher);
    updateNeedImmediate(id);
    markComputedConsumed(id);
  }
}

void ObservablesGraph::unsubscribeWatcher(NodeId id, IStateWatcher *watcher)
{
  FRPDBG("[FRP] unsubscribeWatcher(node=%d, watcher=%p)", id.index, watcher);
  NodeSlot *s = resolve(id);
  if (!s)
    return;
  G_ASSERT(!s->isIteratingWatchers);
  erase_item_by_value(s->watchers, watcher);
  updateNeedImmediate(id);
  updateComputedConsumed(id);
}


// --------------------------------------------------------------------------
// Graph processing
// --------------------------------------------------------------------------

static bool logerr_graph_error(HSQUIRRELVM vm, const char *err_msg)
{
  if (SQ_SUCCEEDED(sqstd_formatcallstackstring(vm)))
  {
    const char *stack = nullptr;
    G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, -1, &stack)));
    G_ASSERT(stack);
    logerr("[frp:error] %s\n%s", err_msg, stack);
    sq_pop(vm, 1);
  }
  else
    logerr("[frp:error] %s (no stack)", err_msg);

  return false;
}

static bool are_sq_obj_equal(HSQUIRRELVM vm, const HSQOBJECT &a, const HSQOBJECT &b) { return sq_obj_is_equal(vm, &a, &b); }


void ObservablesGraph::markDependentsDirty(NodeId id)
{
  NodeSlot *s = resolve(id);
  if (!s)
    return;
  for (NodeId dep : s->dependents)
  {
    NodeSlot *ds = resolve(dep);
    if (ds && ds->nodeState == NodeState::CLEAN)
    {
      ds->nodeState = NodeState::CHECK;
      markDependentsDirty(dep);
    }
  }
}


void ObservablesGraph::onEnterTriggerRoot() { ++triggerNestDepth; }

void ObservablesGraph::onExitTriggerRoot()
{
  --triggerNestDepth;

  if (triggerNestDepth < 0)
  {
    G_ASSERTF(false, "triggerNestDepth = %d", triggerNestDepth);
    triggerNestDepth = 0;
  }

  if (!triggerNestDepth)
    curTriggerSubscribersCounter = 0;
}


void ObservablesGraph::onEnterRecalc() { ++recalcNestDepth; }

void ObservablesGraph::onExitRecalc()
{
  --recalcNestDepth;

  if (recalcNestDepth < 0)
  {
    G_ASSERTF(false, "recalcNestDepth = %d", recalcNestDepth);
    recalcNestDepth = 0;
  }
}


void ObservablesGraph::notifyGraphChanged()
{
  if (recalcNestDepth > 0)
  {
    if (inRecalc.isValid())
    {
      String rinfo;
      fillInfo(inRecalc, rinfo);
      logerr_graph_error(vm,
        String(0, "FRP graph %p changed during recalc of %s\nMove side-effects to subscriber", this, rinfo.c_str()).c_str());
    }
    else
    {
      logerr_graph_error(vm,
        String(0, "FRP graph %p changed during graph computation\nMove side-effects to subscriber", this).c_str());
    }
  }
}


static bool isInChangeCauseCycle(ObservablesGraph *graph, NodeId id)
{
  NodeSlot *s = graph->resolve(id);
  if (!s)
    return false;
  NodeId cur = s->lastChangeCause;
  while (cur.isValid() && cur != id)
  {
    NodeSlot *cs = graph->resolve(cur);
    if (!cs)
      return false;
    cur = cs->lastChangeCause;
  }
  return cur == id;
}


bool ObservablesGraph::trigger(NodeId id)
{
  NodeSlot *s = resolve(id);
  if (!s)
    return logerr_graph_error(vm, String(0, "trigger: stale NodeId (index=%u, gen=%u)", id.index, id.generation).c_str());
  ++s->version;
  markDependentsDirty(id);
  return triggerRoot(id);
}


bool ObservablesGraph::notifyWatchers(NodeId id)
{
  NodeSlot *s = resolve(id);
  if (!s)
    return true;
  bool ok = true;
  G_ASSERT(!s->isIteratingWatchers);
  s->isIteratingWatchers = true;
  for (IStateWatcher *w : s->watchers)
  {
    G_ASSERT(w);
    if (!w->onSourceObservableChanged())
    {
      ok = false;
      logerr_graph_error(vm, "Graph update failed (notifying watcher)");
    }
  }
  s->isIteratingWatchers = false;
  return ok;
}


bool ObservablesGraph::triggerRoot(NodeId id)
{
  NodeSlot *s = resolve(id);
  if (!s)
    return logerr_graph_error(vm, String(0, "triggerRoot: stale NodeId (index=%u, gen=%u)", id.index, id.generation).c_str());

  if (s->isInTrigger)
    return logerr_graph_error(vm, "Trigger root: already in trigger - probably side effect in Computed");

  s->lastChangeCause = currentlyDispatchingSource;

  if (inRecalc.isValid())
  {
    String info, rinfo;
    fillInfo(id, info);
    fillInfo(inRecalc, rinfo);
    logerr("%s triggered during recalc of %s", info.c_str(), rinfo.c_str());
  }

  auto t0 = profile_ref_ticks();
  onEnterTriggerRoot();

  bool ok = true;
  NodeIdVec changedNodes; // nodes that changed during pull

  s->isInTrigger = true;

  // Queue deferred root for later, or include in immediate processing
  if (!s->needImmediate)
    deferredNotifyQueue.insert(id);

  // Pull-based evaluation of downstream immediate Computed nodes.
  // If needImmediate is false on the root, no downstream node can be immediate
  // (needImmediate propagates upward through sources), so the DFS is skipped.
  if (s->needImmediate)
  {
    {
      TIME_PROFILE(frp_trigger_pull);

      // Collect all downstream Computed nodes via DFS
      dag::Vector<NodeId, framemem_allocator> downstream;
      {
        dag::Vector<NodeId, framemem_allocator> stack;
        stack.push_back(id);
        while (!stack.empty())
        {
          NodeId cur = stack.back();
          stack.pop_back();
          NodeSlot *cs = resolve(cur);
          if (!cs)
            continue;
          for (NodeId dep : cs->dependents)
          {
            NodeSlot *ds = resolve(dep);
            if (ds && !ds->isMarked)
            {
              ds->isMarked = true;
              downstream.push_back(dep);
              stack.push_back(dep);
            }
          }
        }
      }

      // Pull immediate+used nodes
      for (NodeId dep : downstream)
      {
        NodeSlot *ds = resolve(dep);
        if (ds && ds->nodeState != NodeState::CLEAN && ds->needImmediate && ds->computedHasActiveConsumers)
          pull(dep, changedNodes);
      }

      // Clean up isMarked
      for (NodeId dep : downstream)
      {
        NodeSlot *ds = resolve(dep);
        if (ds)
          ds->isMarked = false;
      }

      DA_PROFILE_TAG(frp_trigger_pull, "downstream=%d", (int)downstream.size());
    }

    // Re-resolve after pull (slot vector may have been reallocated by script callbacks)
    s = resolve(id);

    // Notify watchers of root + all changed computed nodes
    if (s)
      if (!notifyWatchers(id))
        ok = false;
    for (NodeId nid : changedNodes)
      if (!notifyWatchers(nid))
        ok = false;
  }

  // Call subscribers
  if (!callScriptSubscribers(s && s->needImmediate ? id : NodeId{}, changedNodes))
    ok = false;

  // Re-resolve after script callbacks
  s = resolve(id);
  if (s)
    s->isInTrigger = false;

  onExitTriggerRoot();
  usecTrigger += profile_time_usec(t0);

  return ok;
}


void ObservablesGraph::collectScriptSubscribers(NodeId id, Tab<SubscriberCall> &container) const
{
  const NodeSlot *s = resolve(id);
  if (!s || s->scriptSubscribers.empty())
    return;
  Sqrat::Object val(s->value, vm);
  int start = container.size();
  int count = s->scriptSubscribers.size();
  append_items(container, count);
  for (int i = 0; i < count; ++i)
  {
    SubscriberCall &sc = container[start + i];
    sc.func = s->scriptSubscribers[i];
    sc.value = val;
    sc.source = id;
    sc.check = (i >= s->numNoCheckSubscribers);
  }
}


bool ObservablesGraph::callScriptSubscribers(NodeId triggered_node, NodeIdVec &notify_queue)
{
  if (triggered_node.isValid())
    collectScriptSubscribers(triggered_node, subscriberCallsQueue);
  for (NodeId nid : notify_queue)
    collectScriptSubscribers(nid, subscriberCallsQueue);
  notify_queue.clear();

  if (subscriberCallsQueue.size() > MAX_ALLOWED_SUBSCRIBERS_QUEUE_LENGTH)
  {
    logerr_graph_error(vm,
      String(0, "Max subscribers queue length exceeded (current size = %d), see log for details", subscriberCallsQueue.size())
        .c_str());
    dumpSubscribersQueue(subscriberCallsQueue);
    subscriberCallsQueue.clear();
    return false;
  }
  maxSubscribersQueueLen = ::max<int>(maxSubscribersQueueLen, subscriberCallsQueue.size());

  bool isCallingSubscribers = !curSubscriberCalls.empty();
  if (isCallingSubscribers) // leave processing to top level call
    return true;

  auto t0 = profile_ref_ticks();
  bool ok = true;
  for (;;)
  {
    if (subscriberCallsQueue.empty())
      break;

    G_ASSERT(curSubscriberCalls.empty());
    curSubscriberCalls.swap(subscriberCallsQueue);

    curTriggerSubscribersCounter += curSubscriberCalls.size();
    G_ASSERT(triggerNestDepth > 0);
    if (triggerNestDepth > 0 && curTriggerSubscribersCounter > MAX_SUBSCRIBERS_PER_TRIGGER)
    {
      logerr_graph_error(vm,
        String(0, "Subscribers calls count exceeded (%d), see log for details", curTriggerSubscribersCounter).c_str());
      dumpSubscribersQueue(curSubscriberCalls);
      dumpSubscribersQueue(subscriberCallsQueue);
      curSubscriberCalls.clear();
      subscriberCallsQueue.clear();
      ok = false;
      break;
    }

    for (SubscriberCall &sc : curSubscriberCalls)
    {
      currentlyDispatchingSource = sc.source;
      callingSubscriber = sc.check && checkSubscribers;
      curSubscriberThresholdUsec = slowSubscriberThresholdUsec;
      auto t1 = profile_ref_ticks();
      if (!sc.func.Execute(sc.value))
        ok = false; // VM already logged the script error
      else
      {
        auto callUsec = profile_time_usec(t1);
        bool needToLog = callUsec > curSubscriberThresholdUsec;
#if DAGOR_DBGLEVEL > 0
        needToLog |= callUsec > slowestSubscribersAllTime.back().timeUsec;
#endif
        if (needToLog)
        {
          SubscriberTiming timing;
          timing.timeUsec = callUsec;
          SQFunctionInfo fi;
          if (SQ_SUCCEEDED(sq_ext_getfuncinfo(sc.func.GetFunc(), &fi)))
            timing.name.printf(0, "%s (%s:%d)", fi.name, fi.source, fi.line);
          else
            timing.name = "<unknown>";

          if (callUsec > curSubscriberThresholdUsec)
          {
            String sourceInfo;
            NodeSlot *srcSlot = resolve(sc.source);
            if (srcSlot)
              fillInfo(sc.source, sourceInfo);
            String chain;
            bool uncertain = srcSlot && srcSlot->numDirtySources > 1;
            if (uncertain)
              chain.aprintf(0, " (%d dirty srcs)", srcSlot->numDirtySources);
            NodeId cause = srcSlot ? srcSlot->lastChangeCause : NodeId{};
            for (int depth = 0; cause.isValid() && depth < 10; depth++)
            {
              NodeSlot *causeSlot = resolve(cause);
              if (!causeSlot)
                break;
              String causeInfo;
              fillInfo(cause, causeInfo);
              chain.aprintf(0, " <- %s%s", uncertain ? "(?) " : "", causeInfo.c_str());
              if (!uncertain && causeSlot->numDirtySources > 1)
              {
                uncertain = true;
                chain.aprintf(0, " (%d dirty srcs)", causeSlot->numDirtySources);
              }
              cause = causeSlot->lastChangeCause;
            }
            String fmt(0, "slow subscriber call (%%dus>%%dus): %s, subscriber of %%s%%s", timing.name.c_str());
            logerr(fmt.c_str(), callUsec, curSubscriberThresholdUsec, sourceInfo.empty() ? "<unknown>" : sourceInfo.c_str(),
              chain.c_str());
          }
#if DAGOR_DBGLEVEL > 0
          {
            SubscriberTiming episodeTiming = timing;
            for (auto &tm : slowestSubscribersAllTime)
              if (timing.timeUsec > tm.timeUsec)
                eastl::swap(timing, tm);
            for (auto &tm : slowestSubscribersLastEpisode)
              if (episodeTiming.timeUsec > tm.timeUsec)
                eastl::swap(episodeTiming, tm);
          }
#endif
        }
      }
    }
    currentlyDispatchingSource = NodeId{};
    callingSubscriber = false;
    curSubscriberThresholdUsec = -1;
    curSubscriberCalls.clear();
  }
  usecSubscribers += profile_time_usec(t0);
  return ok;
}


void ObservablesGraph::dumpSubscribersQueue(Tab<SubscriberCall> &queue)
{
  SQFunctionInfo fi;
  debug("[FRP] subscribers queue dump (%d functions):", queue.size());
  for (SubscriberCall &sc : queue)
  {
    Sqrat::Var<const char *> valStr = sc.value.GetVar<const char *>();

    if (SQ_SUCCEEDED(sq_ext_getfuncinfo(sc.func.GetFunc(), &fi)))
      debug("* %s (%s:%d), value = %s", fi.name, fi.source, fi.line, valStr.value);
    else
      debug("* <unknown>, value = %s", valStr.value);
  }
}


bool ObservablesGraph::updateDeferred()
{
  TIME_PROFILE(frp_update_deferred);
  FRPDBG("[FRP] ObservablesGraph(%p)::updateDeferred()", this);
  G_ASSERT(triggerNestDepth == 0);

  auto t0 = profile_ref_ticks();
  onEnterTriggerRoot();

  bool ok = true;

  // Cross-frame cycle detection
  ++deferredUpdateGen;
  constexpr int MAX_CONSECUTIVE_DEFERRED = 10;
  for (NodeId nid : deferredNotifyQueue)
  {
    NodeSlot *s = resolve(nid);
    if (!s)
      continue;
    uint16_t frameGap = deferredUpdateGen - s->lastDeferredTriggerGen;
    if (frameGap <= 2)
      ++s->deferredTriggerStreak;
    else
      s->deferredTriggerStreak = 1;
    s->lastDeferredTriggerGen = deferredUpdateGen;
    if (s->deferredTriggerStreak == MAX_CONSECUTIVE_DEFERRED && isInChangeCauseCycle(this, nid))
    {
      String msg;
      String info;
      fillInfo(nid, info);
      msg.printf(0, "[FRP] Observable triggered in %d consecutive deferred updates - cross-frame cycle: %s", MAX_CONSECUTIVE_DEFERRED,
        info.c_str());
      if (s->lastChangeCause.isValid())
      {
        fillInfo(s->lastChangeCause, info);
        msg.aprintf(0, " | caused by: %s", info.c_str());
      }
      logerr("%s", msg.c_str());
    }
  }

  NodeIdVec deferredBatch(deferredNotifyQueue.begin(), deferredNotifyQueue.end());
  deferredNotifyQueue.clear();

  // Pull-based evaluation of dirty computed nodes
  auto startRecalc = profile_ref_ticks();
  onEnterRecalc();

  NodeIdVec changedNodes;
  {
    TIME_PROFILE(frp_deferred_recalc);
    computedRecalcs = 0;

    // Collect dirty computed nodes
    dag::Vector<NodeId, framemem_allocator> dirtyNodes;
    for (uint32_t i = 0; i < slots.size(); ++i)
    {
      NodeSlot &slot = slots[i];
      if (slot.alive && slot.isComputed && slot.nodeState != NodeState::CLEAN)
        dirtyNodes.push_back(NodeId{i, slot.generation});
    }

    // Pull all dirty, used computed nodes
    for (NodeId dep : dirtyNodes)
    {
      NodeSlot *ds = resolve(dep);
      if (ds && ds->nodeState != NodeState::CLEAN && ds->computedHasActiveConsumers)
        pull(dep, changedNodes);
    }

    DA_PROFILE_TAG(frp_deferred_recalc, "dirty=%d recalc=%d", (int)dirtyNodes.size(), computedRecalcs);
  }

  onExitRecalc();
  int timeRecalc = profile_time_usec(startRecalc);

  // Mark changed computed nodes to detect overlap with deferredBatch
  for (NodeId nid : changedNodes)
  {
    NodeSlot *s = resolve(nid);
    if (s)
      s->isMarked = true;
  }

  // Merge deferred roots into changedNodes (skip duplicates)
  for (NodeId nid : deferredBatch)
  {
    NodeSlot *s = resolve(nid);
    if (s && !s->isMarked)
      changedNodes.push_back(nid);
  }

  // Notify watchers + clean up isMarked
  for (NodeId nid : changedNodes)
  {
    NodeSlot *s = resolve(nid);
    if (s)
      s->isMarked = false;
    if (!notifyWatchers(nid))
      ok = false;
  }

  auto t1 = profile_ref_ticks();
  {
    TIME_PROFILE(frp_deferred_notify);
    if (!callScriptSubscribers(NodeId{}, changedNodes))
      ok = false;
  }
  int timeNotify = profile_time_usec(t1);

  onExitTriggerRoot();

  int updateTime = profile_time_usec(t0);
  usecUpdateDeferred += updateTime;

  if (updateTime > slowUpdateThresholdUsec && nodeCount() > 0 && is_app_working())
  {
#if DAGOR_DBGLEVEL > 0
    timeTotalRecalc += timeRecalc;
    timeTotalNotify += timeNotify;
    timeMaxRecalc = max(timeMaxRecalc, timeRecalc);
    timeMaxNotify = max(timeMaxNotify, timeNotify);
#endif

    if (++slowUpdateFrames == 60 && !reportedSlowUpdate)
    {
      reportedSlowUpdate = true;
      G_UNUSED(timeRecalc);
      G_UNUSED(timeNotify);
#if DAGOR_DBGLEVEL > 0
      String msg;
      msg.aprintf(0, "%6d us max  %6d us avg: RECALC\n", timeMaxRecalc, timeTotalRecalc / slowUpdateFrames);
      msg.aprintf(0, "%6d us max  %6d us avg: NOTIFYING SUBSCRIBERS\n", timeMaxNotify, timeTotalNotify / slowUpdateFrames);
      msg.aprintf(0, "Slowest subscribers during this episode:\n");
      for (auto &s : slowestSubscribersLastEpisode)
        if (s.timeUsec > 0)
          msg.aprintf(1024, "%6d us: SUBS %s\n", s.timeUsec, s.name.c_str());
      msg.aprintf(0, "Slowest subscribers all-time:\n");
      for (auto &s : slowestSubscribersAllTime)
        if (s.timeUsec > 0)
          msg.aprintf(1024, "%6d us: SUBS %s\n", s.timeUsec, s.name.c_str());
      logerr("every FRP update is consistently slow (> %d us), last: %d us:\n%s", slowUpdateThresholdUsec, updateTime, msg.c_str());
#endif
    }
  }
  else
  {
    slowUpdateFrames = 0;
#if DAGOR_DBGLEVEL > 0
    timeTotalRecalc = 0;
    timeTotalNotify = 0;
    timeMaxRecalc = 0;
    timeMaxNotify = 0;
    slowestSubscribersLastEpisode = {};
#endif
  }

  FRPDBG("[FRP] / ObservablesGraph(%p)::updateDeferred()", this);
  return ok;
}


bool ObservablesGraph::pull(NodeId id, NodeIdVec &changed_nodes)
{
  NodeSlot *s = resolve(id);
  if (!s || s->nodeState == NodeState::CLEAN)
    return false;

  if (s->isPulling)
  {
    String info;
    fillInfo(id, info);
    logerr("[FRP] Circular dependency detected during pull: %s", info.c_str());
    s->nodeState = NodeState::CLEAN;
    return false;
  }

  s->isPulling = true;

  if (s->nodeState == NodeState::CHECK)
  {
    // Verify if any source actually changed
    for (auto &src : s->sources)
    {
      NodeSlot *srcSlot = resolve(src.id);
      if (srcSlot && srcSlot->isComputed)
        pull(src.id, changed_nodes);
      // Re-resolve after pull (slot vector may have been reallocated)
      srcSlot = resolve(src.id);
      if (srcSlot && srcSlot->version != src.lastSeenVersion)
      {
        s->lastChangeCause = src.id;
        s->nodeState = NodeState::DIRTY;
        break;
      }
    }

    if (s->nodeState == NodeState::CHECK)
    {
      s->numDirtySources = 0;
      s->nodeState = NodeState::CLEAN;
      s->isPulling = false;
      return false;
    }

    // Count other dirty sources for diagnostics
    s->numDirtySources = 1;
    for (auto &src : s->sources)
    {
      NodeSlot *srcSlot = resolve(src.id);
      if (srcSlot && src.id != s->lastChangeCause && srcSlot->version != src.lastSeenVersion && s->numDirtySources < 255)
        s->numDirtySources++;
    }
  }

  // state == DIRTY: recompute
  bool ok = true;
  bool changed = recalculate(id, ok);

  // Re-resolve after recalculate (script callbacks may reallocate)
  s = resolve(id);
  if (!s)
    return changed;

  if (changed)
    changed_nodes.push_back(id);

  s->isPulling = false;
  return changed;
}


bool ObservablesGraph::recalculate(NodeId id, bool &ok)
{
  TIME_PROFILE(computed_recalculate);

  NodeSlot *s = resolve(id);
  if (!s)
    return false;

#if FRP_DEBUG_MODE
  {
    FRPDBG("@#! RECALC [%s:%d %s]", s->initInfo->initSourceFileName.c_str(), s->initInfo->initSourceLine,
      s->initInfo->initFuncName.c_str());
  }
#endif

  auto prevRecalc = inRecalc;
  inRecalc = id;

  auto t0 = profile_ref_ticks();

  Sqrat::Function func(vm, Sqrat::Object(vm), s->func);
  Sqrat::optional<Sqrat::Object> optNewVal;
  if (s->funcAcceptsCurVal)
    optNewVal = func.Eval<Sqrat::Object>(Sqrat::Object(s->value, vm));
  else
    optNewVal = func.Eval<Sqrat::Object>();
  bool callSucceeded = optNewVal.has_value();
  Sqrat::Object newVal = callSucceeded ? SQRAT_STD::move(optNewVal.value()) : Sqrat::Object();

  inRecalc = prevRecalc;

  // Re-resolve after script callback
  s = resolve(id);
  if (!s)
    return false;

  s->nodeState = NodeState::CLEAN;

  // Snapshot source versions after recompute
  for (auto &src : s->sources)
  {
    NodeSlot *srcSlot = resolve(src.id);
    if (srcSlot)
      src.lastSeenVersion = srcSlot->version;
  }

  int timeUsec = profile_time_usec(t0);
  (s->computedHasActiveConsumers ? usecUsedComputed : usecUnusedComputed) += timeUsec;
  computedRecalcs++;

  if (timeUsec > 5000 && is_app_working())
  {
    String info;
    fillInfo(id, info);
    logwarn("slow Computed (%dus): %s", timeUsec, info.c_str());
  }

  if (!callSucceeded)
  {
    ok = false;
    return false;
  }

  s->timeChangeReq = ::get_time_msec();

  const HSQOBJECT &objOld = s->value;
  const HSQOBJECT &objNew = newVal.GetObject();
  if (sq_fast_equal_by_value_deep(&objOld, &objNew, 1))
    return false;

  replace_value(vm, *s, newVal.GetObject(), true); // Computed values are always immutable
  s->timeChanged = s->timeChangeReq;
  ++s->version;
  varTraces->save(id.index, s->value, vm);

  return true;
}


void ObservablesGraph::ensureUpToDate(NodeId id)
{
  NodeSlot *s = resolve(id);
  if (s && s->nodeState != NodeState::CLEAN)
  {
    NodeIdVec changed;
    pull(id, changed);
    // Defer notifications to next updateDeferred()
    for (NodeId nid : changed)
      deferredNotifyQueue.insert(nid);
  }
}


void ObservablesGraph::recalcAllComputedValues()
{
  // Mark all computed nodes as DIRTY
  for (auto &s : slots)
    if (s.alive && s.isComputed)
      s.nodeState = NodeState::DIRTY;

  // Pull all used computed nodes
  NodeIdVec changedNodes;
  onEnterTriggerRoot();
  for (uint32_t i = 0; i < slots.size(); ++i)
  {
    NodeSlot &s = slots[i];
    if (s.alive && s.isComputed && s.nodeState != NodeState::CLEAN && s.computedHasActiveConsumers)
      pull(NodeId{i, s.generation}, changedNodes);
  }
  for (NodeId nid : changedNodes)
    notifyWatchers(nid);
  callScriptSubscribers(NodeId{}, changedNodes);
  onExitTriggerRoot();
}


ObservablesGraph::Stats ObservablesGraph::gatherGraphStats() const
{
  Stats st{};

  for (uint32_t i = 0; i < slots.size(); ++i)
  {
    const NodeSlot &s = slots[i];
    if (!s.alive)
      continue;
    st.totalSubscribers += (int)s.scriptSubscribers.size();
    st.totalWatchers += (int)s.watchers.size();
    if (s.isComputed)
    {
      st.computedTotal++;
      if (!s.computedHasActiveConsumers)
        st.computedUnused++;
      if (s.scriptSubscribers.empty())
        st.computedUnsubscribed++;
    }
    else
    {
      st.watchedTotal++;
      if (s.scriptSubscribers.empty())
      {
        st.watchedUnsubscribed++;
        if (s.watchers.empty())
          st.watchedUnused++;
      }
    }
  }

  return st;
}


void ObservablesGraph::markImmediate(NodeId id)
{
  NodeSlot *s = resolve(id);
  if (!s || s->needImmediate)
    return;
  s->needImmediate = true;
  if (s->isComputed)
  {
    for (auto &src : s->sources)
      markImmediate(src.id);
  }
}

void ObservablesGraph::updateNeedImmediate(NodeId id)
{
  NodeSlot *s = resolve(id);
  if (!s)
    return;

  bool imm = !s->isDeferred;
  if (!imm)
  {
    for (NodeId dep : s->dependents)
    {
      NodeSlot *ds = resolve(dep);
      if (ds && ds->needImmediate)
      {
        imm = true;
        break;
      }
    }
  }

  if (imm == s->needImmediate)
    return;
  s->needImmediate = imm;
  if (s->isComputed)
  {
    for (auto &src : s->sources)
    {
      if (imm)
        markImmediate(src.id);
      else
        updateNeedImmediate(src.id);
    }
  }
}


void ObservablesGraph::markComputedConsumed(NodeId id)
{
  NodeSlot *s = resolve(id);
  if (!s || !s->isComputed || s->computedHasActiveConsumers)
    return;

  s->computedHasActiveConsumers = true;
  for (auto &src : s->sources)
    markComputedConsumed(src.id);
}


void ObservablesGraph::updateComputedConsumed(NodeId id)
{
  NodeSlot *s = resolve(id);
  if (!s || !s->isComputed)
    return;

  bool consumed = !s->scriptSubscribers.empty();
  if (!consumed)
  {
    for (NodeId dep : s->dependents)
    {
      NodeSlot *ds = resolve(dep);
      if (ds && ds->computedHasActiveConsumers)
      {
        consumed = true;
        break;
      }
    }
    if (!consumed && !s->watchers.empty())
      consumed = true;
  }

  if (consumed == s->computedHasActiveConsumers)
    return;

  s->computedHasActiveConsumers = consumed;
  for (auto &src : s->sources)
  {
    if (consumed)
      markComputedConsumed(src.id);
    else
      updateComputedConsumed(src.id);
  }
}


// --------------------------------------------------------------------------
// Stub observable classes
// --------------------------------------------------------------------------

static bool is_same_class(const Sqrat::Object &a, const Sqrat::Object &b)
{
  return a.GetType() == OT_CLASS && b.GetType() == OT_CLASS && a.GetObject()._unVal.pClass == b.GetObject()._unVal.pClass;
}


SQInteger ObservablesGraph::register_stub_observable_class(HSQUIRRELVM vm)
{
  ObservablesGraph *self = get_from_vm(vm);
  Sqrat::Var<Sqrat::Object> cls(vm, 2);

  if (eastl::find_if(self->stubObservableClases.begin(), self->stubObservableClases.end(),
        [&cls](const Sqrat::Object &o) { return is_same_class(o, cls.value); }) == self->stubObservableClases.end())
  {
    self->stubObservableClases.push_back(cls.value);
  }

  return 0;
}


bool ObservablesGraph::isStubObservableInstance(const Sqrat::Object &inst) const
{
  if (DAGOR_LIKELY(stubObservableClases.empty()))
    return false;

  SqStackChecker chk(vm);
  sq_pushobject(vm, inst.GetObject());
  G_VERIFY(SQ_SUCCEEDED(sq_getclass(vm, -1)));
  Sqrat::Object cls = Sqrat::Var<Sqrat::Object>(vm, -1).value;
  chk.restore();

  bool res = eastl::find_if(stubObservableClases.begin(), stubObservableClases.end(),
               [&cls](const Sqrat::Object &o) { return is_same_class(o, cls); }) != stubObservableClases.end();
  return res;
}


// --------------------------------------------------------------------------
// Info and diagnostics
// --------------------------------------------------------------------------

void ObservablesGraph::fillInfo(NodeId id, Sqrat::Table &t) const
{
  const NodeSlot *s = resolve(id);
  if (!s || !s->initInfo)
    return;
  s->initInfo->fillInfo(t);
  t.SetValue("value", Sqrat::Object(s->value, vm));
}

void ObservablesGraph::fillInfo(NodeId id, String &str) const
{
  const NodeSlot *s = resolve(id);
  if (!s)
  {
    str = "<stale node>";
    return;
  }
  if (!s->initInfo)
  {
    str = "<no init info>";
    return;
  }
  s->initInfo->toString(str);

  sq_pushobject(vm, s->value);
  if (SQ_SUCCEEDED(sq_tostring(vm, -1)))
  {
    const char *valStr = nullptr;
    G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, -1, &valStr)));
    str.aprintf(0, ", value = %s", valStr);
    sq_pop(vm, 1);
  }
  sq_pop(vm, 1);
}


// --------------------------------------------------------------------------
// Script subscriber management
// --------------------------------------------------------------------------

SQInteger ObservablesGraph::subscribe(HSQUIRRELVM vm, bool check_behavior)
{
  if (!Sqrat::check_signature<WatchedHandle *>(vm))
    return SQ_ERROR;

  Sqrat::Var<WatchedHandle *> self(vm, 1);
  WatchedHandle *h = self.value;
  if (!h || !h->graph)
    return sq_throwerror(vm, "Invalid observable");

  NodeSlot *s = h->graph->resolve(h->id);
  if (!s)
    return sq_throwerror(vm, "Stale observable");

  HSQOBJECT func;
  sq_getstackobj(vm, 2, &func);

  SQInteger nparams = 0, nfreevars = 0;
  G_VERIFY(SQ_SUCCEEDED(sq_getclosureinfo(vm, 2, &nparams, &nfreevars)));
  bool valid = (nparams == 2) || (nparams <= -2);
  if (!valid)
    return sqstd_throwerrorf(vm, "Subscriber function must accept 2 parameters (actual count is %d)", nparams);

  bool isNew = eastl::find_if(s->scriptSubscribers.begin(), s->scriptSubscribers.end(), [&](const Sqrat::Function &f) {
    G_ASSERT(vm == f.GetVM());
    return are_sq_obj_equal(vm, func, f.GetFunc());
  }) == s->scriptSubscribers.end();

  if (isNew)
  {
    if (!check_behavior)
    {
      if (s->numNoCheckSubscribers >= 255)
        return sq_throwerror(vm, "Non-checked subscriber count is 255 max. Limit exceeded.");
      // Insert at partition boundary (before first checked subscriber)
      s->scriptSubscribers.insert(s->scriptSubscribers.begin() + s->numNoCheckSubscribers,
        Sqrat::Function(vm, Sqrat::Object(vm), func));
      s->numNoCheckSubscribers++;
    }
    else
    {
      // Checked subscribers go at the end
      s->scriptSubscribers.push_back(Sqrat::Function(vm, Sqrat::Object(vm), func));
    }
    if (s->isComputed)
      h->graph->markComputedConsumed(h->id);
  }

  sq_push(vm, 1);
  return 1;
}


SQInteger ObservablesGraph::unsubscribe(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<WatchedHandle *>(vm))
    return SQ_ERROR;

  Sqrat::Var<WatchedHandle *> self(vm, 1);
  WatchedHandle *h = self.value;
  if (!h || !h->graph)
    return sq_throwerror(vm, "Invalid observable");

  NodeSlot *s = h->graph->resolve(h->id);
  if (!s)
    return sq_throwerror(vm, "Stale observable");

  HSQOBJECT func;
  sq_getstackobj(vm, 2, &func);

  for (int i = 0, n = s->scriptSubscribers.size(); i < n; ++i)
  {
    G_ASSERT(vm == s->scriptSubscribers[i].GetVM());
    if (are_sq_obj_equal(vm, func, s->scriptSubscribers[i].GetFunc()))
    {
      if (i < s->numNoCheckSubscribers)
        s->numNoCheckSubscribers--;
      erase_items(s->scriptSubscribers, i, 1);
      if (s->scriptSubscribers.empty() && s->isComputed)
        h->graph->updateComputedConsumed(h->id);
      break;
    }
  }
  return 1;
}


SQInteger ObservablesGraph::sqTrigger(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<WatchedHandle *>(vm))
    return SQ_ERROR;

  Sqrat::Var<WatchedHandle *> self(vm, 1);
  WatchedHandle *h = self.value;
  if (!h || !h->graph)
    return sq_throwerror(vm, "Invalid observable");

  h->graph->trigger(h->id);
  return 1;
}

SQInteger ObservablesGraph::dbgGetListeners(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<WatchedHandle *>(vm))
    return SQ_ERROR;

  Sqrat::Var<WatchedHandle *> self(vm, 1);
  WatchedHandle *h = self.value;
  if (!h || !h->graph)
    return sq_throwerror(vm, "Invalid observable");

  NodeSlot *s = h->graph->resolve(h->id);
  if (!s)
    return sq_throwerror(vm, "Stale observable");

  uint32_t nWatchers = s->watchers.size();
  Sqrat::Array arrWatchers(vm, nWatchers);
  for (uint32_t i = 0; i < nWatchers; ++i)
    arrWatchers.SetValue(i, s->watchers[i]->dbgGetWatcherScriptInstance());

  uint32_t nSubscribers = s->scriptSubscribers.size();
  Sqrat::Array arrSubscribers(vm, nSubscribers);
  for (uint32_t i = 0; i < nSubscribers; ++i)
    arrSubscribers.SetValue(i, s->scriptSubscribers[i]);

  Sqrat::Table res(vm);
  res.SetValue("watchers", arrWatchers);
  res.SetValue("subscribers", arrSubscribers);
  sq_pushobject(vm, res);

  return 1;
}


// --------------------------------------------------------------------------
// Source collection for Computed
// --------------------------------------------------------------------------

using VisitedSet = eastl::hash_set<SQRawObjectVal>;

static bool try_add_source(dag::Vector<SourceEntry> &sources, WatchedHandle *h, const char *var_name)
{
  if (!h || !h->id.isValid())
    return false;
  bool isUnique =
    eastl::find_if(sources.begin(), sources.end(), [id = h->id](const SourceEntry &src) { return src.id == id; }) == sources.end();
  if (!isUnique)
    return false;
  SourceEntry &src = sources.push_back();
  src.varName = var_name;
  src.id = h->id;
  return true;
}

static bool collect_sources_from_free_vars(ObservablesGraph *graph, HSQUIRRELVM vm, HSQOBJECT func_obj,
  dag::Vector<SourceEntry> &sources, VisitedSet &visited)
{
  auto [_, inserted] = visited.insert(func_obj._unVal.raw);
  if (!inserted)
    return false;

  bool areSourcesFound = false;

  SqStackChecker stackCheck(vm);
  sq_pushobject(vm, func_obj);
  SQInteger nparams = 0, nfreevars = 0;
  G_VERIFY(SQ_SUCCEEDED(sq_getclosureinfo(vm, -1, &nparams, &nfreevars)));

  for (SQInteger i = 0; i < nfreevars; ++i)
  {
    const char *varName = sq_getfreevariable(vm, -1, i);
    G_ASSERT(varName);
    if (varName)
    {
      SQObjectType varType = sq_gettype(vm, -1);
      if (varType == OT_CLOSURE)
      {
        HSQOBJECT f;
        sq_getstackobj(vm, -1, &f);
        if (graph->doCollectSourcesRecursively || _closure(f)->_function->_inside_hoisted_scope)
        {
          if (collect_sources_from_free_vars(graph, vm, f, sources, visited))
            areSourcesFound = true;
        }
      }
      else if (Sqrat::ClassType<WatchedHandle>::IsClassInstance(vm, -1))
      {
        if (try_add_source(sources, Sqrat::Var<WatchedHandle *>(vm, -1).value, varName))
          areSourcesFound = true;
      }
      else if (varType == OT_INSTANCE && graph->isStubObservableInstance(Sqrat::Var<Sqrat::Object>(vm, -1).value))
      {
        areSourcesFound = true;
      }
    }
    sq_pop(vm, 1);
  }
  sq_pop(vm, 1); // func_obj

  stackCheck.check();

  G_ASSERT_RETURN(func_obj._type == OT_CLOSURE, areSourcesFound);
  SQFunctionProto *proto = func_obj._unVal.pClosure->_function;
  for (SQInteger iConst = 0; iConst < proto->_nliterals; ++iConst)
  {
    SQObjectPtr &constant = proto->_literals[iConst];
    SQObjectType constType = sq_type(constant);

    if (constType == OT_CLOSURE)
    {
      if (graph->doCollectSourcesRecursively || _closure(constant)->_function->_inside_hoisted_scope)
      {
        if (collect_sources_from_free_vars(graph, vm, constant, sources, visited))
          areSourcesFound = true;
      }
    }
    else if (Sqrat::ClassType<WatchedHandle>::IsClassInstance(constant))
    {
      SqStackChecker stackCheck2(vm);
      sq_pushobject(vm, constant);
      if (try_add_source(sources, Sqrat::Var<WatchedHandle *>(vm, -1).value, "#constant"))
        areSourcesFound = true;
      sq_pop(vm, 1);
    }
    else if (constType == OT_INSTANCE && graph->isStubObservableInstance(Sqrat::Object(HSQOBJECT(constant), vm)))
    {
      areSourcesFound = true;
    }
  }

  return areSourcesFound;
}


// --------------------------------------------------------------------------
// WatchedHandle
// --------------------------------------------------------------------------

WatchedHandle::~WatchedHandle()
{
  if (graph && id.isValid())
    graph->destroyNode(id);
}


Sqrat::Object WatchedHandle::getValue()
{
  if (!graph || !id.isValid())
    return Sqrat::Object();
  return graph->getValue(id);
}

Sqrat::Object WatchedHandle::getValueDeprecated()
{
  if (use_deprecated_methods_warning && graph)
    logerr_graph_error(graph->vm, ".value is deprecated, use get() instead");
  return getValue();
}

bool WatchedHandle::getDeferred() const
{
  if (!graph)
    return false;
  const NodeSlot *s = graph->resolve(id);
  return s ? s->isDeferred : false;
}

void WatchedHandle::setDeferred(bool v)
{
  if (!graph)
    return;
  NodeSlot *s = graph->resolve(id);
  if (s)
  {
    s->isDeferred = v;
    graph->updateNeedImmediate(id);
  }
}

int WatchedHandle::getTimeChangeReq() const
{
  if (!graph)
    return 0;
  const NodeSlot *s = graph->resolve(id);
  return s ? s->timeChangeReq : 0;
}

int WatchedHandle::getTimeChanged() const
{
  if (!graph)
    return 0;
  const NodeSlot *s = graph->resolve(id);
  return s ? s->timeChanged : 0;
}

Sqrat::Object WatchedHandle::trace()
{
  if (!graph)
    return Sqrat::Object();
  NodeSlot *s = graph->resolve(id);
  if (!s)
    return Sqrat::Object();
  char buf[2048] = {0};
  graph->varTraces->print(id.index, buf, sizeof(buf));
  if (buf[0])
    return Sqrat::Object((const char *)buf, graph->vm, -1);
  return Sqrat::Object();
}

void WatchedHandle::whiteListMutatorClosure(Sqrat::Object closure)
{
  G_ASSERT(closure.GetType() == OT_CLOSURE);
  if (graph && id.isValid())
  {
    HSQOBJECT obj = closure.GetObject();
    sq_addref(graph->vm, &obj);
    graph->mutatorWhiteLists[id.index].push_back(obj);
  }
}

bool WatchedHandle::checkMutationAllowed(HSQUIRRELVM vm)
{
  bool isAllowed = true;
  if (graph && id.isValid())
  {
    auto it = graph->mutatorWhiteLists.find(id.index);
    if (it != graph->mutatorWhiteLists.end())
    {
      SqStackChecker stackCheck(vm);
      if (SQ_FAILED(sq_getcallee(vm)))
        return false;

      HSQOBJECT callee;
      sq_getstackobj(vm, -1, &callee);
      G_ASSERT(sq_type(callee) == OT_CLOSURE);
      if (sq_type(callee) == OT_CLOSURE)
      {
        isAllowed = false;
        for (HSQOBJECT &obj : it->second)
        {
          if (sq_type(obj) == OT_CLOSURE && obj._unVal.pClosure == callee._unVal.pClosure)
          {
            isAllowed = true;
            break;
          }
        }
        if (!isAllowed)
          (void)sq_throwerror(vm,
            "Trying to modify Observable with restricted access (check whiteListMutatorClosure() calls to fix this)");
      }
      sq_pop(vm, 1);
    }
  }

  if (isAllowed && graph && graph->callingSubscriber)
    logerr_graph_error(vm, "Trying to modify Watched from subscriber, use subscribe_with_nasty_disregard_of_frp_update() for this");

  return isAllowed;
}


SQInteger WatchedHandle::script_ctor(HSQUIRRELVM vm)
{
  SQInteger nParams = sq_gettop(vm);
  if (nParams > 2)
    return sq_throwerror(vm, "Too many arguments");

  Sqrat::Object initialValue(vm);
  if (nParams >= 2)
  {
    HSQOBJECT ho;
    sq_getstackobj(vm, 2, &ho);
    initialValue = Sqrat::Object(ho, vm);
  }

  ObservablesGraph *graph = ObservablesGraph::get_from_vm(vm);
  HSQOBJECT hInitVal = initialValue.GetObject();
  NodeId nodeId = graph->createWatched(hInitVal);

  WatchedHandle *inst = new WatchedHandle(nodeId, graph);
  Sqrat::ClassType<WatchedHandle>::SetManagedInstance(vm, 1, inst);
  return 0;
}


SQInteger WatchedHandle::updateViaMethod(HSQUIRRELVM vm) { return update(vm, 2); }


SQInteger WatchedHandle::update(HSQUIRRELVM vm, int arg_pos)
{
  if (!Sqrat::check_signature<WatchedHandle *>(vm))
    return SQ_ERROR;

  Sqrat::Var<WatchedHandle *> varSelf(vm, 1);
  WatchedHandle *self = varSelf.value;
  if (!self->graph || !self->id.isValid())
    return sq_throwerror(vm, "Invalid observable");
  if (self->graph->vm != vm)
    return sq_throwerror(vm, "Invalid VM");

  if (!self->checkMutationAllowed(vm))
    return SQ_ERROR;

  if (sq_gettype(vm, arg_pos) == OT_CLOSURE)
    return sq_throwerror(vm, "Updating observable via callback is no longer supported, use mutate()");

  NodeSlot *s = self->graph->resolve(self->id);
  if (!s)
    return sq_throwerror(vm, "Stale observable");

  s->timeChangeReq = ::get_time_msec();

  HSQOBJECT hNewVal;
  sq_getstackobj(vm, arg_pos, &hNewVal);
  if (sq_obj_is_equal(vm, &s->value, &hNewVal))
    return 0;

  replace_value(vm, *s, hNewVal, self->graph->forceImmutable);
  s->timeChanged = s->timeChangeReq;
  ++s->version;
  self->graph->markDependentsDirty(self->id);

  self->graph->varTraces->save(self->id.index, s->value, vm);

  self->graph->triggerRoot(self->id);
  return 0;
}


SQInteger WatchedHandle::mutate(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<WatchedHandle *>(vm))
    return SQ_ERROR;

  Sqrat::Var<WatchedHandle *> varSelf(vm, 1);
  WatchedHandle *self = varSelf.value;
  if (!self->graph || !self->id.isValid())
    return sq_throwerror(vm, "Invalid observable");
  if (self->graph->vm != vm)
    return sq_throwerror(vm, "Invalid VM");

  if (!self->checkMutationAllowed(vm))
    return SQ_ERROR;

  NodeSlot *s = self->graph->resolve(self->id);
  if (!s)
    return sq_throwerror(vm, "Stale observable");

  // Validate callback
  SQInteger nParams = 0, nFreeVars = 0;
  G_VERIFY(SQ_SUCCEEDED(sq_getclosureinfo(vm, 2, &nParams, &nFreeVars)));
  if (nParams != 2)
    return sqstd_throwerrorf(vm, "Callback must accept a value to mutate, but arguments count is %d", nParams);

  SQObjectType tp = sq_type(s->value);
  if (tp != OT_TABLE && tp != OT_ARRAY && tp != OT_USERDATA && tp != OT_INSTANCE && tp != OT_CLASS)
    return sqstd_throwerrorf(vm, "Only containers can be mutated by callback, current value is of type '%s'", sq_objtypestr(tp));

  SqStackChecker stackCheck(vm);

  sq_push(vm, 2);  // closure
  sq_pushnull(vm); // 'this' for call
  Sqrat::Object mutableVal(s->value, vm);
  if (self->graph->forceImmutable)
    mutableVal.UnfreezeSelf();
  sq_pushobject(vm, mutableVal);
  SQRESULT result = sq_call(vm, 2, true, true);
  stackCheck.restore();

  if (SQ_FAILED(result))
    return sq_throwerror(vm, "Callback failure");

  // Re-resolve after callback
  s = self->graph->resolve(self->id);
  if (!s)
    return sq_throwerror(vm, "Observable destroyed during mutate");

  // Store mutated value back
  replace_value(vm, *s, mutableVal.GetObject(), self->graph->forceImmutable);
  s->timeChangeReq = ::get_time_msec();
  s->timeChanged = s->timeChangeReq;

  ++s->version;
  self->graph->markDependentsDirty(self->id);
  self->graph->varTraces->save(self->id.index, s->value, vm);

  self->graph->triggerRoot(self->id);
  return 0;
}


SQInteger WatchedHandle::modify(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<WatchedHandle *>(vm))
    return SQ_ERROR;

  Sqrat::Var<WatchedHandle *> varSelf(vm, 1);
  WatchedHandle *self = varSelf.value;
  if (!self->graph || !self->id.isValid())
    return sq_throwerror(vm, "Invalid observable");
  if (self->graph->vm != vm)
    return sq_throwerror(vm, "Invalid VM");

  if (!self->checkMutationAllowed(vm))
    return SQ_ERROR;

  NodeSlot *s = self->graph->resolve(self->id);
  if (!s)
    return sq_throwerror(vm, "Stale observable");

  const SQInteger closurePos = 2;

  SQInteger nParams = 0, nFreeVars = 0;
  G_VERIFY(SQ_SUCCEEDED(sq_getclosureinfo(vm, 2, &nParams, &nFreeVars)));
  if (nParams != 2)
    return sqstd_throwerrorf(vm, "Callback must be a function with 1 argument (previous value), but arguments count is %d", nParams);

  SQInteger prevTop = sq_gettop(vm);

  sq_pushnull(vm); // 'this' for call
  sq_pushobject(vm, s->value);
  SQRESULT result = sq_call(vm, 2, true, true);
  if (SQ_FAILED(result))
    return sq_throwerror(vm, "Callback failure");

  G_UNUSED(prevTop);
  G_ASSERT(sq_gettop(vm) == prevTop + 1);

  return update(vm, closurePos + 1);
}


static SQInteger format_handle_tostring(HSQUIRRELVM vm, WatchedHandle *self, const char *type_name)
{
  if (!self->graph || !self->id.isValid())
  {
    String stale(0, "%s (stale)", type_name);
    sq_pushstring(vm, stale, stale.length());
    return 1;
  }

  NodeSlot *s = self->graph->resolve(self->id);
  if (!s)
  {
    String stale(0, "%s (stale)", type_name);
    sq_pushstring(vm, stale, stale.length());
    return 1;
  }

  sq_pushobject(vm, s->value);

  const char *name = s->initInfo ? s->initInfo->initFuncName.c_str() : "<unknown>";
  String str(0, "%s (0x%p) %s | %d watchers, %d subscribers", type_name, self, name, (int)s->watchers.size(),
    (int)s->scriptSubscribers.size());
  if (s->isComputed)
    str.aprintf(0, ", %d sources", (int)s->sources.size());
  if (SQ_SUCCEEDED(sq_tostring(vm, -1)))
  {
    const char *valStr = nullptr;
    G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, -1, &valStr)));
    str.aprintf(0, ", value = %s", valStr);
    sq_pop(vm, 1);
  }
  sq_pop(vm, 1);
  sq_pushstring(vm, str, str.length());
  return 1;
}


SQInteger WatchedHandle::_tostring(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<WatchedHandle *>(vm))
    return SQ_ERROR;
  return format_handle_tostring(vm, Sqrat::Var<WatchedHandle *>(vm, 1).value, "Observable");
}


// --------------------------------------------------------------------------
// NativeWatched
// --------------------------------------------------------------------------

void NativeWatched::create(ObservablesGraph *g, HSQOBJECT initial_value)
{
  G_ASSERT(!graph && !id.isValid());
  graph = g;
  id = g->createWatched(initial_value);
}

void NativeWatched::destroy()
{
  if (graph && id.isValid())
    graph->destroyNode(id);
  id = {};
  graph = nullptr;
}

bool NativeWatched::setValue(const Sqrat::Object &val)
{
  G_ASSERT_RETURN(graph, false);
  return graph->setValue(id, val);
}

Sqrat::Object NativeWatched::getValue()
{
  G_ASSERT_RETURN(graph, Sqrat::Object());
  return graph->getValue(id);
}


// --------------------------------------------------------------------------
// ComputedHandle
// --------------------------------------------------------------------------

bool ComputedHandle::getUsed() const
{
  if (!graph)
    return false;
  const NodeSlot *s = graph->resolve(id);
  return s ? (bool)s->computedHasActiveConsumers : false;
}


SQInteger ComputedHandle::script_ctor(HSQUIRRELVM vm)
{
  if (sq_gettop(vm) < 2 || sq_gettype(vm, 2) != OT_CLOSURE)
    return sq_throwerror(vm, "Computed object requires closure as argument");

  SQInteger nparams = 0, nfreevars = 0;
  G_VERIFY(SQ_SUCCEEDED(sq_getclosureinfo(vm, 2, &nparams, &nfreevars)));
  if (nparams != 1 && nparams != 2)
    return sqstd_throwerrorf(vm, "Computed function must accept 1 or 2 arguments, but has %d", nparams);

  ObservablesGraph *graph = ObservablesGraph::get_from_vm(vm);
  if (!graph)
    return sq_throwerror(vm, "Internal error, no graph");

  HSQOBJECT hFunc;
  sq_getstackobj(vm, 2, &hFunc);
  Sqrat::Function func(vm, Sqrat::Object(vm), hFunc);

  dag::Vector<SourceEntry> collectedSources;
  VisitedSet visited;
  bool haveSources = collect_sources_from_free_vars(graph, vm, hFunc, collectedSources, visited);

  if (!haveSources)
    return sq_throwerror(vm, "Computed must have at least one source observable");
  collectedSources.shrink_to_fit();

  NodeId nodeId = graph->createComputed(hFunc, eastl::move(collectedSources), nparams == 2);

  ComputedHandle *inst = new ComputedHandle(nodeId, graph);
  Sqrat::ClassType<ComputedHandle>::SetManagedInstance(vm, 1, inst);

  // Calculate initial value
  Sqrat::optional<Sqrat::Object> optInitialValue;
  if (nparams == 1)
    optInitialValue = func.Eval<Sqrat::Object>();
  else
  {
    HSQOBJECT hInitialNone;
    sq_resetobject(&hInitialNone);
    hInitialNone._type = OT_USERPOINTER;
    hInitialNone._unVal.pUserPointer = computed_initial_update;
    Sqrat::Object initialNone(hInitialNone, vm);
    optInitialValue = func.Eval<Sqrat::Object>(initialNone);
  }
  if (!optInitialValue)
    return sq_throwerror(vm, "Failed to calculate initial computed value - error in function");
  Sqrat::Object initialValue = SQRAT_STD::move(optInitialValue.value());

  // Setup computed: store initial value and wire up dependencies
  {
    NodeSlot *ns = graph->resolve(nodeId);
    if (!ns)
      return sq_throwerror(vm, "Node lost during setup");

    sq_release(vm, &ns->value);
    ns->value = initialValue.GetObject();
    sq_addref(vm, &ns->value);
    ns->value._flags |= SQOBJ_FLAG_IMMUTABLE;

    // Snapshot source versions and wire up dependent edges
    for (auto &src : ns->sources)
    {
      NodeSlot *srcSlot = graph->resolve(src.id);
      if (srcSlot)
        src.lastSeenVersion = srcSlot->version;
      graph->addDependent(src.id, nodeId);
    }

    // Propagate needImmediate/used through sources
    if (ns->needImmediate)
    {
      for (auto &src : ns->sources)
        graph->markImmediate(src.id);
    }
  }

  return 0;
}


SQInteger ComputedHandle::get_sources(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<ComputedHandle *>(vm))
    return SQ_ERROR;
  Sqrat::Var<ComputedHandle *> self(vm, 1);
  ComputedHandle *h = self.value;
  if (!h || !h->graph)
    return sq_throwerror(vm, "Invalid computed");

  NodeSlot *s = h->graph->resolve(h->id);
  if (!s)
    return sq_throwerror(vm, "Stale computed");

  Sqrat::Table res(vm);
  for (SourceEntry &src : s->sources)
  {
    // Return the source observable value (not the handle) - matching old behavior
    NodeSlot *srcSlot = h->graph->resolve(src.id);
    if (srcSlot)
      res.SetValue(src.varName.c_str(), Sqrat::Object(srcSlot->value, vm));
  }
  sq_pushobject(vm, res);
  return 1;
}


SQInteger ComputedHandle::_tostring(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<ComputedHandle *>(vm))
    return SQ_ERROR;
  return format_handle_tostring(vm, Sqrat::Var<ComputedHandle *>(vm, 1).value, "ComputedValue");
}


// --------------------------------------------------------------------------
// Script module bindings
// --------------------------------------------------------------------------

static SQInteger forbid_computed_modify(HSQUIRRELVM vm) { return sq_throwerror(vm, "Can't modify computed observable"); }


static SQInteger set_subscriber_validation(HSQUIRRELVM vm)
{
  ObservablesGraph *graph = ObservablesGraph::get_from_vm(vm);
  SQBool val;
  sq_getbool(vm, 2, &val);
  graph->checkSubscribers = val;
  return 0;
}

static SQInteger make_all_observables_immutable(HSQUIRRELVM vm)
{
  ObservablesGraph *graph = ObservablesGraph::get_from_vm(vm);
  SQBool val;
  sq_getbool(vm, 2, &val);
  graph->forceImmutable = val;
  return 0;
}

static SQInteger recalc_all_computed_values(HSQUIRRELVM vm)
{
  ObservablesGraph *graph = ObservablesGraph::get_from_vm(vm);
  graph->resetPerfTimers();
  graph->recalcAllComputedValues();
  return 0;
}

static SQInteger set_recursive_sources(HSQUIRRELVM vm)
{
  ObservablesGraph *graph = ObservablesGraph::get_from_vm(vm);
  SQBool val;
  sq_getbool(vm, 2, &val);
  graph->doCollectSourcesRecursively = val;
  return 0;
}

static SQInteger set_slow_update_threshold_usec(HSQUIRRELVM vm)
{
  ObservablesGraph *graph = ObservablesGraph::get_from_vm(vm);
  SQInteger val = 1500;
  sq_getinteger(vm, 2, &val);
  graph->slowUpdateThresholdUsec = val;
  return 0;
}

static SQInteger get_slow_update_threshold_usec(HSQUIRRELVM vm)
{
  ObservablesGraph *graph = ObservablesGraph::get_from_vm(vm);
  sq_pushinteger(vm, graph->slowUpdateThresholdUsec);
  return 1;
}

static SQInteger this_subscriber_call_may_take_up_to_usec(HSQUIRRELVM vm)
{
  ObservablesGraph *graph = ObservablesGraph::get_from_vm(vm);
  SQInteger val = 1500;
  sq_getinteger(vm, 2, &val);
  if (graph->curSubscriberThresholdUsec == -1)
    logerr("[FRP] `this_subscriber_call_may_take_up_to_usec()` called outside of any subscriber");
  graph->curSubscriberThresholdUsec = val;
  return 0;
}

static SQInteger set_slow_subscriber_threshold_usec(HSQUIRRELVM vm)
{
  ObservablesGraph *graph = ObservablesGraph::get_from_vm(vm);
  SQInteger val = 1500;
  sq_getinteger(vm, 2, &val);
  graph->slowSubscriberThresholdUsec = val;
  return 0;
}

static SQInteger get_slow_subscriber_threshold_usec(HSQUIRRELVM vm)
{
  ObservablesGraph *graph = ObservablesGraph::get_from_vm(vm);
  sq_pushinteger(vm, graph->slowSubscriberThresholdUsec);
  return 1;
}

static SQInteger update_deferred(HSQUIRRELVM vm)
{
  ObservablesGraph *graph = ObservablesGraph::get_from_vm(vm);
  graph->updateDeferred();
  return 0;
}

static SQInteger gather_graph_stats(HSQUIRRELVM vm)
{
  ObservablesGraph *graph = ObservablesGraph::get_from_vm(vm);
  auto stats = graph->gatherGraphStats();
  Sqrat::Table res(vm);
  res.SetValue("watchedTotal", stats.watchedTotal);
  res.SetValue("watchedUnused", stats.watchedUnused);
  res.SetValue("watchedUnsubscribed", stats.watchedUnsubscribed);
  res.SetValue("computedTotal", stats.computedTotal);
  res.SetValue("computedUnused", stats.computedUnused);
  res.SetValue("computedUnsubscribed", stats.computedUnsubscribed);
  res.SetValue("totalSubscribers", stats.totalSubscribers);
  res.SetValue("totalWatchers", stats.totalWatchers);
  res.SetValue("slotsAllocated", (int)graph->slotsAllocated());
  res.SetValue("slotsFree", (int)graph->slotsFree());
  res.SetValue("computedRecalcs", graph->computedRecalcs);
  res.SetValue("deferredQueueSize", (int)graph->deferredNotifyQueue.size());
  res.SetValue("usecUsedComputed", graph->usecUsedComputed);
  res.SetValue("usecUnusedComputed", graph->usecUnusedComputed);
  res.SetValue("usecSubscribers", graph->usecSubscribers);
  res.SetValue("usecTrigger", graph->usecTrigger);
  res.SetValue("usecUpdateDeferred", graph->usecUpdateDeferred);
  sq_pushobject(vm, res);
  return 1;
}


void bind_frp_classes(SqModules *module_mgr)
{
  HSQUIRRELVM vm = module_mgr->getVM();

  ///@class frp/Watched
  // Keep internal name "ScriptValueObservable" for ClassType compatibility
  Sqrat::Class<WatchedHandle, Sqrat::NoCopy<WatchedHandle>> watchedClass(vm, "ScriptValueObservable");
  watchedClass //
    .SquirrelCtor(WatchedHandle::script_ctor, -1)
    .SquirrelFuncDeclString(
      +[](HSQUIRRELVM vm) { return ObservablesGraph::subscribe(vm, true); }, "instance.subscribe(handler: function): instance")
    .SquirrelFuncDeclString(
      +[](HSQUIRRELVM vm) { return ObservablesGraph::subscribe(vm, false); },
      "instance.subscribe_with_nasty_disregard_of_frp_update(handler: function): instance")
    .SquirrelFuncDeclString(&ObservablesGraph::unsubscribe, "instance.unsubscribe(handler: function): any")
    .SquirrelFuncDeclString(&ObservablesGraph::dbgGetListeners, "instance.dbgGetListeners(): table")
    .Prop("deferred", &WatchedHandle::getDeferred, &WatchedHandle::setDeferred)
    .Func("setDeferred", &WatchedHandle::setDeferred)
    .Prop("value", &WatchedHandle::getValueDeprecated)
    .Func("get", &WatchedHandle::getValue)
    .Prop("timeChangeReq", &WatchedHandle::getTimeChangeReq)
    .Prop("timeChanged", &WatchedHandle::getTimeChanged)
    .SquirrelFuncDeclString(&ObservablesGraph::sqTrigger, "instance.trigger(): instance")
    .Func("trace", &WatchedHandle::trace)
    .Func("whiteListMutatorClosure", &WatchedHandle::whiteListMutatorClosure)
    .SquirrelFuncDeclString(&WatchedHandle::updateViaMethod, "instance.set(value: any): null")
    .SquirrelFuncDeclString(&WatchedHandle::modify, "instance.modify(fn: function): null")
    .SquirrelFuncDeclString(&WatchedHandle::mutate, "instance.mutate(fn: function): null")
    /**/;

  ///@class frp/Computed
  ///@extends Watched
  Sqrat::DerivedClass<ComputedHandle, WatchedHandle, Sqrat::NoCopy<ComputedHandle>> computedClass(vm, "ComputedValue");
  computedClass //
    .SquirrelCtor(ComputedHandle::script_ctor, -2)
    .SquirrelFuncDeclString(forbid_computed_modify, "instance.mutate(...): null")
    .SquirrelFuncDeclString(forbid_computed_modify, "instance.set(...): null")
    .SquirrelFuncDeclString(forbid_computed_modify, "instance.modify(...): null")
    .SquirrelFuncDeclString(
      +[](HSQUIRRELVM vm) { return sq_throwerror(vm, "Triggering computed observable is not allowed"); },
      "instance.trigger(...): instance")
    .Prop("used", &ComputedHandle::getUsed)
    .SquirrelFuncDeclString(ComputedHandle::get_sources, "instance.getSources(): table")
    /**/;

  Sqrat::DerivedClass<NativeWatched, WatchedHandle, Sqrat::NoConstructor<NativeWatched>> nativeWatchedClass(vm, "NativeWatched");

  Sqrat::Table exports(vm);
  exports.Bind("Watched", watchedClass);
  exports.Bind("Computed", computedClass);
  ///@module frp
  exports //
    .SquirrelFuncDeclString(set_subscriber_validation, "set_subscriber_validation(val: bool): null")
    .SquirrelFuncDeclString(make_all_observables_immutable, "make_all_observables_immutable(val: bool): null")
    .SquirrelFuncDeclString(set_recursive_sources, "set_recursive_sources(val: bool): null")
    .SquirrelFuncDeclString(set_slow_update_threshold_usec, "set_slow_update_threshold_usec(val: int): null")
    .SquirrelFuncDeclString(get_slow_update_threshold_usec, "get_slow_update_threshold_usec(): int")
    .SquirrelFuncDeclString(set_slow_subscriber_threshold_usec, "set_slow_subscriber_threshold_usec(val: int): null")
    .SquirrelFuncDeclString(get_slow_subscriber_threshold_usec, "get_slow_subscriber_threshold_usec(): int")
    .SquirrelFuncDeclString(this_subscriber_call_may_take_up_to_usec, "this_subscriber_call_may_take_up_to_usec(val: int): null")
    .SquirrelFuncDeclString(recalc_all_computed_values, "recalc_all_computed_values(): null")
    .SquirrelFuncDeclString(gather_graph_stats, "gather_graph_stats(): table")
    .SquirrelFuncDeclString(update_deferred, "update_deferred(): null")
    .SquirrelFuncDeclString(ObservablesGraph::register_stub_observable_class, "register_stub_observable_class(cls: class): null")
    .Func("warn_on_deprecated_methods", [](bool on) { use_deprecated_methods_warning = on; })
    /**/;

  sq_pushobject(vm, exports);
  {
    ///@const FRP_INITIAL
    sq_pushstring(vm, "FRP_INITIAL", -1);
    sq_pushuserpointer(vm, computed_initial_update);
    G_VERIFY(SQ_SUCCEEDED(sq_rawset(vm, -3)));
  }
  sq_poptop(vm);

  module_mgr->addNativeModule("frp", exports);
}


} // namespace sqfrp
