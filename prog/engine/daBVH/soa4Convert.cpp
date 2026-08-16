// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <vecmath/dag_vecMath.h>
#include <daBVH/dag_swBLAS_soa4Convert.h>
#include <debug/dag_assert.h>
#include <string.h>

namespace soa4
{

// The cap serves two contracts: a malformed buffer fails conversion instead of overflowing the C
// stack here, and every ACCEPTED tree stays within the SoA4 walkers' fixed-stack bound
// (soa4::MAX_TREE_DEPTH == the engine's BVH_MAX_BLAS_DEPTH invariant; real SAH trees never exceed it).
static constexpr int MAX_CONVERT_DEPTH = MAX_TREE_DEPTH;

static inline uint32_t ld32(const uint8_t *p, int o) { return *(const uint32_t *)(p + o); }
static inline uint16_t ld16(const uint8_t *p, int o) { return *(const uint16_t *)(p + o); }

// ============================================================================
// Stackless -> SoA4. Two passes: a linear sizing walk, then one resize_noinit + a PRE-ORDER emit at
// a running offset -- a parent node is written immediately before its children subtrees (better
// locality for nearest-first descent than post-order append), W1 apex bases are re-pointed inline
// (vertsOfs is known upfront), and there is no per-node realloc/zero-fill or patch pass. Verts are
// copied verbatim. All structural violations set `failed` instead of asserting: conversion runs in
// release builds on load-time data, so a bad tree must degrade to "no conversion", not UB.
// ============================================================================
namespace
{
// Apex byte validity shared by both conversion directions: a real leaf apex is an 8-aligned byte
// inside the vert region; anything else marks a degenerate no-hit body carried verbatim.
static inline bool apexByteValid(int apex, int vert_bytes) { return apex >= 0 && (apex & 7) == 0 && apex < vert_bytes; }

struct Builder
{
  const uint8_t *src = nullptr;
  int srcVertsOfs = 0;
  int srcVertBytes = 0;
  int srcEnd = 0;         // end of the source tree region; every skip-derived offset must stay inside
  int treeBytesLimit = 0; // sized output tree region; every emitted block must stay inside
  dag::Vector<uint8_t> &buf;
  int vertsOfs = 0;
  bool shortEnabled = true; // set by build(): short bodies need the [tree][verts] span within 23-bit base reach
  bool failed = false;

  Builder(dag::Vector<uint8_t> &out) : buf(out) {}

  // bvhIO-style apex validity: a normal leaf's apex is a real vert21 vertex; a degenerate no-hit leaf
  // (writeDoubleQuadLeaf overflow: W1..W3 == 0) points back into the tree body instead -- such a leaf
  // must be carried VERBATIM (no base re-pointing, never short) so it stays "no geometry here".
  bool srcLeafApexValid(int src_leaf_hdr) const
  {
    const uint32_t W1 = ld32(src, src_leaf_hdr + BVH_BLAS_NODE_SIZE);
    const int apex = (src_leaf_hdr + BVH_BLAS_NODE_SIZE + (int)((W1 & QUAD_BASE_MASK) << QUAD_BASE_ALIGN_SHIFT)) - srcVertsOfs;
    return apexByteValid(apex, srcVertBytes);
  }

  // A source stackless leaf (header at src_leaf_hdr) qualifies for the short 4-byte body iff its W2/W3
  // carry nothing beyond flipA -- what writeDoubleQuadLeaf emits for a singleton (no quad B) whose
  // user bits are 0 -- its apex is re-pointable, and the whole span fits the 23-bit short base
  // (shortEnabled). A stamped singleton keeps the full body: the short form has nowhere to put W3.
  bool leafIsShort(int src_leaf_hdr) const
  {
    return shortEnabled && (ld32(src, src_leaf_hdr + 20) & ~QUAD_FLIPA_FLAG) == 0 && ld32(src, src_leaf_hdr + 24) == 0 &&
           srcLeafApexValid(src_leaf_hdr);
  }

  // Pass 1, LINEAR (no recursion): SoA4 tree bytes = 16*slots + 12*Lfull + 4*Lshort, where slots =
  // L + I - P1 (16 = box + word per child slot; 12/4 = inline leaf body): every leaf/internal is a
  // child slot of exactly one span, except the single child of a 1-child span (promoted, no slot). A
  // 1-child internal span shows locally: its first child's subtree size equals the whole span (skip);
  // the top span is checked by its child count. slots == 0 (whole BLAS is one leaf) degenerates to a
  // lone 16B [W0..W3] block.
  int sizeTree(int blasStart, int blasSize)
  {
    const int end = blasStart + blasSize;
    int L = 0, Ls = 0, I = 0, P1 = 0;
    for (int c = blasStart; c < end;)
    {
      if (c + BVH_BLAS_NODE_SIZE > end) // truncated header: corrupt input
      {
        failed = true;
        return 0;
      }
      const uint32_t skip = ld32(src, c + 12);
      int next;
      if (skip & BLAS_LEAF_FLAG)
      {
        if (c + BVH_BLAS_LEAF_SIZE > end)
        {
          failed = true;
          return 0;
        }
        ++L;
        Ls += leafIsShort(c) ? 1 : 0;
        next = c + BVH_BLAS_LEAF_SIZE;
      }
      else
      {
        ++I;
        if ((int)skip > 0 && c + 2 * BVH_BLAS_NODE_SIZE <= end)
        {
          const uint32_t cs = ld32(src, c + BVH_BLAS_NODE_SIZE + 12); // first child's skip word
          P1 += (int)skip == ((cs & BLAS_LEAF_FLAG) ? BVH_BLAS_LEAF_SIZE : BVH_BLAS_NODE_SIZE + (int)cs);
        }
        next = c + BVH_BLAS_NODE_SIZE;
      }
      if (next <= c) // corrupt skip: the walk would not terminate
      {
        failed = true;
        return 0;
      }
      c = next;
    }
    int nTop = 0;
    for (int c = blasStart; c < end; ++nTop)
    {
      const uint32_t skip = ld32(src, c + 12);
      const int next = c + ((skip & BLAS_LEAF_FLAG) ? BVH_BLAS_LEAF_SIZE : BVH_BLAS_NODE_SIZE + (int)skip);
      if (next <= c)
      {
        failed = true;
        return 0;
      }
      c = next;
    }
    P1 += nTop == 1;
    const int slots = L + I - P1;
    return slots > 0 ? 16 * slots + 12 * (L - Ls) + 4 * Ls : 16;
  }

  // Resolve a child through 1-child chains to the leaf or multi-child node behind them (SAH never emits
  // such chains; kept so any input converts correctly). Every dereference is bounded by the enclosing
  // span end `e`: a skip word is data, not trusted structure. Returns the final stackless offset, or
  // -1 on corrupt input (chain does not terminate, or a read would leave the span).
  int resolveChild(int srcOfs, int e)
  {
    for (int guard = 0; guard <= MAX_CONVERT_DEPTH; ++guard)
    {
      if (srcOfs + BVH_BLAS_NODE_SIZE > e)
        break;
      const uint32_t skip = ld32(src, srcOfs + 12);
      if (skip & BLAS_LEAF_FLAG)
      {
        if (srcOfs + BVH_BLAS_LEAF_SIZE > e)
          break;
        return srcOfs;
      }
      if ((int)skip < 0 || srcOfs + BVH_BLAS_NODE_SIZE + (int)skip > e)
        break; // subtree span escapes the parent span: corrupt
      const int s = srcOfs + BVH_BLAS_NODE_SIZE;
      if (s + BVH_BLAS_NODE_SIZE > e)
        break;
      const uint32_t cs = ld32(src, s + 12);
      const int firstChildBytes = (cs & BLAS_LEAF_FLAG) ? BVH_BLAS_LEAF_SIZE : BVH_BLAS_NODE_SIZE + (int)cs;
      if (firstChildBytes < (int)skip)
        return srcOfs; // the first child's subtree does not fill the span: >= 2 children, a real node
      srcOfs = s;      // lone child: promote through it
    }
    failed = true;
    return -1;
  }

  // Pass 2: emit the node for the stackless children-span [s, e) -- n child boxes (SoA), n child words
  // (leaf -> W0, internal -> tagged offset), then the leaf children's 12B/4B bodies -- and return its
  // tagged pointer (nodeOfs | (n-1); blocks stay 4-aligned so the low tag bits are free). A span that
  // resolves to a single leaf cannot be emitted here (its W0/body belong in the PARENT node), so it is
  // bubbled up as QUAD_LEAF_FLAG | srcOfs; build() handles the degenerate whole-BLAS-is-one-leaf case.
  uint32_t emitSpan(int s, int e, int &cur, int depth)
  {
    if (DAGOR_UNLIKELY(failed || depth > MAX_CONVERT_DEPTH))
    {
      failed = true;
      return 0;
    }
    int childSrc[4];
    uint32_t childSkip[4];
    unsigned shortMask = 0;
    int n = 0, fullK = 0, shortK = 0;
    for (int c = s; c < e; ++n)
    {
      if (DAGOR_UNLIKELY(n == 4)) // create_bvh_node_sah caps every span fanout at 4; a wider one cannot be encoded
      {
        failed = true;
        return 0;
      }
      if (DAGOR_UNLIKELY(c + BVH_BLAS_NODE_SIZE > e)) // truncated child header: corrupt
      {
        failed = true;
        return 0;
      }
      const int r = resolveChild(c, e);
      if (DAGOR_UNLIKELY(r < 0))
        return 0;
      childSrc[n] = r;
      childSkip[n] = ld32(src, r + 12);
      if (childSkip[n] & BLAS_LEAF_FLAG)
      {
        if (leafIsShort(r))
        {
          shortMask |= 1u << n;
          ++shortK;
        }
        else
          ++fullK;
      }
      const uint32_t skip = ld32(src, c + 12);
      const int next = c + ((skip & BLAS_LEAF_FLAG) ? BVH_BLAS_LEAF_SIZE : BVH_BLAS_NODE_SIZE + (int)skip);
      if (DAGOR_UNLIKELY(next <= c || next > e)) // child subtrees must tile the parent span
      {
        failed = true;
        return 0;
      }
      c = next;
    }
    if (n == 1)
    {
      if (childSkip[0] & BLAS_LEAF_FLAG)
        return QUAD_LEAF_FLAG | (uint32_t)childSrc[0]; // lone leaf: bubble to the parent node
      return emitSpan(childSrc[0] + BVH_BLAS_NODE_SIZE, childSrc[0] + BVH_BLAS_NODE_SIZE + (int)childSkip[0], cur, depth + 1);
    }
    const int size = 16 * n + 12 * fullK + 4 * shortK;
    const int nodeOfs = cur;
    // Node offsets must fit the child-ref [25:2] field and the 23-bit LeafRef parent field, and the
    // block must fit the SIZED output region: on corrupt input the emit walk can visit nodes the
    // linear sizing pass never counted, and this guard keeps those from writing past the allocation
    // (the cur != treeBytes reconciliation in build() only runs after the fact).
    if (DAGOR_UNLIKELY((uint32_t)nodeOfs >= (1u << 25) || nodeOfs + size > treeBytesLimit))
    {
      failed = true;
      return 0;
    }
    const int s2 = n * 2; // bytes per SoA axis-array
    cur += size;
    int bodyOfs = nodeOfs + 16 * n; // inline leaf bodies follow the child words, in lane order
    uint8_t *p = buf.data() + nodeOfs;
    for (int i = 0; i < n; ++i)
    {
      const uint8_t *h = src + childSrc[i]; // [min.x|max.x<<16][min.y|max.y<<16][min.z|max.z<<16][skip]
      *(uint16_t *)(p + 0 * s2 + i * 2) = *(const uint16_t *)(h + 0);
      *(uint16_t *)(p + 1 * s2 + i * 2) = *(const uint16_t *)(h + 4);
      *(uint16_t *)(p + 2 * s2 + i * 2) = *(const uint16_t *)(h + 8);
      *(uint16_t *)(p + 3 * s2 + i * 2) = *(const uint16_t *)(h + 2);
      *(uint16_t *)(p + 4 * s2 + i * 2) = *(const uint16_t *)(h + 6);
      *(uint16_t *)(p + 5 * s2 + i * 2) = *(const uint16_t *)(h + 10);
      uint32_t word;
      if (childSkip[i] & BLAS_LEAF_FLAG)
      {
        word = childSkip[i]; // leaf child: the word IS its W0 (bit 31 = QUAD_LEAF_FLAG set)
        const bool isShort = (shortMask >> i) & 1;
        const int srcBody = childSrc[i] + BVH_BLAS_NODE_SIZE;
        const uint32_t W1 = ld32(src, srcBody + 0);
        const uint32_t srcRelBase = (W1 & QUAD_BASE_MASK) << QUAD_BASE_ALIGN_SHIFT;
        const int apexByteInVertRegion = (srcBody + (int)srcRelBase) - srcVertsOfs; // == apexVertIdx * 8
        const int newRelBase = (vertsOfs + apexByteInVertRegion) - bodyOfs;
        uint32_t *b = (uint32_t *)(buf.data() + bodyOfs);
        if (isShort) // 4B body: W1 with bit 23 = flipA (from source W2), base truncated to 23 bits
        {
          if (DAGOR_UNLIKELY((uint32_t)(newRelBase >> QUAD_BASE_ALIGN_SHIFT) >= SHORT_W1_FLIP)) // shortEnabled gate must hold
          {
            failed = true;
            return 0;
          }
          const uint32_t flip = (ld32(src, srcBody + 4) & QUAD_FLIPA_FLAG) ? SHORT_W1_FLIP : 0u;
          b[0] = (W1 & ~QUAD_BASE_MASK) | flip | (uint32_t)(newRelBase >> QUAD_BASE_ALIGN_SHIFT);
        }
        else if (srcLeafApexValid(childSrc[i]))
        {
          b[0] = (W1 & ~QUAD_BASE_MASK) | ((uint32_t)(newRelBase >> QUAD_BASE_ALIGN_SHIFT) & QUAD_BASE_MASK);
          b[1] = ld32(src, srcBody + 4);
          b[2] = ld32(src, srcBody + 8);
        }
        else // degenerate no-hit leaf: no valid apex to re-point, keep the body verbatim
        {
          const uint32_t W2 = ld32(src, srcBody + 4), W3 = ld32(src, srcBody + 8);
          // only the flag-only all-zero no-hit body is legitimate (writeDoubleQuadLeaf); any other
          // invalid-apex payload is crafted offsets that must never reach a vertex decoder -- W0
          // included, its o1/o2/o3A bits feed the walker's quad-A decode via the copied child word
          if (DAGOR_UNLIKELY((childSkip[i] & ~QUAD_LEAF_FLAG) | W1 | W2 | W3))
          {
            failed = true;
            return 0;
          }
          b[0] = W1;
          b[1] = W2;
          b[2] = W3;
        }
        bodyOfs += isShort ? 4 : 12;
      }
      else
        word = emitSpan(childSrc[i] + BVH_BLAS_NODE_SIZE, childSrc[i] + BVH_BLAS_NODE_SIZE + (int)childSkip[i], cur, depth + 1);
      *(uint32_t *)(p + 6 * s2 + i * 4) = word;
    }
    return (uint32_t)nodeOfs | (uint32_t)(n - 1) | (shortMask << PTR_SHORT_SHIFT);
  }

  ConvertResult build(int blasStart, int blasSize, int vertBytes)
  {
    ConvertResult res;
    srcVertBytes = vertBytes;
    // The LeafRef encoding carries a 23-bit (32MB) parent node offset -- a larger tree is not
    // representable. Real BLASes stay far below (producers cap them; the format tops out at 64MB).
    if ((int64_t)blasSize + LEAF_BYTES > (int64_t)LEAF_ENTRY_OFS_MASK)
      return res;
    // Short bodies address verts through a 23-bit base: only safe when the whole [tree][verts] span
    // fits it; otherwise emit full bodies only (pure memory trade, no correctness impact).
    shortEnabled = (int64_t)blasSize + LEAF_BYTES + vertBytes <= (int64_t)(SHORT_W1_FLIP - 1) << QUAD_BASE_ALIGN_SHIFT;
    srcEnd = blasStart + blasSize;
    const int treeBytes = sizeTree(blasStart, blasSize);
    if (failed)
      return res;
    treeBytesLimit = treeBytes;
    vertsOfs = (treeBytes + 7) & ~7; // 8-align the vert21 region (tree blocks are only 4-aligned)
    buf.resize_noinit(vertsOfs + srcVertBytes);
    if (vertsOfs > treeBytes)
      memset(buf.data() + treeBytes, 0, vertsOfs - treeBytes); // defined pad: conversions must be byte-reproducible
    int cur = 0;
    uint32_t rootRef = emitSpan(blasStart, blasStart + blasSize, cur, 0);
    if (failed)
      return res;
    if (rootRef & QUAD_LEAF_FLAG)
    {
      // Degenerate whole-BLAS-is-one-leaf (never for real meshes): a lone 16B [W0 W1 W2 W3] block,
      // referenced with tag 0 and handled by the dedicated pop path in rayClosest/rayAnyHit.
      const int srcLeafHdr = (int)(rootRef & ~QUAD_LEAF_FLAG);
      // A no-hit/invalid-apex root is refused outright: the boxless root block would hand its body
      // straight to the vertex-decoding walkers. Only writeDoubleQuadLeaf's encode-overflow logerr
      // path makes such a tree; callers get their reported fallbacks, not a silently empty BLAS.
      if (!srcLeafApexValid(srcLeafHdr))
      {
        failed = true;
        return res;
      }
      const int srcBody = srcLeafHdr + BVH_BLAS_NODE_SIZE;
      const uint32_t W1 = ld32(src, srcBody + 0);
      const uint32_t srcRelBase = (W1 & QUAD_BASE_MASK) << QUAD_BASE_ALIGN_SHIFT;
      const int apexByteInVertRegion = (srcBody + (int)srcRelBase) - srcVertsOfs;
      // The root block keeps no lane box to gate this leaf later, so every vertex address its body
      // can decode must be valid up front (the bvhIO load path index-validates in-node leaves instead).
      // W1's base bits are zeroed for the decode so o3A rides along and relBaseBytes reads 0.
      const uint32_t apexVertIdx = (uint32_t)apexByteInVertRegion / 8u;
      const uint32_t vc = (uint32_t)srcVertBytes / 8u;
      const QuadLeafFields rootF =
        decodeQuadLeafFields(ld32(src, srcLeafHdr + 12), W1 & ~QUAD_BASE_MASK, ld32(src, srcBody + 4), ld32(src, srcBody + 8));
      if (!validateQuadLeafVertexIndices(rootF, apexVertIdx, vc))
      {
        failed = true;
        return res;
      }
      const int newRelBase = (vertsOfs + apexByteInVertRegion) - (cur + 4);
      uint32_t *w = (uint32_t *)(buf.data() + cur);
      w[0] = ld32(src, srcLeafHdr + 12);
      w[1] = (W1 & ~QUAD_BASE_MASK) | ((uint32_t)(newRelBase >> QUAD_BASE_ALIGN_SHIFT) & QUAD_BASE_MASK);
      w[2] = ld32(src, srcBody + 4);
      w[3] = ld32(src, srcBody + 8);
      rootRef = (uint32_t)cur; // tag 0
      cur += LEAF_BYTES;
    }
    if (cur != treeBytes) // emit/sizing mismatch: structural corruption, refuse the buffer
      return res;
    memcpy(buf.data() + vertsOfs, src + srcVertsOfs, srcVertBytes);
    res.root.v = (int32_t)rootRef;
    res.treeBytes = treeBytes;
    res.vertsOfs = vertsOfs;
    return res;
  }
};
} // namespace

ConvertResult buildFromStackless(const uint8_t *src, int blas_start, int blas_size, int src_verts_ofs, int vert_bytes,
  dag::Vector<uint8_t> &out)
{
  G_ASSERT_RETURN(src && blas_size > 0 && vert_bytes >= 0, ConvertResult());
  Builder b(out);
  b.src = src;
  b.srcVertsOfs = src_verts_ofs;
  return b.build(blas_start, blas_size, vert_bytes);
}

// ============================================================================
// SoA4 -> stackless (the GPU upload format). A sizing walk (the SoA4 buffer does not store its
// stackless size), then one resize_noinit + a single recursive emit at a running offset with W1 apex
// bases re-pointed inline. Mirrors writeQuadBVH2 (root box suppressed); the result is byte-identical
// to the stackless buffer the SoA4 tree was built from (SAH never emits the 1-child chain nodes that
// are the only shape the converters would collapse).
// ============================================================================
namespace
{
struct ToStackless
{
  const uint8_t *src = nullptr; // SoA4 buffer
  int srcVertsOfs = 0;          // also the end of the source tree region (tree precedes verts)
  int srcVertBytes = 0;
  dag::Vector<uint8_t> &buf;
  int vertsOfs = 0;
  bool failed = false;

  ToStackless(dag::Vector<uint8_t> &out) : buf(out) {}

  // Stackless bytes of the subtree behind an internal child ref: 28 per leaf child, 16 + recursion
  // per internal child. Also the validation pass: child refs are data, so every node span (boxes +
  // words + inline bodies) is bounded to the source tree region before emitNodeChildren re-walks the
  // same refs and dereferences them.
  int sizeSubtree(uint32_t ptr, int depth)
  {
    const int N = (int)(ptr & TAG_MASK) + 1;
    const int nodeOfs = (int)(ptr & PTR_OFS_MASK);
    if (DAGOR_UNLIKELY(depth > MAX_CONVERT_DEPTH || nodeOfs + 16 * N > srcVertsOfs))
    {
      failed = true;
      return 0;
    }
    const uint32_t *w = (const uint32_t *)(src + nodeOfs + 12 * N);
    const unsigned shortMask = (ptr >> PTR_SHORT_SHIFT) & 15u;
    int bytes = 0, bodyBytes = 0;
    for (int i = 0; i < N; ++i)
      if (w[i] & QUAD_LEAF_FLAG)
      {
        bytes += BVH_BLAS_LEAF_SIZE;
        bodyBytes += ((shortMask >> i) & 1) ? 4 : 12;
      }
      else
        bytes += BVH_BLAS_NODE_SIZE + sizeSubtree(w[i], depth + 1);
    if (DAGOR_UNLIKELY(nodeOfs + 16 * N + bodyBytes > srcVertsOfs)) // inline bodies must stay in the tree region
    {
      failed = true;
      return 0;
    }
    return bytes;
  }

  // Copy SoA4 child `pi`'s box (from packed parent node `pn`, axis stride `ps2`) into a stackless header at.
  void writeBox(int at, const uint8_t *pn, int pi, int ps2)
  {
    const uint16_t mnx = ld16(pn, 0 * ps2 + pi * 2), mny = ld16(pn, 1 * ps2 + pi * 2), mnz = ld16(pn, 2 * ps2 + pi * 2);
    const uint16_t mxx = ld16(pn, 3 * ps2 + pi * 2), mxy = ld16(pn, 4 * ps2 + pi * 2), mxz = ld16(pn, 5 * ps2 + pi * 2);
    *(uint32_t *)(buf.data() + at + 0) = (uint32_t)mnx | ((uint32_t)mxx << 16);
    *(uint32_t *)(buf.data() + at + 4) = (uint32_t)mny | ((uint32_t)mxy << 16);
    *(uint32_t *)(buf.data() + at + 8) = (uint32_t)mnz | ((uint32_t)mxz << 16);
  }

  // Emit a leaf body at `at` from the degenerate-root 16B [W0 W1 W2 W3] block at `leaf`.
  void emitLeafBody(int at, int leaf)
  {
    const uint32_t W0 = ld32(src, leaf + 0), W1 = ld32(src, leaf + 4), W2 = ld32(src, leaf + 8), W3 = ld32(src, leaf + 12);
    const uint32_t relBase = (W1 & QUAD_BASE_MASK) << QUAD_BASE_ALIGN_SHIFT;
    const int apex = (leaf + 4 + (int)relBase) - srcVertsOfs;
    const int newRelBase = (vertsOfs + apex) - (at + 16);
    // An invalid-apex root keeps W1 verbatim: there is no apex to re-point through. Deliberately
    // permissive where Builder::build refuses such roots -- no accepted SoA4 tree carries one, so
    // a mirrored rejection here would be dead code.
    const bool apexValid = apexByteValid(apex, srcVertBytes);
    uint32_t *w = (uint32_t *)(buf.data() + at + 12);
    w[0] = W0;
    w[1] = apexValid ? (W1 & ~QUAD_BASE_MASK) | ((uint32_t)(newRelBase >> QUAD_BASE_ALIGN_SHIFT) & QUAD_BASE_MASK) : W1;
    w[2] = W2;
    w[3] = W3;
  }

  // Emit the stackless children of SoA4 node `ptr` at running offset `cur`: a leaf child (word bit 31
  // set = its W0) becomes a 28B stackless leaf fed from the node's box lane + inline body (a short 4B
  // body reconstructs the canonical singleton W2 = flipA<<29, W3 = 0 the builder emits); an internal
  // child recurses, its skip stored once the children's extent is known.
  void emitNodeChildren(uint32_t ptr, int &cur, int depth)
  {
    if (DAGOR_UNLIKELY(failed || depth > MAX_CONVERT_DEPTH))
    {
      failed = true;
      return;
    }
    const int N = (int)(ptr & TAG_MASK) + 1;
    const uint8_t *n = src + (ptr & PTR_OFS_MASK);
    const unsigned shortMask = (ptr >> PTR_SHORT_SHIFT) & 15u;
    const int s2 = N * 2;
    const uint32_t *w = (const uint32_t *)(n + 6 * s2);
    int bodyOfs = (int)(ptr & PTR_OFS_MASK) + 16 * N; // inline leaf bodies, lane order
    for (int i = 0; i < N; ++i)
    {
      const int at = cur;
      if (w[i] & QUAD_LEAF_FLAG) // leaf child
      {
        cur += BVH_BLAS_LEAF_SIZE;
        writeBox(at, n, i, s2);
        const bool isShort = (shortMask >> i) & 1;
        const uint32_t W1 = ld32(src, bodyOfs + 0);
        const uint32_t baseMask = isShort ? (QUAD_BASE_MASK & ~SHORT_W1_FLIP) : QUAD_BASE_MASK;
        const uint32_t relBase = (W1 & baseMask) << QUAD_BASE_ALIGN_SHIFT;
        const int apex = (bodyOfs + (int)relBase) - srcVertsOfs;
        // A degenerate no-hit leaf (invalid apex; always a full body) is carried verbatim -- see
        // Builder::srcLeafApexValid.
        const bool apexValid = apexByteValid(apex, srcVertBytes);
        const int newRelBase = (vertsOfs + apex) - (at + 16);
        uint32_t *o = (uint32_t *)(buf.data() + at + 12);
        o[0] = w[i]; // W0
        o[1] = apexValid ? (W1 & ~QUAD_BASE_MASK) | ((uint32_t)(newRelBase >> QUAD_BASE_ALIGN_SHIFT) & QUAD_BASE_MASK) : W1;
        o[2] = isShort ? ((W1 & SHORT_W1_FLIP) ? QUAD_FLIPA_FLAG : 0u) : ld32(src, bodyOfs + 4);
        o[3] = isShort ? 0u : ld32(src, bodyOfs + 8);
        bodyOfs += isShort ? 4 : 12;
      }
      else
      {
        cur += BVH_BLAS_NODE_SIZE; // box header; skip stored once the children's extent is known
        writeBox(at, n, i, s2);
        emitNodeChildren(w[i], cur, depth + 1);
        *(uint32_t *)(buf.data() + at + 12) = (uint32_t)(cur - (at + BVH_BLAS_NODE_SIZE));
      }
    }
  }

  StacklessResult build(RootRef root, int vertBytes)
  {
    StacklessResult res;
    srcVertBytes = vertBytes;
    const bool rootIsNode = ((uint32_t)root.v & TAG_MASK) != 0;
    if (!rootIsNode && (int)((uint32_t)root.v & PTR_OFS_MASK) + LEAF_BYTES > srcVertsOfs)
      return res; // degenerate-root block must sit inside the tree region
    const int treeBytes = rootIsNode ? sizeSubtree((uint32_t)root.v, 0) : BVH_BLAS_LEAF_SIZE;
    if (failed)
      return res;
    vertsOfs = (treeBytes + 7) & ~7;
    buf.resize_noinit(vertsOfs + srcVertBytes);
    if (vertsOfs > treeBytes)
      memset(buf.data() + treeBytes, 0, vertsOfs - treeBytes);
    int cur = 0;
    if (rootIsNode)
      emitNodeChildren((uint32_t)root.v, cur, 0); // root is an internal node: emit its children, root box suppressed
    else
    {
      // Degenerate single-leaf BLAS (does not occur for real meshes): emit the lone leaf at top level with a
      // full-range box (always intersects; the exact leaf test still gates correctness).
      *(uint32_t *)(buf.data() + 0) = 0xFFFF0000u; // min=0, max=0xFFFF per axis
      *(uint32_t *)(buf.data() + 4) = 0xFFFF0000u;
      *(uint32_t *)(buf.data() + 8) = 0xFFFF0000u;
      emitLeafBody(0, (int)((uint32_t)root.v & PTR_OFS_MASK));
      cur = BVH_BLAS_LEAF_SIZE;
    }
    if (failed || cur != treeBytes) // emit/sizing mismatch: structural corruption, refuse the buffer
      return res;
    memcpy(buf.data() + vertsOfs, src + srcVertsOfs, srcVertBytes);
    res.treeBytes = treeBytes;
    res.vertsOfs = vertsOfs;
    return res;
  }
};
} // namespace

StacklessResult buildStackless(const uint8_t *src, RootRef root, int src_verts_ofs, int vert_bytes, dag::Vector<uint8_t> &out)
{
  G_ASSERT_RETURN(src && root.valid() && vert_bytes >= 0, StacklessResult());
  ToStackless t(out);
  t.src = src;
  t.srcVertsOfs = src_verts_ofs;
  return t.build(root, vert_bytes);
}

} // namespace soa4
