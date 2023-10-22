/************************************************************************
  functions on the shaders
************************************************************************/
// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef __SHFUNC_H
#define __SHFUNC_H

#include "shExprTypes.h"
#include <generic/dag_tab.h>

struct Color4;

namespace functional
{
//************************************************************************
//* built-in function enum
//************************************************************************
enum FunctionId
{
  BF_TIME_PHASE,
  BF_SIN,
  BF_COS,
  BF_POW,
  BF_FSEL,
  BF_SQRT,
  BF_MIN,
  BF_MAX,
  BF_ANIM_FRAME,
  BF_WIND_COEFF,
  BF_FADE_VAL,
  BF_VECPOW,
  BF_SRGBREAD,
  BF_GET_DIMENSIONS,
  BF_GET_VIEWPORT,
};

//************************************************************************
//* function argument
//************************************************************************
struct FuncArgument
{
  union Value
  {
    float c[4];
    float r;

    Color4 &c4() { return *(Color4 *)(void *)c; }
  };

  Value val;
  shexpr::ValueType vt;

  FuncArgument()
  {
    val.c[0] = val.c[1] = val.c[2] = val.c[3] = 0;
    vt = shexpr::VT_REAL;
  }
};

typedef Tab<FuncArgument> ArgList;

// prepare argument array
int prepareArgs(FunctionId func, FuncArgument *a, int num);
void prepareArgs(FunctionId func, ArgList &args);

// return operand count for specified function
int getOpCount(FunctionId func);

// return result value type for specified function
shexpr::ValueType getValueType(FunctionId func);

// evaluate function - return false, if error. if output value type is real, return value in res.r.
bool evaluate(FunctionId func, Color4 &res, const ArgList &args);

// evaluate using registers array of exec_stcode.
void callFunction(FunctionId id, int out_reg, const int *in_regs, char *regs);

// return function id by name; return false, if invalid function
bool getFuncId(const char *name, FunctionId &ret_func);

// return name by function id
inline const char *getFuncName(FunctionId id)
{
  switch (id)
  {
    case BF_TIME_PHASE: return "BF_TIME_PHASE";
    case BF_SIN: return "BF_SIN";
    case BF_COS: return "BF_COS";
    case BF_POW: return "BF_POW";
    case BF_VECPOW: return "BF_VECPOW";
    case BF_SRGBREAD: return "BF_SRGBREAD";
    case BF_FSEL: return "BF_FSEL";
    case BF_SQRT: return "BF_SQRT";
    case BF_MIN: return "BF_MIN";
    case BF_MAX: return "BF_MAX";
    case BF_ANIM_FRAME: return "BF_ANIM_FRAME";
    case BF_WIND_COEFF: return "BF_WIND_COEFF";
    case BF_FADE_VAL: return "BF_FADE_VAL";
    case BF_GET_DIMENSIONS: return "BF_GET_DIMENSIONS";
    case BF_GET_VIEWPORT: return "BF_GET_VIEWPORT";
    default: G_ASSERT(0);
  }
  return "<???>";
}

// return true, if function is dynamic always
bool isAlwaysDynamic(FunctionId id);
} // namespace functional


#endif //__SHFUNC_H
