// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

/************************************************************************
  intervals support for shaders
/************************************************************************/
#ifndef __INTERVALS_H
#define __INTERVALS_H

#include "varTypes.h"
#include "shaderSave.h"
#include <generic/dag_tab.h>
#include <util/dag_globDef.h>
#include <util/dag_string.h>

typedef Range<real> RealValueRange;

class IntervalValue
{
public:
  static const real VALUE_NEG_INFINITY;
  static const real VALUE_INFINITY;

  IntervalValue() : bounds(VALUE_NEG_INFINITY, VALUE_INFINITY){};

  explicit inline IntervalValue(const char *init_name, const RealValueRange &init_bounds) : bounds(init_bounds)
  {
    nameId = IntervalValue::addIntervalNameId(init_name);
    G_ASSERT(nameId >= 0 && nameId < INTERVAL_NOT_INIT);
  }
  explicit inline IntervalValue(int init_name_id, const RealValueRange &init_bounds) : bounds(init_bounds) { nameId = init_name_id; }

  inline bool valueIsEqual(real other_value) const { return other_value >= bounds.getMin() && other_value < bounds.getMax(); }

  inline int getNameId() const { return nameId; }
  inline const char *getNameStr() const { return IntervalValue::getIntervalName(nameId); }

  inline const RealValueRange &getBounds() const { return bounds; };

  static int addIntervalNameId(const char *name);
  static int getIntervalNameId(const char *name);
  static const char *getIntervalName(int id);
  static void resetIntervalNames();

  struct Adapter
  {
    static const char *getName(int id) { return IntervalValue::getIntervalName(id); }
    static int addName(const char *name) { return IntervalValue::addIntervalNameId(name); }
  };

private:
  NameId<Adapter> nameId;
  RealValueRange bounds; // value's bounds
};

class Interval
{
public:
  /************************************************************************/
  /* boolean expressions for this class
  /************************************************************************/
  enum BooleanExpr
  {
    EXPR_NOTINIT,
    EXPR_EQ,         // ==
    EXPR_GREATER,    // >
    EXPR_GREATER_EQ, // >=
    EXPR_SMALLER,    // <
    EXPR_SMALLER_EQ, // <=
    EXPR_NOT_EQ,     // !=
  };

  explicit Interval(const char *init_name, ShaderVariant::VarType interval_type, bool optional = false, eastl::string file_name = "");
  Interval() : valueList(midmem), optional(false) {}
  ~Interval() { clear(); }

  inline void clear() { clear_and_shrink(valueList); }

  // return different value index range for this interval
  inline const ShaderVariant::ValueRange getIndexRange() const { return ShaderVariant::ValueRange(0, valueList.size() - 1); }

  // return interval's name
  inline int getNameId() const { return nameId; }
  inline const char *getNameStr() const { return IntervalValue::getIntervalName(nameId); }

  // add interval value to a list  (return false, if value already exists)
  bool addValue(const char *value_name, const real smaller_than);

  // return true, if interval value already exists
  bool valueExists(int value_name_id) const { return getValue(value_name_id) != NULL; }

  IntervalValue *getValue(int value_name_id);
  const IntervalValue *getValue(int value_name_id) const { return const_cast<Interval *>(this)->getValue(value_name_id); }

  // return the number of values.
  int getValueCount() const { return valueList.size(); }

  // return value name by index.
  const char *getValueName(int index) const { return valueList[index].getNameStr(); }
  RealValueRange getValueRange(int index) const { return valueList[index].getBounds(); }

  // normalize value for this interval - retrun internal value index
  inline int normalizeValue(real value) const
  {
    for (int i = 0; i < valueList.size(); i++)
      if (valueList[i].valueIsEqual(value))
        return i;

    return -1;
  }

  // check boolean expression
  bool checkExpression(ShaderVariant::ValueType left_op_normalized, const Interval::BooleanExpr expr, const char *right_op,
    String &error_msg) const;

  // return true, if intervals are identical
  inline bool operator==(const Interval &other) const
  {
    if (nameId != other.nameId || valueList.size() != other.valueList.size())
      return false;

    return mem_eq(valueList, other.valueList.data());
  }

  // return true, if intervals are not identically
  inline bool operator!=(const Interval &other) const { return !operator==(other); }

  // get/set variable index for this interval
  inline void setVarType(ShaderVariant::VarType v) { intervalType = v; }
  inline ShaderVariant::VarType getVarType() const { return intervalType; }
  const eastl::string &getFileName() const { return fileName; }

  bool isOptional() const { return optional; }

private:
  NameId<IntervalValue::Adapter> nameId;
  bool optional;
  ShaderVariant::VarType intervalType;
  SerializableTab<IntervalValue> valueList;
  bindump::string fileName;
};

class IntervalList
{
public:
  void clear();

  // add new interval (return false, if interval exists)
  bool addInterval(const Interval &interval);

  // return true, if interval exists
  bool intervalExists(const char *name) const;

  // return number of intervals in the list
  inline int getCount() const { return intervals.size(); };

  // return interval by index
  inline const Interval *getInterval(const int index) const
  {
    return ((index >= 0) && (index < intervals.size())) ? intervals[index].get() : nullptr;
  }

  ShaderVariant::ExtType getIntervalIndex(int name_id) const;
  String getStringInfo() const;

private:
  SerializableTab<bindump::Ptr<Interval>> intervals;

  const Interval *getIntervalByNameId(int name_id) const;
};

#endif //__INTERVALS_H
