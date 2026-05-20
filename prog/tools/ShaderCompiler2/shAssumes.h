// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "commonUtils.h"
#include "intervals.h"
#include "shlexterm.h"
#include "hashStrings.h"
#include <ioSys/dag_dataBlock.h>
#include <EASTL/optional.h>
#include <ska_hash_map/flat_hash_map2.hpp>


class ShaderAssumesTable
{
  struct AssumeRecord
  {
    float val;
    bool isFallback;
  };

  const ShaderAssumesTable *parent;
  HashStrings *intervalNameMap;
  ska::flat_hash_map<int, AssumeRecord> assumes{};
  const char *debugName = nullptr;

public:
  ShaderAssumesTable(const DataBlock &config_assumes, HashStrings &interval_name_map, const ShaderAssumesTable *parent_table = nullptr,
    const char *debug_name = nullptr);

  PINNED_TYPE(ShaderAssumesTable)

  // @NOTE: is_fallback = true means 'assume_if_not_assumed': if this var is already assumed, the call to addAssume is nop.
  //          however, if it was assumed with is_fallback = true before, it is an error.
  //        is_fallback = false means 'assume': if it was assumed with is_fallback = false before, it's an error, but if
  //          it was assumed only as a fallback before, it is overriden.

  float addAssume(int nid, float val, bool is_fallback, Parser &parser, Terminal *t = nullptr);
  float addIntervalAssume(int nid, const IntervalValue &interv_val, bool is_fallback, Parser &parser, Terminal *t = nullptr);
  eastl::optional<float> getAssumedVal(int nid, bool var_is_global) const;

  float addAssume(const char *name, float val, bool is_fallback, Parser &parser, Terminal *t = nullptr)
  {
    return addAssume(intervalNameMap->addNameId(name), val, is_fallback, parser, t);
  }
  float addIntervalAssume(const char *name, const IntervalValue &interv_val, bool is_fallback, Parser &parser, Terminal *t = nullptr)
  {
    return addIntervalAssume(intervalNameMap->addNameId(name), interv_val, is_fallback, parser, t);
  }
  eastl::optional<float> getAssumedVal(const char *varname, bool var_is_global) const
  {
    if (int nid = intervalNameMap->getNameId(varname); nid >= 0)
      return getAssumedVal(nid, var_is_global);
    else
      return eastl::nullopt;
  }

  const char *getDebugName() const { return debugName; }
};
