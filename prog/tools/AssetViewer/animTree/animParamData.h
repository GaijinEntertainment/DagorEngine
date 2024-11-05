// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <ioSys/dag_dataBlock.h>
#include <util/dag_string.h>

struct AnimParamData
{
  int pid = 0;
  DataBlock::ParamType type = DataBlock::TYPE_NONE;
  String name;
  bool dependent = false;
};
