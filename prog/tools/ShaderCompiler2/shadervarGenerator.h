// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "globVar.h"
#include "intervals.h"
#include <vector>
#include <string>
#include <map>

enum class ShadervarGeneratorMode
{
  None,
  Remove,
  Check,
  Generate,
};

struct GeneratedPathInfo
{
  std::string matcher;
  std::string replacer;
};
using GeneratedPathInfos = std::vector<GeneratedPathInfo>;

template <typename T>
struct VariableInfo
{
  int priority = std::numeric_limits<int>::max();
  std::string generated_file_name;
  T var;
};

class ShadervarGenerator
{
  std::map<std::string, VariableInfo<ShaderGlobal::Var>> mShadervars;
  std::map<std::string, VariableInfo<Interval>> mIntervals;

public:
  void addShadervarsAndIntervals(dag::Span<ShaderGlobal::Var> shadervars, const IntervalList &intervals);
  void generateShadervars();
};
