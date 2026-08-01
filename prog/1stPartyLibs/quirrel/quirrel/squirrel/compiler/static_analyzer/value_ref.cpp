#include <assert.h>

#include "value_ref.h"
#include "node_equal_checker.h"


namespace SQCompilation
{

static const ValueRefState mergeMatrix[VRS_NUM_OF_STATES][VRS_NUM_OF_STATES] = {
 // VRS_EXPRESSION  VRS_INITIALIZED  VRS_MULTIPLE   VRS_UNKNOWN    VRS_DECLARED
  { VRS_EXPRESSION, VRS_MULTIPLE,    VRS_MULTIPLE,  VRS_UNKNOWN,   VRS_MULTIPLE  }, // VRS_EXPRESSION
  { VRS_MULTIPLE,   VRS_INITIALIZED, VRS_MULTIPLE,  VRS_UNKNOWN,   VRS_MULTIPLE  }, // VRS_INITIALIZED
  { VRS_MULTIPLE,   VRS_MULTIPLE,    VRS_MULTIPLE,  VRS_UNKNOWN,   VRS_MULTIPLE  }, // VRS_MULTIPLE
  { VRS_UNKNOWN,    VRS_UNKNOWN,     VRS_UNKNOWN,   VRS_UNKNOWN,   VRS_UNKNOWN   }, // VRS_UNKNOWN
  { VRS_MULTIPLE,   VRS_MULTIPLE,    VRS_MULTIPLE,  VRS_UNKNOWN,   VRS_DECLARED  }  // VRS_DECLARED
};


void ValueRef::intersectValue(const ValueRef *other) {
  assert(info == other->info);

  flagsPositive &= other->flagsPositive;
  flagsNegative &= other->flagsNegative;

  assert((flagsNegative & flagsPositive) == 0);

  assigned &= other->assigned;
  if (lastAssigneeScope) {
    if (other->lastAssigneeScope && other->lastAssigneeScope->depth > lastAssigneeScope->depth) {
      lastAssigneeScope = other->lastAssigneeScope;
    }
  }
  else {
    lastAssigneeScope = other->lastAssigneeScope;
  }

  // TODO: intersect bounds

  if (isConstant())
    return;

  if (!NodeEqualChecker().check(expression, other->expression)) {
    kill(VRS_MULTIPLE, false);
  }
}

void ValueRef::merge(const ValueRef *other, bool joinFlags) {
  assert(info == other->info);

  assigned &= other->assigned;
  if (lastAssigneeScope) {
    if (other->lastAssigneeScope && other->lastAssigneeScope->depth > lastAssigneeScope->depth) {
      lastAssigneeScope = other->lastAssigneeScope;
    }
  }
  else {
    lastAssigneeScope = other->lastAssigneeScope;
  }

  // Control-flow join: a guarantee (flagsNegative) survives only when both
  // paths provide it, a possibility (flagsPositive) when either does.
  // Otherwise narrowing learned in one branch leaks past the join.
  if (joinFlags) {
    flagsPositive |= other->flagsPositive;
    flagsNegative &= other->flagsNegative;
    assert((flagsNegative & flagsPositive) == 0);
  }

  if (state != other->state) {
    ValueRefState k = mergeMatrix[other->state][state];
    kill(k);
    return;
  }

  if (isConstant()) {
    assert(other->isConstant());
    return;
  }

  if (!NodeEqualChecker().check(expression, other->expression)) {
    kill(VRS_MULTIPLE);
  }
}

}
