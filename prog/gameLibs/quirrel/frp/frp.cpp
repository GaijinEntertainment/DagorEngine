#include <quirrel/frp/dag_frp.h>
#include <quirrel/sqModules/sqModules.h>
#include <quirrel/sqStackChecker.h>
#include <sqext.h>

#include <math/dag_mathUtils.h>
#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_statDrv.h>
#include <sqstdaux.h>

#include <EASTL/algorithm.h>
#include <EASTL/string.h>
#include <EASTL/hash_map.h>
#include <dag/dag_vectorMap.h>

#include <squirrel/sqpcheader.h>
#include <squirrel/sqvm.h>
#include <squirrel/sqstate.h>
#include <squirrel/sqfuncproto.h>
#include <squirrel/sqclosure.h>

// workaround for android build
// synchronize condition to one in dag_frp.h header
#if !_TARGET_PC_WIN || DAGOR_DBGLEVEL <= 0
#undef VT_TRACE_SINGLE
#define VT_TRACE_SINGLE(...)
#endif

#if 0
#define FRPDBG debug_ctx
#else
#define FRPDBG(...)
#endif

#define MAX_ALLOWED_SUBSCRIBERS_QUEUE_LENGTH 200
#define MAX_SUBSCRIBERS_PER_TRIGGER          2000


namespace Sqrat
{

template <>
struct InstanceToString<sqfrp::ScriptValueObservable>
{
  static SQInteger Format(HSQUIRRELVM vm) { return sqfrp::ScriptValueObservable::_tostring(vm); }
};

template <>
struct InstanceToString<sqfrp::ComputedValue>
{
  static SQInteger Format(HSQUIRRELVM vm) { return sqfrp::ComputedValue::_tostring(vm); }
};

} // namespace Sqrat


namespace sqfrp
{


bool throw_frp_script_errors = true;

static const int computed_initial_update_dummy = _MAKE4C('INIT');
static const SQUserPointer computed_initial_update = (SQUserPointer)&computed_initial_update_dummy;

static const int dont_check_nested_dummy = _MAKE4C('!NST');
static const SQUserPointer dont_check_nested = (SQUserPointer)&dont_check_nested_dummy;

template <typename T>
static bool has_invalid_class_references(const Sqrat::Object &value, const char *dbg_name, int depth, String &out_info,
  Tab<String> &path)
{
  HSQUIRRELVM vm = value.GetVM();
  SQObjectType valType = value.GetType();

  out_info.clear();

  if (depth >= 50)
  {
    out_info.printf(0, "%s reference depth = %d, probably circular references", dbg_name, depth);
    return true;
  }

  if (valType == OT_INSTANCE)
  {
    HSQOBJECT ho = value.GetObject();
    bool canLeak = Sqrat::ClassType<T>::IsClassInstance(ho);
    if (canLeak)
    {
      out_info.printf(0, "Storing %s", dbg_name);
      return true;
    }
  }


  if (valType == OT_CLOSURE)
  {
    SqStackChecker stackCheck(vm);
    sq_pushobject(vm, value.GetObject());
    SQInteger nparams = 0, nfreevars = 0;
    G_VERIFY(SQ_SUCCEEDED(sq_getclosureinfo(vm, -1, &nparams, &nfreevars)));
    for (SQInteger i = 0; i < nfreevars; ++i)
    {
      const char *varName = sq_getfreevariable(vm, -1, i);
      G_ASSERT_CONTINUE(varName);
      if (Sqrat::ClassType<T>::IsClassInstance(vm, -1))
      {
        SQFunctionInfo fi;
        const char *funcName;
        String funcInfoStr;
        if (SQ_SUCCEEDED(sq_ext_getfuncinfo(value.GetObject(), &fi)))
          funcName = fi.name;
        else
          funcName = "<unknown>";

        path.push_back(String(varName));
        path.push_back(String(funcName));
        out_info.printf(0, "Capturing %s '%s' via free var in %s", dbg_name, varName, funcName);
      }
      sq_pop(vm, 1);
    }
    sq_pop(vm, 1);

    stackCheck.check();

    if (!out_info.empty())
      return true;
  }

  if (valType == OT_ARRAY || valType == OT_TABLE || valType == OT_CLASS || valType == OT_INSTANCE)
  {
    bool needCheck = true;
    if (valType == OT_INSTANCE)
    {
      SQUserPointer typetag;
      HSQOBJECT ho = value.GetObject();
      G_VERIFY(SQ_SUCCEEDED(sq_getobjtypetag(&ho, &typetag)));
      needCheck = !typetag; // check only script instances, skip native
    }

    if (needCheck)
    {
      Sqrat::Object::iterator it;
      while (value.Next(it))
      {
        Sqrat::Object slotVal(it.getValue(), vm);
        if (has_invalid_class_references<T>(slotVal, dbg_name, depth + 1, out_info, path))
        {
          Sqrat::Var<const char *> keyVar = Sqrat::Object(it.getKey(), vm).GetVar<const char *>();
          path.push_back(String(keyVar.value, keyVar.valueLen));
          return true;
        }
      }
    }
  }
  return false;
}

static void join_path(const Tab<String> &path, String &res)
{
  res.clear();
  for (int i = path.size() - 1; i >= 0; --i)
  {
    if (!res.empty())
      res.append(" > ");
    res.aprintf(0, "'%s'", path[i].c_str());
  }
}

static bool has_script_observable_references(const Sqrat::Object &value, String &out_info, String &path_str)
{
  Tab<String> path(framemem_ptr());
  bool res = has_invalid_class_references<ScriptValueObservable>(value, "script observable", 0, out_info, path);
  join_path(path, path_str);
  return res;
}


template <class O>
static HSQOBJECT get_observable_instance(HSQUIRRELVM vm, O *obs)
{
  Sqrat::ClassData<O> *cd = Sqrat::ClassType<O>::getClassData(vm);
  G_ASSERT(cd);
  if (cd)
  {
    auto it = cd->instances->find(obs);
    if (it != cd->instances->end())
      return it->second;
  }
  HSQOBJECT hNull;
  sq_resetobject(&hNull);
  return hNull;
}


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


void NotifyQueue::add(BaseObservable *item)
{
  // if (eastl::find(container.begin(), container.end(), item) == container.end())
  //   container.push_back(item);
  container.insert(item);
}

void NotifyQueue::remove(BaseObservable *item) { container.erase_first(item); }


static const int observable_graph_key_dummy = _MAKE4C('FRPG');
static const int *observable_graph_key = &observable_graph_key_dummy;


ObservablesGraph::ObservablesGraph(HSQUIRRELVM vm_) : vm(vm_)
{
  SqStackChecker chk(vm_);

  sq_pushregistrytable(vm_);
  sq_pushuserpointer(vm, (SQUserPointer)observable_graph_key);

  if (SQ_SUCCEEDED(sq_rawget(vm, -2)))
  {
    sq_pop(vm, 1);
    DAG_FATAL("Only one FRP graph per VM is allowed");
  }

  sq_pushuserpointer(vm, (SQUserPointer)observable_graph_key);
  sq_pushuserpointer(vm_, this);
  G_VERIFY(SQ_SUCCEEDED(sq_rawset(vm_, -3)));
  sq_pop(vm, 1);
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


void ObservablesGraph::addObservable(BaseObservable *obs)
{
  allObservables.insert(obs);
  observablesListModified = true;
}


void ObservablesGraph::removeObservable(BaseObservable *obs)
{
  allObservables.erase(obs);
  observablesListModified = true;

  notifyQueueCache.remove(obs);
}

template <typename A, typename B>
eastl::pair<B, A> flip_pair(const eastl::pair<A, B> &p)
{
  return eastl::pair<B, A>(p.second, p.first);
}

void ObservablesGraph::notifyObservablesOnShutdown(bool exiting)
{
  int t0 = get_time_msec();
  debug_ctx("@#@ [FRP] graph shutdown (exiting = %d): %d observables total", exiting, int(allObservables.size()));

  Tab<Sqrat::Object> clearedObjects(framemem_ptr());
  int nClearedObjects = 0, nRestarts = 0;

  for (;;)
  {
    ++nRestarts;
    observablesListModified = false;
    for (eastl::hash_set<BaseObservable *>::iterator it = allObservables.begin(); it != allObservables.end(); ++it)
    {
      (*it)->onGraphShutdown(exiting, clearedObjects);
      if (observablesListModified) // restart traversal
      {
        debug_ctx("[FRP] shutdown cp#1: restart, cleared %d objects", int(clearedObjects.size()));
        nClearedObjects += clearedObjects.size();
        clearedObjects.clear();
        sq_collectgarbage(vm);
        break;
      }
    }
    if (!observablesListModified) // fully traversed
      break;
  }

  nClearedObjects += clearedObjects.size();
  clearedObjects.clear();

  debug_ctx("[FRP] shutdown cp#2 allObservables.size() = %d, %d script objects cleared, %d restarts performed",
    int(allObservables.size()), nClearedObjects, nRestarts);

  for (int iter = 0;; ++iter)
  {
    debug_ctx("[FRP] collecting garbage, iter = %d, allObservables.size() = %d", iter, int(allObservables.size()));

    observablesListModified = false;
    sq_collectgarbage(vm);
    if (!observablesListModified)
      break;
  }

  debug_ctx("[FRP] shutdown finished in %d ms, allObservables.size() = %d, %d script objects cleared total", get_time_msec() - t0,
    int(allObservables.size()), nClearedObjects);

#if DAGOR_DBGLEVEL > 0
  checkGcUnreachable();
#endif
}

bool ObservablesGraph::checkGcUnreachable()
{
  if (SQ_FAILED(sq_resurrectunreachable(vm))) // cannot do anything useful here without GC
    return true;

  bool isClean = true;
  String s;
  Sqrat::Var<Sqrat::Array> loops(vm, -1);

  if (loops.value.GetType() == OT_NULL)
    debug("[FRP] no unreachable items");
  else
  {
    SQInteger n = loops.value.Length();
    debug("[FRP] %d unreachable items", n);
    for (SQInteger i = 0; i < n; ++i)
    {
      Sqrat::Object item = loops.value.GetSlot(i);
      sq_pushobject(vm, item.GetObject());
      if (Sqrat::ClassType<BaseObservable>::IsClassInstance(vm, -1))
      {
        isClean = false;
        BaseObservable *obs = item.Cast<BaseObservable *>();
        obs->fillInfo(s);
        if (s != "unknown, NATIVE:-1")
          logerr("[FRP] unreachable observable: %s", s.c_str());
      }
      sq_pop(vm, 1);
    }
  }

  sq_pop(vm, 1); // sq_resurrectunreachable() result

  return isClean;
}


void ObservablesGraph::checkLeaks()
{
  debug("[FRP] checking leaks, generation = %d", generation);
  String ssi, nestedWatchedInfo;
  eastl::hash_map<eastl::string, int> infoCounts;
  for (BaseObservable *o : allObservables)
  {
    if (o->generation >= 0 && o->generation != this->generation)
    {
      o->fillInfo(ssi);
      eastl::string s(ssi.c_str());
      auto cit = infoCounts.find(s);
      if (cit == infoCounts.end())
        infoCounts[s] = 1;
      else
        infoCounts[s] = cit->second + 1;

      Sqrat::Object trace = o->trace();
      const HSQOBJECT hTrace = trace;
      const char *strTrace = sq_objtostring(&hTrace);
      if (strTrace && *strTrace)
      {
        debug("* %s:", ssi.c_str());
        debug("  %s", strTrace);
      }

      // #if CHECK_NESTED_OBSERVABLES
      //       Sqrat::Object val = o->getValueForNotify();
      //       if (has_script_observable_references(val, nestedWatchedInfo))
      //         debug("Note: [%s] at observable: %s", nestedWatchedInfo.c_str(), ssi.c_str());
      // #endif
    }
  }

  dag::VectorMap<int, eastl::string, eastl::greater<int>> sortedSsi;
  eastl::transform(infoCounts.begin(), infoCounts.end(), eastl::inserter(sortedSsi, sortedSsi.begin()), flip_pair<eastl::string, int>);

  if (sortedSsi.size())
  {
    debug("");
    debug("Leaked observables:");
    for (auto dbgIt : sortedSsi)
      debug("%d : %s", dbgIt.first, dbgIt.second.c_str());
    debug("");
  }
  else
    debug("No leaked observables found");
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


bool ObservablesGraph::callScriptSubscribers(BaseObservable *triggered_node, String &err_msg)
{
  triggered_node->collectScriptSubscribers(subscriberCallsQueue);
  for (BaseObservable *node : notifyQueueCache.container)
    node->collectScriptSubscribers(subscriberCallsQueue);
  notifyQueueCache.container.clear();

  if (subscriberCallsQueue.size() > MAX_ALLOWED_SUBSCRIBERS_QUEUE_LENGTH)
  {
    err_msg.printf(0, "Max subscribers queue length exceeded (current size = %d), see log for details", subscriberCallsQueue.size());
    dumpSubscribersQueue(subscriberCallsQueue);
    subscriberCallsQueue.clear();
    return false;
  }
  maxSubscribersQueueLen = ::max<int>(maxSubscribersQueueLen, subscriberCallsQueue.size());

  bool isCallingSubscribers = !curSubscriberCalls.empty();
  if (isCallingSubscribers) // leave processing to top level call
    return true;

  bool ok = true;
  for (;;)
  {
    if (subscriberCallsQueue.empty()) // done, nothing left in queue
      break;

    G_ASSERT(curSubscriberCalls.empty());
    curSubscriberCalls.swap(subscriberCallsQueue);

    curTriggerSubscribersCounter += curSubscriberCalls.size();
    G_ASSERT(triggerNestDepth > 0);
    if (triggerNestDepth > 0 && curTriggerSubscribersCounter > MAX_SUBSCRIBERS_PER_TRIGGER)
    {
      err_msg.printf(0, "Subscribers calls count exceeded (%d), see log for details", curTriggerSubscribersCounter);
      dumpSubscribersQueue(curSubscriberCalls);
      dumpSubscribersQueue(subscriberCallsQueue);
      curSubscriberCalls.clear();
      subscriberCallsQueue.clear(); // just in case
      return false;
    }

    for (SubscriberCall &sc : curSubscriberCalls)
    {
      if (!sc.func.Execute(sc.value))
      {
        String fullName;
        SQFunctionInfo fi;
        if (SQ_SUCCEEDED(sq_ext_getfuncinfo(sc.func.GetFunc(), &fi)))
          fullName.printf(0, "%s (%s:%d)", fi.name, fi.source, fi.line);
        else
          fullName = "<unknown>";

        err_msg.printf(0, "Subscriber function call failed (%s)", fullName.c_str());
        ok = false;
      }
    }
    curSubscriberCalls.clear();
  }
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


BaseObservable::BaseObservable(ObservablesGraph *graph_) : graph(graph_), generation(graph_->generation)
{
  FRPDBG("@#@ BaseObservable::ctor(%p)", this);
  G_ASSERT(graph);
  if (graph)
    graph->addObservable(this);
}


BaseObservable::~BaseObservable()
{
  FRPDBG("@#@ BaseObservable::dtor(%p), notifying %d watchers", this, int(watchers.size()));

  isIteratingWatchers = true;
  for (IStateWatcher *w : watchers)
  {
    FRPDBG("@#@ %p->onObservableRelease(%p)", w, this);
    w->onObservableRelease(this);
  }
  isIteratingWatchers = false;

  if (graph)
    graph->removeObservable(this);

  FRPDBG("@#@ / BaseObservable::dtor(%p)", this);
}


static void select_err_msg(String &err_msg, const char *val)
{
  if (err_msg.empty())
    err_msg = val;
}

static bool logerr_graph_error(HSQUIRRELVM vm, const char *err_msg)
{
  if (SQ_SUCCEEDED(sqstd_formatcallstackstring(vm)))
  {
    const SQChar *stack = nullptr;
    G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, -1, &stack)));
    G_ASSERT(stack);
    logerr("[frp:error] %s\n%s", err_msg, stack);
    sq_pop(vm, 1);
  }
  else
    logerr("[frp:error] %s (no stack)", err_msg);

  return false; // don't break execution
}

static bool raise_graph_error(HSQUIRRELVM vm, const char *err_msg)
{
  if (throw_frp_script_errors)
  {
    (void)sq_throwerror(vm, err_msg);
    return true;
  }
  else
    return logerr_graph_error(vm, err_msg);
}

bool BaseObservable::triggerRoot(String &err_msg)
{
  G_ASSERT(this);
  if (isInTrigger)
  {
    err_msg = "Trigger root: already in trigger - probably side effect in Computed";
    return false;
  }

  graph->onEnterTriggerRoot();

  bool ok = true;

  if (!graph->notifyQueueCache.container.empty())
  {
    ok = false;
    const char *localMsg = "Triggered while propagating changes - possibly a circular reference";

    debug_ctx(localMsg);
    String info;
    fillInfo(info);
    debug_ctx("Triggered observable: %s", info.c_str());
    debug_ctx("Notify queue (%d items):", int(graph->notifyQueueCache.container.size()));
    for (BaseObservable *o : graph->notifyQueueCache.container)
    {
      o->fillInfo(info);
      debug_ctx("  * %s", info.c_str());
    }

    select_err_msg(err_msg, localMsg);
  }

  isInTrigger = true;
  isIteratingWatchers = true;

  IStateWatcher::DependenciesQueue sorted_deps;
#if TIME_PROFILER_ENABLED
  if (!watchers.empty())
#endif
  {
    TIME_PROFILE(frp_collect_deps);
    int maxDepth = 0;
    for (IStateWatcher *w : watchers)
      maxDepth = max(maxDepth, w->collectDependencies(sorted_deps, /*depth*/ 0));
    DA_PROFILE_TAG(frp_collect_deps, "deps.size=%d maxDepth=%d", (int)sorted_deps.size(), maxDepth);
  }

  // all dependent watchers network (mind the leaves-to-root sorting order)
  for (auto it = sorted_deps.rbegin(); it != sorted_deps.rend(); ++it)
  {
    if (!(*it)->onTriggerBegin())
    {
      ok = false;
      select_err_msg(err_msg, "Graph update failed (trigger begin)");
    }
  }

  // first level watchers only
  for (IStateWatcher *w : watchers)
  {
    if (!w->onSourceObservableChanged())
    {
      ok = false;
      select_err_msg(err_msg, "Graph update failed (notifying root change)");
    }
  }

  // all dependent watchers network (mind the leaves-to-root sorting order)
  for (auto it = sorted_deps.rbegin(); it != sorted_deps.rend(); ++it)
  {
    if (!(*it)->onTriggerEnd())
    {
      ok = false;
      select_err_msg(err_msg, "Graph update failed (trigger end)");
    }
  }

  isIteratingWatchers = false;

  String subscribersErr;
  if (!graph->callScriptSubscribers(this, subscribersErr))
  {
    ok = false;
    select_err_msg(err_msg, subscribersErr);
  }

  isInTrigger = false;

  graph->onExitTriggerRoot();

  return ok;
}


void BaseObservable::collectScriptSubscribers(Tab<SubscriberCall> &container) const
{
  if (scriptSubscribers.empty())
    return;
  Sqrat::Object val = getValueForNotify();
  int start = container.size();
  int count = scriptSubscribers.size();
  append_items(container, count);
  for (int i = 0; i < count; ++i)
  {
    container[start + i].func = scriptSubscribers[i];
    container[start + i].value = val;
  }
}


void BaseObservable::subscribeWatcher(IStateWatcher *watcher)
{
  FRPDBG("@#@ BaseObservable(%p)::subscribeWatcher(%p)", this, watcher);
  G_ASSERT(!isIteratingWatchers);
  G_ASSERT(watcher);
  if (watcher)
  {
    if (find_value_idx(watchers, watcher) < 0)
      watchers.push_back(watcher);
  }
  FRPDBG("@#@ %p watchers count = %d", this, int(watchers.size()));
}


void BaseObservable::unsubscribeWatcher(IStateWatcher *watcher)
{
  FRPDBG("@#@ BaseObservable(%p)::unsubscribeWatcher(%p)", this, watcher);
  G_ASSERT(!isIteratingWatchers);
  erase_item_by_value(watchers, watcher);
}


static bool are_sq_obj_equal(HSQUIRRELVM vm, const HSQOBJECT &a, const HSQOBJECT &b)
{
  SQInteger res = 0;
  return sq_direct_cmp(vm, &a, &b, &res) && (res == 0);
}


SQInteger BaseObservable::subscribe(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<BaseObservable *>(vm))
    return SQ_ERROR;

  Sqrat::Var<BaseObservable *> self(vm, 1);
  HSQOBJECT func;
  sq_getstackobj(vm, 2, &func);

  SQInteger nparams = 0, nfreevars = 0;
  G_VERIFY(SQ_SUCCEEDED(sq_getclosureinfo(vm, 2, &nparams, &nfreevars)));
  bool valid = (nparams == 2) || (nparams <= -2);
  if (!valid)
    return sqstd_throwerrorf(vm, "Subscriber function must accept 2 parameters (actual count is %d)", nparams);

  bool isNew =
    eastl::find_if(self.value->scriptSubscribers.begin(), self.value->scriptSubscribers.end(), [&](const Sqrat::Function &f) {
      G_ASSERT(vm == f.GetVM());
      return are_sq_obj_equal(vm, func, f.GetFunc());
    }) == self.value->scriptSubscribers.end();

  if (isNew)
    self.value->scriptSubscribers.push_back(Sqrat::Function(vm, Sqrat::Object(vm), func));

  sq_push(vm, 1);
  return 1;
}


SQInteger BaseObservable::unsubscribe(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<BaseObservable *>(vm))
    return SQ_ERROR;

  Sqrat::Var<BaseObservable *> self(vm, 1);
  HSQOBJECT func;
  sq_getstackobj(vm, 2, &func);

  for (int i = 0, n = self.value->scriptSubscribers.size(); i < n; ++i)
  {
    G_ASSERT(vm == self.value->scriptSubscribers[i].GetVM());
    if (are_sq_obj_equal(vm, func, self.value->scriptSubscribers[i].GetFunc()))
    {
      erase_items(self.value->scriptSubscribers, i, 1);
      break;
    }
  }
  return 1;
}


SQInteger BaseObservable::sqTrigger(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<BaseObservable *>(vm))
    return SQ_ERROR;

  Sqrat::Var<BaseObservable *> self(vm, 1);
  String errMsg;
  if (!self.value->triggerRoot(errMsg))
  {
    if (raise_graph_error(vm, errMsg))
      return SQ_ERROR;
  }
  return 1;
}

SQInteger BaseObservable::dbgGetListeners(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<BaseObservable *>(vm))
    return SQ_ERROR;

  Sqrat::Var<BaseObservable *> self(vm, 1);

  uint32_t nWatchers = self.value->watchers.size();
  Sqrat::Array arrWatchers(vm, nWatchers);
  for (uint32_t i = 0; i < nWatchers; ++i)
    arrWatchers.SetValue(i, self.value->watchers[i]->dbgGetWatcherScriptInstance());

  uint32_t nSubscribers = self.value->scriptSubscribers.size();
  Sqrat::Array arrSubscribers(vm, nSubscribers);
  for (uint32_t i = 0; i < nSubscribers; ++i)
    arrSubscribers.SetValue(i, self.value->scriptSubscribers[i]);

  Sqrat::Table res(vm);
  res.SetValue("watchers", arrWatchers);
  res.SetValue("subscribers", arrSubscribers);
  sq_pushobject(vm, res);

  return 1;
}


void BaseObservable::onGraphShutdown(bool exiting, Tab<Sqrat::Object> &cleared_storage)
{
  G_UNUSED(exiting);
  FRPDBG("@#@ BaseObservable(%p)::onGraphShutdown(exiting = %d)", this, exiting);
  cleared_storage.reserve(cleared_storage.size() + scriptSubscribers.size());
  for (Sqrat::Function &f : scriptSubscribers)
    cleared_storage.push_back(Sqrat::Object(f.GetFunc(), graph->vm));
  scriptSubscribers.clear();
}


HSQOBJECT BaseObservable::dbgGetCurrentSqInstance() { return get_observable_instance(graph->vm, this); }


ScriptValueObservable::ScriptValueObservable(ObservablesGraph *graph_) : BaseObservable(graph_)
{
  initInfo.init(graph_->vm);
  VT_TRACE_SINGLE(this, value.GetObject(), graph_->vm);

  timeChangeReq = timeChanged = ::get_time_msec();
}


ScriptValueObservable::ScriptValueObservable(Sqrat::Object val) :
  BaseObservable(ObservablesGraph::get_from_vm(val.GetVM())), value(val)
{
  if (graph->forceImmutable)
    value.FreezeSelf();

  initInfo.init(val.GetVM());
  VT_TRACE_SINGLE(this, value.GetObject(), val.GetVM());

  timeChangeReq = timeChanged = ::get_time_msec();
}


bool ScriptValueObservable::setValue(const Sqrat::Object &new_val, String &err_msg)
{
  HSQUIRRELVM vm = value.GetVM();
  if (!vm)
    vm = new_val.GetVM();
  if (!vm)
  {
    err_msg = "No valid VM";
    return false;
  }

  timeChangeReq = ::get_time_msec();

  SQInteger cmpRes = 0;
  const HSQOBJECT &objOld = value.GetObject();
  const HSQOBJECT &objNew = new_val.GetObject();
  bool compared = sq_direct_cmp(vm, &objOld, &objNew, &cmpRes);
  if (!compared || cmpRes != 0)
  {
    value = new_val;
    if (graph->forceImmutable)
      value.FreezeSelf();

    timeChanged = timeChangeReq;

    VT_TRACE_SINGLE(this, value.GetObject(), vm);
    if (!triggerRoot(err_msg))
    {
      logerr_graph_error(vm, err_msg);
      return false;
    }
  }
  return true;
}


Sqrat::Object ScriptValueObservable::trace()
{
#if DAGOR_DBGLEVEL > 0 && _TARGET_PC_WIN
  HSQUIRRELVM vm = value.GetVM();
  if (!vm)
    return Sqrat::Object();

  char buf[2048] = {0};
  varTrace.printStack(buf, sizeof(buf));
  return Sqrat::Object((const SQChar *)buf, vm, -1);
#else
  return Sqrat::Object();
#endif
}


bool ScriptValueObservable::checkMutationAllowed(HSQUIRRELVM vm)
{
  bool isAllowed = true;
  SqStackChecker stackCheck(vm);
  // check if mutation is allowed for caller
  if (!mutatorClosuresWhiteList.empty())
  {
    if (SQ_FAILED(sq_getcallee(vm)))
      return false;

    HSQOBJECT callee;
    sq_getstackobj(vm, -1, &callee);
    G_ASSERT(sq_type(callee) == OT_CLOSURE);
    if (sq_type(callee) == OT_CLOSURE)
    {
      isAllowed = false;
      for (Sqrat::Object &item : mutatorClosuresWhiteList)
      {
        if (item.GetType() == OT_CLOSURE && item.GetObject()._unVal.pClosure == callee._unVal.pClosure)
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

  return isAllowed;
}


bool ScriptValueObservable::mutateCurValue(HSQUIRRELVM vm, SQInteger closure_pos)
{
  SQInteger nParams = 0, nFreeVars = 0;
  G_VERIFY(SQ_SUCCEEDED(sq_getclosureinfo(vm, closure_pos, &nParams, &nFreeVars)));
  if (nParams != 2)
  {
    sqstd_throwerrorf(vm, "Callback must accept a value to mutate, but arguments count is %d", nParams);
    return false;
  }

  SQObjectType tp = value.GetType();
  if (tp != OT_TABLE && tp != OT_ARRAY && tp != OT_USERDATA && tp != OT_INSTANCE && tp != OT_CLASS)
  {
    (void)sqstd_throwerrorf(vm, "Only containers can be mutated by callback, current value is of type '%s'", sq_objtypestr(tp));
    return false;
  }

  SqStackChecker stackCheck(vm);

  sq_push(vm, closure_pos);
  sq_pushnull(vm); // 'this' for call
  Sqrat::Object mutableVal = value;
  if (graph->forceImmutable)
    mutableVal.UnfreezeSelf();
  sq_pushobject(vm, mutableVal);
  SQRESULT result = sq_call(vm, 2, true, true);
  stackCheck.restore();

  if (SQ_FAILED(result))
  {
    (void)sq_throwerror(vm, "Callback failure");
    return false;
  }
  value = mutableVal;
  if (graph->forceImmutable)
    value.FreezeSelf();
  timeChangeReq = ::get_time_msec();
  timeChanged = timeChangeReq;

  return true;
}


SQInteger ScriptValueObservable::update(HSQUIRRELVM vm, int arg_pos)
{
  if (!Sqrat::check_signature<ScriptValueObservable *>(vm))
    return SQ_ERROR;

  Sqrat::Var<ScriptValueObservable *> varSelf(vm, 1);
  ScriptValueObservable *self = varSelf.value;
  if (self->graph->vm != vm)
    return sq_throwerror(vm, "Invalid VM");

  if (!self->checkMutationAllowed(vm))
    return SQ_ERROR; // error was thrown from checkMutationAllowed

  if (sq_gettype(vm, arg_pos) == OT_CLOSURE)
    return sq_throwerror(vm, "Updating observable via callback is no longer supported, use mutate()");
  else
  {
    self->timeChangeReq = ::get_time_msec();

    HSQOBJECT hNewVal;
    sq_getstackobj(vm, arg_pos, &hNewVal);
    const HSQOBJECT &hOldVal = self->value.GetObject();
    SQInteger cmpRes = 0;
    bool compared = sq_direct_cmp(vm, &hOldVal, &hNewVal, &cmpRes);
    if (compared && cmpRes == 0)
      return 0;

    self->value = Sqrat::Object(hNewVal, vm);
    self->timeChanged = self->timeChangeReq;
  }

  VT_TRACE_SINGLE(self, self->value.GetObject(), vm);

  self->reportNestedObservables(vm, self->value);

  String errMsg;
  if (!self->triggerRoot(errMsg))
  {
    if (raise_graph_error(vm, errMsg))
      return SQ_ERROR;
  }
  return 0;
}


SQInteger ScriptValueObservable::mutate(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<ScriptValueObservable *>(vm))
    return SQ_ERROR;

  Sqrat::Var<ScriptValueObservable *> varSelf(vm, 1);
  ScriptValueObservable *self = varSelf.value;
  if (self->graph->vm != vm)
    return sq_throwerror(vm, "Invalid VM");

  if (!self->checkMutationAllowed(vm))
    return SQ_ERROR; // error was thrown from checkMutationAllowed

  if (!self->mutateCurValue(vm, 2))
    return SQ_ERROR;

  VT_TRACE_SINGLE(self, self->value.GetObject(), vm);

  self->reportNestedObservables(vm, self->value);

  String errMsg;
  if (!self->triggerRoot(errMsg))
  {
    if (raise_graph_error(vm, errMsg))
      return SQ_ERROR;
  }
  return 0;
}


SQInteger ScriptValueObservable::modify(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<ScriptValueObservable *>(vm))
    return SQ_ERROR;

  Sqrat::Var<ScriptValueObservable *> varSelf(vm, 1);
  ScriptValueObservable *self = varSelf.value;
  if (self->graph->vm != vm)
    return sq_throwerror(vm, "Invalid VM");

  if (!self->checkMutationAllowed(vm))
    return SQ_ERROR; // error was thrown from checkMutationAllowed

  const SQInteger closurePos = 2;

  SQInteger nParams = 0, nFreeVars = 0;
  G_VERIFY(SQ_SUCCEEDED(sq_getclosureinfo(vm, 2, &nParams, &nFreeVars)));
  if (nParams != 2)
    return sqstd_throwerrorf(vm, "Callback must be a function with 1 argument (previous value), but arguments count is %d", nParams);

  SQInteger prevTop = sq_gettop(vm);

  sq_pushnull(vm); // 'this' for call
  sq_pushobject(vm, self->value);
  SQRESULT result = sq_call(vm, 2, true, true);
  if (SQ_FAILED(result))
    return sq_throwerror(vm, "Callback failure");

  G_UNUSED(prevTop);
  G_ASSERT(sq_gettop(vm) == prevTop + 1); // result of callback added on top of the stack

  return update(vm, closurePos + 1);
}


void ScriptValueObservable::reportNestedObservables(HSQUIRRELVM vm, const Sqrat::Object &val)
{
  if (checkNestedObservable && graph->checkNestedObservable)
  {
    String errMsg, path;
    if (has_script_observable_references(val, errMsg, path))
    {
      SqStackChecker check(vm);
      const char *stack = nullptr;
      if (SQ_SUCCEEDED(sqstd_formatcallstackstring(vm)))
        sq_getstring(vm, -1, &stack);

      String ssi;
      fillInfo(ssi);
      logerr("[FRP] %s [A]\nObservable checked: %s\npath = %s\n%s", errMsg.c_str(), ssi.c_str(), path.c_str(),
        stack ? stack : "<no stack>");

      if (stack)
        sq_pop(vm, 1);

      checkNestedObservable = false; // once per run to avoid spam in logs
    }
  }
}


SQInteger ScriptValueObservable::updateViaCallMm(HSQUIRRELVM vm) { return update(vm, 3); }

SQInteger ScriptValueObservable::updateViaMethod(HSQUIRRELVM vm) { return update(vm, 2); }


SQInteger ScriptValueObservable::_newslot(HSQUIRRELVM vm) { return sq_throwerror(vm, "<- is not supported (use mutate() instead)"); }


SQInteger ScriptValueObservable::_delslot(HSQUIRRELVM vm)
{
  return sq_throwerror(vm, "delete is not supported (use mutate() instead)");
}


void ScriptValueObservable::onGraphShutdown(bool exiting, Tab<Sqrat::Object> &cleared_storage)
{
  FRPDBG("@#@ ScriptValueObservable(%p)::onGraphShutdown::onGraphShutdown(exiting = %d)", this, exiting);

  // protect observable from being destructed by zeroing refcount during
  // release of script subscribers
  HSQUIRRELVM vm = graph->vm;
  HSQOBJECT hInstance = get_observable_instance<ScriptValueObservable>(vm, this);
  FRPDBG("@#@ script instance is %s", sq_isnull(hInstance) ? "NULL" : "NOT NULL");
  sq_addref(vm, &hInstance);

  if (exiting)
    value.Release();

  BaseObservable::onGraphShutdown(exiting, cleared_storage);

  append_items(cleared_storage, mutatorClosuresWhiteList.size(), mutatorClosuresWhiteList.data());
  mutatorClosuresWhiteList.clear();

  sq_release(vm, &hInstance);
}


void ScriptValueObservable::fillInfo(Sqrat::Table &t) const
{
  initInfo.fillInfo(t);
  t.SetValue("value", value);
}

void ScriptValueObservable::fillInfo(String &s) const
{
  initInfo.toString(s);

  HSQUIRRELVM vm = graph->vm;
  G_ASSERT(vm);
  sq_pushobject(vm, value.GetObject());
  if (SQ_SUCCEEDED(sq_tostring(vm, -1)))
  {
    const char *valStr = nullptr;
    G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, -1, &valStr)));
    s.aprintf(0, ", value = %s", valStr);
    sq_pop(vm, 1);
  }
  sq_pop(vm, 1);
}


void ScriptValueObservable::whiteListMutatorClosure(Sqrat::Object closure)
{
  G_ASSERT(closure.GetType() == OT_CLOSURE);
  mutatorClosuresWhiteList.push_back(closure);
}


HSQOBJECT ScriptValueObservable::dbgGetCurrentSqInstance()
{
  HSQOBJECT o = get_observable_instance(graph->vm, this);
  if (!sq_isnull(o))
    return o;
  return BaseObservable::dbgGetCurrentSqInstance();
}


SQInteger ScriptValueObservable::script_ctor(HSQUIRRELVM vm)
{
  SQInteger nParams = sq_gettop(vm);
  if (nParams > 3)
    return sq_throwerror(vm, "Too many arguments");

  Sqrat::Object initialValue(vm);
  bool checkNestedObservable = true;
  int totalValues = 0;
  for (int i = 2, n = sq_gettop(vm); i <= n; ++i)
  {
    SQObjectType argType = sq_gettype(vm, i);
    if (argType == OT_USERPOINTER)
    {
      SQUserPointer up;
      G_VERIFY(SQ_SUCCEEDED(sq_getuserpointer(vm, i, &up)));
      if (up == dont_check_nested)
        checkNestedObservable = false;
      else
        return sqstd_throwerrorf(vm, "Invalid userpointer argument at %d", i);
    }
    else
    {
      HSQOBJECT ho;
      sq_getstackobj(vm, i, &ho);
      initialValue = Sqrat::Object(ho, vm);
      ++totalValues;
    }
  }

  if (totalValues > 1)
    return sq_throwerror(vm, "Too many value arguments");

  ObservablesGraph *graph = ObservablesGraph::get_from_vm(vm);
  if (checkNestedObservable && graph->checkNestedObservable)
  {
    String errMsg, path;
    if (has_script_observable_references(initialValue, errMsg, path))
    {
      SqStackChecker check(vm);
      const char *stack = nullptr;
      if (SQ_SUCCEEDED(sqstd_formatcallstackstring(vm)))
        sq_getstring(vm, -1, &stack);

      logerr("[FRP] %s [B]\npath = %s\n%s", errMsg.c_str(), path.c_str(), stack ? stack : "<no stack>");

      if (stack)
        sq_pop(vm, 1);

      checkNestedObservable = false; // report once per run
    }
  }

  ScriptValueObservable *inst = new ScriptValueObservable(initialValue);
  inst->checkNestedObservable = checkNestedObservable;
  Sqrat::ClassType<ScriptValueObservable>::SetManagedInstance(vm, 1, inst);
  return 0;
}


ComputedValue::ComputedValue(Sqrat::Function func_, Sqrat::Object initial_value, bool pass_cur_val_to_cb) :
  ScriptValueObservable(func_.GetVM()), func(func_), funcAcceptsCurVal(pass_cur_val_to_cb)
{
  FRPDBG("@#@ ComputedValue::ctor(%p | %p)", this, (IStateWatcher *)this);

  value = initial_value;
  value.FreezeSelf();

  SQFunctionInfo fi;
  if (SQ_SUCCEEDED(sq_ext_getfuncinfo(func.GetFunc(), &fi)))
  {
    initInfo.initFuncName = fi.name;
    initInfo.initSourceFileName = fi.source;
    initInfo.initSourceLine = fi.line;
  }

  collectSourcesFromFreeVars(func_.GetVM(), func_.GetFunc());
}


ComputedValue::~ComputedValue()
{
  FRPDBG("@#@ ComputedValue::dtor(%p | %p | %p), unsubscribing from %d sources", this, (BaseObservable *)this, (IStateWatcher *)this,
    (int)sources.size());
  for (ComputedSource &src : sources)
  {
    FRPDBG("@#@ %p (%s)->unsubscribeWatcher(%p | %p)", src.observable, src.varName.c_str(), this, (IStateWatcher *)this);
    src.observable->unsubscribeWatcher(this);
  }
  FRPDBG("@#@ / ComputedValue::dtor(%p | %p  | %p)", this, (BaseObservable *)this, (IStateWatcher *)this);
}


HSQOBJECT ComputedValue::dbgGetCurrentSqInstance()
{
  HSQOBJECT o = get_observable_instance(graph->vm, this);
  if (!sq_isnull(o))
    return o;
  return ScriptValueObservable::dbgGetCurrentSqInstance();
}


Sqrat::Object ComputedValue::dbgGetWatcherScriptInstance() { return Sqrat::Object(dbgGetCurrentSqInstance(), graph->vm); }


void ComputedValue::collectSourcesFromFreeVars(HSQUIRRELVM vm, HSQOBJECT func_obj)
{
  // G_ASSERT(sources.empty());
  SqStackChecker stackCheck(vm);
  sq_pushobject(vm, func_obj);
  SQInteger nparams = 0, nfreevars = 0;
  G_VERIFY(SQ_SUCCEEDED(sq_getclosureinfo(vm, -1, &nparams, &nfreevars)));
  FRPDBG("@#@ ComputedValue(%p | %p)::collectSourcesFromFreeVars(), nfreevars = %d", this, (IStateWatcher *)this, int(nfreevars));

  for (SQInteger i = 0; i < nfreevars; ++i)
  {
    const char *varName = sq_getfreevariable(vm, -1, i);
    G_ASSERT(varName);
    FRPDBG("@#@ var #%d/%d name = %s", int(i), int(nfreevars), varName);
    if (varName)
    {
      if (sq_gettype(vm, -1) == OT_CLOSURE)
      {
        HSQOBJECT f;
        sq_getstackobj(vm, -1, &f);
        if (_closure(f)->_function->_hoistingLevel > 0)
          collectSourcesFromFreeVars(vm, f);
      }
      else if (Sqrat::ClassType<BaseObservable>::IsClassInstance(vm, -1))
      {
        Sqrat::Var<BaseObservable *> obsVar(vm, -1);
        bool isUnique = eastl::find_if(sources.begin(), sources.end(),
                          [o = obsVar.value](const ComputedSource &src) { return src.observable == o; }) == sources.end();

        if (isUnique)
        {
          FRPDBG("@#@ %p (%s)->subscribeWatcher(%p | %p)", obsVar.value, varName, this, (IStateWatcher *)this);
          obsVar.value->subscribeWatcher(this);
          ComputedSource &src = sources.push_back();
          src.varName = varName;
          src.observable = obsVar.value;
        }
      }
    }
    sq_pop(vm, 1);
  }
  sq_pop(vm, 1); // func_obj

  stackCheck.check();

  G_ASSERT_RETURN(func_obj._type == OT_CLOSURE, );
  SQFunctionProto *proto = func_obj._unVal.pClosure->_function;
  for (SQInteger iConst = 0; iConst < proto->_nliterals; ++iConst)
  {
    SQObjectPtr &constant = proto->_literals[iConst];

    if (sq_type(constant) == OT_CLOSURE)
    {
      // should not happen but just in case
      if (_closure(constant)->_function->_hoistingLevel > 0)
        collectSourcesFromFreeVars(vm, constant);
    }
    else if (Sqrat::ClassType<BaseObservable>::IsClassInstance(constant))
    {
      SqStackChecker stackCheck2(vm);

      sq_pushobject(vm, constant);
      Sqrat::Var<BaseObservable *> obsVar(vm, -1);
      FRPDBG("@#@ %p (%s)->subscribeWatcher(%p | %p)", obsVar.value, "#constant", this, (IStateWatcher *)this);
      obsVar.value->subscribeWatcher(this);
      ComputedSource &src = sources.push_back();
      src.varName = "#constant";
      src.observable = obsVar.value;
      sq_pop(vm, 1); // constant
    }
  }
}


int ComputedValue::collectDependencies(DependenciesQueue &out_queue, int depth)
{
  int maxDepth = ++depth;
  if (eastl::find(out_queue.begin(), out_queue.end(), this) != out_queue.end())
    return maxDepth;
  for (IStateWatcher *w : watchers)
    maxDepth = max(maxDepth, w->collectDependencies(out_queue, depth));
  if (eastl::find(out_queue.begin(), out_queue.end(), this) == out_queue.end())
    out_queue.push_back(this);
  return maxDepth;
}


bool ComputedValue::onTriggerBegin()
{
  if (isInTrigger)
  {
    String info;
    fillInfo(info);
    String msg(0, "Computed: already in trigger - probably side effect in Computed\n%s", info.c_str());
    logerr_graph_error(graph->vm, msg);
    return false;
  }
  isInTrigger = true;
  return true;
}


bool ComputedValue::onSourceObservableChanged()
{
  G_ASSERT(isStateValid);
  needRecalc = true;
  return true;
}


bool ComputedValue::onTriggerEnd()
{
  bool ok = true;

  if (needRecalc)
  {
    ok = recalculate();
  }

  needRecalc = false;
  isInTrigger = false;

  return ok;
}


bool ComputedValue::recalculate()
{
  Sqrat::Object newVal;
  bool callSucceeded = false;
  if (funcAcceptsCurVal)
    callSucceeded = func.Evaluate(value, newVal);
  else
    callSucceeded = func.Evaluate(newVal);

  if (!callSucceeded)
    return false;

  timeChangeReq = ::get_time_msec();

  HSQUIRRELVM vm = func.GetVM();
  SQInteger cmpRes = 0;
  const HSQOBJECT &objOld = value.GetObject();
  const HSQOBJECT &objNew = newVal.GetObject();
  bool compared = sq_direct_cmp(vm, &objOld, &objNew, &cmpRes);
  bool changed = !compared || cmpRes != 0;
  if (!changed)
    return true;

  reportNestedObservables(vm, newVal);

  value = newVal;
  value.FreezeSelf();
  timeChanged = timeChangeReq;
  VT_TRACE_SINGLE(this, value.GetObject(), vm);

  bool ok = invalidateWatchers();

  graph->notifyQueueCache.add(this);

  return ok;
}


bool ComputedValue::invalidateWatchers()
{
  bool ok = true;
  G_ASSERT(!isIteratingWatchers);
  isIteratingWatchers = true;
  for (IStateWatcher *w : watchers)
  {
    G_ASSERT(w);
    if (!w->onSourceObservableChanged())
      ok = false;
  }
  isIteratingWatchers = false;
  return ok;
}


void ComputedValue::onGraphShutdown(bool exiting, Tab<Sqrat::Object> &cleared_storage)
{
  FRPDBG("@#@ ComputedValue(%p)::onGraphShutdown::onGraphShutdown(exiting = %d)", this, exiting);

  if (exiting)
    isShuttingDown = true;

  // protect observable from being destructed by zeroing refcount during
  // release of script subscribers
  HSQUIRRELVM vm = graph->vm;
  HSQOBJECT hInstance = get_observable_instance<ComputedValue>(vm, this);
  FRPDBG("@#@ script instance is %s", sq_isnull(hInstance) ? "NULL" : "NOT NULL");
  sq_addref(vm, &hInstance);

  if (exiting)
  {
    cleared_storage.push_back(Sqrat::Object(func.GetFunc(), graph->vm));
    func.Release();
  }

  ScriptValueObservable::onGraphShutdown(exiting, cleared_storage);

  sq_release(vm, &hInstance);
}


void ComputedValue::onObservableRelease(BaseObservable *observable)
{
  FRPDBG("@#@ ComputedValue(%p | %p)::onObservableRelease(%p)", this, (IStateWatcher *)this, observable);

  isStateValid = false;

  String sourceName(framemem_ptr());
  for (int i = 0, n = sources.size(); i < n; ++i)
  {
    if (sources[i].observable == observable)
    {
      if (!isShuttingDown)
        sourceName = sources[i].varName; // for diagnostics
      erase_items(sources, i, 1);
      break;
    }
  }

  if (!isShuttingDown)
  {
    logerr("[FRP] Computed %s (%s:%d) source Observable '%s' was released too early", initInfo.initFuncName.c_str(),
      initInfo.initSourceFileName.c_str(), int(initInfo.initSourceLine), sourceName.empty() ? "<N/A>" : sourceName.c_str());
  }
}

SQInteger ComputedValue::get_sources(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<ComputedValue *>(vm))
    return SQ_ERROR;
  Sqrat::Var<ComputedValue *> self(vm, 1);
  Sqrat::Table res(vm);
  for (ComputedSource &src : self.value->sources)
    res.SetValue(src.varName.c_str(), src.observable);
  sq_pushobject(vm, res);
  return 1;
}


SQInteger ComputedValue::script_ctor(HSQUIRRELVM vm)
{
  if (sq_gettop(vm) < 2 || sq_gettype(vm, 2) != OT_CLOSURE)
    return sq_throwerror(vm, "Computed object requires closure as argument");

  SQInteger nparams = 0, nfreevars = 0;
  G_VERIFY(SQ_SUCCEEDED(sq_getclosureinfo(vm, 2, &nparams, &nfreevars)));
  if (nparams != 1 && nparams != 2)
    return sqstd_throwerrorf(vm, "Computed function must accept 1 or 2 arguments, but has %d", nparams);

  bool checkNestedObservable = true;
  if (sq_gettop(vm) > 2)
  {
    SQUserPointer up;
    if (SQ_FAILED(sq_getuserpointer(vm, 3, &up)) || up != dont_check_nested)
      return sq_throwerror(vm, "Invalid argument at pos 3");
    checkNestedObservable = false;
  }

  Sqrat::Object initialValue;
  HSQOBJECT hFunc;
  sq_getstackobj(vm, 2, &hFunc);
  Sqrat::Function func(vm, Sqrat::Object(vm), hFunc);

  bool initialCalcOk = false;
  if (nparams == 1)
    initialCalcOk = func.Evaluate(initialValue);
  else
  {
    HSQOBJECT hInitialNone;
    sq_resetobject(&hInitialNone);
    hInitialNone._type = OT_USERPOINTER;
    hInitialNone._unVal.pUserPointer = computed_initial_update;
    Sqrat::Object initialNone(hInitialNone, vm);
    initialCalcOk = func.Evaluate(initialNone, initialValue);
  }
  if (!initialCalcOk)
    return sq_throwerror(vm, "Failed to calculate initial computed value - error in function");

  ObservablesGraph *graph = ObservablesGraph::get_from_vm(vm);
  if (checkNestedObservable && graph->checkNestedObservable)
  {
    String errMsg, path;
    if (has_script_observable_references(initialValue, errMsg, path))
    {
      SqStackChecker check(vm);
      const char *stack = nullptr;
      if (SQ_SUCCEEDED(sqstd_formatcallstackstring(vm)))
        sq_getstring(vm, -1, &stack);

      logerr("[FRP] %s [C]\npath = %s\n%s", errMsg.c_str(), path.c_str(), stack ? stack : "<no stack>");
      if (stack)
        sq_pop(vm, 1);
      checkNestedObservable = false; // report once per run
    }
  }

  ComputedValue *inst = new ComputedValue(func, initialValue, nparams == 2);
  inst->checkNestedObservable = checkNestedObservable;
  Sqrat::ClassType<ComputedValue>::SetManagedInstance(vm, 1, inst);
  return 0;
}


SQInteger forbid_computed_modify(HSQUIRRELVM vm) { return sq_throwerror(vm, "Can't modify computed observable"); }


SQInteger ScriptValueObservable::_tostring(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<ScriptValueObservable *>(vm))
    return SQ_ERROR;

  Sqrat::Var<ScriptValueObservable *> varSelf(vm, 1);
  ScriptValueObservable *self = varSelf.value;
  sq_pushobject(vm, self->value);

  String s(0, "Observable (0x%p) %s | %d watchers, %d subscribers", self, self->initInfo.initFuncName.c_str(), self->watchers.size(),
    self->scriptSubscribers.size());
  if (SQ_SUCCEEDED(sq_tostring(vm, -1)))
  {
    const char *valStr = nullptr;
    G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, -1, &valStr)));
    s.aprintf(0, ", value = %s", valStr);
    sq_pop(vm, 1);
  }
  sq_pop(vm, 1);
  sq_pushstring(vm, s, s.length());
  return 1;
}


SQInteger ComputedValue::_tostring(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<ComputedValue *>(vm))
    return SQ_ERROR;

  Sqrat::Var<ComputedValue *> varSelf(vm, 1);
  ComputedValue *self = varSelf.value;
  sq_pushobject(vm, self->value);

  String s(0, "ComputedValue (0x%p) %s | %d watchers, %d subscribers, %d sources", self, self->initInfo.initFuncName.c_str(),
    self->watchers.size(), self->scriptSubscribers.size(), self->sources.size());
  if (SQ_SUCCEEDED(sq_tostring(vm, -1)))
  {
    const char *valStr = nullptr;
    G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, -1, &valStr)));
    s.aprintf(0, ", value = %s", valStr);
    sq_pop(vm, 1);
  }
  sq_pop(vm, 1);
  sq_pushstring(vm, s, s.length());
  return 1;
}

static SQInteger set_nested_observable_debug(HSQUIRRELVM vm)
{
  ObservablesGraph *graph = ObservablesGraph::get_from_vm(vm);
  SQBool val;
  sq_getbool(vm, 2, &val);
  graph->checkNestedObservable = val;
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


void bind_frp_classes(SqModules *module_mgr)
{
  HSQUIRRELVM vm = module_mgr->getVM();

  Sqrat::Class<BaseObservable, Sqrat::NoConstructor<BaseObservable>> baseObservable(vm, "BaseObservable");
  baseObservable.SquirrelFunc("subscribe", &BaseObservable::subscribe, 2, "xc")
    .SquirrelFunc("unsubscribe", &BaseObservable::unsubscribe, 2, "xc")
    .SquirrelFunc("dbgGetListeners", &BaseObservable::dbgGetListeners, 1, "x");

  ///@class frp/Watched
  Sqrat::DerivedClass<ScriptValueObservable, BaseObservable> scriptValueObservable(vm, "ScriptValueObservable");
  scriptValueObservable.SquirrelCtor(ScriptValueObservable::script_ctor, -1)
    .Prop("value", &ScriptValueObservable::getValue)
    .Func("get", &ScriptValueObservable::getValue)
    .Prop("timeChangeReq", &ScriptValueObservable::getTimeChangeReq)
    .Prop("timeChanged", &ScriptValueObservable::getTimeChanged)
    .SquirrelFunc("trigger", &ScriptValueObservable::sqTrigger, 1, "x")
    .Func("trace", &ScriptValueObservable::trace)
    .Func("whiteListMutatorClosure", &ScriptValueObservable::whiteListMutatorClosure)
    .SquirrelFunc("update", &ScriptValueObservable::updateViaMethod, 2, "x")
    .SquirrelFunc("set", &ScriptValueObservable::updateViaMethod, 2, "x")
    .SquirrelFunc("modify", &ScriptValueObservable::modify, 2, "xc")
    .SquirrelFunc("_call", &ScriptValueObservable::updateViaCallMm, 3, "x")
    .SquirrelFunc("mutate", &ScriptValueObservable::mutate, 2, "xc")
    .SquirrelFunc("_newslot", &ScriptValueObservable::_newslot, 3, "x")
    .SquirrelFunc("_delslot", &ScriptValueObservable::_delslot, 2, "x");

  ///@class frp/Computed
  ///@extends Watched
  Sqrat::DerivedClass<ComputedValue, ScriptValueObservable> sqComputedValue(vm, "ComputedValue");
  sqComputedValue.SquirrelCtor(ComputedValue::script_ctor, -2)
    .SquirrelFunc("update", forbid_computed_modify, 0)
    .SquirrelFunc("mutate", forbid_computed_modify, 0)
    .SquirrelFunc("modify", forbid_computed_modify, 0)
    .SquirrelFunc("_call", forbid_computed_modify, 0)
    .SquirrelFunc("_set", forbid_computed_modify, 0)
    .SquirrelFunc("_newslot", forbid_computed_modify, 0)
    .SquirrelFunc("_delslot", forbid_computed_modify, 0)
    .SquirrelFunc(
      "trigger", [](HSQUIRRELVM vm) { return sq_throwerror(vm, "Triggering computed observable is not allowed"); }, 0)
    .SquirrelFunc("getSources", ComputedValue::get_sources, 1, "x");


  Sqrat::Table exports(vm);
  exports.Bind("Watched", scriptValueObservable);
  exports.Bind("Computed", sqComputedValue);
  ///@module frp
  exports.SquirrelFunc("set_nested_observable_debug", set_nested_observable_debug, 2, ".b")
    .SquirrelFunc("make_all_observables_immutable", make_all_observables_immutable, 2, ".b");

  sq_pushobject(vm, exports);
  {
    ///@const FRP_INITIAL
    sq_pushstring(vm, "FRP_INITIAL", -1);
    sq_pushuserpointer(vm, computed_initial_update);
    G_VERIFY(SQ_SUCCEEDED(sq_rawset(vm, -3)));

    ///@const FRP_DONT_CHECK_NESTED
    sq_pushstring(vm, "FRP_DONT_CHECK_NESTED", -1);
    sq_pushuserpointer(vm, dont_check_nested);
    G_VERIFY(SQ_SUCCEEDED(sq_rawset(vm, -3)));
  }
  sq_poptop(vm);

  module_mgr->addNativeModule("frp", exports);
}


} // namespace sqfrp
