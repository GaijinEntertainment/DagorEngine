#pragma once

#include <squirrel.h>
#include "var_scope.h"
#include "symbol_info.h"


namespace SQCompilation
{

struct SymbolInfo;
class Expr;

enum ValueRefState {
  VRS_EXPRESSION,
  VRS_INITIALIZED,
  VRS_MULTIPLE,
  VRS_UNKNOWN,
  VRS_DECLARED,
  VRS_NUM_OF_STATES
};


struct ValueRef {

  SymbolInfo *info;
  enum ValueRefState state;
  const Expr *expression;

  /*
    used to track mixed assignments
    local x = 10 // eid = 1
    let y = x    // eid = 2
    x = 20       // eid = 3
  */
  int32_t evalIndex;

  // Track aliasing: if this variable was initialized from another variable,
  // store a reference to it so we can check its current flags dynamically.
  const ValueRef *origin;
  int32_t originEvalIndex;  // origin's evalIndex at the time of this declaration

  ValueRef(SymbolInfo *i, int32_t eid)
    : info(i), evalIndex(eid), state(), expression(nullptr)
  {
    assigned = false;
    lastAssigneeScope = nullptr;
    flagsPositive = flagsNegative = 0;
    origin = nullptr;
    originEvalIndex = -1;
  }

  bool hasValue() const {
    return state == VRS_EXPRESSION || state == VRS_INITIALIZED;
  }

  bool isConstant() const {
    return info->isConstant();
  }

  bool assigned;
  const VarScope *lastAssigneeScope;

  unsigned flagsPositive, flagsNegative;

  void kill(ValueRefState k = VRS_UNKNOWN, bool clearFlags = true) {
    if (!isConstant()) {
      state = k;
      expression = nullptr;
    }
    if (clearFlags) {
      flagsPositive = 0;
      flagsNegative = 0;
    }
  }

  void intersectValue(const ValueRef *other);

  void merge(const ValueRef *other);
};

}
