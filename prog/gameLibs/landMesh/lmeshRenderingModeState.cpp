// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "lmeshRenderingModeState.h"

// This TU must reference no d3d/shader/engine symbols: stub-linked tool DLLs that only read
// the mode pull this obj alone instead of lmeshRenderer.obj and its dependency web.
LMeshRenderingMode lmesh_ambient_rendering_mode = LMeshRenderingMode::RENDERING_LANDMESH;

LMeshRenderingMode landmesh::get_rendering_mode_shadervar() { return lmesh_ambient_rendering_mode; }
