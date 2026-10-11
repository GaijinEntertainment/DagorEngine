// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/c_common.h>
enum class BlendNodeType;

struct BlendNodeData
{
  PropPanel::TLeafHandle handle;
  int id;
  BlendNodeType type;
};
