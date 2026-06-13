//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>

// Shared land-mesh render-control flags. Kept in a neutral header (not
// lmeshRenderer.h) so the virtual-texture / clipmap code -- and the future
// LandVtexRenderer -- can read/poke them without depending on LandMeshRenderer.
// LandMeshRenderer re-exports these as compat aliases (see lmeshRenderer.h).
namespace landmesh
{
extern uint32_t lmesh_render_flags;

enum LMeshRenderFlags
{
  RENDER_DECALS = 1,
  RENDER_COMBINED = 2,
  RENDER_LANDMESH = 4,
  RENDER_HEIGHTMAP = 8
};
} // namespace landmesh
