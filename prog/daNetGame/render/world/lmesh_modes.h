// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

enum class LMeshRenderingMode : int
{
  RENDERING_WITH_SPLATTING = -1,
  RENDERING_LANDMESH = 0,
  RENDERING_CLIPMAP = 1,
  OBSOLETE_RENDERING_SPOT,
  GRASS_COLOR = 3,
  GRASS_MASK = 4,
  OBSOLETE_SPOT_TO_GRASS_MASK,
  RENDERING_HEIGHTMAP = 6,
  RENDERING_DEPTH = 7,
  RENDERING_SHADOW = 8,
  RENDERING_REFLECTION = 9,
  RENDERING_FEEDBACK = 10,
  LMESH_MAX
};

extern LMeshRenderingMode lmesh_rendering_mode;

LMeshRenderingMode set_lmesh_rendering_mode(LMeshRenderingMode mode);
