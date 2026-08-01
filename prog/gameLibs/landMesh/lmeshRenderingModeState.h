// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <landMesh/lmeshRenderer.h>

// One process-wide mode cache: the shadervar is global GPU state, so caching it per renderer
// instance would skip needed writes as soon as a second writer appears.
extern LMeshRenderingMode lmesh_ambient_rendering_mode;
