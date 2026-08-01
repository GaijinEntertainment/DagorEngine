// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daBVH/dag_swBLAS_ray.h>
#include <daBVH/dag_swBLAS_soa4.h>

// Out-of-line (own TU, to keep SoA traversal out of the inlined-everywhere trace dispatch's i-cache)
// FILTERED BLAS rayCasts: closest-hit on the nearest-first rayBLAS_OrderedFree, any-hit on the plain
// memory-order rayBLAS_Free. 4-wide SoA leaf test with cull mode bound at compile time;
// the type-erased per-leaf `accept` predicate (recovers the leaf's source node, applies the caller's
// node filter) runs ONLY on leaves the SoA test hit (rare), keeping the indirect call off the hot path.
//   - a REJECTED leaf restores r.t so it cannot prune a farther accepted hit (closest-hit gating);
//   - any-hit stops at the first accepted hit, closest-hit keeps walking the pruned tree.
// On a hit: returns true, out_data_ofs = accepted leaf body offset, out_sub_tri = winning sub-tri
// 0..3 (0/1 = quad A tri 1/2, 2/3 = quad B tri 1/2), r.t = hit distance in the ray's t-parameterization.
// Feed an UNNORMALIZED box-space ray (origin = local*scale+ofs, dir = local_dir*scale) so t ==
// resource-local t (see BlasBoxRay in collisionGameRes.cpp).
namespace collision_blas
{
// (ctx, leaf body offset) -> true if the leaf's source node passes the caller's filter.
using LeafAccept = bool (*)(void *ctx, int data_ofs);

// The stackless entries have no caller since CollisionResource switched to SoA4 storage; they are
// kept for the planned unified-memory configuration (collision traversal over the GPU stackless
// buffer), where they become the dispatch target again.
bool rayBLASClosestFilteredOOL(RayData &r, int start_offset, int blas_size, LeafAccept accept, void *ctx, int &out_data_ofs,
  int &out_sub_tri);
bool rayBLASClosestFilteredOOLCullCCW(RayData &r, int start_offset, int blas_size, LeafAccept accept, void *ctx, int &out_data_ofs,
  int &out_sub_tri);
bool rayBLASAnyHitFilteredOOL(RayData &r, int start_offset, int blas_size, LeafAccept accept, void *ctx, int &out_data_ofs,
  int &out_sub_tri);
bool rayBLASAnyHitFilteredOOLCullCCW(RayData &r, int start_offset, int blas_size, LeafAccept accept, void *ctx, int &out_data_ofs,
  int &out_sub_tri);

// SoA4 twins of the filtered entries above, for grids/chunks that store the SoA4 CPU layout. Same
// ray/t contract; the leaf filter and the hit are identified by the persistent soa4::LeafRef
// instead of a body offset (0 = no accepted hit; a valid ref is never 0).
using LeafAcceptRef = bool (*)(void *ctx, soa4::LeafRef ref);

bool raySoa4ClosestFilteredOOL(RayData &r, soa4::RootRef root, LeafAcceptRef accept, void *ctx, soa4::LeafRef &out_ref,
  int &out_sub_tri);
bool raySoa4ClosestFilteredOOLCullCCW(RayData &r, soa4::RootRef root, LeafAcceptRef accept, void *ctx, soa4::LeafRef &out_ref,
  int &out_sub_tri);
bool raySoa4AnyHitFilteredOOL(RayData &r, soa4::RootRef root, LeafAcceptRef accept, void *ctx, soa4::LeafRef &out_ref,
  int &out_sub_tri);
bool raySoa4AnyHitFilteredOOLCullCCW(RayData &r, soa4::RootRef root, LeafAcceptRef accept, void *ctx, soa4::LeafRef &out_ref,
  int &out_sub_tri);
} // namespace collision_blas
