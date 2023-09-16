#include <sqDebugger/scriptProfiler.h>
#include <perfMon/dag_statDrv.h>
#include <util/dag_hashedKeyMap.h>
#include <dag/dag_vector.h>

#include <stdio.h>
#include <new>

#include <util/dag_string.h>
#include <debug/dag_fatal.h>
#include <debug/dag_debug.h>
#include <debug/dag_log.h>
#include <debug/dag_logSys.h>
#include <perfMon/dag_perfTimer.h>
#include <generic/dag_tab.h>
#include <osApiWrappers/dag_files.h>
#include <assert.h>

#include <squirrel/sqvm.h>
#include <squirrel/sqstate.h>
#include <squirrel/sqstring.h>
#include <squirrel/sqtable.h>
#include <squirrel/sqarray.h>
#include <squirrel/sqfuncproto.h>
#include <squirrel/sqclosure.h>
#include <squirrel/sqclass.h>

#include <quirrel/sqModules/sqModules.h>
#include <quirrel/sqStackChecker.h>

#include <util/dag_stlqsort.h>
#include <memory/dag_framemem.h>
#include <EASTL/hash_map.h>


namespace scriptprofile
{
struct CallFrame;
class FuncProfile;
class PerfProfile;
} // namespace scriptprofile

struct FuncId;

struct scriptprofile::CallFrame
{
#if DA_PROFILER_ENABLED
  da_profiler::ScopedEvent dapEvent;
#endif
  CallFrame(int _id, int64_t _start, FuncId *_func_ptr, bool _native, uint32_t token) :
    id(_id),
    start(_start),
    calleeTime(0),
    funcPtr(_func_ptr),
    native(_native)
#if DA_PROFILER_ENABLED
    ,
    dapEvent(token)
#endif
  {
    G_UNUSED(token);
  }

  int64_t start = 0;
  int64_t calleeTime = 0;
  FuncId *funcPtr = nullptr;
  int id = -1;
  bool native = false;
};

static dag::Vector<scriptprofile::CallFrame> callStack;


FuncId *get_func_id(const HSQOBJECT &func)
{
  G_ASSERT(sq_type(func) == OT_CLOSURE || sq_type(func) == OT_NATIVECLOSURE);
  if (sq_type(func) == OT_CLOSURE)
    return (FuncId *)func._unVal.pClosure->_function;
  else
    return (FuncId *)func._unVal.pNativeClosure;
}


class scriptprofile::FuncProfile
{
public:
  void Set(int _id, const char *_name, FuncId *_func_ptr, bool _bNative, const char *source_name, int source_line)
  {
    id = _id;
    funcName = _name;
    funcPtr = _func_ptr;
    bNative = _bNative;
    sourceName = source_name;
    sourceLine = source_line;

#if DA_PROFILER_ENABLED
    {
      String dapFuncName(framemem_ptr());
      if (strchr(_name, ':') || !source_name) // generated name or no source name
        dapFuncName = _name;
      else
      {
        G_ASSERT(_name);
        const char *fnSlash = ::max(strrchr(source_name, '/'), strrchr(source_name, _SC('\\')));
        dapFuncName.printf(0, "%s(%s:%d)", _name, fnSlash ? (fnSlash + 1) : source_name, source_line);
      }
      dapDescription = DA_PROFILE_ADD_DESCRIPTION(sourceName, source_line, dapFuncName.c_str());
    }
#endif
  }

public:
  long long allTime = 0;
  long long selfTime = 0;
  FuncId *funcPtr = nullptr;
  const char *sourceName = nullptr;
  int callCount = 0;
  int sourceLine = 0;
  String funcName;
  int id = 0;
#if DA_PROFILER_ENABLED
  uint32_t dapDescription = 0;
  uint32_t getDapToken() const { return dapDescription; }
#else
  uint32_t getDapToken() const { return 0; }
#endif
  bool bNative = false;
};


static void sq_profiler_debug_hook(HSQUIRRELVM v, SQInteger type, const SQChar *sourcename, SQInteger line, const SQChar *funcname)
{
  G_UNUSED(sourcename);
  G_UNUSED(line);
  G_UNUSED(funcname);

  switch (type)
  {
    case 'c':
      if (scriptprofile::isStarted())
      {
        SQInteger cssize = v->_callsstacksize;
        SQVM::CallInfo &ci = v->_callsstack[cssize - 1];
        scriptprofile::onEnterFunction(v, ci._closure);
      }
      return;

    case 'r':
      if (scriptprofile::isStarted())
        scriptprofile::onLeaveFunction(v);
      return;

    default: return;
  }
}


class scriptprofile::PerfProfile
{
public:
  PerfProfile() : data(midmem_ptr()) {}

  ~PerfProfile()
  {
#if DAGOR_DBGLEVEL > 0
    writeProfile();
#endif
  }

  void init(HSQUIRRELVM vm)
  {
    if (isStarted() && curVM && curVM != vm)
    {
      if (callStack.size())
        logerr("cant start of another VM profiling if one VM profiling is still running");
      else
      {
        stop(curVM);
        actualStop();
      }
    }

    curVM = vm;

    ptrToIndex.clear();
    clear_and_shrink(data);

    // add dummy for unknown functions
    FuncProfile &prof = data.push_back();
    prof.Set(0, "unknown", nullptr, false, nullptr, -1); //-V522

    sq_pushroottable(vm);
    Tab<SQTable *> nodes(framemem_ptr());
    recurse_collect_global_functions(vm, String(), nodes);
    sq_pop(vm, 1);

    callStack.clear();
    callStack.reserve(64);
  }

  void shutdown(HSQUIRRELVM vm)
  {
    if (vm != curVM)
      return;
    if (isStarted())
    {
      stop(vm);
      actualStop();
      writeProfile();
    }

    ptrToIndex.clear();
    clear_and_shrink(data);
    callStack.clear();
  }

  void start(HSQUIRRELVM vm)
  {

    if (isStarted())
    {
      shouldStop = false;
      return;
    }
    if (vm != curVM)
      init(vm);

    if (!vm->_debughook_native)
      sq_setnativedebughook(vm, sq_profiler_debug_hook);

    bStarted = true;
    shouldStop = false;
    startMsec = get_time_msec();
  }

  bool isStarted() const { return bStarted; }

  void stop(HSQUIRRELVM vm)
  {
    if (curVM && vm != curVM)
    {
      logerr("Trying to stop squirrel profiler started from another VM");
      return;
    }
    shouldStop = true;
  }

  struct SortCmp
  {
    bool operator()(const scriptprofile::FuncProfile *a, const scriptprofile::FuncProfile *b) const { return a->allTime > b->allTime; }
  };

  void mergeFunctionsData()
  {
    int dataCnt = data.size();
    for (int i = 0; i < dataCnt; i++)
    {
      FuncProfile &func = data[i];
      if (!func.allTime)
        continue;

      for (int j = i + 1; j < dataCnt; j++)
      {
        FuncProfile &other = data[j];
        if (other.allTime && func.funcName == other.funcName)
        {
          func.allTime += other.allTime;
          func.selfTime += other.selfTime;
          func.callCount += other.callCount;
          other.allTime = 0;
          other.selfTime = 0;
          other.callCount = 0;
        }
      }
    }
  }

  bool getProfileResult(String &res)
  {
    clear_and_shrink(res);
    mergeFunctionsData();

    int64_t all_time = 0;
    int maxNameLen = 0;
    for (const FuncProfile &fp : data)
    {
      if (!fp.allTime)
        continue;
      all_time += fp.selfTime;
      maxNameLen = ::max(maxNameLen, fp.funcName.length());
    }
    if (!all_time)
      return false;

    maxNameLen = ::min(maxNameLen, 60);

    res.reserve(16384);
    res.aprintf(512, "type;name;time_all(ms);time_self(ms);time_all(%%);"
                     "time_self(%%);call_count;avg_time_all(ms);avg_time_self(ms);relative_all(%%);relative_self(%%);source\n");

    int dataCnt = data.size();
    Tab<const scriptprofile::FuncProfile *> data_ptrs(tmpmem_ptr());
    data_ptrs.resize(data.size());
    for (int i = 0; i < data.size(); ++i)
      data_ptrs[i] = &data[i];
    stlsort::sort(data_ptrs.data(), data_ptrs.data() + dataCnt, SortCmp());

    int testTimeMsec = max(stopMsec - startMsec, 1);

    char strName[1024] = {0}, formatStr[16] = {0};
    _snprintf(formatStr, sizeof(formatStr), "%%%ds", maxNameLen);
    formatStr[sizeof(formatStr) - 1] = 0;
    for (int i = 0; i < dataCnt; ++i)
    {
      const FuncProfile &fp = *data_ptrs[i];
      if (!fp.allTime)
        break;
      const int64_t allTime = profile_usec_from_ticks_delta(fp.allTime), selfTime = profile_usec_from_ticks_delta(fp.selfTime);
      int64_t call_time_all = (fp.callCount == 0) ? 0 : (allTime / fp.callCount);
      int64_t call_time_self = (fp.callCount == 0) ? 0 : (selfTime / fp.callCount);
      _snprintf(strName, sizeof(strName), formatStr, fp.funcName.str());
      strName[sizeof(strName) - 1] = 0;

      int allTimePct = int((fp.allTime / (double)all_time) * 10000.0f);
      int selfTimePct = int((selfTime / (double)all_time) * 10000.0f);
      double allRelativePct = (allTime / (double)testTimeMsec) * 100.0f / 1000.0;   // usec->msec
      double selfRelativePct = (selfTime / (double)testTimeMsec) * 100.0f / 1000.0; // usec->msec

      res.aprintf(512, "%s;%s;%4d.%03d;%4d.%03d;%3d.%02d%%;%3d.%02d%%;%5d;%4d.%03d;%4d.%03d;%9.4f%%;%9.4f%%; %s:%d\n",
        fp.bNative ? "native" : "script", strName, int(allTime) / 1000, int(allTime) % 1000, int(selfTime) / 1000,
        int(selfTime) % 1000, allTimePct / 100, allTimePct % 100, selfTimePct / 100, selfTimePct % 100, fp.callCount,
        int(call_time_all) / 1000, int(call_time_all) % 1000, int(call_time_self) / 1000, int(call_time_self) % 1000, allRelativePct,
        selfRelativePct, fp.sourceName ? fp.sourceName : "---", fp.sourceLine);
    }
    res.aprintf(0, "Time profiled: %d msec", testTimeMsec);
    return true;
  }

  void writeProfile()
  {
    String res;
    if (getProfileResult(res))
      debug("Squirrel Profiler Dump:\n%s\nEnd of Dump", res.str());
  }

  void saveProfileToFile(const char *fileName)
  {
    String res;
    if (!getProfileResult(res))
      return;

    String filePath(256, "%s/%s", ::get_log_directory(), fileName);

    file_ptr_t pProfFile = df_open(filePath.c_str(), DF_WRITE | DF_CREATE);
    if (!pProfFile)
      return;

    df_write(pProfFile, res.c_str(), res.length());
    df_close(pProfFile);
  }

  void resetValues()
  {
    if (!curVM)
      return;

    if (isStarted())
      startMsec = get_time_msec();

    for (FuncProfile &fp : data)
    {
      fp.allTime = fp.selfTime = 0;
      fp.callCount = 0;
    }
  }

  int64_t getTotalTime()
  {
    int64_t totalTime = 0;
    for (const FuncProfile &fp : data)
      totalTime += fp.selfTime;
    return profile_usec_from_ticks_delta(totalTime);
  }

  void addFunction(const char *name, FuncId *func_ptr, bool bNative, const char *source_name, int source_line)
  {
    FuncProfile *prof = nullptr;
    int newId = 0;

    for (FuncProfile &fp : data)
    {
      if (fp.funcPtr == func_ptr)
      {
        G_ASSERT(fp.bNative == bNative);
        G_ASSERT(bNative || (strcmp(fp.sourceName, source_name) == 0));
        newId = fp.id;
        prof = &fp;
        break; // is it ok to overwrite this function's data in the following code?
      }
    }

    if (!prof)
    {
      data.push_back(FuncProfile());
      prof = &data.back();
      newId = int(data.size() - 1);
    }

    prof->Set(newId, name, func_ptr, bNative, source_name, source_line); //-V522
    ptrToIndex.emplace_new(func_ptr, newId);
  }


  void get_closure_source_info(const SQFunctionProto *proto, const char **fn, int *line)
  {
    *fn = sq_type(proto->_sourcename) == OT_STRING ? _stringval(proto->_sourcename) : nullptr;
    *line = proto->_lineinfos[0]._line;
  }

  void recurse_collect_global_functions(HSQUIRRELVM vm, const String &base, Tab<SQTable *> &ptableNodes)
  {
    HSQOBJECT mainHandle;
    sq_getstackobj(vm, -1, &mainHandle);
    G_ASSERT(mainHandle._type == OT_TABLE || mainHandle._type == OT_ARRAY);
    if (find_value_idx(ptableNodes, mainHandle._unVal.pTable) >= 0)
      return;
    ptableNodes.push_back(mainHandle._unVal.pTable);

    SqStackChecker stackCheck(vm);

    sq_pushnull(vm);
    while (SQ_SUCCEEDED(sq_next(vm, -2)))
    {
      int rawtype = _RAW_TYPE(sq_gettype(vm, -1));
      const char *name = nullptr;
      sq_getstring(vm, -2, &name);

      String fullname(base + name);

      if (rawtype == _RT_CLASS)
      {
        HSQOBJECT classHandle;
        sq_getstackobj(vm, -1, &classHandle);

        sq_pushnull(vm);
        while (SQ_SUCCEEDED(sq_next(vm, -2)))
        {
          SQObjectType tp = sq_gettype(vm, -1);
          if (tp == OT_CLOSURE || tp == OT_NATIVECLOSURE)
          {
            HSQOBJECT handle;
            sq_getstackobj(vm, -1, &handle);
            bool bNative = (tp == OT_NATIVECLOSURE);
            FuncId *func = get_func_id(handle);
            const char *sourceName = nullptr;
            int sourceLine = -1;
            if (!bNative)
              get_closure_source_info(handle._unVal.pClosure->_function, &sourceName, &sourceLine);

            bool parentOnly = false; //<< ????
            if (classHandle._unVal.pClass->_base)
            {
              const SQClassMemberVec &methods = classHandle._unVal.pClass->_base->_methods;
              for (int i = 0; i < methods.size(); i++)
              {
                const SQObjectPtr &method = methods[i].val;
                if (!sq_isclosure(method) && !sq_isnativeclosure(method))
                  continue;
                if (get_func_id(method) == func)
                {
                  parentOnly = true;
                  break;
                }
              }
            }

            if (!parentOnly)
            {
              const char *name2 = nullptr;
              sq_getstring(vm, -2, &name2);
              addFunction(fullname + "." + name2, func, bNative, sourceName, sourceLine);
            }
          }

          //
          sq_pop(vm, 2); // pops key and val before the nex iteration
        }
        sq_pop(vm, 1);
      }
      else if (rawtype == _RT_CLOSURE)
      {
        HSQOBJECT closureHandle;
        sq_getstackobj(vm, -1, &closureHandle);
        const char *sourceName = nullptr;
        int sourceLine = -1;
        get_closure_source_info(closureHandle._unVal.pClosure->_function, &sourceName, &sourceLine);
        addFunction(fullname, get_func_id(closureHandle), false, sourceName, sourceLine);
      }
      else if (rawtype == _RT_NATIVECLOSURE)
      {
        HSQOBJECT nativeClosureHandle;
        sq_getstackobj(vm, -1, &nativeClosureHandle);
        addFunction(fullname, get_func_id(nativeClosureHandle), true, nullptr, -1);
      }
      else if ((rawtype == _RT_TABLE) || (rawtype == _RT_ARRAY))
      {
        const char *tablename;
        if (SQ_FAILED(sq_getstring(vm, -2, &tablename)))
          ; // debug("[SQ CLASS INFO] skipping table '%s' field with non-string index", base.c_str());
        else
          recurse_collect_global_functions(vm, base + tablename + ".", ptableNodes);
      }

      sq_pop(vm, 2); // pops key and val before the nex iteration
    }
    sq_pop(vm, 1); // pops the null iterator

    ptableNodes.pop_back();
  }


  FuncProfile *findProfileFuncDecs(FuncId *closure)
  {
    auto it = ptrToIndex.findVal(closure);
    if (it != nullptr)
      return &(data[*it]);
    else
      return nullptr;
  }

  FuncProfile *getProfileFuncDecs(FuncId *closure, bool native)
  {
    if (!isStarted())
      return nullptr;

    FuncProfile *pFunc = findProfileFuncDecs(closure);
    if (pFunc)
      return pFunc;

    if (!native)
    {
      SQFunctionProto *proto = (SQFunctionProto *)closure; //====
      if (sq_type(proto->_name) == OT_STRING)
      {
        G_STATIC_ASSERT(sizeof(SQChar) == 1);

        const char *sourceName = nullptr;
        int sourceLine = -1;
        get_closure_source_info(proto, &sourceName, &sourceLine);
        addFunction(_stringval(proto->_name), closure, native, sourceName, sourceLine);
        FuncProfile *pFunc = findProfileFuncDecs(closure);
        if (pFunc)
          return pFunc;
      }
    }

    return &data[0]; // 'unknown'
  }

  void enterFunc(const HSQOBJECT &func)
  {
    if (!isStarted())
      return;

    FuncId *funcPtr = get_func_id(func);
    bool isNative = (func._type == OT_NATIVECLOSURE);

    FuncProfile *pFunc = getProfileFuncDecs(funcPtr, isNative);
    G_ASSERT(pFunc);

    pFunc->callCount++; //-V522

    int actual_id = pFunc->id;
    if (actual_id == 0)
    {
      // G_ASSERT(!callStack.empty() || !bNative);

      if (!callStack.empty())
      {
        actual_id = callStack.back().id; //-V522
      }
    }

    callStack.emplace_back(CallFrame(actual_id, ::profile_ref_ticks(), funcPtr, isNative, pFunc->getDapToken()));
  }

  void leaveFunc()
  {
    if (callStack.empty())
      return;

    CallFrame &pFrame = callStack.back();
    G_ASSERT(pFrame.id >= 0 && pFrame.id < data.size());
    FuncProfile &func = data[pFrame.id]; //-V522

    int64_t delta = ::profile_ref_ticks() - pFrame.start;
    func.allTime += delta;
    func.selfTime += (delta - pFrame.calleeTime);

    callStack.pop_back();

    if (!callStack.empty())
      callStack.back().calleeTime += delta; //-V522
    else if (isDetectingSlowCalls())
    {
      if (func.allTime > slowCallThresholdTicks)
      {
        debug("Script call with duration of %d msec detected", profile_usec_from_ticks_delta(func.allTime) / 1000);
        writeProfile();
      }
      for (FuncProfile &fp : data)
      {
        fp.allTime = fp.selfTime = 0;
        fp.callCount = 0;
      }
    }
    if (callStack.empty() && shouldStop)
      actualStop();
  }
  void actualStop()
  {
    stopMsec = get_time_msec();
    if (curVM)
      sq_setnativedebughook(curVM, nullptr);
    curVM = nullptr;

    bStarted = false;
    shouldStop = false;
  }

  bool isDetectingSlowCalls() { return slowCallThresholdTicks > 0; }

  HashedKeyMap<FuncId *, int, nullptr, oa_hashmap_util::MumStepHash<FuncId *>> ptrToIndex;

  Tab<FuncProfile> data;

  HSQUIRRELVM curVM = nullptr;

  bool bStarted = false, shouldStop = false;
  int startMsec = 0;
  int stopMsec = 0;
  int64_t slowCallThresholdTicks = 0;
};

static scriptprofile::PerfProfile g_PerfProfile;

// scriptprofile functions
#if DAGOR_DBGLEVEL > 0

void scriptprofile::initProfile(HSQUIRRELVM vm) { g_PerfProfile.init(vm); }

bool scriptprofile::getProfileResult(String &res) { return g_PerfProfile.getProfileResult(res); }

void scriptprofile::start(HSQUIRRELVM vm) { g_PerfProfile.start(vm); }

void scriptprofile::stop(HSQUIRRELVM vm) { g_PerfProfile.stop(vm); }

bool scriptprofile::isStarted() { return g_PerfProfile.isStarted(); }

void scriptprofile::onEnterFunction(HSQUIRRELVM vm, const HSQOBJECT &func)
{
  if (vm == g_PerfProfile.curVM)
    g_PerfProfile.enterFunc(func);
}

void scriptprofile::onLeaveFunction(HSQUIRRELVM vm)
{
  if (vm == g_PerfProfile.curVM)
    g_PerfProfile.leaveFunc();
}


static SQInteger start_profiler_sq(HSQUIRRELVM vm)
{
  scriptprofile::initProfile(vm);
  scriptprofile::start(vm);
  return 0;
}

static SQInteger stop_profiler_sq(HSQUIRRELVM vm)
{
  scriptprofile::stop(vm);
  g_PerfProfile.writeProfile();
  return 0;
}

static SQInteger stop_and_save_to_file_sq(HSQUIRRELVM vm)
{
  scriptprofile::stop(vm);

  const char *fn;
  sq_getstring(vm, 2, &fn);
  g_PerfProfile.saveProfileToFile(fn);
  return 0;
}

static void dump_profiler()
{
  bool isStarted = g_PerfProfile.isStarted();
  if (isStarted)
    scriptprofile::stop(g_PerfProfile.curVM);
  g_PerfProfile.writeProfile();
  if (isStarted)
    scriptprofile::start(g_PerfProfile.curVM);
}

static void reset_values() { g_PerfProfile.resetValues(); }

static int64_t get_total_time() { return g_PerfProfile.getTotalTime(); }

static void detect_slow_calls(int msec_threshold)
{
  g_PerfProfile.slowCallThresholdTicks = profile_ticks_frequency() * 1000 / (msec_threshold);
}


void scriptprofile::register_profiler_module(HSQUIRRELVM vm, SqModules *module_mgr)
{
  Sqrat::Table exports(vm);

  ///@module dagor.profiler
  exports.Func("dump", dump_profiler)
    .Func("reset_values", reset_values)
    .Func("get_total_time", get_total_time)
    .Func("detect_slow_calls", detect_slow_calls)
    .SquirrelFunc("start", start_profiler_sq, 1)
    .SquirrelFunc("stop", stop_profiler_sq, 1)
    .SquirrelFunc("stop_and_save_to_file", stop_and_save_to_file_sq, 2, ".s");

  module_mgr->addNativeModule("dagor.profiler", exports);
}


void scriptprofile::shutdown(HSQUIRRELVM vm) { g_PerfProfile.shutdown(vm); }


#endif
