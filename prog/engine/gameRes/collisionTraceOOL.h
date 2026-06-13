// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daBVH/dag_swBLAS_ray.h>

// Out-of-line (own TU, to keep SoA traversal out of the inlined-everywhere trace dispatch's i-cache)
// FILTERED BLAS rayCasts on rayBLAS_Free. 4-wide SoA leaf test with cull mode bound at compile time;
// the type-erased per-leaf `accept` predicate (recovers the leaf's source node, applies the caller's
// node filter) runs ONLY on leaves the SoA test hit (rare), keeping the indirect call off the hot path.
//   - a REJECTED leaf restores r.t so it cannot prune a farther accepted hit (closest-hit gating);
//   - any-hit stops at the first accepted hit, closest-hit keeps walking the pruned tree.
// On a hit: returns true, out_data_ofs = accepted leaf body offset, out_sub_tri = winning sub-tri
// (0 = first/single, 1 = second tri of a quad), r.t = hit distance in the ray's t-parameterization.
// Feed an UNNORMALIZED box-space ray (origin = local*scale+ofs, dir = local_dir*scale) so t ==
// resource-local t (see BlasBoxRay in collisionGameRes.cpp).
namespace collision_blas
{
// (ctx, leaf body offset) -> true if the leaf's source node passes the caller's filter.
using LeafAccept = bool (*)(void *ctx, int data_ofs);

bool rayBLASClosestFilteredOOL(RayData &r, int start_offset, int blas_size, LeafAccept accept, void *ctx, int &out_data_ofs,
  int &out_sub_tri);
bool rayBLASClosestFilteredOOLCullCCW(RayData &r, int start_offset, int blas_size, LeafAccept accept, void *ctx, int &out_data_ofs,
  int &out_sub_tri);
bool rayBLASAnyHitFilteredOOL(RayData &r, int start_offset, int blas_size, LeafAccept accept, void *ctx, int &out_data_ofs,
  int &out_sub_tri);
bool rayBLASAnyHitFilteredOOLCullCCW(RayData &r, int start_offset, int blas_size, LeafAccept accept, void *ctx, int &out_data_ofs,
  int &out_sub_tri);
} // namespace collision_blas
