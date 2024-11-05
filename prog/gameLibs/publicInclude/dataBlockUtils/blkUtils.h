//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/initializer_list.h>

class DataBlock;

bool check_param_exist(const DataBlock &blk, std::initializer_list<const char *> param_names);
bool check_all_params_exist(const DataBlock &blk, const char *prop_name, std::initializer_list<const char *> param_names);
bool check_all_params_exist_in_subblocks(const DataBlock &blk, const char *subblock_name, const char *prop_name,
  std::initializer_list<const char *> param_names);
