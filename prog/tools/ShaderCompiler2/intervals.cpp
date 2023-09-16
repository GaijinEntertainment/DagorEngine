#include "intervals.h"
#include "varMap.h"
#include "shLog.h"
#include "hashStrings.h"
#include <debug/dag_debug.h>
#include <generic/dag_tabUtils.h>
#include <math/dag_math3d.h>
#include "nameMap.h"

const real IntervalValue::VALUE_NEG_INFINITY = -MAX_REAL;
const real IntervalValue::VALUE_INFINITY = MAX_REAL;

static HashStrings intervalMap;

int IntervalValue::addIntervalNameId(const char *name) { return intervalMap.addNameId(name); }
int IntervalValue::getIntervalNameId(const char *name) { return intervalMap.getNameId(name); }
const char *IntervalValue::getIntervalName(int id) { return intervalMap.getName(id); }
void IntervalValue::resetIntervalNames() { intervalMap.reset(); }

Interval::Interval(const char *init_name, ShaderVariant::VarType interval_type, bool optional, eastl::string file_name) :
  valueList(midmem), intervalType(interval_type), optional(optional), fileName(eastl::move(file_name))
{
  nameId = IntervalValue::addIntervalNameId(init_name);
}

// add interval value to a list  (return false, if value already exists)
bool Interval::addValue(const char *value_name, const real smaller_than)
{
  int id = IntervalValue::addIntervalNameId(value_name);
  if (valueExists(id))
    return false;

  RealValueRange range(IntervalValue::VALUE_NEG_INFINITY, smaller_than);

  if (valueList.size() != 0)
  {
    range.setBounds(valueList.back().getBounds().getMax(), smaller_than);
    if (!range.isValid())
      return false;
  }

  valueList.push_back(IntervalValue(id, range));

  return true;
}

IntervalValue *Interval::getValue(int value_name_id)
{
  for (int i = 0; i < valueList.size(); i++)
    if (valueList[i].getNameId() == value_name_id)
      return &valueList[i];
  return NULL;
}

bool Interval::checkExpression(ShaderVariant::ValueType left_op_normalized, const Interval::BooleanExpr expr, const char *right_op,
  String &error_msg) const
{
  if (expr == EXPR_NOTINIT)
    return false;

  ShaderVariant::ValueRange indexRange = getIndexRange();

  if (!indexRange.isInRange(left_op_normalized))
  {
    error_msg.printf(0, 256, "illegal normalized value (%d) for this interval (%s)", left_op_normalized, getNameStr());
    return false;
  }

  ShaderVariant::ValueType right_op_normalized = -1;
  int right_op_id = IntervalValue::addIntervalNameId(right_op);

  if (right_op_id != -1)
    for (int i = 0; i < valueList.size(); i++)
      if (valueList[i].getNameId() == right_op_id)
      {
        right_op_normalized = i;
        break;
      }

  if (!indexRange.isInRange(right_op_normalized))
  {
    error_msg.printf(0, 256, "undefined value (%s) for this interval (%s)", right_op, getNameStr());
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
bool IntervalList::intervalExists(const char *name) const
{
  int id = IntervalValue::getIntervalNameId(name);
  return id != -1 && getIntervalIndex(id) != INTERVAL_NOT_INIT;
}

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

String IntervalList::getStringInfo() const
{
  String out;

  for (int i = 0; i < intervals.size(); i++)
  {
    if (out.length())
      out.aprintf(2, " ");
    out.aprintf(64, "%s(%d, %d)", intervals[i]->getNameStr(), (int)intervals[i]->getIndexRange().getMin(),
      (int)intervals[i]->getIndexRange().getMax());
  }

  return "intervals=<" + out + ">";
}

void IntervalList::clear() { clear_and_shrink(intervals); }
