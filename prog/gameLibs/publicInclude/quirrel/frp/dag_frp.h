//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

/*****

FRP (Functional Reactive Programming) for Quirrel and daRg

Graph-owned node architecture:
  All node data lives in graph-owned NodeSlot pool with generational indices.
  Script observables (WatchedHandle/ComputedHandle) are lightweight {NodeId, graph*}.
  Handle destruction calls destroyNode() which frees the node immediately.
  Stale generational IDs are safely handled by resolve() returning nullptr.

Uses a version-based push-pull propagation model:
  Write path (.set): store value, increment version, forward-mark dependents as CHECK.
  Read path (.get):  if not CLEAN, pull -- verify source versions, recompute only if changed.
  Immediate mode:    write + eagerly pull all downstream nodes (synchronous).
  Deferred mode:     write + mark dirty; pull happens at updateDeferred() time.

******/


#include "dag_frpStateWatcher.h"
#include <util/dag_compilerDefs.h>
#include <generic/dag_tab.h>
#include <generic/dag_fixedVectorSet.h>
#include <generic/dag_carray.h>
#include <EASTL/vector_map.h>
#include <EASTL/hash_set.h>
#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>
#include <dag/dag_vector.h>
#include <util/dag_string.h>
#include <util/dag_simpleString.h>
#include <memory/dag_framemem.h>
#include <sqrat.h>
#include <osApiWrappers/dag_spinlock.h>


class SqModules;

namespace sqfrp
{
class WatchedHandle;
class ComputedHandle;
class ObservablesGraph;
struct NodeVarTraces;
void graph_viewer();

using NodeIdVec = dag::Vector<NodeId, framemem_allocator>;

struct NodeIdHash
{
  size_t operator()(const NodeId &id) const { return size_t(id.index) * 2654435761u; }
};

// Push-pull node state for version-based propagation
enum class NodeState : uint8_t
{
  CLEAN, // Value is up-to-date
  CHECK, // Some upstream changed; need to verify if sources actually differ
  DIRTY, // Known to need recomputation
};


struct ScriptSourceInfo
{
  SimpleString initFuncName, initSourceFileName;
  int initSourceLine = -1;

  void init(HSQUIRRELVM vm);
  void fillInfo(Sqrat::Table &) const;
  void toString(String &) const;
};


struct SubscriberCall
{
  Sqrat::Function func;
  Sqrat::Object value;
  NodeId source;
  bool check = false;
};


struct SourceEntry
{
  NodeId id;
  uint32_t lastSeenVersion = 0;
  eastl::string varName; // debug: closure variable name
};


struct NodeSlot
{
  // Value (all nodes)
  HSQOBJECT value;

  // Computed-only
  HSQOBJECT func;
  dag::Vector<SourceEntry> sources;

  // Dependency edges
  dag::Vector<NodeId> dependents;        // downstream computed nodes
  dag::Vector<IStateWatcher *> watchers; // external: daRg Elements
  dag::Vector<Sqrat::Function> scriptSubscribers;
  uint8_t numNoCheckSubscribers = 0; // no-check subscribers occupy indices [0, numNoCheckSubscribers)

  // State
  uint32_t version = 0;
  uint32_t generation = 0; // generational index for stale-ID detection
  NodeState nodeState = NodeState::CLEAN;

  // Flags (packed bitfield)
  union
  {
    struct
    {
      bool alive : 1;
      bool isComputed : 1;
      bool isDeferred : 1;
      bool needImmediate : 1;
      bool computedHasActiveConsumers : 1;
      bool isInTrigger : 1;
      bool isIteratingWatchers : 1;
      bool isPulling : 1;
      bool isMarked : 1;
      bool funcAcceptsCurVal : 1;
    };
    uint32_t flags = 0;
  };

  // Diagnostics
  eastl::unique_ptr<ScriptSourceInfo> initInfo;
  NodeId lastChangeCause; // may be stale (freed node); consumers use resolve() and handle null
  uint8_t numDirtySources = 0;
  uint8_t deferredTriggerStreak = 0;
  uint16_t lastDeferredTriggerGen = 0;
  int timeChangeReq = 0;
  int timeChanged = 0;

  NodeSlot()
  {
    sq_resetobject(&value);
    sq_resetobject(&func);
  }
};


class ObservablesGraph
{
public:
  ObservablesGraph(HSQUIRRELVM vm_, const char *graph_name);
  ~ObservablesGraph();

  void setName(const char *name);

  // Node creation
  NodeId createWatched(HSQOBJECT initial_value);
  NodeId createComputed(HSQOBJECT func_obj, dag::Vector<SourceEntry> &&sources, bool pass_cur_val);

  // Node access (inline for cross-TU inlining -- these are tiny and hot)
  inline NodeSlot &node(NodeId id)
  {
    G_ASSERT(id.index < slots.size());
    NodeSlot &s = slots[id.index];
    G_ASSERT(s.alive && s.generation == id.generation);
    return s;
  }
  inline const NodeSlot &node(NodeId id) const
  {
    G_ASSERT(id.index < slots.size());
    const NodeSlot &s = slots[id.index];
    G_ASSERT(s.alive && s.generation == id.generation);
    return s;
  }
  inline NodeSlot *resolve(NodeId id)
  {
    if (id.index >= slots.size())
      return nullptr;
    NodeSlot &s = slots[id.index];
    if (!s.alive || s.generation != id.generation)
      return nullptr;
    return &s;
  }
  inline const NodeSlot *resolve(NodeId id) const
  {
    if (id.index >= slots.size())
      return nullptr;
    const NodeSlot &s = slots[id.index];
    if (!s.alive || s.generation != id.generation)
      return nullptr;
    return &s;
  }

  // Value access
  Sqrat::Object getValue(NodeId id);
  bool setValue(NodeId id, const Sqrat::Object &new_val);

  // Lifecycle
  void destroyNode(NodeId id);
  void shutdown(bool exiting);

  // Subscriptions
  void addDependent(NodeId source, NodeId dependent);
  void removeDependent(NodeId source, NodeId dependent);
  void subscribeWatcher(NodeId id, IStateWatcher *w);
  void unsubscribeWatcher(NodeId id, IStateWatcher *w);

  // Graph processing
  bool trigger(NodeId id);
  bool updateDeferred();
  void markDependentsDirty(NodeId id);
  void recalcAllComputedValues();
  bool callScriptSubscribers(NodeId triggered_node, NodeIdVec &notify_queue);

  // Script bindings (static methods operating on WatchedHandle)
  static SQInteger subscribe(HSQUIRRELVM vm, bool check_behavior);
  static SQInteger unsubscribe(HSQUIRRELVM vm);
  static SQInteger sqTrigger(HSQUIRRELVM vm);
  static SQInteger dbgGetListeners(HSQUIRRELVM vm);

  static ObservablesGraph *get_from_vm(HSQUIRRELVM vm);

  void onEnterTriggerRoot();
  void onExitTriggerRoot();
  void onEnterRecalc();
  void onExitRecalc();

  static SQInteger register_stub_observable_class(HSQUIRRELVM vm);
  bool isStubObservableInstance(const Sqrat::Object &obj) const;

  void notifyGraphChanged();

  // Info and diagnostics
  void fillInfo(NodeId id, Sqrat::Table &t) const;
  void fillInfo(NodeId id, String &s) const;
  void collectScriptSubscribers(NodeId id, Tab<SubscriberCall> &container) const;

  struct Stats
  {
    int watchedTotal{0};
    int watchedUnused{0};
    int watchedUnsubscribed{0};
    int computedTotal{0};
    int computedUnused{0};
    int computedUnsubscribed{0};
    int totalSubscribers{0};
    int totalWatchers{0};
  };

  Stats gatherGraphStats() const;

  // Node iteration (for imgui viewer etc)
  template <typename F>
  void forEachNode(F cb) const
  {
    for (uint32_t i = 0; i < slots.size(); ++i)
      if (slots[i].alive)
        cb(NodeId{i, slots[i].generation}, slots[i]);
  }

  uint32_t nodeCount() const { return (uint32_t)slots.size() - (uint32_t)freeList.size(); }
  uint32_t slotsAllocated() const { return (uint32_t)slots.size(); }
  uint32_t slotsFree() const { return (uint32_t)freeList.size(); }

private:
  friend class WatchedHandle;
  friend class ComputedHandle;
  void dumpSubscribersQueue(Tab<SubscriberCall> &queue);

  bool triggerRoot(NodeId id);
  bool notifyWatchers(NodeId id);
  NodeId allocSlot();
  void freeSlot(NodeId id);
  void flushPendingRelease();
  void releaseMutatorWhiteList(uint32_t slot_index);
  bool pull(NodeId id, NodeIdVec &changed_nodes);
  bool recalculate(NodeId id, bool &ok);
  void ensureUpToDate(NodeId id);
  void markImmediate(NodeId id);
  void updateNeedImmediate(NodeId id);
  void markComputedConsumed(NodeId id);
  void updateComputedConsumed(NodeId id);
  void onNodeGraphShutdown(NodeId id, bool exiting, Tab<Sqrat::Object> &cleared_storage);

  dag::Vector<NodeSlot> slots;
  dag::Vector<uint32_t> freeList;
  dag::Vector<HSQOBJECT> pendingRelease;
  eastl::vector_map<uint32_t, dag::Vector<HSQOBJECT>> mutatorWhiteLists;
  eastl::unique_ptr<NodeVarTraces> varTraces;

public:
  HSQUIRRELVM vm = nullptr;
  SimpleString graphName;

  OSSpinlock slotsLock; //< for external access to slots

  int usecUsedComputed = 0;
  int usecUnusedComputed = 0;
  int usecSubscribers = 0;
  int usecTrigger = 0;
  int usecUpdateDeferred = 0;
  int computedRecalcs = 0;

  struct SubscriberTiming
  {
    String name;
    int timeUsec = 0;
  };
#if DAGOR_DBGLEVEL > 0
  carray<SubscriberTiming, 8> slowestSubscribersAllTime;
  carray<SubscriberTiming, 8> slowestSubscribersLastEpisode;
#endif

  void resetPerfTimers()
  {
    usecUsedComputed = 0;
    usecUnusedComputed = 0;
    usecSubscribers = 0;
    usecTrigger = 0;
    usecUpdateDeferred = 0;
  }

  NodeId inRecalc;

  static constexpr int SLOW_MUL =
#if DAGOR_THREAD_SANITIZER
    100
#elif DAGOR_ADDRESS_SANITIZER
    10
#else
    1
#endif
    ;
  int slowUpdateThresholdUsec = 1500 * SLOW_MUL;
  int slowSubscriberThresholdUsec = 10000 * SLOW_MUL;
  int curSubscriberThresholdUsec = -1;
  bool forceImmutable = false;
  bool doCollectSourcesRecursively = false;
  bool reportedSlowUpdate = false;
  bool callingSubscriber = false;
  bool checkSubscribers = false;

  // Deferred notify queue: nodes whose subscribers fire at next updateDeferred().
  eastl::hash_set<NodeId, NodeIdHash> deferredNotifyQueue;

private:
  Tab<SubscriberCall> subscriberCallsQueue, curSubscriberCalls;
  NodeId currentlyDispatchingSource;
  uint16_t deferredUpdateGen = 0;
  int maxSubscribersQueueLen = 0;
  int curTriggerSubscribersCounter = 0;
  int triggerNestDepth = 0;
  int recalcNestDepth = 0;
  int slowUpdateFrames = 0;
#if DAGOR_DBGLEVEL > 0
  int64_t timeTotalRecalc = 0;
  int64_t timeTotalNotify = 0;
  int timeMaxRecalc = 0;
  int timeMaxNotify = 0;
#endif

  eastl::vector<Sqrat::Object> stubObservableClases;
};


// Script binding layer: lightweight handle to a graph-owned node
class WatchedHandle
{
public:
  NodeId id;
  ObservablesGraph *graph = nullptr;

  WatchedHandle() = default;
  WatchedHandle(NodeId id_, ObservablesGraph *g) : id(id_), graph(g) {}
  WatchedHandle(WatchedHandle &&o) noexcept : id(o.id), graph(o.graph) { o.graph = nullptr; }
  WatchedHandle &operator=(WatchedHandle &&o)
  {
    if (this != &o)
    {
      if (graph && id.isValid())
        graph->destroyNode(id);
      id = o.id;
      graph = o.graph;
      o.graph = nullptr;
    }
    return *this;
  }
  WatchedHandle(const WatchedHandle &) = delete;
  WatchedHandle &operator=(const WatchedHandle &) = delete;
  virtual ~WatchedHandle();

  Sqrat::Object getValue();
  Sqrat::Object getValueDeprecated();
  bool getDeferred() const;
  void setDeferred(bool v);
  int getTimeChangeReq() const;
  int getTimeChanged() const;
  Sqrat::Object trace();

  void whiteListMutatorClosure(Sqrat::Object closure);
  bool checkMutationAllowed(HSQUIRRELVM vm);

  static SQInteger script_ctor(HSQUIRRELVM vm);
  static SQInteger updateViaMethod(HSQUIRRELVM vm);
  static SQInteger update(HSQUIRRELVM vm, int arg_pos);
  static SQInteger mutate(HSQUIRRELVM vm);
  static SQInteger modify(HSQUIRRELVM vm);
  static SQInteger _tostring(HSQUIRRELVM vm);
};


class ComputedHandle : public WatchedHandle
{
public:
  using WatchedHandle::WatchedHandle;

  bool getUsed() const;

  static SQInteger script_ctor(HSQUIRRELVM vm);
  static SQInteger get_sources(HSQUIRRELVM vm);
  static SQInteger _tostring(HSQUIRRELVM vm);
};


// C++-owned observable node. Unlike WatchedHandle, does NOT detach on destruction.
// Use create()/destroy() for explicit lifecycle management.
// Derives from WatchedHandle so script sees it as instanceof Watched.
class NativeWatched : public WatchedHandle
{
public:
  NativeWatched() = default;
  ~NativeWatched() override { graph = nullptr; } // prevent base dtor destroyNode

  NativeWatched(const NativeWatched &) = delete;
  NativeWatched &operator=(const NativeWatched &) = delete;

  void create(ObservablesGraph *g, HSQOBJECT initial_value);
  void destroy();

  bool setValue(const Sqrat::Object &val);
  Sqrat::Object getValue();
};


void bind_frp_classes(SqModules *module_mgr);

dag::Vector<ObservablesGraph *> get_all_graphs();

extern int pull_frp_imgui; // reference this to link imgui graph viewer module

} // namespace sqfrp
