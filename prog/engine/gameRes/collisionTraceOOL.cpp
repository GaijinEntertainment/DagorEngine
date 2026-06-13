// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <vecmath/dag_vecMath.h>
#include <math/dag_Point2.h>
#include "collisionTraceOOL.h"

namespace collision_blas
{

// rayBLAS_Free invokes the HitCb through a const ref after rayLeaf_SoA lowered r.t to this leaf's
// hit, so the hit state is mutable. AnyHit is a compile-time switch so the early-exit collapses
// per specialization.
template <bool AnyHit>
struct FilteredHitCb
{
  LeafAccept accept;
  void *ctx;
  mutable float acceptedT;
  mutable int bestDataOfs; // 0 = no accepted hit yet (a real leaf body offset is always > 0)
  mutable int bestSubTri;
  bool operator()(RayData &r, int data_ofs) const
  {
    if (!accept(ctx, data_ofs))
    {
      r.t = acceptedT; // reject: undo this leaf's prune so a farther accepted hit still wins
      return false;
    }
    acceptedT = r.t;
    bestDataOfs = data_ofs;
    bestSubTri = r.bestSubTri;
    return AnyHit; // closest-hit keeps walking the pruned tree; any-hit stops at the first accept
  }
};

template <bool CullCCW, bool AnyHit>
static inline bool traceFiltered(RayData &r, int start_offset, int blas_size, LeafAccept accept, void *ctx, int &out_data_ofs,
  int &out_sub_tri)
{
  FilteredHitCb<AnyHit> cb{accept, ctx, r.t, 0, 0};
  rayBLAS_Free<CullCCW>(r, start_offset, blas_size, cb);
  out_data_ofs = cb.bestDataOfs;
  out_sub_tri = cb.bestSubTri;
  // FilteredHitCb::operator() mutates bestDataOfs through the const-ref cb during rayBLAS_Free; PVS
  // misses that callback mutation and wrongly flags this comparison as always-false.
  return cb.bestDataOfs != 0; //-V547
}

bool rayBLASClosestFilteredOOL(RayData &r, int s, int sz, LeafAccept a, void *ctx, int &od, int &ost)
{
  return traceFiltered<false, false>(r, s, sz, a, ctx, od, ost);
}
bool rayBLASClosestFilteredOOLCullCCW(RayData &r, int s, int sz, LeafAccept a, void *ctx, int &od, int &ost)
{
  return traceFiltered<true, false>(r, s, sz, a, ctx, od, ost);
}
bool rayBLASAnyHitFilteredOOL(RayData &r, int s, int sz, LeafAccept a, void *ctx, int &od, int &ost)
{
  return traceFiltered<false, true>(r, s, sz, a, ctx, od, ost);
}
bool rayBLASAnyHitFilteredOOLCullCCW(RayData &r, int s, int sz, LeafAccept a, void *ctx, int &od, int &ost)
{
  return traceFiltered<true, true>(r, s, sz, a, ctx, od, ost);
}

} // namespace collision_blas
