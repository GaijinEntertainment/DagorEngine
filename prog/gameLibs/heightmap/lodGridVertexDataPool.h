// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <heightmap/heightmapCulling.h>

enum
{
  MAX_HW_INSTANCING = 440,
  VDATA_OFS = 3,
  MAX_VDATA = 4
};

// Shared pool of lod grid index buffers keyed by patch dim; refcounted by every
// HeightmapRenderer/SimpleHeightmapRenderer instance via LodGridVertexData::init/close.
extern LodGridVertexData lod_grid_vdata[MAX_VDATA];
