//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>
#include <util/dag_string.h>
#include <sqrat.h>

#define MAX_SQ_DEBUGGERS 6

class SqModules;


enum CodeLineType
{
  SRC_LINE_INACTIVE = 0,
  SRC_LINE_EXISTS = 1,
  SRC_LINE_EXECUTED = 2,
};


class SourceCodeCoverage
{
public:
  String sourcename;
  Tab<char> lineType; // CodeLineType

  void init(const char *filename_);
  void clear();
  void clearCoverage();
  void onLineExists(int lineNum);
  void onLineExecuted(int lineNum);
};


class SourceCodeCoverageList
{
  String lastFileName;
  int lastIndex;

public:
  Tab<SourceCodeCoverage> sources;

  SourceCodeCoverageList();
  void clear();
  void clearCoverage();
  void clearByIndex(int index);
  void onLineExists(const char *sourcename, int line);
  void onLineExecuted(const char *sourcename, int line);
};


class DagSqDebugger;

typedef void (*DagSqBreakCb)(DagSqDebugger &debugger, const char *sourcename, int line);


class DagSqBreakpoint
{
public:
  int lineNum;
  String sourcename;
  String condition;
};


class DagSqDebugger
{
  Tab<String> sources_list;
  int curLineNum = -1;
  volatile int pauseOutOfLineNum = -1;
  volatile int stackDepthToBreak = 999999;
  bool areFunctionsRegistered = false;

  SourceCodeCoverageList sourceCodeCoverageList;

public:
  volatile bool debugger_enabled = false;
  volatile bool suspended = false;
  volatile bool executingImmediate = false;
  volatile bool breakOnEvent = false;
  bool runtimeErrorState = false;
  int stackDepth = 0;
  String lastRuntimeError = String("");
  DagSqBreakCb breakCallback = nullptr;
  Tab<DagSqBreakpoint> breakpoints;
  HSQUIRRELVM vm = nullptr;
  String description = String("");
  Sqrat::Function objPrintFunc;

  void enable(bool enable);
  void run();
  void pause();
  void stepInto();
  void stepOut();
  void stepOver();
  void clearSourceList();
  static void addSource(HSQUIRRELVM vm, const char *sourcename);
  void getSourceList(Tab<String> &output);
  void setBreakCb(DagSqBreakCb break_callback);
  bool setBreakpoint(const char *sourcename, int line, const char *condition);
  bool isBreakpoint(const char *sourcename, int line);
  void removeBreakpoint(const char *sourcename, int line);
  void clearSourceCoverage();
  bool getSourceCoverage(const char *sourcename, Tab<char> &result);

  void onNewLineExecuted(const char *sourcename, int line);
  void onLineExists(const char *sourcename, int line);
  void onBreakpoint(const char *sourcename, int line);

  bool checkForUITimeout = false;
  int lastActiveUITime = 0;

  void breakFromNativeCode(const char *function_name);

private:
  DagSqDebugger() = default;
  void init(SqModules *module_mgr, const char *description);
  void shutdown();

  static SQInteger setObjPrintFunction(HSQUIRRELVM vm);

  friend class DagSqDebuggers;
};


class DagSqDebuggers
{
  DagSqDebugger debuggers[MAX_SQ_DEBUGGERS];

public:
  DagSqDebuggers() = default;

  void initDebugger(int index, SqModules *module_mgr, const char *description);
  void shutdownDebugger(int index);
  bool isVmRegistered(HSQUIRRELVM vm);
  DagSqDebugger &get(HSQUIRRELVM vm);
  DagSqDebugger &get(int index);
  const char *getDescription(HSQUIRRELVM vm);
  const char *getDescription(int index);
};


extern DagSqDebuggers dag_sq_debuggers;
