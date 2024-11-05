// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel\c_common.h>
enum class BlendNodeType;

struct BlendNodeData
{
  int id;
  PropPanel::TLeafHandle handle;
  BlendNodeType type;
};
