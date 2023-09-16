//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
#include <EASTL/hash_set.h>
#include <EASTL/vector.h>
#include <EASTL/vector_set.h>
#include <EASTL/string.h>
#include <util/dag_string.h>
#include <memory/dag_framemem.h>
#include <sqrat.h>
#include <squirrel/vartrace.h>


class SqModules;

namespace sqfrp
{

class IStateWatcher;
class BaseObservable;


class NotifyQueue
{
public:
  void add(BaseObservable *item);
  void remove(BaseObservable *item);
  // typedef eastl::vector<BaseObservable*> Container; // do we need determenistic order?
  typedef eastl::vector_set<BaseObservable *> Container; // do we need determenistic order?
  Container container;
};


class IStateWatcher
{
public:
  // Sorted from leaves to root to reduce memory copying on insertions
  typedef eastl::vector<IStateWatcher *, framemem_allocator> DependenciesQueue;
  virtual int collectDependencies(DependenciesQueue &out_queue, int depth)
  {
    if (eastl::find(out_queue.begin(), out_queue.end(), this) == out_queue.end())
      out_queue.push_back(this);
    return depth + 1;
  }

  virtual bool onSourceObservableChanged() = 0;
  virtual void onObservableRelease(BaseObservable *observable) = 0;
  virtual bool onTriggerBegin() { return true; }
  virtual bool onTriggerEnd() { return true; }
  virtual Sqrat::Object dbgGetWatcherScriptInstance() { return Sqrat::Object(); }
};


struct ScriptSourceInfo
{
  eastl::string initFuncName, initSourceFileName;
  SQInteger initSourceLine = -1;

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
public:
  ObservablesGraph(HSQUIRRELVM vm_);
  ~ObservablesGraph() = default;

  void addObservable(BaseObservable *obs);
  void removeObservable(BaseObservable *obs);
  void notifyObservablesOnShutdown(bool exiting);
  void checkLeaks();
  bool callScriptSubscribers(BaseObservable *triggered_node, String &err_msg);

  static ObservablesGraph *get_from_vm(HSQUIRRELVM vm);

  void onEnterTriggerRoot();
  void onExitTriggerRoot();

private:
  bool checkGcUnreachable();
  void dumpSubscribersQueue(Tab<SubscriberCall> &queue);

public:
  HSQUIRRELVM vm = nullptr;
  eastl::hash_set<BaseObservable *> allObservables;
  bool observablesListModified = false; //< to protect against iterator invalidation

  NotifyQueue notifyQueueCache;
  Tab<SubscriberCall> subscriberCallsQueue, curSubscriberCalls;

  int generation = 0;
  int maxSubscribersQueueLen = 0;
  bool checkNestedObservable = false;
  bool forceImmutable = false;

private:
  int curTriggerSubscribersCounter = 0;
  int triggerNestDepth = 0;
};


class BaseObservable
{
public:
  BaseObservable(ObservablesGraph *graph);
  virtual ~BaseObservable();

  bool triggerRoot(String &err_msg);

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
  virtual Sqrat::Object trace() { return Sqrat::Object(); }
  virtual HSQOBJECT dbgGetCurrentSqInstance();

  void collectScriptSubscribers(Tab<SubscriberCall> &container) const;

public:
  Tab<IStateWatcher *> watchers;
  int generation = 0;

protected:
  ObservablesGraph *graph;
  Tab<Sqrat::Function> scriptSubscribers;

  // some debug stuff
  bool isIteratingWatchers = false;
  bool isInTrigger = false;
};


class ScriptValueObservable : public BaseObservable
{
public:
  ScriptValueObservable(ObservablesGraph *graph);
  ScriptValueObservable(Sqrat::Object val);
  ~ScriptValueObservable() = default;

  static SQInteger script_ctor(HSQUIRRELVM vm);

  Sqrat::Object getValue() { return value; }
  int getTimeChangeReq() { return timeChangeReq; }
  int getTimeChanged() { return timeChanged; }
  const Sqrat::Object &getValueRef() const { return value; }
  virtual Sqrat::Object trace() override;
  bool setValue(const Sqrat::Object &new_val, String &err_msg);

  virtual Sqrat::Object getValueForNotify() const { return value; }

  virtual void onGraphShutdown(bool exiting, Tab<Sqrat::Object> &cleared_storage) override;
  virtual void fillInfo(Sqrat::Table &) const override;
  virtual void fillInfo(String &) const override;
  virtual HSQOBJECT dbgGetCurrentSqInstance() override;

  static SQInteger update(HSQUIRRELVM vm, int arg_pos);
  static SQInteger updateViaCallMm(HSQUIRRELVM vm);
  static SQInteger updateViaMethod(HSQUIRRELVM vm);
  static SQInteger mutate(HSQUIRRELVM vm);
  bool mutateCurValue(HSQUIRRELVM vm, SQInteger closure_pos);

  static SQInteger _newslot(HSQUIRRELVM vm);
  static SQInteger _delslot(HSQUIRRELVM vm);
  static SQInteger _tostring(HSQUIRRELVM vm);

  const ScriptSourceInfo &getInitInfo() const { return initInfo; }

  void whiteListMutatorClosure(Sqrat::Object closure);

  void reportNestedObservables(HSQUIRRELVM vm, const Sqrat::Object &val);
  bool checkMutationAllowed(HSQUIRRELVM vm); //< throws script error if mutation is forbidden


public:
  bool checkNestedObservable = true;

protected:
  Sqrat::Object value;
  ScriptSourceInfo initInfo;
  int timeChangeReq = 0;
  int timeChanged = 0;

  Tab<Sqrat::Object> mutatorClosuresWhiteList;

#if DAGOR_DBGLEVEL > 0 && _TARGET_PC_WIN
  VarTrace varTrace;
#endif
};


class ComputedValue : public ScriptValueObservable, public IStateWatcher
{
public:
  ComputedValue(Sqrat::Function func, Sqrat::Object initial_value, bool pass_cur_val_to_cb);
  virtual ~ComputedValue();

  virtual int collectDependencies(DependenciesQueue &out_queue, int depth) override;
  virtual bool onTriggerBegin() override;
  virtual bool onTriggerEnd() override;
  virtual bool onSourceObservableChanged() override;
  virtual void onObservableRelease(BaseObservable *observable) override;
  virtual void onGraphShutdown(bool exiting, Tab<Sqrat::Object> &cleared_storage) override;
  virtual HSQOBJECT dbgGetCurrentSqInstance() override;
  virtual Sqrat::Object dbgGetWatcherScriptInstance() override;

  static SQInteger script_ctor(HSQUIRRELVM vm);

  static SQInteger get_sources(HSQUIRRELVM vm);
  static SQInteger _tostring(HSQUIRRELVM vm);

private:
  void collectSourcesFromFreeVars(HSQUIRRELVM vm, HSQOBJECT func_obj);
  bool recalculate();
  bool invalidateWatchers();

private:
  struct ComputedSource
  {
    String varName;
    BaseObservable *observable;
  };

  Sqrat::Function func;
  Tab<ComputedSource> sources;
  bool isStateValid = true;
  bool needRecalc = false;
  bool isShuttingDown = false;
  bool funcAcceptsCurVal = false;
};

void bind_frp_classes(SqModules *module_mgr);

extern bool throw_frp_script_errors;

} // namespace sqfrp
