// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_resPtr.h>

void render_landmesh_to_heightmap(Texture *heightmapTex,
  float heightmap_size,
  const Point2 &center_pos,
  const BBox2 *box,
  const float water_level,
  Point4 &world_to_heightmap);
