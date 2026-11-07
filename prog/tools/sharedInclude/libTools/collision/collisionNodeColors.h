// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_e3dColor.h>
#include <util/dag_globDef.h>

// Debug draw colors of collision nodes.
inline constexpr unsigned int collision_node_colors[] = {0xFFFF0000, 0xFF00FF00, 0xFF0000FF, 0xFFFFFF00, 0xFFFF00FF, 0xFF00FFFF,
  0xFF800000, 0xFF008000, 0xFF000080, 0xFF808000, 0xFF800080, 0xFF008080};
inline constexpr unsigned int collision_box_node_color = 0xFFFFFFFF;
inline constexpr unsigned int collision_sphere_node_color = 0xFFFFFF00;
inline constexpr unsigned int collision_capsule_node_color = 0xFFFF00FF;

// index is the position of the drawn object in its own list, for example the node index.
inline E3DCOLOR collision_node_color(int index) { return E3DCOLOR(collision_node_colors[index % countof(collision_node_colors)]); }
