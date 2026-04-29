// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <heightmap/dag_lodsRiverMasks.h>

// Build LOD patch bitmask mipchain from critical path.
// path           - full-res bitmask from extract_critical_path, must be at least ceil(w*w/32) uint32_t
// w              - full resolution width (square grid)
// patch_dim_bits - log2 of patch dimension (e.g. 3 for 8x8 patches)
// out            - output LodsRiverMasks struct (finestLod, masks)
void hmap_sdf_build_lod_masks(const uint32_t *path, int w, int patch_dim_bits, LodsRiverMasks &out);
