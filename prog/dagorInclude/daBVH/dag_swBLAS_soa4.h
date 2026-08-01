//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// SoA4 CPU BLAS layout and its ray traversals.
//
// The stackless layout (dag_swBLAS_ray.h, also the GPU upload format) stores ONE box per node
// ("[3x(min|max<<16)][skip]") with children laid consecutively and a skip word jumping a whole
// subtree. SoA4 instead stores, per internal node, its N (2..4) CHILD boxes already transposed to
// SoA plus one child WORD per child, then the leaf children's bodies inline -- 16*N + 12*fullLK +
// 4*shortLK bytes:
//   uint16 minX[N], minY[N], minZ[N], maxX[N], maxY[N], maxZ[N];  // 12*N B: the N child boxes (SoA)
//   uint32 word[N];  // leaf child -> its W0 verbatim; internal child -> child node ref (see below)
//   bodies[LK];      // leaf bodies in child-lane order: full = 12B (W1 W2 W3), short = 4B
// The two word cases cannot collide: a leaf's W0 always has bit 31 set (QUAD_LEAF_FLAG) while node
// offsets stay below 32 MB (bit 31 clear). An internal child ref packs, besides the offset,
// everything needed to decode the TARGET node before reading it:
//   bits [1:0]   = N-1 of the target node (2/3/4 children; 0 is reserved for the degenerate
//                  whole-BLAS-is-one-leaf root, a lone 16-byte [W0 W1 W2 W3] block)
//   bits [25:2]  = target node byte offset (4-aligned)
//   bits [29:26] = target node's SHORT-leaf lane mask
// SHORT leaf: a single-quad leaf (no quad B) stores a 4-byte body instead of 12 -- its source W2/W3
// carry exactly one payload bit (W2 = flipA<<29, W3 = 0; see packQuadA/writeDoubleQuadLeaf), so the
// body is just W1 with bit 23 repurposed for flipA (apex base truncated to 23 bits = 32 MB; gated
// at conversion). Quad-B fields are implied zero, which the shared hasB sentinel (o1B == o2B)
// already reads as "no quad B".
// A full leaf costs 12 (box) + 4 (W0 word) + 12 (body) = 28 bytes == stackless; a short leaf costs
// 20; an internal child 16 == stackless -- so the SoA4 tree is stackless size minus 8 bytes per
// singleton. The buffer layout is [tree][pad to 8][vert21 verts], same as stackless; the vert
// region is byte-identical, so vertex-index attribution (blasNodeRanges etc.) is layout-neutral.
//
// Why: the SIMD slab test wants child boxes in SoA in one cache line. The stackless path reads N
// scattered 16B nodes and transposes at runtime; SoA4 removes both. A leaf child needs no pointer
// chase: its W0 is the child word and the body sits behind the words. Any-hit tests leaf children
// on the spot; closest-hit defers them distance-ordered via self-describing LeafRef entries.
// Converters to/from the stackless layout: dag_swBLAS_soa4Convert.h (round trip byte-identical).

#include <daBVH/dag_swBLASRootRef.h>
#include <daBVH/dag_swBLAS_leaf.h>
#include <daBVH/swBVHDefine.hlsli> // BVH_MAX_BLAS_DEPTH
#include <math/dag_bits.h>

namespace soa4
{
static constexpr int LEAF_BYTES = 16;    // degenerate whole-BLAS-is-one-leaf root block: [W0 W1 W2 W3]
static constexpr uint32_t TAG_MASK = 3u; // child word low 2 bits: 1/2/3 = internal node w/ 2/3/4 kids (0 = degenerate root leaf)
static constexpr uint32_t PTR_OFS_MASK = 0x03FFFFFCu; // internal word bits [25:2]: target node byte offset
static constexpr uint32_t PTR_SHORT_SHIFT = 26;       // internal word bits [29:26]: target node's short-leaf lane mask
static constexpr uint32_t SHORT_W1_FLIP = 1u << 23;   // short (4B) leaf body: W1 bit 23 = flipA, base truncated to 23 bits

// Max SoA4 tree depth a converter or deserializer may accept (a real SAH BLAS never nests deeper).
// It sizes rayClosest/rayAnyHit's fixed traversal stacks (4 slots per level, comfortably above the
// <= 3 pushes a 4-wide node can make, with no overflow fallback), so a deeper tree must be REFUSED
// at conversion/deserialize, never walked.
static constexpr int MAX_TREE_DEPTH = BVH_MAX_BLAS_DEPTH;

// RootRef lives in dag_swBLASRootRef.h so root-storing consumers (rasterizer task structs) need
// not parse this header.

// LeafRef: the persistent, self-describing reference to one leaf of a SoA4 tree -- bit 31 set (a
// node offset never has it), the parent's short-leaf lane mask in bits [30:27], the parent's child
// count - 1 in bits [26:25], the 4-aligned parent node offset in [24:2], and the child lane in the
// free low 2 bits. Everything a later decode needs (W0 word, body offset, short flag) re-derives
// from the parent node, so the ref stays valid as long as the tree buffer does. The reserved
// count-field value 0 marks the degenerate whole-BLAS-is-one-leaf root block, whose offset then
// sits in bits [24:2] with lane 0. 0 is never a valid ref ("no hit").
// The closest-hit traversal uses the same encoding for its deferred-leaf stack entries, so the ref
// handed to a hit callback is exactly the entry that located the leaf.
typedef uint32_t LeafRef;
static constexpr uint32_t LEAF_ENTRY_FLAG = 0x80000000u;
static constexpr uint32_t LEAF_ENTRY_SHORT_SHIFT = 27;
static constexpr uint32_t LEAF_ENTRY_N_SHIFT = 25;
static constexpr uint32_t LEAF_ENTRY_OFS_MASK = 0x01FFFFFCu;

static __forceinline LeafRef makeLeafRef(uint32_t node_ptr, int lane)
{
  return LEAF_ENTRY_FLAG | (((node_ptr >> PTR_SHORT_SHIFT) & 15u) << LEAF_ENTRY_SHORT_SHIFT) |
         ((node_ptr & TAG_MASK) << LEAF_ENTRY_N_SHIFT) | (node_ptr & LEAF_ENTRY_OFS_MASK) | (uint32_t)lane;
}
static __forceinline LeafRef makeRootLeafRef(uint32_t block_ofs) { return LEAF_ENTRY_FLAG | block_ofs; } // count field 0

// Load 4 contiguous uint16 (one SoA box-axis array) as float lanes {p0,p1,p2,p3}. For an N<4 node the top
// 4-N lanes read into the next axis array (harmless: traversal only uses lanes [0,N), and every node's
// arrays stay inside the node).
static __forceinline vec4f loadU16x4f(const uint8_t *p)
{
  vec4i raw = v_ldui_half((const int *)p); // low 64 bits = {p0|p1<<16, p2|p3<<16, _, _}
  vec4i lo = v_andi(raw, v_splatsi(0xFFFF));
  vec4i hi = v_srli(raw, 16);
  return v_cvt_vec4f(v_interleave_lo_i32(lo, hi)); // {lo0,hi0,lo1,hi1} = {p0,p1,p2,p3}
}

// Per-N lane kill masks for the 4-wide SoA node tests: lanes >= N read into the next axis array
// (harmless bytes inside the node) and must be forced dead. Indexed by [N - 2].
alignas(16) inline constexpr unsigned LANE_KILL[3][4] = {{0, 0, ~0u, ~0u}, {0, 0, 0, ~0u}, {0, 0, 0, 0}};
alignas(16) inline constexpr int LANE_IDX[4] = {0, 1, 2, 3};

// Inline body offset of leaf lane `lane` in node `node_ofs`: bodies are lane-ordered, 4 bytes for
// short leaves and 12 for full ones.
static __forceinline int leafBodyOfs(int node_ofs, int n, unsigned leaf_all, unsigned short_mask, int lane)
{
  const unsigned below = leaf_all & ((1u << lane) - 1);
  return node_ofs + 16 * n + 4 * (int)__popcount(below) + 8 * (int)__popcount(below & ~short_mask);
}

// Resolved location of the leaf a LeafRef points at (one node read; no vertex decode).
struct LeafLoc
{
  int bodyOfs;  // inline body byte offset in the tree buffer
  uint32_t w0;  // the leaf's W0 (skip word bit layout, bit 31 set)
  bool isShort; // 4-byte singleton body (W1 with SHORT_W1_FLIP) instead of the 12-byte W1 W2 W3
};

static __forceinline LeafLoc decodeLeafRef(const uint8_t *data, LeafRef ref)
{
  LeafLoc l;
  const int Nf = (int)((ref >> LEAF_ENTRY_N_SHIFT) & 3);
  if (Nf == 0) // degenerate whole-BLAS-is-one-leaf root block
  {
    const int block = (int)(ref & LEAF_ENTRY_OFS_MASK);
    l.bodyOfs = block + 4;
    l.w0 = *(const uint32_t *)(data + block);
    l.isShort = false;
    return l;
  }
  const int parent = (int)(ref & LEAF_ENTRY_OFS_MASK);
  const int lane = (int)(ref & TAG_MASK);
  const int N = Nf + 1;
  const unsigned shortMask = (ref >> LEAF_ENTRY_SHORT_SHIFT) & 15u;
  const uint32_t *w = (const uint32_t *)(data + parent + 12 * N);
  const unsigned leafAll = (unsigned)v_signmask(v_cast_vec4f(v_ldui((const int *)w))) & ((1u << N) - 1);
  l.bodyOfs = leafBodyOfs(parent, N, leafAll, shortMask, lane);
  l.w0 = w[lane];
  l.isShort = (shortMask >> lane) & 1;
  return l;
}

// Decoded bit fields of the referenced leaf, via the shared layout authority. A short body
// synthesizes the quad-B-free fields (W2 = flipA, W3 = 0) the same way its writer implied them.
static __forceinline QuadLeafFields leafFields(const uint8_t *data, const LeafLoc &l)
{
  if (!l.isShort)
  {
    const uint32_t *b = (const uint32_t *)(data + l.bodyOfs);
    return decodeQuadLeafFields(l.w0, b[0], b[1], b[2]);
  }
  const uint32_t w1raw = *(const uint32_t *)(data + l.bodyOfs);
  return decodeQuadLeafFields(l.w0, w1raw & ~SHORT_W1_FLIP, (w1raw & SHORT_W1_FLIP) ? QUAD_FLIPA_FLAG : 0u, 0u);
}

// Fields-based twin of swblas_rayLeaf_SoA for short (4-byte) bodies: the quad-B-free fields are
// synthesized by the caller; the apex base is resolved against body_ofs exactly like the full decode.
template <bool CullCCW>
static __forceinline bool rayLeafFields_SoA(RayData &r, int body_ofs, const QuadLeafFields &f)
{
  const uint8_t *data = r.data;
  const int baseA = body_ofs + (int)f.relBaseBytes;
  vec4f xs, ys, zs;
  swblas_unpack4SoA<8>(data, baseA, f.o1, f.o2, f.o3, xs, ys, zs);
  vec4f c0x, c0y, c0z, c1x, c1y, c1z, c2x, c2y, c2z;
  swblas_quadCorners2(f.flipSecond, xs, ys, zs, c0x, c0y, c0z, c1x, c1y, c1z, c2x, c2y, c2z);
  // quadCorners2 scratch lanes 2,3 hold degenerate (no-flip) or lane-0/1-duplicate (flip) triangles;
  // degenerates self-reject in rayTriangle4_SoA and duplicates return the same t, so the min reduce is
  // unaffected either way (bestSubTri maps 2,3 back to 0,1 below).
  vec4f us, vs;
  vec4f ts = rayTriangle4_SoA<CullCCW>(r.rayOrigin, r.rayDir, c0x, c0y, c0z, c1x, c1y, c1z, c2x, c2y, c2z, us, vs);
  // Branchless best-lane pick, as in swblas_rayLeaf_SoA (lowest equal lane == scalar < tie-break;
  // flip-case duplicate scratch lanes 2,3 carry the same t as 0,1 and lose the tie by lane order).
  vec4f m2 = v_min(ts, v_perm_zwxy(ts));
  vec4f tMin = v_min(m2, v_perm_yzwx(m2));
  if (v_extract_x(tMin) >= r.t)
    return false;
  const int bestLane = (int)__bsf_unsafe((unsigned)v_truemask(v_cmp_eq(ts, tMin)));
  alignas(16) float uArr[4], vArr[4];
  v_st(uArr, us);
  v_st(vArr, vs);
  r.t = v_extract_x(tMin);
  r.bCoord.x = uArr[bestLane];
  r.bCoord.y = vArr[bestLane];
  r.bestSubTri = (int8_t)(bestLane & 1); // lanes 2,3 are lane 0,1 duplicates
  return true;
}

// Intersect leaf lane data at body_ofs: full body -> the shared production decode; short body ->
// synthesize the quad-B-free fields from W0 + the 4-byte W1 (bit 23 = flipA) and run the same math.
template <bool CullCCW>
static __forceinline bool rayLeafAt(RayData &r, int body_ofs, uint32_t w0, bool is_short)
{
  if (!is_short)
    return swblas_rayLeaf_SoA<CullCCW, 8>(r, body_ofs, w0);
  const uint32_t w1raw = *(const uint32_t *)(r.data + body_ofs);
  const QuadLeafFields f = decodeQuadLeafFields(w0, w1raw & ~SHORT_W1_FLIP, (w1raw & SHORT_W1_FLIP) ? QUAD_FLIPA_FLAG : 0u, 0u);
  return rayLeafFields_SoA<CullCCW>(r, body_ofs, f);
}

// One decoded internal node's reference data: geometry pointer, lane count N (2..4 from the pointer
// tag) and the child words (bit 31 set = leaf child, the word is its W0; clear = tagged child ref).
// The 16B word load may read past an N<4 node's words -- always inside the buffer (verts follow the
// tree); lanes >= N never use those bytes.
struct NodeRef
{
  const uint8_t *n; // node base
  int N, s2;        // child count and SoA axis-array stride; axis k at n + k*s2, child words at n + 6*s2
  vec4i wv;
  const uint32_t *w() const { return (const uint32_t *)(n + 6 * s2); }
  unsigned leafMask() const { return (unsigned)v_signmask(v_cast_vec4f(wv)) & ((1u << N) - 1); }
  __forceinline NodeRef(const uint8_t *data, uint32_t ptr) : n(data + (ptr & PTR_OFS_MASK)), N((int)(ptr & TAG_MASK) + 1), s2(N * 2)
  {
    wv = v_ldui((const int *)(n + 6 * s2));
  }
};

// The reference data plus the N child boxes as six SoA float-lane arrays for the 4-wide box tests;
// per-lane scalar-box consumers (iterateLeafRefsNode via laneBox) use NodeRef and skip these loads.
struct NodeSoA : NodeRef
{
  vec4f mnx, mny, mnz, mxx, mxy, mxz;
  __forceinline NodeSoA(const uint8_t *data, uint32_t ptr) : NodeRef(data, ptr)
  {
    mnx = loadU16x4f(n + 0 * s2), mny = loadU16x4f(n + 1 * s2), mnz = loadU16x4f(n + 2 * s2);
    mxx = loadU16x4f(n + 3 * s2), mxy = loadU16x4f(n + 4 * s2), mxz = loadU16x4f(n + 5 * s2);
  }
};

// Default closest-hit callback: remember the hit's LeafRef in bestTriOffset (as its raw 32-bit word;
// a valid ref always has bit 31 set, so "no hit" stays encodable as 0) and keep walking.
struct BestRefCb
{
  bool operator()(RayData &r, LeafRef ref) const
  {
    r.bestTriOffset = (int)ref;
    return false;
  }
};

// Default any-hit callback: the first hit within r.t is the blocker, stop the walk.
struct StopOnHitCb
{
  bool operator()(RayData &r, LeafRef ref) const
  {
    r.bestTriOffset = (int)ref;
    return true;
  }
};

// Closest-hit over SoA4: load the node's N SoA child boxes (no transpose), one vector slab test, sort the
// survivors near->far with the flat branchless network. Descend-first: the nearest surviving child
// becomes the current node directly (no push/pop round trip); only the 2nd..Nth survivors are stacked.
// Deferred culling compares whole packed keys against tKey = pack(r.t, 0), one 64-bit compare per pop
// (dist == r.t sorts >= tKey, so the boundary is culled exactly like the float compare did).
// N (2..4) comes from the pointer's tag.
// HitCb contract matches the stackless walkers': cb(r, ref) returns true to stop the walk; a callback
// that rejects a hit must restore r.t before returning false. The walk itself is the ray traversal
// over r.data (the SoA4 buffer); r.bestTriOffset must be 0 on entry.
template <bool CullCCW = false, class HitCb = BestRefCb>
static bool rayClosest(RayData &r, RootRef root, const HitCb &cb = HitCb())
{
  const uint8_t *data = r.data;
  const vec4f ox = v_splat_x(r.rayOrigin), oy = v_splat_y(r.rayOrigin), oz = v_splat_z(r.rayOrigin);
  const vec4f ix = v_splat_x(r.rayDirInv), iy = v_splat_y(r.rayDirInv), iz = v_splat_z(r.rayDirInv);
  uint64_t stack[MAX_TREE_DEPTH * 4];
  int sp = 0;

  uint64_t tKey = swblas_packChildKey(r.t, 0);
  vec4f rtV = v_splats(r.t);                       // slab-test clamp; refreshed only when a hit shrinks r.t
  uint64_t cur = swblas_packChildKey(0.f, root.v); // seed with the root (also handles a root that is itself a leaf)
  for (;;)
  {
    if (cur < tKey) // deferred cull: a closer hit may have landed after this node was pushed
    {
      const uint32_t ptr = (uint32_t)cur;
      if (ptr & LEAF_ENTRY_FLAG) // deferred leaf: re-derive W0 and the inline body from the parent node
      {
        const LeafLoc l = decodeLeafRef(data, ptr);
        if (rayLeafAt<CullCCW>(r, l.bodyOfs, l.w0, l.isShort))
        {
          if (cb(r, (LeafRef)ptr))
            return true;
          tKey = swblas_packChildKey(r.t, 0); // r.t shrank
          rtV = v_splats(r.t);
        }
      }
      else if (ptr & TAG_MASK) // internal node
      {
        const NodeSoA nd(data, ptr);
        const vec4f t0x = v_mul(v_sub(nd.mnx, ox), ix), t1x = v_mul(v_sub(nd.mxx, ox), ix);
        const vec4f t0y = v_mul(v_sub(nd.mny, oy), iy), t1y = v_mul(v_sub(nd.mxy, oy), iy);
        const vec4f t0z = v_mul(v_sub(nd.mnz, oz), iz), t1z = v_mul(v_sub(nd.mxz, oz), iz);
        const vec4f tEnter = v_max(v_max(v_min(t0x, t1x), v_min(t0y, t1y)), v_max(v_min(t0z, t1z), v_zero()));
        const vec4f tExit = v_min(v_min(v_max(t0x, t1x), v_max(t0y, t1y)), v_min(v_max(t0z, t1z), rtV));
        // No operand can be NaN here (uint16 boxes and v_rcp_safe dirInv are finite), so cmp_gt == miss.
        const vec4i kill = v_ori(v_cast_vec4i(v_cmp_gt(tEnter, tExit)), v_ldi((const int *)LANE_KILL[nd.N - 2]));
        // Every surviving child -- leaf or internal -- goes through one distance-sorted key path; a leaf
        // lane's word (its W0) is swapped for its LeafRef so the pop can locate it again.
        // Dead lanes (slab miss or lane >= N) are OR'd to ~0ull so the sort sinks them.
        const vec4i leafEntryV = v_ori(v_splatsi((int)makeLeafRef(ptr, 0)), v_ldi(LANE_IDX));
        const vec4i kb = v_ori(v_cast_vec4i(tEnter), kill);
        const vec4i co = v_ori(v_btseli(nd.wv, leafEntryV, v_srai(nd.wv, 31)), kill);
        alignas(16) uint64_t k4[4];
        v_sti((int *)&k4[0], v_interleave_lo_i32(co, kb));
        v_sti((int *)&k4[2], v_interleave_hi_i32(co, kb));
        const unsigned live = ~(unsigned)v_truemask(v_cast_vec4f(kill)) & 15u;
        const int nh = __popcount(live);
        if (nh == 1) // single survivor: its lane is known from the mask, no sort or stack traffic
        {
          cur = k4[__bsf_unsafe(live)];
          continue;
        }
        if (nh)
        {
          swblas_sort4net(k4[0], k4[1], k4[2], k4[3]);
          // <= 4 children per node and depth <= MAX_TREE_DEPTH (converter-enforced) keep sp within the 4/level stack.
          G_ASSERT(sp + 3 <= (int)(sizeof(stack) / sizeof(stack[0])));
          for (int i = nh - 1; i >= 1; --i) // push far-first so the nearest of the remainder pops first
            stack[sp++] = k4[i];
          cur = k4[0]; // nearest surviving child: descend without stack traffic
          continue;
        }
      }
      else // degenerate whole-BLAS-is-one-leaf root block
      {
        const int leaf = (int)(ptr & PTR_OFS_MASK);
        if (swblas_rayLeaf_SoA<CullCCW, 8>(r, leaf + 4, *(const uint32_t *)(data + leaf)))
        {
          if (cb(r, makeRootLeafRef(ptr & PTR_OFS_MASK)))
            return true;
          tKey = swblas_packChildKey(r.t, 0); // r.t shrank
          rtV = v_splats(r.t);
        }
      }
    }
    if (!sp)
      break;
    cur = stack[--sp];
  }
  return r.bestTriOffset != 0;
}

// Any-hit over SoA4: no sort (order is irrelevant for any-hit), leaf children are tested on the spot.
// Descend-first: the first surviving child becomes the current node; the rest are stacked. Hit lanes
// come from one signmask instead of a stored mask array. N (2..4) is the pointer tag.
// The default callback stops at the first hit within r.t (shadow blocker). A filtering callback may
// return false to reject a hit (restoring r.t) and the walk continues, same contract as the stackless
// rayBLAS_Free. r.bestTriOffset must be 0 on entry; returns true only when a callback accepted a hit.
template <bool CullCCW = false, class HitCb = StopOnHitCb>
static bool rayAnyHit(RayData &r, RootRef root, const HitCb &cb = HitCb())
{
  const uint8_t *data = r.data;
  const vec4f ox = v_splat_x(r.rayOrigin), oy = v_splat_y(r.rayOrigin), oz = v_splat_z(r.rayOrigin);
  const vec4f ix = v_splat_x(r.rayDirInv), iy = v_splat_y(r.rayDirInv), iz = v_splat_z(r.rayDirInv);
  int stack[MAX_TREE_DEPTH * 4];
  int sp = 0;

  uint32_t cur = (uint32_t)root.v;
  for (;;)
  {
    if (cur & TAG_MASK) // internal node
    {
      const NodeSoA nd(data, cur);
      const uint32_t *w = nd.w();
      const vec4f t0x = v_mul(v_sub(nd.mnx, ox), ix), t1x = v_mul(v_sub(nd.mxx, ox), ix);
      const vec4f t0y = v_mul(v_sub(nd.mny, oy), iy), t1y = v_mul(v_sub(nd.mxy, oy), iy);
      const vec4f t0z = v_mul(v_sub(nd.mnz, oz), iz), t1z = v_mul(v_sub(nd.mxz, oz), iz);
      const vec4f tEnter = v_max(v_max(v_min(t0x, t1x), v_min(t0y, t1y)), v_max(v_min(t0z, t1z), v_zero()));
      const vec4f tExit = v_min(v_min(v_max(t0x, t1x), v_max(t0y, t1y)), v_min(v_max(t0z, t1z), v_splats(r.t)));
      const unsigned m = (unsigned)v_truemask(v_cmp_le(tEnter, tExit)) & ((1u << nd.N) - 1); // lanes >= N: garbage, masked
      const unsigned leafAll = nd.leafMask();
      unsigned leafHit = m & leafAll;
      if (leafHit)
      {
        const int nodeOfs = (int)(cur & PTR_OFS_MASK);
        const unsigned shortMask = (cur >> PTR_SHORT_SHIFT) & 15u;
        do
        {
          const int i = (int)__bsf_unsafe(leafHit);
          leafHit &= leafHit - 1;
          const int bodyOfs = leafBodyOfs(nodeOfs, nd.N, leafAll, shortMask, i);
          if (rayLeafAt<CullCCW>(r, bodyOfs, w[i], (shortMask >> i) & 1))
            if (cb(r, makeLeafRef(cur, i)))
              return true; // accepted blocker; a rejecting cb restored r.t and the walk continues
        } while (leafHit);
      }
      unsigned mi = m & ~leafAll; // internal survivors
      if (mi)
      {
        const unsigned first = __bsf_unsafe(mi);
        // <= 4 children per node and depth <= MAX_TREE_DEPTH (converter-enforced) keep sp within the 4/level stack.
        G_ASSERT(sp + 3 <= (int)(sizeof(stack) / sizeof(stack[0])));
        for (mi &= mi - 1; mi; mi &= mi - 1)
          stack[sp++] = (int)w[__bsf_unsafe(mi)];
        cur = w[first];
        continue;
      }
    }
    else // degenerate whole-BLAS-is-one-leaf root block
    {
      const int leaf = (int)(cur & PTR_OFS_MASK);
      if (swblas_rayLeaf_SoA<CullCCW, 8>(r, leaf + 4, *(const uint32_t *)(data + leaf)))
        if (cb(r, makeRootLeafRef(cur & PTR_OFS_MASK)))
          return true;
    }
    if (!sp)
      return r.bestTriOffset != 0; // covers callbacks that record hits but keep walking
    cur = (uint32_t)stack[--sp];
  }
}

// Out-of-line entries (defined in soa4TraversalOOL.cpp) for callers that do not need a custom
// callback and want no template instantiation in their TU.
bool rayClosestOOL(RayData &r, RootRef root);
bool rayClosestOOLCullCCW(RayData &r, RootRef root);
bool rayAnyHitOOL(RayData &r, RootRef root);
bool rayAnyHitOOLCullCCW(RayData &r, RootRef root);

// ============================================================================
// Filtered iteration (overlap walkers, face accessors) -- the SoA4 counterparts of the stackless
// BLASTraverse::iterateFiltered/iterateFilteredVerts/iterateLeafOffsets. Same box set and the same
// pre-order visit order as the stackless walk over the equivalent tree, so consumers see identical
// leaf/triangle sequences. Exception: the degenerate single-leaf root stores no box, so it is
// filtered against rootLeafBox (rebuilt from its verts, writeQuadBox-quantized) -- identical to the
// stored box on the bvhIO load path, conservative-only slack for in-process float-built trees.
// ============================================================================

// Vertex loaders (vert21 packed, stride 8 -- the only vertex format SoA4 trees carry). Aliases of
// the shared stride loader so the stackless and SoA4 surfaces reference one authority.
using Vert21Loader = Vert21StrideLoader<8>;       // box-space coords (divided by 32)
using Vert21LoaderRaw = Vert21StrideLoaderRaw<8>; // unscaled raw coords; caller scales its query by 32 instead

// Fetch the 3 verts of sub-triangle 0..3 (0/1 = quad A tri 1/2, 2/3 = quad B tri 1/2) of an
// already resolved leaf (e.g. a validated token from resolve-and-check paths); decodes the header
// fields itself, so a caller can never pair fields with the wrong body offset. Mirrors the
// stackless QuadLeafVerts::decodeTri winding rules (2nd tri is the strip (v1,v2,v3), flip swaps
// the first two corners), so refetch/normal consumers see identical order.
template <class VL>
static __forceinline void fetchLeafTri(const uint8_t *data, const LeafLoc &l, int subTri, const VL &vl, vec3f &a, vec3f &b, vec3f &c)
{
  const QuadLeafFields f = leafFields(data, l);
  int base = l.bodyOfs + (int)f.relBaseBytes;
  int o1 = f.o1, o2 = f.o2, o3 = f.o3;
  bool flip = f.flipSecond;
  if (subTri >= 2)
  {
    base += (int)f.deltaB * 8;
    o1 = f.o1b;
    o2 = f.o2b;
    o3 = f.o3b;
    flip = f.flipSecondB;
  }
  const bool single = (o3 == o2);
  if (single || (subTri & 1) == 0)
  {
    a = vl(data, base, 0);
    b = vl(data, base, o1);
    c = vl(data, base, o2);
  }
  else
  {
    const vec3f v1 = vl(data, base, o1), v2 = vl(data, base, o2), v3 = vl(data, base, o3);
    swblas_secondTriCorners(flip, v1, v2, v3, a, b, c);
  }
}

// Fetch one sub-triangle straight from a walker/ray-callback LeafRef.
template <class VL>
static __forceinline void fetchLeafTri(const uint8_t *data, LeafRef ref, int subTri, const VL &vl, vec3f &a, vec3f &b, vec3f &c)
{
  fetchLeafTri(data, decodeLeafRef(data, ref), subTri, vl, a, b, c);
}

// Child lane box as float vec3fs (same integer box-space values the stackless decodeRaw yields).
static __forceinline void laneBox(const uint8_t *n, int s2, int lane, vec3f &bmin, vec3f &bmax)
{
  auto u16 = [&](int k) { return (float)*(const uint16_t *)(n + k * s2 + lane * 2); };
  bmin = v_make_vec4f(u16(0), u16(1), u16(2), 0.f);
  bmax = v_make_vec4f(u16(3), u16(4), u16(5), 0.f);
}

// Box of the degenerate single-leaf root, rebuilt from the leaf's own vert21 verts (the 16-byte
// root block stores no box). Quantized with the writeQuadBox rule (floor min / ceil max onto the
// u16 lattice), so it matches the box a stackless walk would test wherever that box was itself
// rebuilt from the packed verts (the bvhIO load path); any residual slack contains no vertex, so
// exact tests downstream cannot diverge.
// Format invariant this decode relies on: a root leaf always has a valid apex and every vertex
// address its body decodes lies inside the vert region (buildFromStackless validates the whole
// root body and refuses other sources; both bvhIO deserializers bounds-check every leaf and
// refuse RAW roots), and in-node no-hit bodies are all-zero, whose decoded loads stay inside
// the leaf block itself.
static __forceinline void rootLeafBox(const uint8_t *data, int body_ofs, const QuadLeafFields &f, vec3f &bmin, vec3f &bmax)
{
  const int baseA = body_ofs + (int)f.relBaseBytes;
  auto vert = [&](int base, int o) { return RayData::unpackVert21(data + base + o * 8); };
  bmin = vert(baseA, 0), bmax = bmin;
  auto add = [&](vec3f v) {
    bmin = v_min(bmin, v);
    bmax = v_max(bmax, v);
  };
  add(vert(baseA, f.o1));
  add(vert(baseA, f.o2));
  if (!f.isSingle)
    add(vert(baseA, f.o3));
  if (f.hasB)
  {
    const int baseB = baseA + (int)f.deltaB * 8;
    add(vert(baseB, 0));
    add(vert(baseB, f.o1b));
    add(vert(baseB, f.o2b));
    if (!f.isSingleB)
      add(vert(baseB, f.o3b));
  }
  vec4i bminI, bmaxI;
  swblas_quantize_box_u16(bmin, bmax, bminI, bmaxI);
  bmin = v_cvt_vec4f(bminI);
  bmax = v_cvt_vec4f(bmaxI);
}

template <class NodeTest, class LeafCb>
static bool iterateLeafRefsNode(const uint8_t *data, uint32_t ptr, const NodeTest &nodeTest, const LeafCb &leafCb)
{
  const NodeRef nd(data, ptr);
  const int nodeOfs = (int)(ptr & PTR_OFS_MASK);
  const uint32_t *w = nd.w();
  // The parent context (child words, short-lane mask) is already in hand here, so the LeafLoc
  // handed to leafCb costs a mask fold per leaf instead of the full decodeLeafRef a consumer
  // would otherwise re-run.
  const unsigned shortMask = (ptr >> PTR_SHORT_SHIFT) & 15u;
  const unsigned leafAll = nd.leafMask();
  for (int i = 0; i < nd.N; ++i)
  {
    vec3f bmin, bmax;
    laneBox(nd.n, nd.s2, i, bmin, bmax);
    if (!nodeTest(bmin, bmax))
      continue;
    if (w[i] & QUAD_LEAF_FLAG)
    {
      const LeafLoc l{leafBodyOfs(nodeOfs, nd.N, leafAll, shortMask, i), w[i], ((shortMask >> i) & 1) != 0};
      if (leafCb(bmin, bmax, makeLeafRef(ptr, i), l))
        return true;
    }
    else if (iterateLeafRefsNode(data, w[i], nodeTest, leafCb))
      return true;
  }
  return false;
}

// Leaf-granular, vertex-free walker: for every leaf whose box passes nodeTest,
// leafCb(bmin, bmax, ref, loc) ONCE -- no vertex decode, no per-sub-tri expansion (the counterpart
// of iterateLeafOffsets). bmin/bmax is the leaf's STORED lane box (the same u16 values the
// stackless walk tests), except for the degenerate single-leaf root, which stores no box and gets
// its rootLeafBox rebuild. `loc` is the ref's already-resolved LeafLoc so consumers do not re-run
// decodeLeafRef; the ref stays the persistent token to store.
template <class NodeTest, class LeafCb>
static bool iterateLeafRefs(const uint8_t *data, RootRef root, const NodeTest &nodeTest, const LeafCb &leafCb)
{
  if (!root.valid())
    return false;
  const uint32_t ptr = (uint32_t)root.v;
  if (!(ptr & TAG_MASK))
  {
    const LeafRef ref = makeRootLeafRef(ptr & PTR_OFS_MASK);
    const LeafLoc l = decodeLeafRef(data, ref);
    vec3f bmin, bmax;
    rootLeafBox(data, l.bodyOfs, leafFields(data, l), bmin, bmax);
    return nodeTest(bmin, bmax) && leafCb(bmin, bmax, ref, l);
  }
  return iterateLeafRefsNode(data, ptr, nodeTest, leafCb);
}

// Generic filtered traversal: test each child box, call leafCb per sub-triangle with its vertices.
// NodeTest: (vec3f bmin, vec3f bmax) -> bool (true = overlap, traverse/emit)
// LeafCb:   (vec3f v0, vec3f v1, vec3f v2, LeafRef ref, int subTri, int apex_byte_ofs) -> bool (true = stop all)
// apex_byte_ofs is the leaf's quad-A apex byte offset (quad-B sub-tris carry it too) -- the value a
// decodeLeafRef + leafFields re-run on ref would yield, already in hand here, so leaf-to-first-vertex
// consumers need no re-decode.
template <class NodeTest, class LeafCb, class VL>
static bool iterateFiltered(const uint8_t *data, RootRef root, const NodeTest &nodeTest, const LeafCb &leafCb, const VL &vl)
{
  return iterateLeafRefs(data, root, nodeTest, [&](vec3f, vec3f, LeafRef ref, const LeafLoc &l) {
    // Decode each quad's corners once and reshuffle them for the 2nd triangle (the strip/flip rules
    // of fetchLeafTri), instead of re-unpacking 3 verts per sub-tri -- leaf-scan walkers hit every
    // overlapping leaf, so the shared strip corners must not be loaded twice.
    const QuadLeafFields f = leafFields(data, l);
    const int baseA = l.bodyOfs + (int)f.relBaseBytes;
    const vec3f v0 = vl(data, baseA, 0), v1 = vl(data, baseA, f.o1), v2 = vl(data, baseA, f.o2);
    if (leafCb(v0, v1, v2, ref, 0, baseA))
      return true;
    if (!f.isSingle)
    {
      const vec3f v3 = vl(data, baseA, f.o3);
      vec3f sa, sb, sc;
      swblas_secondTriCorners(f.flipSecond, v1, v2, v3, sa, sb, sc);
      if (leafCb(sa, sb, sc, ref, 1, baseA))
        return true;
    }
    if (f.hasB)
    {
      const int baseB = baseA + (int)f.deltaB * 8;
      const vec3f b0 = vl(data, baseB, 0), b1 = vl(data, baseB, f.o1b), b2 = vl(data, baseB, f.o2b);
      if (leafCb(b0, b1, b2, ref, 2, baseA))
        return true;
      if (!f.isSingleB)
      {
        const vec3f b3 = vl(data, baseB, f.o3b);
        vec3f sa, sb, sc;
        swblas_secondTriCorners(f.flipSecondB, b1, b2, b3, sa, sb, sc);
        if (leafCb(sa, sb, sc, ref, 3, baseA))
          return true;
      }
    }
    return false;
  });
}

// Geometry-only sibling: the callback never sees the sub-tri lane index (same split of concerns as
// the stackless iterateFilteredVerts).
// LeafCb: (vec3f v0, v1, v2, LeafRef ref, int apex_byte_ofs) -> bool (stop all).
template <class NodeTest, class LeafCb, class VL>
static bool iterateFilteredVerts(const uint8_t *data, RootRef root, const NodeTest &nodeTest, const LeafCb &leafCb, const VL &vl)
{
  return iterateFiltered(
    data, root, nodeTest,
    [&leafCb](vec3f v0, vec3f v1, vec3f v2, LeafRef ref, int, int apex_byte_ofs) { return leafCb(v0, v1, v2, ref, apex_byte_ofs); },
    vl);
}

} // namespace soa4
