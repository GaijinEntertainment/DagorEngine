// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/dag_assert.h>

#include <EASTL/algorithm.h>


enum class OptimizationLevel : unsigned char
{
  O0 = 0,
  O1 = 1,
  O2 = 2,
  O3 = 3,
  O4 = 4,
  Od = 5,
};

inline OptimizationLevel arg_to_optimization_level(const bool full_debug, int level)
{
  level = eastl::min(level, (int)OptimizationLevel::Od - 1);
  return full_debug ? OptimizationLevel::Od : (OptimizationLevel)level;
}

inline const char *optimization_level_to_compiler_arg(const OptimizationLevel l)
{
  switch (l)
  {
    case OptimizationLevel::O0: return "-O0";
    case OptimizationLevel::O1: return "-O1";
    case OptimizationLevel::O2: return "-O2";
    case OptimizationLevel::O3: return "-O3";
    case OptimizationLevel::O4: return "-O4";
    case OptimizationLevel::Od: return "-Od";
    default: G_ASSERT_RETURN(!"unsupported compiler optimization level", "-O4");
  }
}
