// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <ioSys/dag_dataBlock.h>
#include <util/dag_string.h>

struct AnimParamData
{
  String name;
  int pid = 0;
  DataBlock::ParamType type = DataBlock::TYPE_NONE;
  bool dependent = false;
};

struct DependentParamData
{
  int pid = 0;
  int dependentParamPid = 0;
  bool dependent = false;
};
