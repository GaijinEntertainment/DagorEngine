#pragma once

#include <memory.h> // memset
#include <squirrel.h>
#include "squtils.h"

#define VAR_TRACE_SAVE_VALUES 0

#define VAR_TRACE_STACK_DEPTH 4
#define VAR_TRACE_STACK_HISTORY 4
#define STACK_NOT_INITIALIZED (-2)


struct VarTrace
{
  struct VarStackRecord
  {
    const SQChar * fileName;
    int line;
  };

  struct HistoryRecord
  {
    VarStackRecord stack[VAR_TRACE_STACK_DEPTH];
#if VAR_TRACE_SAVE_VALUES != 0
    int count;
    SQChar val[31];
    SQChar flags;
#endif
  };

  int pos;
  int setCnt;
  HistoryRecord history[VAR_TRACE_STACK_HISTORY];

  VarTrace()
  {
    memset(this, 0, sizeof(*this));
    clear();
  }

  void clear()
  {
    pos = 0;
    setCnt = 0;
#if VAR_TRACE_SAVE_VALUES != 0
    history[0].count = 0;
#endif
    for (int i = 0; i < VAR_TRACE_STACK_HISTORY; i++)
    {
      history[i].stack[0].line = STACK_NOT_INITIALIZED;
#if VAR_TRACE_SAVE_VALUES != 0
      history[i].val[0] = 0;
#endif
    }
  }

  void saveStack(const SQObject & value, const SQObject &vm);
  void saveStack(const SQObject & value, HSQUIRRELVM vm);
  void printStack(char * buf, int size);

private:
  bool isStacksEqual(int a, int b);
};

#if SQ_VAR_TRACE_ENABLED == 1

#define VT_CODE(code) code
#define VT_COMMA ,
#define VT_DECL_VEC SQVarTraceVec varTrace
#define VT_DECL_CTR(ss_) , varTrace(ss_)
#define VT_DECL_SINGLE VarTrace varTrace
#define VT_DECL_ARG VT_COMMA VarTrace * var_trace_arg
#define VT_DECL_ARG_DEF VT_DECL_ARG = NULL
#define VT_ARG VT_COMMA var_trace
#define VT_ARG_PARAM var_trace
#define VT_REF(ptr) VT_COMMA &(ptr->varTrace)
#define VT_RESIZE(x) varTrace.resize(x)
#define VT_RESERVE(x) varTrace.reserve(x)
#define VT_TRACE(x, val, vm) varTrace[x].saveStack(val, vm)
#define VT_TRACE_SINGLE(ptr, val, vm) ptr->varTrace.saveStack(val, vm)
#define VT_CLEAR_SINGLE(ptr) ptr->varTrace.clear()
#define VT_INSERT(x, val, vm) { VarTrace tmp; varTrace.insert(x, tmp); VT_TRACE(x, val, vm); }
#define VT_PUSHBACK(val, vm) { VarTrace tmp; varTrace.push_back(tmp); VT_TRACE(varTrace.size() - 1, val, vm); }
#define VT_POPBACK() varTrace.pop_back()
#define VT_REMOVE(x) varTrace.remove(x)
#define VT_CLONE_TO(to) to->varTrace.copy(varTrace)
#define VT_COPY_SINGLE(from, to) to->varTrace = from->varTrace


typedef sqvector<VarTrace> SQVarTraceVec;

#else

#define VT_CODE(code)
#define VT_COMMA
#define VT_DECL_VEC
#define VT_DECL_CTR(ss_)
#define VT_DECL_SINGLE
#define VT_DECL_ARG
#define VT_DECL_ARG_DEF
#define VT_ARG VT_COMMA
#define VT_ARG_PARAM
#define VT_REF(ptr)
#define VT_RESIZE(x)
#define VT_RESERVE(x)
#define VT_TRACE(x, val, vm)
#define VT_TRACE_SINGLE(ptr, val, vm)
#define VT_CLEAR_SINGLE(ptr)
#define VT_INSERT(x, val, vm)
#define VT_PUSHBACK(val, vm)
#define VT_POPBACK()
#define VT_REMOVE(x)
#define VT_CLONE_TO(to)
#define VT_COPY_SINGLE(from, to)

#endif
