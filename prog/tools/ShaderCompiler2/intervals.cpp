// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "intervals.h"
#include "shTargetContext.h"
#include "deSerializationContext.h"
#include "varMap.h"
#include "shLog.h"
#include "hashStrings.h"
#include <debug/dag_debug.h>
#include <generic/dag_tabUtils.h>
#include <math/dag_math3d.h>
#include "nameMap.h"

const real IntervalValue::VALUE_NEG_INFINITY = -MAX_REAL;
const real IntervalValue::VALUE_INFINITY = MAX_REAL;

const char *IntervalValue::Adapter::getName(int id) { return get_shaders_de_serialization_ctx()->intervalNameMap().getName(id); }
int IntervalValue::Adapter::addName(const char *name) { return get_shaders_de_serialization_ctx()->intervalNameMap().addNameId(name); }

// add interval value to a list  (return false, if value already exists)
bool Interval::addValue(int name_id, const real smaller_than)
{
  if (valueExists(name_id))
    return false;

  RealValueRange range(IntervalValue::VALUE_NEG_INFINITY, smaller_than);

  if (valueList.size() != 0)
  {
    range.setBounds(valueList.back().getBounds().getMax(), smaller_than);
    if (!range.isValid())
      return false;
  }

  valueList.push_back(IntervalValue(name_id, range));

  return true;
}

IntervalValue *Interval::getValueByNameId(int value_name_id)
{
  for (int i = 0; i < valueList.size(); i++)
    if (valueList[i].getNameId() == value_name_id)
      return &valueList[i];
  return NULL;
}

bool Interval::checkExpression(ShaderVariant::ValueType left_op_normalized, const Interval::BooleanExpr expr, const char *right_op,
  String &error_msg, shc::TargetContext &ctx) const
{
  if (expr == EXPR_NOTINIT)
    return false;

  ShaderVariant::ValueRange indexRange = getIndexRange();

  if (!indexRange.isInRange(left_op_normalized))
  {
    error_msg.printf(0, 256, "illegal normalized value (%d) for this interval (%s)", left_op_normalized,
      get_interval_name(*this, ctx));
    return false;
  }

  ShaderVariant::ValueType right_op_normalized = -1;
  int right_op_id = ctx.intervalNameMap().addNameId(right_op);

  if (right_op_id != -1)
    for (int i = 0; i < valueList.size(); i++)
      if (valueList[i].getNameId() == right_op_id)
      {
        right_op_normalized = i;
        break;
      }

  if (!indexRange.isInRange(right_op_normalized))
  {
    error_msg.printf(0, 256, "undefined value (%s) for this interval (%s)", right_op, get_interval_name(*this, ctx));
    return false;
  }

  switch (expr)
  {
    case EXPR_EQ: return left_op_normalized == right_op_normalized;
    case EXPR_GREATER: return left_op_normalized > right_op_normalized;
    case EXPR_GREATER_EQ: return left_op_normalized >= right_op_normalized;
    case EXPR_SMALLER: return left_op_normalized < right_op_normalized;
    case EXPR_SMALLER_EQ: return left_op_normalized <= right_op_normalized;
    case EXPR_NOT_EQ: return left_op_normalized != right_op_normalized;
  }

  return false;
}

// add new interval (return false, if interval exists)
bool IntervalList::addInterval(const Interval &interval)
{
  const Interval *oldInterval = getIntervalByNameId(interval.getNameId());

  if (oldInterval)
  {
    // intervals not identical - report error
    if (*oldInterval != interval)
      return false;
    else
      return true;
  }

  intervals.push_back().create(interval);
  return true;
}

// return true, if interval exists
bool IntervalList::intervalExists(int name_id) const { return name_id != -1 && getIntervalIndex(name_id) != INTERVAL_NOT_INIT; }

const Interval *IntervalList::getIntervalByNameId(int name_id) const
{
  for (int i = 0; i < intervals.size(); i++)
    if (intervals[i]->getNameId() == name_id)
      return intervals[i];
  return NULL;
}

ShaderVariant::ExtType IntervalList::getIntervalIndex(int name_id) const
{
  for (int i = 0; i < intervals.size(); i++)
    if (intervals[i]->getNameId() == name_id)
      return (ShaderVariant::ExtType)i;
  return INTERVAL_NOT_INIT;
}

String IntervalList::getStringInfo(const shc::TargetContext &ctx) const
{
  String out;

  for (int i = 0; i < intervals.size(); i++)
  {
    if (out.length())
      out.aprintf(2, " ");
    out.aprintf(64, "%s(%d, %d)", get_interval_name(*intervals[i], ctx), (int)intervals[i]->getIndexRange().getMin(),
      (int)intervals[i]->getIndexRange().getMax());
  }

  return "intervals=<" + out + ">";
}

void IntervalList::clear() { clear_and_shrink(intervals); }

const char *get_interval_value_name(const IntervalValue &value, const shc::TargetContext &ctx)
{
  return ctx.intervalNameMap().getName(value.getNameId());
}
const char *get_interval_value_name(const Interval &ival, int value_idx, const shc::TargetContext &ctx)
{
  return get_interval_value_name(ival.getValue(value_idx), ctx);
}
const char *get_interval_name(const Interval &ival, const shc::TargetContext &ctx)
{
  return ctx.intervalNameMap().getName(ival.getNameId());
}
