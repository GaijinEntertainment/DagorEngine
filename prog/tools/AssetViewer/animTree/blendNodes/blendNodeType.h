// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "../animTreePanelPids.h"
#include <generic/dag_tab.h>
#include <util/dag_string.h>

inline const int ANIM_BLEND_NODE_A2D = PID_ANIM_BLEND_NODES_A2D;
inline const int ANIM_BLEND_NODE_LEAF = PID_ANIM_BLEND_NODES_LEAF;
inline const int ANIM_BLEND_NODE_IRQ = PID_ANIM_BLEND_NODES_IRQ;
inline const int ANIM_BLEND_NODE_INCLUDE = PID_ANIM_BLEND_NODES_INCLUDE;

inline const Tab<String> blend_node_types{String("single"), String("continuous"), String("still"), String("parametric")};
enum class BlendNodeType
{
  IRQ = -4,
  A2D = -3,
  INCLUDE = -2,
  UNSET = -1,
  SINGLE,
  CONTINUOUS,
  STILL,
  PARAMETRIC,
  SIZE
};

const char *get_blend_node_icon_name(BlendNodeType type);
