// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/string.h>
#include <ska_hash_map/flat_hash_map2.hpp>

class DataBlock;

using ShaderSeparatorToPropsType =
  ska::flat_hash_map<eastl::string /*separators*/, ska::flat_hash_set<eastl::string> /*shader props*/>;
ska::flat_hash_map<eastl::string /*shader*/, ShaderSeparatorToPropsType> gatherParameterSeparators(const DataBlock &appBlk,
  const char *appDir);