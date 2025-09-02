// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shAssumes.h"
#include "shLog.h"
#include "shErrorReporting.h"

float ShaderAssumesTable::addAssume(const char *name, float val, bool is_fallback, Parser &parser, Terminal *t)
{
  const eastl::string key{name};
  if (is_fallback)
  {
    if (auto valueMaybe = getAssumedVal(name, true))
    {
      report_debug_message(parser, *t,
        "WARNING: Will not override (%s (for %s) old=%g -> new=%g) in 'assume_if_not_assumed' statement", name,
        debugName ? debugName : "", *valueMaybe, val);
      return *valueMaybe;
    }
  }
  auto [it, inserted] = assumes.emplace(key, AssumeRecord{val, is_fallback});
  if (!inserted)
  {
    // The same assume can not be redeclared with a different val at a given scope,
    // and the same fallback assume can't be redeclared either.
    if (it->second.isFallback == is_fallback && it->second.val != val)
    {
      if (is_fallback)
      {
        report_error(parser, t, "Only one soft override of %s (for %s) with 'assume_if_not_assumed' statement is allowed", name,
          debugName ? debugName : "");
      }
      else
      {
        report_error(parser, t, "Overriding %s (for %s) old=%g -> new=%g in 'assume' statement is not allowed", name,
          debugName ? debugName : "", it->second.val, val);
      }
      return it->second.val;
    }

    if (!is_fallback)
      it->second = AssumeRecord{val, false};
  }
  return val;
}

float ShaderAssumesTable::addIntervalAssume(const char *name, const IntervalValue &interv_val, bool is_fallback, Parser &parser,
  Terminal *t)
{
  const float val = (interv_val.getBounds().getMin() + interv_val.getBounds().getMax()) * 0.5;
  return addAssume(name, val, is_fallback, parser, t);
}

eastl::optional<float> ShaderAssumesTable::getAssumedVal(const char *varname, bool var_is_global) const
{
  const eastl::string key{varname};
  eastl::optional<float> localRes{};
  auto it = assumes.find(key);
  if (it != assumes.end())
    localRes = it->second.val;
  else
    localRes = getAssumedValFromConfig(varname);

  if (localRes || !var_is_global)
    return localRes;

  return parent ? parent->getAssumedVal(varname, true) : localRes;
}

eastl::optional<float> ShaderAssumesTable::getAssumedValFromConfig(const char *varname) const
{
  int idx = config.findParam(varname);

  if (idx == -1)
    return eastl::nullopt;

  switch (config.getParamType(idx))
  {
    case DataBlock::TYPE_INT: return (float)config.getInt(idx);
    case DataBlock::TYPE_REAL: return config.getReal(idx);
    default: sh_debug(SHLOG_ERROR, "Assume variables: type in \"%s\" is neither \"real\" nor \"int\"", varname);
  }
  return eastl::nullopt;
}
