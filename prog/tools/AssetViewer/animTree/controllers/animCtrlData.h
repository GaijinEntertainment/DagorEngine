// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "ctrlType.h"
#include <dag\dag_vector.h>
#include <propPanel\c_common.h>

struct AnimCtrlData
{
  int id;
  PropPanel::TLeafHandle handle;
  CtrlType type;
  dag::Vector<int> childs;
};
