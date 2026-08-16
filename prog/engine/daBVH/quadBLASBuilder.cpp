// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daBVH/swCommon.h>
#include <daBVH/dag_quadBLASBuilder.h>
#include <daBVH/swBLASLeafDefs.hlsli>
#include <daBVH/swBVHDefine.hlsli>
#include <daBVH/dag_quadLeafEncode.h>
#include <daBVH/dag_swBLAS_leaf.h> // swblas_quantize_box_u16
#include <util/dag_hashedKeyMap.h>
#include <util/dag_stlqsort.h>
#include <memory/dag_framemem.h>
#include <EASTL/fixed_vector.h>
#include <debug/dag_log.h>

namespace build_bvh
{

template <typename IdxT>
static bool findSharedEdgeImpl(const IdxT *triA, const IdxT *triB, int &v0, int &v1, int &v2, int &v3, bool &bFwd)
{
  // Find shared edge and unique vertices for both triangles.
  int sharedA[2], nSharedA = 0, uniqAVal = -1, uniqAIdx = -1;
  for (int a = 0; a < 3; ++a)
  {
    bool found = false;
    for (int b = 0; b < 3; ++b)
      if (triA[a] == triB[b])
      {
        if (nSharedA < 2)
          sharedA[nSharedA++] = a;
        found = true;
        break;
      }
    if (!found)
    {
      uniqAVal = triA[a];
      uniqAIdx = a;
    }
  }
  if (nSharedA != 2)
    return false;
  int uniqBVal = -1;
  for (int b = 0; b < 3; ++b)
    if (triB[b] != triA[sharedA[0]] && triB[b] != triA[sharedA[1]])
    {
      uniqBVal = triB[b];
      break;
    }
  if (uniqAVal < 0 || uniqBVal < 0)
    return false;

  // Strip quad: v0,v2 = shared edge in triA's winding (prev/next of triA's apex), v1 = triA apex,
  // v3 = triB apex. encodeQuad relabels this to (apex, shared, shared, otherApex), picks the apex with
  // the smaller signed-offset spread, and derives the strip flip from bFwd (below).
  // Signed 13-bit offsets carry any sign, so there is no min-index-must-be-v0/v1 constraint -- every
  // edge-paired quad whose verts encodeQuad can reach from one apex is accepted; the span gate in
  // buildQuadPrims (and encodeQuad's .valid) does the actual encodability test.
  v0 = triA[(uniqAIdx + 2) % 3]; // prev in triA's winding (shared edge end)
  v1 = uniqAVal;                 // triA apex
  v2 = triA[(uniqAIdx + 1) % 3]; // next in triA's winding (shared edge end)
  v3 = uniqBVal;                 // triB apex
  // triA runs the shared edge v2->v0; bFwd records whether triB runs it the opposite way (v0->v2, the
  // manifold-consistent direction). encodeQuad needs triB's true winding because the quad leaf only has
  // a flip bit for its 2nd sub-tri -- forcing flip would reverse an inconsistently-wound triB.
  int pb0 = -1, pb2 = -1;
  for (int b = 0; b < 3; ++b)
  {
    if ((int)triB[b] == v0)
      pb0 = b;
    else if ((int)triB[b] == v2)
      pb2 = b;
  }
  bFwd = (pb0 >= 0 && pb2 >= 0 && (pb0 + 1) % 3 == pb2);
  return true;
}


static uint64_t edgeKey(unsigned a, unsigned b) { return a < b ? ((uint64_t)a << 32) | b : ((uint64_t)b << 32) | a; }

template <typename IdxT>
void buildQuadPrims(Tab<QuadPrim> &prims, int &quadCount, int &singleCount, const IdxT *optIdx, int faceCount, const vec4f *verts4,
  const uint8_t *face_user)
{
  prims.clear();
  prims.reserve(faceCount);
#if DAGOR_DBGLEVEL > 0
  for (int t = 0; face_user && t < faceCount; ++t)
    G_ASSERTF(face_user[t] <= QUAD_LEAF_USER_MASK, "face_user[%d] = %u does not fit %d bits", t, face_user[t], QUAD_LEAF_USER_BITS);
#endif
  // A value the field cannot hold names nothing, so it is never truncated into a different valid
  // index: 0 is the one index every owner has. Read through here rather than checked in a pass, so
  // a release build costs one compare per use and no walk of its own.
  auto userOf = [face_user](int t) -> uint8_t { return face_user && face_user[t] <= QUAD_LEAF_USER_MASK ? face_user[t] : 0; };
  struct EdgeTris
  {
    int t0 = -1, t1 = -1;
  };
  HashedKeyMap<uint64_t, EdgeTris, 0ULL, oa_hashmap_util::MumStepHash<uint64_t>> edgeMap;
  edgeMap.reserve(faceCount * 3);
  for (int t = 0; t < faceCount; ++t)
  {
    // Zero-area duplicate-index faces never enter the edge map: two such faces sharing the repeated
    // edge (x,x) would pair into a quad (marking both matched, bypassing the singles-loop drop), and
    // edgeKey(0,0) collides with the map's EmptyKey sentinel. Kept out here, they always arrive at
    // the singles loop below, which drops them.
    const unsigned i0 = optIdx[t * 3], i1 = optIdx[t * 3 + 1], i2 = optIdx[t * 3 + 2];
    if (i0 == i1 || i1 == i2 || i0 == i2)
      continue;
    for (int e = 0; e < 3; ++e)
    {
      unsigned a = optIdx[t * 3 + e], b = optIdx[t * 3 + (e + 1) % 3];
      auto [et, isNew] = edgeMap.emplace_if_missing(edgeKey(a, b));
      if (et->t0 < 0)
        et->t0 = t;
      else if (et->t1 < 0)
        et->t1 = t;
    }
  }

  struct CandidateQuad
  {
    QuadPrim qp;
    int tA, tB;
    float score;
  };
  // Scratch per-function-call containers go through framemem: they live only for the
  // duration of this call and freeing in declaration-reverse order stays within the stack.
  dag::Vector<CandidateQuad, framemem_allocator> candidates;
  edgeMap.iterate([&](uint64_t, const EdgeTris &et) {
    if (et.t0 < 0 || et.t1 < 0)
      return;
    if (userOf(et.t0) != userOf(et.t1))
      return; // one user value per leaf: faces that disagree stay unpaired (singles below)
    int v0, v1, v2, v3;
    bool bFwd;
    if (!findSharedEdgeImpl(&optIdx[et.t0 * 3], &optIdx[et.t1 * 3], v0, v1, v2, v3, bFwd))
      return;
    // encodeQuad bases the leaf at apex v1 or v3 and stores signed 13-bit offsets to the other three
    // verts, so the quad is encodable iff one of those apexes reaches all three within [QUAD_O_MIN,
    // QUAD_O_MAX]. Plain max-min span is too strict (a centered apex spans up to ~2*QUAD_O_MAX); test
    // exactly what encodeQuad's .valid would. Pairs that fail stay unpaired (kept as singles below).
    auto apexFits = [](long ap, long b, long c, long d) {
      return build_bvh::offFits(b - ap) && build_bvh::offFits(c - ap) && build_bvh::offFits(d - ap);
    };
    if (!apexFits(v1, v2, v0, v3) && !apexFits(v3, v0, v2, v1))
      return;
    QuadPrim qp;
    qp.v[0] = v0;
    qp.v[1] = v1;
    qp.v[2] = v2;
    qp.v[3] = v3;
    qp.bFwd = bFwd;
    qp.user = userOf(et.t0);
    bbox3f cb;
    v_bbox3_init(cb, verts4[v0]);
    v_bbox3_add_pt(cb, verts4[v1]);
    v_bbox3_add_pt(cb, verts4[v2]);
    v_bbox3_add_pt(cb, verts4[v3]);
    float area = build_bvh::calculateSurfaceArea(cb);

    vec3f e0A = v_sub(verts4[v1], verts4[v0]), e1A = v_sub(verts4[v2], verts4[v0]);
    vec3f nA = v_cross3(e0A, e1A);
    vec3f e0B = v_sub(verts4[v3], verts4[v0]), e1B = v_sub(verts4[v2], verts4[v0]);
    vec3f nB = v_cross3(e0B, e1B);
    vec3f lenA = v_length3(nA), lenB = v_length3(nB);
    float coplanar = 1.f;
    if (v_extract_x(lenA) > 1e-10f && v_extract_x(lenB) > 1e-10f)
      coplanar = fabsf(v_extract_x(v_div(v_dot3(nA, nB), v_mul(lenA, lenB))));

    float score = area * (2.f - coplanar);
    candidates.push_back({qp, et.t0, et.t1, score});
  });

  stlsort::sort(candidates.begin(), candidates.end(),
    [](const CandidateQuad &a, const CandidateQuad &b) { return a.score < b.score; });

  dag::Vector<bool, framemem_allocator> matched(faceCount, false);
  quadCount = 0;
  for (auto &c : candidates)
  {
    if (matched[c.tA] || matched[c.tB])
      continue;
    matched[c.tA] = matched[c.tB] = true;
    prims.push_back(c.qp);
    quadCount++;
  }

  singleCount = 0;
  for (int t = 0; t < faceCount; ++t)
  {
    if (matched[t])
      continue;
    const uint32_t a = optIdx[t * 3], b = optIdx[t * 3 + 1], c = optIdx[t * 3 + 2];
    // Drop zero-area duplicate-index faces (kept out of the edge map above, so they are always
    // unmatched here). A single with two equal indices encodes a -1 leaf offset that writeQuadLeaf
    // clamps to 0; the decode then reads it as +1, addressing a vertex outside the prim. A node
    // whose faces are all degenerate yields no prims and is dropped by the caller.
    if (a == b || b == c || a == c)
      continue;
    QuadPrim sp;
    sp.v[0] = a;
    sp.v[1] = b;
    sp.v[2] = c;
    sp.v[3] = ~0u;
    sp.user = userOf(t);
    prims.push_back(sp);
    singleCount++;
  }
}

template void buildQuadPrims<uint16_t>(Tab<QuadPrim> &, int &, int &, const uint16_t *, int, const vec4f *, const uint8_t *);
template void buildQuadPrims<uint32_t>(Tab<QuadPrim> &, int &, int &, const uint32_t *, int, const vec4f *, const uint8_t *);

void writeQuadBox(uint8_t *blasData, int dataOffset, vec4f bmin, vec4f bmax, vec4f scale, vec4f ofs, uint32_t skip, bool useHalves)
{
  bmin = v_madd(bmin, scale, ofs);
  bmax = v_madd(bmax, scale, ofs);
  vec4i bminI, bmaxI;
  if (useHalves)
  {
    bminI = v_float_to_half_down(bmin);
    bmaxI = v_float_to_half_up(bmax);
  }
  else
    swblas_quantize_box_u16(bmin, bmax, bminI, bmaxI);
  build_bvh::write_pair_halves((uint32_t *)(blasData + dataOffset), bminI, bmaxI);
  *(uint32_t *)(blasData + dataOffset + 12) = skip;
}

void addQuadPrimitivesAABBList(bbox3f *boxes, const QuadPrim *prims, int primCount, const vec4f *verts)
{
  for (int i = 0; i < primCount; ++i)
  {
    auto &p = prims[i];
    v_bbox3_init(boxes[i], verts[p.v0()]);
    v_bbox3_add_pt(boxes[i], verts[p.v1()]);
    v_bbox3_add_pt(boxes[i], verts[p.v2()]);
    if (!p.isSingle())
      v_bbox3_add_pt(boxes[i], verts[p.v3()]);
    boxes[i].bmin = v_perm_xyzd(boxes[i].bmin, v_cast_vec4f(v_splatsi(i)));
    boxes[i].bmax = v_perm_xyzd(boxes[i].bmax, v_zero());
  }
}

// ============================================================================
// Double-quad leaf builder (two edge-paired quads = up to 4 triangles per leaf)
// ============================================================================

namespace
{
static bbox3f primBox(const QuadPrim &p, const vec4f *verts)
{
  bbox3f bx;
  v_bbox3_init(bx, verts[p.v0()]);
  v_bbox3_add_pt(bx, verts[p.v1()]);
  v_bbox3_add_pt(bx, verts[p.v2()]);
  if (!p.isSingle())
    v_bbox3_add_pt(bx, verts[p.v3()]);
  return bx;
}

static uint32_t primGroup(const QuadPrim &p, const uint32_t *vert_group) { return vert_group ? vert_group[p.v0()] : 0u; }

// Pair the direct leaf children of each internal node (spatially adjacent by SAH construction).
static void collectLeafPairs(const bbox3f *nodes, const QuadPrim *prims, const uint32_t *vert_group, float mf, int node,
  dag::Vector<DoubleQuadPrim> &out)
{
  int faceIndex = v_extract_wi(v_cast_vec4i(nodes[node].bmin));
  if (faceIndex >= 0) // lone leaf (single-prim root)
  {
    out.push_back(DoubleQuadPrim{prims[faceIndex], QuadPrim{}, false});
    return;
  }
  int childrenCount = v_extract_wi(v_cast_vec4i(nodes[node].bmax));
  // Per-visited-node scratch: a SAH node has few direct leaf kids, so keep them inline (8) and spill
  // to framemem only for the rare wide node, avoiding an O(nodes) churn of tiny heap allocations.
  eastl::fixed_vector<int, 8, true, framemem_allocator> leafKids;
  int startNode = node + 1;
  for (int i = 0; i < childrenCount; ++i)
  {
    int childFace = v_extract_wi(v_cast_vec4i(nodes[startNode].bmin));
    int sub = childFace >= 0 ? 1 : (-childFace + 1);
    if (childFace >= 0)
      leafKids.push_back(startNode);
    else
      collectLeafPairs(nodes, prims, vert_group, mf, startNode, out);
    startNode += sub;
  }
  int k = (int)leafKids.size();
  eastl::fixed_vector<char, 8, true, framemem_allocator> used(k, 0);
  for (int i = 0; i < k; ++i)
  {
    if (used[i])
      continue;
    int pi = v_extract_wi(v_cast_vec4i(nodes[leafKids[i]].bmin));
    QEnc ei = encodeQuad(prims[pi]);
    float sai = build_bvh::calculateSurfaceArea(nodes[leafKids[i]]);
    uint32_t gi = primGroup(prims[pi], vert_group);
    int best = -1;
    float bestSA = 3.4e38f;
    for (int j = i + 1; j < k; ++j)
    {
      if (used[j])
        continue;
      int pj = v_extract_wi(v_cast_vec4i(nodes[leafKids[j]].bmin));
      if (primGroup(prims[pj], vert_group) != gi)
        continue; // never pair across source groups (e.g. collision nodes)
      if (prims[pj].user != prims[pi].user)
        continue; // one user value per leaf
      bbox3f u = nodes[leafKids[i]];
      v_bbox3_add_box(u, nodes[leafKids[j]]);
      float saU = build_bvh::calculateSurfaceArea(u);
      float saj = build_bvh::calculateSurfaceArea(nodes[leafKids[j]]);
      if (saU > mf * (sai + saj))
        continue; // SA-ratio gate: skip pairs that loosen the box too much
      QEnc ej = encodeQuad(prims[pj]);
      uint32_t lo = min(ei.base, ej.base), hi = max(ei.base, ej.base);
      if (!ei.valid || !ej.valid || (hi - lo) > QUADB_BASE_MAX)
        continue; // offset/base-delta overflow: cannot encode this pair
      if (saU < bestSA)
      {
        bestSA = saU;
        best = j;
      }
    }
    if (best >= 0)
    {
      int pj = v_extract_wi(v_cast_vec4i(nodes[leafKids[best]].bmin));
      out.push_back(DoubleQuadPrim{prims[pi], prims[pj], true});
      used[i] = used[best] = 1;
    }
    else
    {
      out.push_back(DoubleQuadPrim{prims[pi], QuadPrim{}, false});
      used[i] = 1;
    }
  }
}

static void writeDoubleQuadLeaf(uint8_t *blasData, const bbox3f *nodes, const DoubleQuadPrim *dq, vec4f scale, vec4f ofs,
  int vertDataOfs, int node, int &dataOffset, int vertStride, bool useHalves)
{
  int idx = v_extract_wi(v_cast_vec4i(nodes[node].bmin));
  const DoubleQuadPrim &d = dq[idx];
  QEnc A = encodeQuad(d.a);
  // An encodable prim has every vertex within the signed 13-bit offset range of its apex (callers must
  // pre-pass meshes through build_bvh::leafOrderVertexFetch; see dag_quadBLASBuilder.h). If one slips
  // through, packQuadA/packQuadB flag it and this leaf is rewritten as a degenerate no-hit leaf below --
  // a visible, harmless failure instead of silent BLAS corruption from clamped offsets/base.
  uint32_t w2hi = 0, w3 = 0;
  bool overflow = false;
  if (d.hasB)
  {
    QEnc B = encodeQuad(d.b);
    if (B.base < A.base) // quad A must hold the smaller base so deltaB >= 0
    {
      QEnc t = A;
      A = B;
      B = t;
    }
    overflow = packQuadB(B, A.base, w2hi, w3); // unencodable B drops to single quad A
  }
  int leafOfs = dataOffset + 16;
  int relBase = int(A.base * vertStride + vertDataOfs) - leafOfs;
  uint32_t w0, w1, w2flip;
  overflow |= packQuadA(A, relBase, w0, w1, w2flip);
  if (overflow)
  {
    LOGERR_ONCE("daBVH: double-quad leaf offset overflow; mesh not pre-passed via leafOrderVertexFetch? "
                "emitting a degenerate (no-hit) leaf instead of clamped geometry");
    // Zero-area leaf: o1=o2=o3=0 and base 0 collapse all three indices to one vertex (a point), and
    // o1b==o2b==0 makes hasB read false. A degenerate triangle never reports a hit, so an overflow that
    // slipped past the pre-pass yields "no geometry here" rather than a wrong intersection from clamped
    // offsets/base. (Reading the leaf body bytes as that one vertex stays in bounds.)
    w0 = QUAD_LEAF_FLAG;
    w1 = w2flip = w2hi = w3 = 0;
  }
  else
  {
    // Both quads carry the same value (buildDoubleQuadPrims never pairs across values), so the
    // A/B base swap above cannot change it. Never stamped on the overflow leaf: its all-zero body
    // is a contract the SoA4 converter and bvhIO both validate.
    G_ASSERTF(!d.hasB || d.a.user == d.b.user, "double-quad leaf mixes user values %u and %u", d.a.user, d.b.user);
    w3 |= ((uint32_t)d.a.user & QUAD_LEAF_USER_MASK) << QUAD_LEAF_USER_SHIFT;
  }

  writeQuadBox(blasData, dataOffset, nodes[node].bmin, nodes[node].bmax, scale, ofs, w0, useHalves);
  *(uint32_t *)(blasData + leafOfs) = w1;
  *(uint32_t *)(blasData + leafOfs + 4) = w2hi | w2flip; // flipA shares W2 with quad B fields
  *(uint32_t *)(blasData + leafOfs + 8) = w3;
  dataOffset += BVH_BLAS_LEAF_SIZE;
}

static int writeDoubleQuadBVH2Impl(uint8_t *blasData, const bbox3f *nodes, const DoubleQuadPrim *dq, vec4f scale, vec4f ofs,
  int vertDataOfs, int node, int root, int &dataOffset, int vertStride, int depth, bool useHalves)
{
  G_ASSERTF(depth <= BVH_MAX_BLAS_DEPTH, "writeDoubleQuadBVH2: depth %d exceeds limit %d", depth, BVH_MAX_BLAS_DEPTH);
  int faceIndex = v_extract_wi(v_cast_vec4i(nodes[node].bmin));
  int childrenCount = v_extract_wi(v_cast_vec4i(nodes[node].bmax));
  if (faceIndex >= 0)
  {
    writeDoubleQuadLeaf(blasData, nodes, dq, scale, ofs, vertDataOfs, node, dataOffset, vertStride, useHalves);
    return 1;
  }
  int nodeSize = -faceIndex;
  int tempdataOffset = 0;
  if (node != root)
  {
    dataOffset += BVH_BLAS_NODE_SIZE;
    tempdataOffset = dataOffset;
  }
  int startNode = node + 1;
  for (int i = 0; i < childrenCount; ++i)
    startNode += writeDoubleQuadBVH2Impl(blasData, nodes, dq, scale, ofs, vertDataOfs, startNode, root, dataOffset, vertStride,
      depth + 1, useHalves);
  if (node != root)
  {
    int offset = dataOffset - tempdataOffset;
    writeQuadBox(blasData, tempdataOffset - BVH_BLAS_NODE_SIZE, nodes[node].bmin, nodes[node].bmax, scale, ofs, offset, useHalves);
  }
  return nodeSize + 1;
}
} // anonymous namespace

void buildDoubleQuadPrims(dag::Vector<DoubleQuadPrim> &out, const QuadPrim *prims, int prims_count, const vec4f *verts,
  const uint32_t *vert_group, float merge_factor)
{
  out.clear();
  if (prims_count <= 0)
    return;
  out.reserve(prims_count);
  Tab<bbox3f> boxes(framemem_ptr());
  boxes.resize(prims_count);
  addQuadPrimitivesAABBList(boxes.data(), prims, prims_count, verts);
  Tab<bbox3f> nodes(framemem_ptr());
  int maxDepth = 0;
  int root = create_bvh_node_sah(nodes, boxes.data(), (uint32_t)prims_count, 4, maxDepth);
  collectLeafPairs(nodes.data(), prims, vert_group, merge_factor, root, out);
}

void addDoubleQuadPrimitivesAABBList(bbox3f *boxes, const DoubleQuadPrim *dq, int dq_count, const vec4f *verts)
{
  for (int i = 0; i < dq_count; ++i)
  {
    bbox3f bx = primBox(dq[i].a, verts);
    if (dq[i].hasB)
      v_bbox3_add_box(bx, primBox(dq[i].b, verts));
    bx.bmin = v_perm_xyzd(bx.bmin, v_cast_vec4f(v_splatsi(i)));
    bx.bmax = v_perm_xyzd(bx.bmax, v_zero());
    boxes[i] = bx;
  }
}

int writeDoubleQuadBVH2(uint8_t *blasData, const bbox3f *nodes, const DoubleQuadPrim *dq, vec4f scale, vec4f ofs, int vertDataOfs,
  int node, int root, int &dataOffset, int vertStride, bool useHalves)
{
  return writeDoubleQuadBVH2Impl(blasData, nodes, dq, scale, ofs, vertDataOfs, node, root, dataOffset, vertStride, 0, useHalves);
}

bool writeDoubleQuadBLAS(dag::Vector<uint8_t> &out_data, bbox3f box, const bbox3f *nodes, int root, const DoubleQuadPrim *dq,
  int dq_count, const uint8_t *verts_data, int vert_stride_bytes, int verts_count)
{
  G_ASSERT_RETURN(nodes != nullptr && dq != nullptr && verts_data != nullptr, false);
  G_ASSERT_RETURN(dq_count > 0 && verts_count > 0 && root >= 0, false);
  G_ASSERT_RETURN(vert_stride_bytes >= 12, false);

  vec4f maxExt = v_max(v_bbox3_size(box), v_splats(blas_size_eps));
  vec4f center = v_mul(v_add(box.bmin, box.bmax), V_C_HALF);
  vec4f scale = v_div(v_splats(65535.f), maxExt);
  vec4f ofs = v_sub(v_splats(32767.5f), v_mul(center, scale));

  const int rootFaceIndex = v_extract_wi(v_cast_vec4i(nodes[root].bmin));
  const int treeBytes = calcBLASTreeBytes(-rootFaceIndex + 1, dq_count);
  static constexpr uint32_t vertex_size = 12;
  const int totalBytes = treeBytes + verts_count * vertex_size;
  // Bail (no BLAS) if the [tree][float3 verts] span would push a leaf's apex base past the unsigned
  // 24-bit W1 field; writeDoubleQuadBVH2 would otherwise clamp it and emit corrupt geometry. SWRT and
  // collision pre-pass their meshes so this never fires; it guards any other / future caller.
  if ((int64_t)treeBytes + (int64_t)(verts_count - 1) * (int)vertex_size > (int64_t)QUAD_BASE_BYTE_MAX)
  {
    LOGERR_ONCE("daBVH: writeDoubleQuadBLAS vert span exceeds the unsigned 24-bit leaf base range; emitting no BLAS");
    out_data.clear();
    return false;
  }

  out_data.resize(totalBytes);
  uint8_t *dst = out_data.data();
  int dataOffset = 0;
  writeDoubleQuadBVH2(dst, nodes, dq, scale, ofs, /*vertDataOfs*/ treeBytes, root, root, dataOffset, 12, false);
  G_ASSERTF(dataOffset == treeBytes, "writeDoubleQuadBLAS: tree write produced %d bytes, expected %d", dataOffset, treeBytes);

  uint8_t *vertDst = dst + treeBytes;
  for (int v = 0; v < verts_count; ++v)
  {
    vec4f xyz = v_ldu_p3_safe(reinterpret_cast<const float *>(verts_data + v * vert_stride_bytes));
    vec4f encoded = v_madd(xyz, scale, ofs);
    alignas(16) float f[4];
    v_st(f, encoded);
    memcpy(vertDst + v * vertex_size, f, vertex_size); // -V1086
  }
  return true;
}

} // namespace build_bvh
