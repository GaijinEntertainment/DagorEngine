// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "swCommon.h"
#include <daBVH/dag_quadBLASBuilder.h>
#include <daBVH/swBLASLeafDefs.hlsli>
#include "shaders/swBVHDefine.hlsli"
#include <util/dag_hashedKeyMap.h>
#include <util/dag_stlqsort.h>
#include <memory/dag_framemem.h>

namespace build_bvh
{

template <typename IdxT>
static bool findSharedEdgeImpl(const IdxT *triA, const IdxT *triB, int &v0, int &v1, int &v2, int &v3)
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
  int uniqBVal = -1, uniqBIdx = -1;
  for (int b = 0; b < 3; ++b)
    if (triB[b] != triA[sharedA[0]] && triB[b] != triA[sharedA[1]])
    {
      uniqBVal = triB[b];
      uniqBIdx = b;
      break;
    }
  if (uniqAVal < 0 || uniqBVal < 0)
    return false;

  // Winding-correct encoding requires minIdx to be v0 or v1.
  // v0,v2 = shared edge (winding-ordered from whichever tri has unique vertex = v1).
  // v1 = unique vertex of "triA" role, v3 = unique vertex of "triB" role.
  // We can choose which physical triangle plays "triA" role to ensure min(v0,v1,v2,v3) is v0 or v1.
  // Assign triA role to the triangle whose unique vertex is minAll, or whose shared vertex ordering
  // places minAll at v0. Try "triA as triA" first, then swap if needed.
  auto tryAssign = [&](const IdxT *tA, int uAIdx, int uA, int uB) -> bool {
    int s0 = tA[(uAIdx + 2) % 3]; // prev in tA's winding
    int s1 = tA[(uAIdx + 1) % 3]; // next in tA's winding
    int m = min(min(s0, uA), min(s1, uB));
    if (m == s0 || m == uA)
    {
      v0 = s0;
      v1 = uA;
      v2 = s1;
      v3 = uB;
      return true;
    }
    return false;
  };

  // Try original assignment (triA role = physical triA)
  if (tryAssign(triA, uniqAIdx, uniqAVal, uniqBVal))
    return true;
  // Try swapped (triA role = physical triB, triB role = physical triA)
  if (tryAssign(triB, uniqBIdx, uniqBVal, uniqAVal))
    return true;
  return false;
}


static uint64_t edgeKey(unsigned a, unsigned b) { return a < b ? ((uint64_t)a << 32) | b : ((uint64_t)b << 32) | a; }

template <typename IdxT>
void buildQuadPrims(Tab<QuadPrim> &prims, int &quadCount, int &singleCount, const IdxT *optIdx, int faceCount, const vec4f *verts4)
{
  prims.clear();
  prims.reserve(faceCount);
  struct EdgeTris
  {
    int t0 = -1, t1 = -1;
  };
  HashedKeyMap<uint64_t, EdgeTris, 0ULL, oa_hashmap_util::MumStepHash<uint64_t>> edgeMap;
  edgeMap.reserve(faceCount * 3);
  for (int t = 0; t < faceCount; ++t)
    for (int e = 0; e < 3; ++e)
    {
      unsigned a = optIdx[t * 3 + e], b = optIdx[t * 3 + (e + 1) % 3];
      auto [et, isNew] = edgeMap.emplace_if_missing(edgeKey(a, b));
      if (et->t0 < 0)
        et->t0 = t;
      else if (et->t1 < 0)
        et->t1 = t;
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
    int v0, v1, v2, v3;
    if (!findSharedEdgeImpl(&optIdx[et.t0 * 3], &optIdx[et.t1 * 3], v0, v1, v2, v3))
      return;
    int minIdx = min(min(v0, v1), min(v2, v3));
    // Winding-correct quad encoding requires minIdx to be v0 (shared, ->fan) or v1 (uniqA, ->strip).
    // When minIdx is v2 or v3, the decoded triangles would have wrong winding - reject as quad.
    if (minIdx != v0 && minIdx != v1)
      return;
    QuadPrim qp;
    qp.v[0] = v0;
    qp.v[1] = v1;
    qp.v[2] = v2;
    int q1, q2, q3;
    if (minIdx == v0)
    {
      qp.v[3] = v3 | 0x80000000u; // fan
      q1 = v1;
      q2 = v2;
      q3 = v3;
    }
    else // minIdx == v1
    {
      qp.v[3] = v3; // strip
      q1 = v2;
      q2 = v0;
      q3 = v3;
    }
    if (q1 - minIdx - 1 < 0 || (unsigned)(q1 - minIdx - 1) > QUAD_O1_MAX || q2 - minIdx - 1 < 0 ||
        (unsigned)(q2 - minIdx - 1) > QUAD_O2_MAX || q3 - minIdx - 1 < 0 || (unsigned)(q3 - minIdx - 1) > QUAD_O3_MAX)
      return;
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
    QuadPrim sp;
    sp.v[0] = optIdx[t * 3];
    sp.v[1] = optIdx[t * 3 + 1];
    sp.v[2] = optIdx[t * 3 + 2];
    sp.v[3] = ~0u;
    prims.push_back(sp);
    singleCount++;
  }
}

template void buildQuadPrims<uint16_t>(Tab<QuadPrim> &, int &, int &, const uint16_t *, int, const vec4f *);
template void buildQuadPrims<uint32_t>(Tab<QuadPrim> &, int &, int &, const uint32_t *, int, const vec4f *);

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
  {
    bminI = v_clampi(v_cvt_floori(bmin), v_zeroi(), v_splatsi(65535));
    bmaxI = v_clampi(v_cvt_ceili(bmax), v_zeroi(), v_splatsi(65535));
  }
  build_bvh::write_pair_halves((uint32_t *)(blasData + dataOffset), bminI, bmaxI);
  *(uint32_t *)(blasData + dataOffset + 12) = skip;
}

void writeQuadLeaf(uint8_t *blasData, const bbox3f *nodes, const QuadPrim *prims, vec4f scale, vec4f ofs, int vertDataOfs, int node,
  int &dataOffset, int vertStride, bool useHalves)
{
  int primIdx = v_extract_wi(v_cast_vec4i(nodes[node].bmin));
  const QuadPrim &p = prims[primIdx];

  writeQuadBox(blasData, dataOffset, nodes[node].bmin, nodes[node].bmax, scale, ofs, 0, useHalves);
  int leafOfs = dataOffset + 16;

  if (p.isSingle())
  {
    // Cyclic rotation to put minIdx first while preserving winding.
    // Original tri = (v0, v1, v2). Cyclic rotations: (v1,v2,v0), (v2,v0,v1).
    uint32_t minIdx = min(p.v0(), min(p.v1(), p.v2()));
    uint32_t a, b;
    if (minIdx == p.v0())
    {
      a = p.v1();
      b = p.v2();
    }
    else if (minIdx == p.v1())
    {
      a = p.v2();
      b = p.v0();
    }
    else
    {
      a = p.v0();
      b = p.v1();
    }
    *(int *)(blasData + leafOfs) = int(minIdx * vertStride + vertDataOfs) - leafOfs;
    int o1 = a - minIdx - 1, o2 = b - minIdx - 1;
    uint32_t skip =
      (o1 & QUAD_O1_MASK) | (((unsigned)o2 & QUAD_O2_MASK) << QUAD_O2_SHIFT) | (((unsigned)o2 & QUAD_O3_MASK) << QUAD_O3_SHIFT);
    skip |= QUAD_LEAF_FLAG;
    *(uint32_t *)(blasData + dataOffset + 12) = skip;
  }
  else // quad
  {
    // QuadPrim from findSharedEdge: v0,v2 = shared edge (winding-ordered), v1 = uniqA, v3 = uniqB.
    // triA = (v0, v1, v2), triB = (v2, v0, v3) - both CCW.
    // Decode: tri1=(q0,q1,q2), fan tri2=(q0,q2,q3), strip tri2=(q2,q1,q3).
    // Winding-correct encoding only works when minIdx is v0 (->fan) or v1 (->strip):
    //   minIdx=v0: fan,  q=(v0, v1, v2, v3). tri1=(v0,v1,v2)=triA, tri2=(v0,v2,v3)=cyclic(triB)
    //   minIdx=v1: strip,q=(v1, v2, v0, v3). tri1=(v1,v2,v0)=cyclic(triA), tri2=(v0,v2,v3)=cyclic(triB)
    // When minIdx is v2 or v3, winding cannot be preserved - handled as single in buildQuadPrims.
    uint32_t minIdx = min(min(p.v0(), p.v1()), min(p.v2(), p.v3()));
    G_ASSERT(minIdx == p.v0() || minIdx == p.v1());
    uint32_t q0, q1, q2, q3;
    bool isFan;
    if (minIdx == p.v0())
    {
      q0 = p.v0();
      q1 = p.v1();
      q2 = p.v2();
      q3 = p.v3();
      isFan = true;
    }
    else // minIdx == p.v1()
    {
      q0 = p.v1();
      q1 = p.v2();
      q2 = p.v0();
      q3 = p.v3();
      isFan = false;
    }
    *(int *)(blasData + leafOfs) = int(q0 * vertStride + vertDataOfs) - leafOfs;
    int o1 = q1 - q0 - 1, o2 = q2 - q0 - 1, o3 = q3 - q0 - 1;
    uint32_t skip =
      (o1 & QUAD_O1_MASK) | (((unsigned)o2 & QUAD_O2_MASK) << QUAD_O2_SHIFT) | (((unsigned)o3 & QUAD_O3_MASK) << QUAD_O3_SHIFT);
    if (isFan)
      skip |= QUAD_FAN_FLAG;
    skip |= QUAD_LEAF_FLAG;
    *(uint32_t *)(blasData + dataOffset + 12) = skip;
  }
  dataOffset += BVH_BLAS_LEAF_SIZE;
}

static int writeQuadBVH2Impl(uint8_t *blasData, const bbox3f *nodes, const QuadPrim *prims, vec4f scale, vec4f ofs, int vertDataOfs,
  int node, int root, int &dataOffset, int vertStride, int depth, bool useHalves)
{
  G_ASSERTF(depth <= 64, "writeQuadBVH2: depth %d exceeds limit", depth);
  int faceIndex = v_extract_wi(v_cast_vec4i(nodes[node].bmin));
  int childrenCount = v_extract_wi(v_cast_vec4i(nodes[node].bmax));

  if (faceIndex >= 0) // leaf
  {
    writeQuadLeaf(blasData, nodes, prims, scale, ofs, vertDataOfs, node, dataOffset, vertStride, useHalves);
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
    startNode += writeQuadBVH2Impl(blasData, nodes, prims, scale, ofs, vertDataOfs, startNode, root, dataOffset, vertStride, depth + 1,
      useHalves);

  if (node != root)
  {
    int offset = (dataOffset - tempdataOffset);
    writeQuadBox(blasData, tempdataOffset - BVH_BLAS_NODE_SIZE, nodes[node].bmin, nodes[node].bmax, scale, ofs, offset, useHalves);
  }
  return nodeSize + 1;
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

int writeQuadBVH2(uint8_t *blasData, const bbox3f *nodes, const QuadPrim *prims, vec4f scale, vec4f ofs, int vertDataOfs, int node,
  int root, int &dataOffset, int vertStride, bool useHalves)
{
  return writeQuadBVH2Impl(blasData, nodes, prims, scale, ofs, vertDataOfs, node, root, dataOffset, vertStride, 0, useHalves);
}

void writeQuadBLAS(dag::Vector<uint8_t> &out_data, bbox3f box, const bbox3f *nodes, int root, const QuadPrim *prims, int prims_count,
  const uint8_t *verts_data, int vert_stride_bytes, int verts_count)
{
  G_ASSERT_RETURN(nodes != nullptr && prims != nullptr && verts_data != nullptr, );
  G_ASSERT_RETURN(prims_count > 0 && verts_count > 0 && root >= 0, );
  G_ASSERT_RETURN(vert_stride_bytes >= 12, );

  vec4f maxExt = v_max(v_bbox3_size(box), v_splats(blas_size_eps));
  vec4f center = v_mul(v_add(box.bmin, box.bmax), V_C_HALF);
  vec4f scale = v_div(v_splats(65535.f), maxExt);
  vec4f ofs = v_sub(v_splats(32767.5f), v_mul(center, scale));

  // Tree byte budget. writeQuadBVH2 emits a 20-byte leaf per QuadPrim and a 16-byte internal
  // node per non-root internal. Leaf-root writes its single leaf (20 B). Internal-root skips
  // writing its own box entry (writeQuadBVH2Impl advances dataOffset only when node != root),
  // so the -16 only applies in the internal-root case.
  const int rootFaceIndex = v_extract_wi(v_cast_vec4i(nodes[root].bmin));
  int treeBytes;
  if (rootFaceIndex >= 0)
    treeBytes = BVH_BLAS_LEAF_SIZE;
  else
  {
    const int modelNodeCount = -rootFaceIndex + 1;
    const int internals = modelNodeCount - prims_count;
    treeBytes = (internals - 1) * BVH_BLAS_NODE_SIZE + prims_count * BVH_BLAS_LEAF_SIZE;
  }
  static constexpr uint32_t vertex_size = 12;
  const int totalBytes = treeBytes + verts_count * vertex_size;

  out_data.resize(totalBytes);
  uint8_t *dst = out_data.data();
  int dataOffset = 0;
  writeQuadBVH2(dst, nodes, prims, scale, ofs, /*vertDataOfs*/ treeBytes, root, root, dataOffset, 12, false);
  G_ASSERTF(dataOffset == treeBytes, "writeQuadBLAS: tree write produced %d bytes, expected %d", dataOffset, treeBytes);

  uint8_t *vertDst = dst + treeBytes;
  for (int v = 0; v < verts_count; ++v)
  {
    // v_ldu_p3_safe reads exactly 12 bytes, matching the documented contract (first 12 bytes at each stride offset).
    vec4f xyz = v_ldu_p3_safe(reinterpret_cast<const float *>(verts_data + v * vert_stride_bytes));
    vec4f encoded = v_madd(xyz, scale, ofs);
    alignas(16) float f[4];
    v_st(f, encoded);
    memcpy(vertDst + v * vertex_size, f, vertex_size); // -V1086
  }
}

} // namespace build_bvh
