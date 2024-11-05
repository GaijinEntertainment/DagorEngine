// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "blendNodeType.h"
#include "../animTreeIcons.h"

const char *get_blend_node_icon_name(BlendNodeType type)
{
  switch (type)
  {
    case BlendNodeType::SINGLE: return BLEND_NODE_SINGLE_ICON;
    case BlendNodeType::CONTINUOUS: return BLEND_NODE_CONTINUOUS_ICON;
    case BlendNodeType::STILL: return BLEND_NODE_STILL_ICON;
    case BlendNodeType::PARAMETRIC: return BLEND_NODE_PARAMETRIC_ICON;

    default: return ANIM_BLEND_NODE_ICON;
  }
}
