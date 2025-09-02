// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "commonUtils.h"
#include "intervals.h"
#include "shlexterm.h"
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

  const DataBlock &config;
  const ShaderAssumesTable *parent;
  ska::flat_hash_map<eastl::string, AssumeRecord> assumes{};
  const char *debugName = nullptr;

public:
  ShaderAssumesTable(const DataBlock &config_assumes, const ShaderAssumesTable *parent_table = nullptr,
    const char *debug_name = nullptr) :
    config{config_assumes}, parent{parent_table}, debugName{debug_name}
  {}

  PINNED_TYPE(ShaderAssumesTable)

  // @NOTE: is_fallback = true means 'assume_if_not_assumed': if this var is already assumed, the call to addAssume is nop.
  //          however, if it was assumed with is_fallback = true before, it is an error.
  //        is_fallback = false means 'assume': if it was assumed with is_fallback = false before, it's an error, but if
  //          it was assumed only as a fallback before, it is overriden.
  float addAssume(const char *name, float val, bool is_fallback, Parser &parser, Terminal *t = nullptr);
  float addIntervalAssume(const char *name, const IntervalValue &interv_val, bool is_fallback, Parser &parser, Terminal *t = nullptr);

  eastl::optional<float> getAssumedVal(const char *varname, bool var_is_global) const;

  const char *getDebugName() const { return debugName; }

private:
  eastl::optional<float> getAssumedValFromConfig(const char *varname) const;
};
