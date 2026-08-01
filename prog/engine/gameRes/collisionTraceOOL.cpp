// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <vecmath/dag_vecMath.h>
#include <math/dag_Point2.h>
#include "collisionTraceOOL.h"

namespace collision_blas
{

// The traversal invokes the HitCb through a const ref after rayLeaf_SoA lowered r.t to this leaf's
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
  // Closest-hit walks nearest-first (an early hit prunes far subtrees); any-hit keeps the plain
  // memory-order walk -- it stops at the first accepted hit, so ordering buys nothing there.
  if constexpr (AnyHit)
    rayBLAS_Free<CullCCW>(r, start_offset, blas_size, cb);
  else
    rayBLAS_OrderedFree<CullCCW>(r, start_offset, blas_size, cb);
  out_data_ofs = cb.bestDataOfs;
  out_sub_tri = cb.bestSubTri;
  // FilteredHitCb::operator() mutates bestDataOfs through the const-ref cb during the traversal; PVS
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

// SoA4 twins: same reject-restores-t / any-hit-stops-at-first-accept contract, hit identity is the
// persistent LeafRef the walker hands the callback (0 = none).
template <bool AnyHit>
struct FilteredRefHitCb
{
  LeafAcceptRef accept;
  void *ctx;
  mutable float acceptedT;
  mutable soa4::LeafRef bestRef; // 0 = no accepted hit yet (a valid LeafRef always has bit 31 set)
  mutable int bestSubTri;
  bool operator()(RayData &r, soa4::LeafRef ref) const
  {
    if (!accept(ctx, ref))
    {
      r.t = acceptedT; // reject: undo this leaf's prune so a farther accepted hit still wins
      return false;
    }
    acceptedT = r.t;
    bestRef = ref;
    bestSubTri = r.bestSubTri;
    return AnyHit; // closest-hit keeps walking the pruned tree; any-hit stops at the first accept
  }
};

template <bool CullCCW, bool AnyHit>
static inline bool traceSoa4Filtered(RayData &r, soa4::RootRef root, LeafAcceptRef accept, void *ctx, soa4::LeafRef &out_ref,
  int &out_sub_tri)
{
  FilteredRefHitCb<AnyHit> cb{accept, ctx, r.t, 0, 0};
  // Closest-hit is nearest-first by construction; any-hit tests leaves on the spot, unordered.
  if constexpr (AnyHit)
    soa4::rayAnyHit<CullCCW>(r, root, cb);
  else
    soa4::rayClosest<CullCCW>(r, root, cb);
  out_ref = cb.bestRef;
  out_sub_tri = cb.bestSubTri;
  return cb.bestRef != 0; //-V547 mutated through the const-ref cb during the traversal (as above)
}

bool raySoa4ClosestFilteredOOL(RayData &r, soa4::RootRef root, LeafAcceptRef a, void *ctx, soa4::LeafRef &oref, int &ost)
{
  return traceSoa4Filtered<false, false>(r, root, a, ctx, oref, ost);
}
bool raySoa4ClosestFilteredOOLCullCCW(RayData &r, soa4::RootRef root, LeafAcceptRef a, void *ctx, soa4::LeafRef &oref, int &ost)
{
  return traceSoa4Filtered<true, false>(r, root, a, ctx, oref, ost);
}
bool raySoa4AnyHitFilteredOOL(RayData &r, soa4::RootRef root, LeafAcceptRef a, void *ctx, soa4::LeafRef &oref, int &ost)
{
  return traceSoa4Filtered<false, true>(r, root, a, ctx, oref, ost);
}
bool raySoa4AnyHitFilteredOOLCullCCW(RayData &r, soa4::RootRef root, LeafAcceptRef a, void *ctx, soa4::LeafRef &oref, int &ost)
{
  return traceSoa4Filtered<true, true>(r, root, a, ctx, oref, ost);
}

} // namespace collision_blas
