// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shAssumes.h"
#include "shLog.h"
#include "shErrorReporting.h"

ShaderAssumesTable::ShaderAssumesTable(const DataBlock &config_assumes, HashStrings &interval_name_map,
  const ShaderAssumesTable *parent_table, const char *debug_name) :
  parent{parent_table}, intervalNameMap{&interval_name_map}, debugName{debug_name}
{
  dblk::iterate_params(config_assumes, [&](int idx, int, int type) {
    const char *name = config_assumes.getParamName(idx);
    float val;
    switch (type)
    {
      case DataBlock::TYPE_INT: val = (float)config_assumes.getInt(idx); break;
      case DataBlock::TYPE_REAL: val = config_assumes.getReal(idx); break;
      default: sh_debug(SHLOG_ERROR, "Assume variables: type in \"%s\" is neither \"real\" nor \"int\"", name); return;
    }
    auto [it, inserted] = assumes.emplace(intervalNameMap->addNameId(name), AssumeRecord{val, true});
    if (!inserted && val != it->second.val)
    {
      sh_debug(SHLOG_ERROR, "Assume variables: \"%s\" redeclared in blk", name);
      return;
    }
  });
}

float ShaderAssumesTable::addAssume(int nid, float val, bool is_fallback, Parser &parser, Terminal *t)
{
  G_ASSERT(nid >= 0);
  if (is_fallback)
  {
    if (auto valueMaybe = getAssumedVal(nid, true))
    {
      report_debug_message(parser, *t,
        "WARNING: Will not override (%s (for %s) old=%g -> new=%g) in 'assume_if_not_assumed' statement",
        intervalNameMap->getName(nid), debugName ? debugName : "", *valueMaybe, val);
      return *valueMaybe;
    }
  }
  auto [it, inserted] = assumes.emplace(nid, AssumeRecord{val, is_fallback});
  if (!inserted)
  {
    G_ASSERT(!is_fallback);
    // The same assume can not be redeclared with a different val at a given scope,
    // and the same fallback assume can't be redeclared either.
    if (!it->second.isFallback && it->second.val != val)
    {
      report_error(parser, t, "Overriding %s (for %s) old=%g -> new=%g in 'assume' statement is not allowed",
        intervalNameMap->getName(nid), debugName ? debugName : "", it->second.val, val);
      return it->second.val;
    }

    it->second = AssumeRecord{val, false};
  }
  return val;
}

float ShaderAssumesTable::addIntervalAssume(int nid, const IntervalValue &interv_val, bool is_fallback, Parser &parser, Terminal *t)
{
  const float val = (interv_val.getBounds().getMin() + interv_val.getBounds().getMax()) * 0.5;
  return addAssume(nid, val, is_fallback, parser, t);
}

eastl::optional<float> ShaderAssumesTable::getAssumedVal(int nid, bool var_is_global) const
{
  G_ASSERT(nid >= 0);
  eastl::optional<float> localRes{};
  auto it = assumes.find(nid);

  if (it != assumes.end())
    localRes = it->second.val;
  if (localRes || !var_is_global)
    return localRes;

  return parent ? parent->getAssumedVal(nid, true) : localRes;
}
