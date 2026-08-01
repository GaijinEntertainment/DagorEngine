//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// Paranoid validation of opaque SoA4 leaf tokens (what tri_ref carries) against a tree region.
// Every property checked here holds by construction for loader-built trees, and must hold for a
// correctly exported one, so hot decode paths never need these checks. Validation is only needed
// at load time or in an exporter, in paranoid mode (dev builds, DAGOR_DBGLEVEL > 0) -- do not put
// it on shipping decode paths.

#include <daBVH/dag_swBLAS_soa4.h>

namespace soa4
{

// Bounds-validate an opaque leaf token against a SoA4 tree region and resolve its LeafLoc.
// A stale/forged token must fail here, never read out of range: the lane is checked against the
// node's child count and the parent span before any word is read, then that the lane really is a
// leaf, then the body span. The words are read scalar (decodeLeafRef's 16-byte node load
// deliberately over-reads N<4 nodes) so every access stays inside the validated span; the result
// is bit-identical to decodeLeafRef's for any token that passes.
static inline bool validateLeafToken(const uint8_t *tree, uint32_t tree_bytes, uint64_t token, LeafLoc &out)
{
  const LeafRef ref = (LeafRef)token;
  if ((uint64_t)ref != token || !(ref & LEAF_ENTRY_FLAG))
    return false;
  const uint32_t nf = (ref >> LEAF_ENTRY_N_SHIFT) & 3;
  const uint32_t ofs = ref & LEAF_ENTRY_OFS_MASK;
  if (nf == 0) // degenerate whole-BLAS-is-one-leaf root block
  {
    if (ofs + (uint32_t)LEAF_BYTES > tree_bytes)
      return false;
    out.bodyOfs = (int)ofs + 4;
    out.w0 = *(const uint32_t *)(tree + ofs);
    out.isShort = false;
  }
  else
  {
    const uint32_t lane = ref & TAG_MASK;
    const uint32_t n = nf + 1;
    if (lane >= n || ofs + 16u * n > tree_bytes)
      return false; // lane past the node's child count, or node span escapes the tree
    const uint32_t *w = (const uint32_t *)(tree + ofs + 12u * n);
    unsigned leafAll = 0;
    for (uint32_t i = 0; i < n; ++i)
      leafAll |= (w[i] >> 31) << i;
    const unsigned shortMask = (ref >> LEAF_ENTRY_SHORT_SHIFT) & 15u;
    out.bodyOfs = leafBodyOfs((int)ofs, (int)n, leafAll, shortMask, (int)lane);
    out.w0 = w[lane];
    out.isShort = (shortMask >> lane) & 1;
  }
  if (!(out.w0 & QUAD_LEAF_FLAG))
    return false; // lane is an internal child: stale/forged token
  return (uint32_t)out.bodyOfs + (out.isShort ? 4u : 12u) <= tree_bytes;
}

// Whether a decoded leaf actually emits sub-triangle lane 0..3 (0/1 = quad A tri 1/2, 2/3 = quad B
// tri 1/2). Lanes are sparse: a single-tri quad A with a full quad B emits {0, 2, 3}, so a
// tri-count comparison cannot validate a lane.
static inline bool leafEmitsSubTri(const QuadLeafFields &f, uint32_t sub_tri)
{
  switch (sub_tri)
  {
    case 0: return true;
    case 1: return !f.isSingle;
    case 2: return f.hasB;
    case 3: return f.hasB && !f.isSingleB;
    default: return false;
  }
}

// Whole-leaf vertex byte-range validation: every vert21 load fetchLeafTri could make for this
// leaf (both quads, all offsets) must land inside [verts_ofs, region_end). validateLeafToken only
// proves the leaf HEADER sits in the tree; the apex/offsets it encodes are still token-reachable
// data and must not be trusted to address verts.
static inline bool leafVertsInRange(const LeafLoc &l, const QuadLeafFields &f, uint32_t verts_ofs, uint32_t region_end)
{
  const int64_t base = (int64_t)l.bodyOfs + f.relBaseBytes;
  int lo = 0, hi = 0; // vert-index offsets from the quad A apex; 0 covers both apexes
  auto fold = [&](int o) {
    lo = o < lo ? o : lo;
    hi = o > hi ? o : hi;
  };
  fold(f.o1);
  fold(f.o2);
  if (!f.isSingle)
    fold(f.o3);
  if (f.hasB)
  {
    const int dB = (int)f.deltaB;
    fold(dB);
    fold(dB + f.o1b);
    fold(dB + f.o2b);
    if (!f.isSingleB)
      fold(dB + f.o3b);
  }
  return base + (int64_t)lo * 8 >= (int64_t)verts_ofs && base + (int64_t)hi * 8 + 8 <= (int64_t)region_end;
}

} // namespace soa4
