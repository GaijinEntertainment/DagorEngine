// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "pointCloudGen.h"
#include <dag/dag_vector.h>
#include <EASTL/fixed_string.h>

namespace plod
{

struct AppOptions
{
  dag::Vector<plod::String> assetsToBuild;
  dag::Vector<plod::String> packsToBuild;
  plod::String appBlk;
  bool dryMode = false;
  bool valid = true;
  bool debug = true;
  bool updateBlk = false;
};

void printOptions();
AppOptions parseOptions();

} // namespace plod
