//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// Per-quad strip leaf encoding, shared by the engine double-quad leaf writer (quadBLASBuilder.cpp) and
// the dagRayBench-only single-quad writer (singleQuadBlasWriter.h). Matches the leaf decode in
// dag_swBLAS_ray.h / swBLAS.dshl: base = apex vertex index, o1/o2/o3 = SIGNED vertex-index offsets to
// the other three verts, flip = triB winding. valid = offsets fit the signed 13-bit fields.

#include <daBVH/dag_quadBLASBuilder.h> // QuadPrim
#include <daBVH/swBLASLeafDefs.hlsli>  // QUAD_* field layout
#include <math/dag_mathBase.h>         // max

namespace build_bvh
{

struct QEnc
{
  uint32_t base;
  int o1, o2, o3;
  bool flip, valid;
};

inline int alng(int x) { return x < 0 ? -x : x; }
inline bool offFits(int o) { return o >= QUAD_O_MIN && o <= QUAD_O_MAX; }
inline int offClamp(int o) { return o < QUAD_O_MIN ? QUAD_O_MIN : (o > QUAD_O_MAX ? QUAD_O_MAX : o); }

inline QEnc encodeQuad(const QuadPrim &p)
{
  QEnc e = {0, 0, 0, 0, false, false};
  if (p.isSingle())
  {
    // Triangle (v0,v1,v2): pick a winding-preserving cyclic rotation, preferring one whose signed
    // offsets both fit (offFits is asymmetric, so the tightest-spread rotation may overflow), then the
    // smaller spread within the same fit class.
    const int t[3] = {(int)p.v0(), (int)p.v1(), (int)p.v2()};
    int bestR = 0;
    int bestS = 0x7fffffff;
    bool bestFits = false;
    for (int r = 0; r < 3; ++r)
    {
      const int o1 = t[(r + 1) % 3] - t[r], o2 = t[(r + 2) % 3] - t[r];
      const bool f = o1 >= QUAD_O_MIN && o1 <= QUAD_O_MAX && o2 >= QUAD_O_MIN && o2 <= QUAD_O_MAX;
      const int s = max(alng(o1), alng(o2));
      if ((f && !bestFits) || (f == bestFits && s < bestS))
      {
        bestFits = f;
        bestS = s;
        bestR = r;
      }
    }
    e.base = (uint32_t)t[bestR];
    e.o1 = int(t[(bestR + 1) % 3] - t[bestR]);
    e.o2 = int(t[(bestR + 2) % 3] - t[bestR]);
    e.o3 = e.o2; // single sentinel (o3 == o2)
    e.valid = offFits(e.o1) && offFits(e.o2);
    return e;
  }
  // Quad: v0,v2 = shared edge, v1/v3 = the two apexes. Relabel to strip n = (apex, sharedEdge.., otherApex)
  // and take the apex with the smaller offset spread. flipSecond is derived from triB's source winding
  // (p.bFwd: triB runs the shared edge v0->v2), NOT forced: the leaf has a flip bit only for each quad's
  // 2nd sub-tri, so when the chosen apex is triB's (qn) its winding fixes the shared-edge order (o1,o2)
  // instead. Forcing flip would reverse an inconsistently-wound triB. Shadows ignore flip.
  const int v0 = p.v0(), v1 = p.v1(), v2 = p.v2(), v3 = p.v3();
  const bool bFwd = p.bFwd;
  const int pn[4] = {v1, v2, v0, v3}; // apex = v1: sub-tri 0 = triA, (o1,o2) fixed by triA's winding
  int qn[4];                          // apex = v3: sub-tri 0 = triB, shared-edge order follows bFwd
  if (bFwd)
  {
    qn[0] = v3, qn[1] = v0, qn[2] = v2, qn[3] = v1;
  }
  else
  {
    qn[0] = v3, qn[1] = v2, qn[2] = v0, qn[3] = v1;
  }
  auto spread = [](const int n[4]) { return max(alng(n[1] - n[0]), max(alng(n[2] - n[0]), alng(n[3] - n[0]))); };
  auto fits = [](const int n[4]) {
    const int o1 = n[1] - n[0], o2 = n[2] - n[0], o3 = n[3] - n[0];
    return o1 >= QUAD_O_MIN && o1 <= QUAD_O_MAX && o2 >= QUAD_O_MIN && o2 <= QUAD_O_MAX && o3 >= QUAD_O_MIN && o3 <= QUAD_O_MAX;
  };
  // Prefer the apex whose three signed offsets all fit, then the tighter spread. offFits is asymmetric
  // ([QUAD_O_MIN, QUAD_O_MAX]); choosing purely by spread magnitude could pick a +QUAD_O_MAX+1 apex that
  // clamps over the other apex that encodes the same quad cleanly.
  const bool pnF = fits(pn), qnF = fits(qn);
  const int *n = (pnF == qnF) ? (spread(pn) <= spread(qn) ? pn : qn) : (pnF ? pn : qn);
  e.base = (uint32_t)n[0];
  e.o1 = int(n[1] - n[0]);
  e.o2 = int(n[2] - n[0]);
  e.o3 = int(n[3] - n[0]);
  e.flip = bFwd;
  e.valid = offFits(e.o1) && offFits(e.o2) && offFits(e.o3);
  return e;
}

// Pack quad A into W0/W1 (offsets + base) and the flipA bit of W2. relBase = apex vertex byte offset
// minus this leaf's W1 (4-aligned, >= 0: verts always follow the tree). Returns true if A overflowed --
// offsets or base clamped to a bounded but wrong read; callers logerr once. Pre-passed meshes never hit this.
inline bool packQuadA(const QEnc &a, int relBase, uint32_t &w0, uint32_t &w1, uint32_t &w2)
{
  bool ovf = !a.valid;
  const uint32_t o1 = (uint32_t)offClamp(a.o1) & QUAD_O_MASK, o2 = (uint32_t)offClamp(a.o2) & QUAD_O_MASK,
                 o3 = (uint32_t)offClamp(a.o3) & QUAD_O_MASK;
  int base = relBase >> QUAD_BASE_ALIGN_SHIFT;
  if ((relBase & 3) != 0 || base < 0 || base >= (1 << QUAD_BASE_BITS))
  {
    ovf = true;
    base = base < 0 ? 0 : (1 << QUAD_BASE_BITS) - 1;
  }
  w0 = o1 | (o2 << QUAD_O2_SHIFT) | ((o3 & QUAD_O3_LO_MASK) << QUAD_O3_LO_SHIFT) | QUAD_LEAF_FLAG;
  w1 = ((uint32_t)base & QUAD_BASE_MASK) | ((o3 >> QUAD_O3_LO_BITS) << QUAD_O3_HI_SHIFT);
  w2 = a.flip ? QUAD_FLIPA_FLAG : 0u;
  return ovf;
}

// Pack quad B into the W2 high bits (deltaB base, o1b, flipB) and W3 (o2b, o3b). aBase is quad A's
// apex vertex index; callers must have ordered B so B.base >= aBase (deltaB is unsigned). Returns true
// on overflow (B invalid or deltaB past QUADB_BASE_MAX); the leaf then drops quad B, so w2hi/w3 read 0.
inline bool packQuadB(const QEnc &b, uint32_t aBase, uint32_t &w2hi, uint32_t &w3)
{
  const uint32_t bB = b.base - aBase;
  if (!b.valid || bB > QUADB_BASE_MAX)
  {
    w2hi = 0;
    w3 = 0;
    return true;
  }
  w2hi = (bB & QUADB_BASE_MASK) | (((uint32_t)b.o1 & QUAD_O_MASK) << QUADB_O1_SHIFT) | (b.flip ? QUAD_FLIPB_FLAG : 0u);
  w3 = ((uint32_t)b.o2 & QUAD_O_MASK) | (((uint32_t)b.o3 & QUAD_O_MASK) << QUADB_O3_SHIFT);
  return false;
}

} // namespace build_bvh
