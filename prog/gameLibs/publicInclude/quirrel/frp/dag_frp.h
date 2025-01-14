//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "dag_frpStateWatcher.h"
#include <util/dag_compilerDefs.h>
#include <generic/dag_tab.h>
#include <generic/dag_fixedVectorSet.h>
#include <EASTL/hash_set.h>
#include <EASTL/vector.h>
#include <EASTL/vector_set.h>
#include <EASTL/string.h>
#include <EASTL/intrusive_list.h>
#include <util/dag_string.h>
#include <util/dag_simpleString.h>
#include <memory/dag_framemem.h>
#include <sqrat.h>
#include <squirrel/vartrace.h>
#include <memory/dag_fixedBlockAllocator.h>
#include <osApiWrappers/dag_spinlock.h>


class SqModules;

namespace sqfrp
{
class ScriptValueObservable;
void graph_viewer();


class NotifyQueue
{
public:
  void add(BaseObservable *item);
  void remove(BaseObservable *item);
  // typedef eastl::vector<BaseObservable*> Container; // do we need determenistic order?
  typedef eastl::vector_set<BaseObservable *> Container; // do we need determenistic order?
  Container container;
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
};


class ObservablesGraph
{
  friend class BaseObservable;
  void addObservable(BaseObservable *obs);
  void removeObservable(BaseObservable *obs);

  friend class ComputedValue;
  template <typename... Args>
  ComputedValue *allocComputed(Args &&...args);

public:
  ObservablesGraph(HSQUIRRELVM vm_, const char *graph_name, int compute_pool_initial_size = 1024);
  ~ObservablesGraph();

  void setName(const char *name);

  // Note: wrapper for bin compat across different SQ_VAR_TRACE_ENABLED libs
  ScriptValueObservable *allocScriptValueObservable();

  bool updateDeferred(String &err_msg);
  void shutdown(bool exiting);
  void checkLeaks();
  void recalcAllComputedValues();
  bool callScriptSubscribers(BaseObservable *triggered_node, String &err_msg);

  static ObservablesGraph *get_from_vm(HSQUIRRELVM vm);

  void onEnterTriggerRoot();
  void onExitTriggerRoot();

  void onEnterRecalc();
  void onExitRecalc();
  int getRecalcDepth() const { return recalcNestDepth; }

  static SQInteger register_stub_observable_class(HSQUIRRELVM vm);
  bool isStubObservableInstance(const Sqrat::Object &obj) const;

  void notifyGraphChanged();

  struct Stats
  {
    int watchedTotal{0};
    int watchedUnused{0};
    int watchedUnsubscribed{0};
    int computedTotal{0};
    int computedUnused{0};
    int computedUnsubscribed{0};
  };

  Stats gatherGraphStats() const;

private:
  bool checkGcUnreachable();
  void dumpSubscribersQueue(Tab<SubscriberCall> &queue);

  eastl::intrusive_list<ComputedValue> allComputed;
  FixedBlockAllocator computedPool; // Note: pool all ComputedValue are allocated from

  template <typename F>
  void forAllComputed(F cb);

public:
  HSQUIRRELVM vm = nullptr;
  SimpleString graphName;

  eastl::hash_set<BaseObservable *> allObservables;
  OSSpinlock allObservablesLock;        //< this is for external access, the graph only locks it when adding/removing observables
  bool observablesListModified = false; //< to protect against iterator invalidation

  NotifyQueue notifyQueueCache;
  NotifyQueue deferredNotifyCache;

  int usecUsedComputed = 0;
  int usecUnusedComputed = 0;
  int usecSubscribers = 0;
  int usecTrigger = 0;
  int usecUpdateDeferred = 0;
  int computedRecalcs = 0;

  void resetPerfTimers()
  {
    usecUsedComputed = 0;
    usecUnusedComputed = 0;
    usecSubscribers = 0;
    usecTrigger = 0;
    usecUpdateDeferred = 0;
  }

  ComputedValue *inRecalc = nullptr;

#if DAGOR_THREAD_SANITIZER
  int slowUpdateThresholdUsec = 1500 * 100;
#else
  int slowUpdateThresholdUsec = 1500;
#endif
  int generation = 0;
  bool checkNestedObservable = false;
  bool forceImmutable = false;
  bool defaultDeferredComputed = true;
  bool defaultDeferredWatched = true;
  bool doCollectSourcesRecursively = false;
  bool needRecalc = false;

private:
  Tab<ComputedValue *> sortedGraph;

  Tab<SubscriberCall> subscriberCallsQueue, curSubscriberCalls;
  int maxSubscribersQueueLen = 0;
  int curTriggerSubscribersCounter = 0;
  int triggerNestDepth = 0;
  int recalcNestDepth = 0;
  int slowUpdateFrames = 0;

  eastl::vector<Sqrat::Object> stubObservableClases;
};


class BaseObservable
{
  friend class ObservablesGraph;

public:
  BaseObservable(ObservablesGraph *graph, bool is_computed = false);
  BaseObservable(BaseObservable &&) = default;
  BaseObservable(const BaseObservable &) = default;
  BaseObservable &operator=(BaseObservable &&) = default;
  BaseObservable &operator=(const BaseObservable &) = default;
  virtual ~BaseObservable();

  ComputedValue *getComputed() { return isComputed ? (ComputedValue *)this : nullptr; }
  const ComputedValue *getComputed() const { return isComputed ? (const ComputedValue *)this : nullptr; }

  bool triggerRoot(String &err_msg, bool call_subscribers = true);

  void subscribeWatcher(IStateWatcher *watcher);
  void unsubscribeWatcher(IStateWatcher *watcher);

  static SQInteger subscribe(HSQUIRRELVM vm);
  static SQInteger unsubscribe(HSQUIRRELVM vm);
  static SQInteger sqTrigger(HSQUIRRELVM vm);
  static SQInteger dbgGetListeners(HSQUIRRELVM vm);

  virtual Sqrat::Object getValueForNotify() const = 0;
  virtual void onGraphShutdown(bool exiting, Tab<Sqrat::Object> &cleared_storage);
  virtual void fillInfo(Sqrat::Table &) const = 0;
  virtual void fillInfo(String &s) const { clear_and_shrink(s); }
  virtual const ScriptSourceInfo *getInitInfo() const { return nullptr; }
  virtual void setName(const char *name);
  virtual Sqrat::Object trace() { return Sqrat::Object(); }
  virtual HSQOBJECT getCurScriptInstance();

  inline bool getNeedImmediate() const { return needImmediate; }
  void updateNeedImmediate(bool certainly_immediate);
  inline bool getDeferred() const { return isDeferred; }
  inline void setDeferred(bool v)
  {
    isDeferred = v;
    updateNeedImmediate(!v);
  }

  void collectScriptSubscribers(Tab<SubscriberCall> &container) const;

protected:
  friend void graph_viewer();

  // all flags
  union
  {
    struct
    {
      // BaseObservable
      bool isIteratingWatchers : 1;
      bool isInTrigger : 1;
      bool willNotify : 1;
      bool isDeferred : 1;
      bool needImmediate : 1;

      // ScriptValueObservable
      bool checkNestedObservable : 1;

      // ComputedValue
      bool isComputed : 1;
      bool isMarked : 1;
      bool isCollected : 1;
      bool wasUpdated : 1;
      bool isStateValid : 1;
      bool needRecalc : 1;
      bool maybeRecalc : 1;
      bool isUsed : 1;
      bool isShuttingDown : 1;
      bool funcAcceptsCurVal : 1;
    };
    uint32_t flags = 0;
  };

public:
  int generation = 0;

protected:
  dag::Vector<Sqrat::Function> scriptSubscribers;
  ObservablesGraph *graph;

public:
  dag::Vector<IStateWatcher *> watchers;
};


class ScriptValueObservable : public BaseObservable
{
  friend class ObservablesGraph;
  ScriptValueObservable(ObservablesGraph *graph);

public:
  ScriptValueObservable(ObservablesGraph *graph, Sqrat::Object val, bool is_computed = false);
  ScriptValueObservable(const ScriptValueObservable &) = delete;
  ScriptValueObservable &operator=(const ScriptValueObservable &) = delete;
  ~ScriptValueObservable();

  static SQInteger script_ctor(HSQUIRRELVM vm);

  Sqrat::Object getValue()
  {
    onValueRequested();
    return value;
  }
  int getTimeChangeReq() { return timeChangeReq; }
  int getTimeChanged() { return timeChanged; }
  const Sqrat::Object &getValueRef()
  {
    onValueRequested();
    return value;
  }
  virtual Sqrat::Object trace() override;
  bool setValue(const Sqrat::Object &new_val, String &err_msg);

  virtual Sqrat::Object getValueForNotify() const { return value; }

  virtual void onGraphShutdown(bool exiting, Tab<Sqrat::Object> &cleared_storage) override;
  virtual void fillInfo(Sqrat::Table &) const override;
  virtual void fillInfo(String &) const override;
  virtual HSQOBJECT getCurScriptInstance() override;

  static SQInteger update(HSQUIRRELVM vm, int arg_pos);
  static SQInteger updateViaCallMm(HSQUIRRELVM vm);
  static SQInteger updateViaMethod(HSQUIRRELVM vm);
  static SQInteger mutate(HSQUIRRELVM vm);
  static SQInteger modify(HSQUIRRELVM vm);
  bool mutateCurValue(HSQUIRRELVM vm, SQInteger closure_pos);

  static SQInteger _newslot(HSQUIRRELVM vm);
  static SQInteger _delslot(HSQUIRRELVM vm);
  static SQInteger _tostring(HSQUIRRELVM vm);

  const ScriptSourceInfo *getInitInfo() const override final { return initInfo.get(); }
  virtual void setName(const char *name) override;

  void whiteListMutatorClosure(Sqrat::Object closure);

  void reportNestedObservables(HSQUIRRELVM vm, const Sqrat::Object &val);
  bool checkMutationAllowed(HSQUIRRELVM vm); //< throws script error if mutation is forbidden


protected:
  Sqrat::Object value;
  eastl::unique_ptr<ScriptSourceInfo> initInfo;
  int timeChangeReq = 0;
  int timeChanged = 0;

  dag::Vector<Sqrat::Object> mutatorClosuresWhiteList;

  virtual void onValueRequested() {}

#if SQ_VAR_TRACE_ENABLED // Warn: bin compat change, must be last member of this struct
  VarTrace varTrace;
#endif
};

// Warn: struct layout is not binary compatible across libs that compiled with different SQ_VAR_TRACE_ENABLED
class ComputedValue final : public ScriptValueObservable, public IStateWatcher, public eastl::intrusive_list_node
{
  struct ComputedSource
  {
    eastl::string varName;
    BaseObservable *observable;
  };

  // Note: expected to be called only by `ObservablesGraph::allocComputed`
  ComputedValue(ObservablesGraph *g, Sqrat::Function func, dag::Vector<ComputedSource> &&sources_, Sqrat::Object initial_value,
    bool pass_cur_val_to_cb);

public:
  ComputedValue(const ComputedValue &) = delete;
  ComputedValue &operator=(const ComputedValue &) = delete;
  virtual ~ComputedValue();
  void operator delete(void *ptr) noexcept; // Frees from graph's `computedPool`

  virtual ComputedValue *getComputed() override { return this; }

  // Sorted from leaves to root to reduce memory copying on insertions
  typedef dag::Vector<ComputedValue *, framemem_allocator> DependenciesQueue;

  int collectDependencies(DependenciesQueue &out_queue, int depth);
  bool onTriggerBegin();
  bool onTriggerEnd(bool deferred);
  virtual bool onSourceObservableChanged() override;
  virtual void onObservableRelease(BaseObservable *observable) override;
  virtual void onGraphShutdown(bool exiting, Tab<Sqrat::Object> &cleared_storage) override;
  virtual HSQOBJECT getCurScriptInstance() override;
  virtual Sqrat::Object dbgGetWatcherScriptInstance() override;

  virtual Sqrat::Object getValueForNotify() const override
  {
    G_ASSERT(!needRecalc);
    G_ASSERT(!maybeRecalc);
    return value;
  }

  static SQInteger script_ctor(HSQUIRRELVM vm);

  static SQInteger get_sources(HSQUIRRELVM vm);
  static SQInteger _tostring(HSQUIRRELVM vm);

  bool dbgTriggerRecalcIfNeeded(String &err_msg);

  inline bool getNeedRecalc() const { return needRecalc; }
  inline bool getUsed() const { return isUsed; }
  void updateUsed(bool certainly_used);

  inline bool getMarked() const { return isMarked; }

  ComputedValue *addImplicitSource(ComputedValue *c)
  {
    implicitSources.insert(c);
    return this;
  }
  bool isImplicitSource(ComputedValue *c) const { return implicitSources.find(c) != implicitSources.end(); }

private:
  using VisitedSet = dag::FixedVectorSet<SQRawObjectVal, 4, true, eastl::use_self<SQRawObjectVal>, framemem_allocator>;
  static bool collect_sources_from_free_vars(ObservablesGraph *graph, HSQUIRRELVM vm, HSQOBJECT func_obj,
    dag::Vector<ComputedSource> &target, VisitedSet &visited);
  bool recalculate(bool &ok);
  bool invalidateWatchers();
  void removeMaybeRecalc();

  virtual void onValueRequested() override;

protected:
  friend class ObservablesGraph;
  friend class BaseObservable;
  friend void graph_viewer();
  Sqrat::Function func;
  dag::Vector<ComputedSource> sources;
  eastl::vector_set<ComputedValue *> implicitSources;
};

void bind_frp_classes(SqModules *module_mgr);

extern bool throw_frp_script_errors;

dag::Vector<ObservablesGraph *> get_all_graphs();

extern int pull_frp_imgui; // reference this to link imgui graph viewer module

} // namespace sqfrp
