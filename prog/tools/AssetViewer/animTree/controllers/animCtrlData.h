// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "ctrlType.h"
#include <dag/dag_vector.h>
#include <propPanel/c_common.h>

struct AnimCtrlData
{
  enum
  {
    CHILD_AS_SELF = -2,
    NOT_FOUND_CHILD = -1,
  };
  int id;
  PropPanel::TLeafHandle handle;
  CtrlType type;
  dag::Vector<int> childs;
};
