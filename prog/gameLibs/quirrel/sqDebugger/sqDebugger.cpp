// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sqDebugger/sqDebugger.h>
#include <sqDebugger/scriptProfiler.h>
#include <sqModules/sqModules.h>
#include <squirrel/sqpcheader.h>
#include <squirrel/squtils.h>
#include <squirrel/sqstate.h>
#include <squirrel/sqobject.h>
#include <squirrel/sqvm.h>
#include <squirrel/sqtable.h>
#include <squirrel/sqclass.h>
#include <osApiWrappers/dag_miscApi.h>


void DagSqDebuggers::initDebugger(int index, SqModules *module_mgr, const char *description)
{
  G_ASSERT(index >= 0 && index < MAX_SQ_DEBUGGERS);

  //  temporarily removed
  //  until we decide what to do with the scripts reloading in WT
  //
  //  G_ASSERTF(debuggers[index].vm == NULL || debuggers[index].vm, "initDebugger: sq_debugger with index %d is already registered",
  //  index);

  debuggers[index].init(module_mgr, description);
}

void DagSqDebuggers::shutdownDebugger(int index)
{
  G_ASSERT(index >= 0 && index < MAX_SQ_DEBUGGERS);

  debuggers[index].shutdown();
}


DagSqDebugger &DagSqDebuggers::get(HSQUIRRELVM vm)
{
  for (int i = 0; i < MAX_SQ_DEBUGGERS; i++)
    if (vm == debuggers[i].vm)
      return debuggers[i];

  G_ASSERTF(0, "Debugger for this VM not found. Call initDebugger() first.");

  return debuggers[0];
}

DagSqDebugger &DagSqDebuggers::get(int index)
{
  G_ASSERT(index >= 0 && index < MAX_SQ_DEBUGGERS);
  return debuggers[index];
}

bool DagSqDebuggers::isVmRegistered(HSQUIRRELVM vm)
{
  G_ASSERT(vm);
  for (int i = 0; i < MAX_SQ_DEBUGGERS; i++)
    if (vm == debuggers[i].vm)
      return true;

  return false;
}


const char *DagSqDebuggers::getDescription(HSQUIRRELVM vm)
{
  for (int i = 0; i < MAX_SQ_DEBUGGERS; i++)
    if (vm == debuggers[i].vm)
      return debuggers[i].description.c_str();

  G_ASSERTF(0, "Description for this debugger not found. Call initDebugger() first.");

  return "";
}

const char *DagSqDebuggers::getDescription(int index)
{
  G_ASSERT_RETURN(index >= 0 && index < MAX_SQ_DEBUGGERS, "Invalid debugger index");
  G_ASSERT_RETURN(debuggers[index].vm, "Debugger has no assigned VM and no description. Call initDebugger() first.");

  return debuggers[index].description.c_str();
}


DagSqDebuggers dag_sq_debuggers;


void SourceCodeCoverage::init(const char *filename_)
{
  sourcename.setStr(filename_);
  clear();
}

void SourceCodeCoverage::clear() { lineType.clear(); }

void SourceCodeCoverage::clearCoverage()
{
  for (char &t : lineType)
    if (t == SRC_LINE_EXECUTED)
      t = SRC_LINE_EXISTS;
}


void SourceCodeCoverage::onLineExists(int lineNum)
{
  if (lineNum >= lineType.size())
  {
    int prevCount = lineType.size();
    int newCount = lineNum + lineNum / 2 + 10;
    lineType.resize(newCount);
    for (int i = prevCount; i < newCount; i++)
      lineType[i] = SRC_LINE_INACTIVE;
  }

  lineType[lineNum] = SRC_LINE_EXISTS;
}

void SourceCodeCoverage::onLineExecuted(int lineNum)
{
  if (lineNum >= lineType.size())
    return;

  lineType[lineNum] = SRC_LINE_EXECUTED;
}

SourceCodeCoverageList::SourceCodeCoverageList() { clear(); }

void SourceCodeCoverageList::clear()
{
  sources.clear();
  sources.reserve(128);
  lastFileName.clear();
  lastIndex = 0;
}

void SourceCodeCoverageList::clearCoverage()
{
  for (SourceCodeCoverage &source : sources)
    source.clearCoverage();
  lastFileName.clear();
  lastIndex = 0;
}

void SourceCodeCoverageList::clearByIndex(int index)
{
  if (index < sources.size())
    sources[index].clear();
}

void SourceCodeCoverageList::onLineExecuted(const char *sourcename, int line)
{
  if (!sourcename || !*sourcename)
    return;

  if (lastFileName == sourcename)
    sources[lastIndex].onLineExecuted(line);
  else
  {
    for (int i = 0; i < sources.size(); i++)
      if (!strcmp(sources[i].sourcename.str(), sourcename))
      {
        lastIndex = i;
        lastFileName = sourcename;
        sources[lastIndex].onLineExecuted(line);
        return;
      }

    SourceCodeCoverage &a = sources.push_back();
    a.init(sourcename);
    lastIndex = sources.size() - 1;
    lastFileName = sourcename;
    sources[lastIndex].onLineExecuted(line);
  }
}

void SourceCodeCoverageList::onLineExists(const char *sourcename, int line)
{
  if (!sourcename || !*sourcename)
    return;

  if (lastFileName == sourcename)
    sources[lastIndex].onLineExists(line);
  else
  {
    for (int i = 0; i < sources.size(); i++)
      if (!strcmp(sources[i].sourcename.str(), sourcename))
      {
        lastIndex = i;
        lastFileName = sourcename;
        sources[lastIndex].onLineExists(line);
        return;
      }

    SourceCodeCoverage &a = sources.push_back();
    a.init(sourcename);
    lastIndex = sources.size() - 1;
    lastFileName = sourcename;
    sources[lastIndex].onLineExists(line);
  }
}


static void sq_debug_hook(HSQUIRRELVM v, SQInteger type, const SQChar *sourcename, SQInteger line, const SQChar *funcname)
{
  DagSqDebugger &debugger = dag_sq_debuggers.get(v);

  switch (type)
  {
    case 'l': debugger.onNewLineExecuted(sourcename, line); return;

    case 'c':
      if (scriptprofile::isStarted())
      {
        SQInteger cssize = v->_callsstacksize;
        SQVM::CallInfo &ci = v->_callsstack[cssize - 1];
        scriptprofile::onEnterFunction(v, ci._closure);
      }
      debugger.stackDepth++;
      return;

    case 'r':
      debugger.stackDepth--;
      if (scriptprofile::isStarted())
      {
        scriptprofile::onLeaveFunction(v);
      }
      return;

    case 'e':
      debugger.onNewLineExecuted(sourcename, line);
      debugger.runtimeErrorState = true;
      debugger.lastRuntimeError.setStr(funcname);
      debugger.onBreakpoint(sourcename, line);
      debugger.runtimeErrorState = false;
      return;

    default: return;
  }
}

static void compile_line_hook(HSQUIRRELVM v, const SQChar *sourcename, SQInteger line)
{
  DagSqDebugger &debugger = dag_sq_debuggers.get(v);
  debugger.onLineExists(sourcename, line);
}


void DagSqDebugger::breakFromNativeCode(const char *function_name)
{
  if (debugger_enabled)
    sq_debug_hook(vm, 'e', "", 0, function_name);
}


void DagSqDebugger::enable(bool enable)
{
  G_ASSERTF(areFunctionsRegistered, "Call DagSqDebugger::init() just after SQVM created.");

  G_ASSERT(vm);
  if (!vm)
    return;

  debugger_enabled = enable;
  sq_setnativedebughook(vm, enable ? sq_debug_hook : NULL);
  sq_notifyallexceptions(vm, true);

  if (!enable && suspended)
    run();
}

void DagSqDebugger::clearSourceList()
{
  sources_list.clear();
  sourceCodeCoverageList.clear();
}

void DagSqDebugger::addSource(HSQUIRRELVM vm, const char *sourcename)
{
  if (sourcename && sourcename[0] && (strcmp(sourcename, "console_buffer") != 0) && strcmp(sourcename, "unnamedbuffer") != 0 &&
      strcmp(sourcename, "interactive") != 0)
  {
    DagSqDebugger &debugger = dag_sq_debuggers.get(vm);
    int index = find_value_idx(debugger.sources_list, String(sourcename));
    if (index < 0)
      debugger.sources_list.push_back(String(sourcename));
    else
      debugger.sourceCodeCoverageList.clearByIndex(index);
  }
}


SQInteger DagSqDebugger::setObjPrintFunction(HSQUIRRELVM vm)
{
  DagSqDebugger &self = dag_sq_debuggers.get(vm);

  Sqrat::Var<Sqrat::Object> func(vm, 2);
  if (!func.value.IsNull())
    self.objPrintFunc = Sqrat::Function(vm, Sqrat::Object(vm), func.value);
  else
    self.objPrintFunc.Release();
  return 0;
}


void DagSqDebugger::init(SqModules *module_mgr, const char *description_)
{
  G_ASSERT(module_mgr);
  G_ASSERT(module_mgr->getVM());
  vm = module_mgr->getVM();
  description = description_;

  sq_set_compile_line_hook(vm, compile_line_hook);

  module_mgr->onCompileFile_cb = addSource;
  areFunctionsRegistered = true;

  Sqrat::Table exports(vm);
  exports.SquirrelFunc("setObjPrintFunc", setObjPrintFunction, 2, ".c");
  module_mgr->addNativeModule("sqdebugger", exports);
}


void DagSqDebugger::shutdown()
{
  if (vm)
    sq_setnativedebughook(vm, nullptr);

  objPrintFunc.Release();
  description.clear();
  vm = nullptr;
}


void DagSqDebugger::getSourceList(Tab<String> &output) { output = sources_list; }

bool DagSqDebugger::setBreakpoint(const char *sourcename, int line, const char * /* condition */)
{
  DagSqBreakpoint &b = breakpoints.push_back();
  b.condition.setStr("");
  b.sourcename.setStr(sourcename);
  b.lineNum = line;
  return false;
}

void DagSqDebugger::removeBreakpoint(const char *sourcename, int line)
{
  for (int i = 0; i < breakpoints.size(); i++)
    if (breakpoints[i].lineNum == line && !strcmp(sourcename, breakpoints[i].sourcename.str()))
    {
      erase_items(breakpoints, i, 1);
      return;
    }
}

void DagSqDebugger::pause()
{
  if (!suspended)
  {
    pauseOutOfLineNum = 999999;
    stackDepthToBreak = 999999;
    curLineNum = -1;
  }
}

void DagSqDebugger::run()
{
  if (suspended)
  {
    suspended = false;
    pauseOutOfLineNum = -1;
    curLineNum = -1;
    stackDepth = 0;
    stackDepthToBreak = 999999;
  }
}

void DagSqDebugger::stepInto()
{
  if (suspended)
  {
    suspended = false;
    pauseOutOfLineNum = curLineNum;
    curLineNum = -1;
    stackDepth = 0;
    stackDepthToBreak = 999999;
  }
}

void DagSqDebugger::stepOut()
{
  if (suspended)
  {
    suspended = false;
    pauseOutOfLineNum = 999999;
    curLineNum = -1;
    stackDepth = 1;
    stackDepthToBreak = 0;
  }
}

void DagSqDebugger::stepOver()
{
  if (suspended)
  {
    suspended = false;
    pauseOutOfLineNum = curLineNum;
    curLineNum = -1;
    stackDepth = 0;
    stackDepthToBreak = 0;
  }
}

void DagSqDebugger::setBreakCb(DagSqBreakCb break_callback) { breakCallback = break_callback; }

bool DagSqDebugger::isBreakpoint(const char *sourcename, int line)
{
  for (int i = 0; i < breakpoints.size(); i++)
    if (breakpoints[i].lineNum == line && !strcmp(sourcename, breakpoints[i].sourcename.str()))
      return true;

  return false;
}

void DagSqDebugger::onBreakpoint(const char *sourcename, int line)
{
  static volatile bool called = false;
  if (called)
    return;
  called = true;

  suspended = true;
  curLineNum = line;
  if (breakCallback)
    breakCallback(*this, sourcename, line);
  //  else
  //  {
  //    while (suspended)
  //      sleep_msec(10);
  //  }
  called = false;
}

void DagSqDebugger::onNewLineExecuted(const char *sourcename, int line)
{
  if (isBreakpoint(sourcename, line))
    onBreakpoint(sourcename, line);
  else if (pauseOutOfLineNum > 0 && line != pauseOutOfLineNum && stackDepth <= stackDepthToBreak)
    onBreakpoint(sourcename, line);

  sourceCodeCoverageList.onLineExecuted(sourcename, line);
}

void DagSqDebugger::onLineExists(const char *sourcename, int line) { sourceCodeCoverageList.onLineExists(sourcename, line); }

void DagSqDebugger::clearSourceCoverage() { sourceCodeCoverageList.clearCoverage(); }

bool DagSqDebugger::getSourceCoverage(const char *sourcename, Tab<char> &result)
{
  for (int i = 0; i < sourceCodeCoverageList.sources.size(); i++)
    if (!strcmp(sourceCodeCoverageList.sources[i].sourcename.str(), sourcename))
    {
      result = sourceCodeCoverageList.sources[i].lineType;
      return true;
    }

  return false;
}
