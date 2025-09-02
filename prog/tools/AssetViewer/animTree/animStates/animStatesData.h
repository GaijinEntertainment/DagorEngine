// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/c_common.h>
#include <util/dag_string.h>

enum class AnimStatesType;

struct AnimStatesData
{
  enum
  {
    LEAVE_CUR_CHILD_TYPE = -3,
    EMPTY_CHILD_TYPE = -2,
    NOT_FOUND_CHILD = -1,
  };
  PropPanel::TLeafHandle handle;
  AnimStatesType type;
  String fileName;
  dag::Vector<int> childs;
};
