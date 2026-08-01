// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// Reading the per-cell weight textures that level binaries were exported with.
// Two paths, one per way of getting at their texels: the CPU decodes them where
// DXT1 exists to pack with, the GPU draws them where it does not. Both are here
// only for data older than the atlas format and go away once it is re-exported.

#include <generic/dag_span.h>
#include <generic/dag_tab.h>

struct LandWeightAtlas;

// no tex flags: a rendered atlas can carry no system copy, whatever the owner wants
LandWeightAtlas *render_land_weight_atlas(dag::Span<Tab<uint8_t>> records, int cells_x, int cells_y, int tex_size, int elem_size);
