// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <vecmath/dag_vecMath.h>
#include <daBVH/dag_swBLAS_soa4.h>
#include <generic/dag_tab.h>
#include <generic/dag_sort.h>
#include <math/dag_mathBase.h>
#include <memory/dag_framemem.h>
#include <util/dag_hashedKeyMap.h>

// Jolt's MeshShape active-edge rule (sFindActiveEdges, 5 degree threshold): an edge with one or 3+
// triangles is active, a shared concave edge is not, a shared convex or back-to-back one past the
// threshold is.
// out_flags[t] bit e = edge (v[e], v[e+1]) active.
static void active_edge_flags(const uint32_t *tri_idx, uint32_t tri_count, const vec4f *verts, uint8_t *out_flags)
{
  struct EdgeRef
  {
    uint64_t key;
    uint32_t tri, edge;
  };
  const uint32_t n = tri_count * 3;
  dag::Vector<EdgeRef, framemem_allocator> edges(n);
  dag::Vector<vec4f, framemem_allocator> normals(tri_count);
  for (uint32_t t = 0; t < tri_count; ++t)
  {
    const uint32_t *v = tri_idx + t * 3;
    for (uint32_t e = 0; e < 3; ++e)
      edges[t * 3 + e] = {((uint64_t)min(v[e], v[(e + 1) % 3]) << 32) | max(v[e], v[(e + 1) % 3]), t, e};
    // an exact-zero cross is a collapsed triangle: Jolt's plane is NaN there and fails every test. The
    // cross is taken in the chunk's vertex order, so a sliver within rounding of zero can come out
    // differently from Jolt's edge-anchored plane.
    const vec3f raw = v_cross3(v_sub(verts[v[1]], verts[v[0]]), v_sub(verts[v[2]], verts[v[0]]));
    normals[t] = v_extract_x(v_length3_sq_x(raw)) == 0.f ? v_zero() : v_norm3(raw);
  }
  // ties by triangle: a shared edge is tested from its lower triangle, as Jolt does
  stlsort::sort(edges.data(), edges.data() + n,
    [](const EdgeRef &x, const EdgeRef &y) { return x.key != y.key ? x.key < y.key : x.tri < y.tri; });
  for (uint32_t i = 0; i < n;)
  {
    uint32_t j = i + 1;
    while (j < n && edges[j].key == edges[i].key)
      ++j;
    bool active = true;
    if (j - i == 2)
    {
      const EdgeRef &e = edges[i];
      const vec3f n1 = normals[e.tri], n2 = normals[edges[i + 1].tri];
      const float cosA = v_extract_x(v_dot3_x(n1, n2));
      if (v_extract_x(v_length3_sq_x(n1)) == 0.f || v_extract_x(v_length3_sq_x(n2)) == 0.f)
        active = false;
      else if (cosA >= -0.999848f) // cos(179 deg): not back to back
      {
        const vec3f edgeDir = v_sub(verts[tri_idx[e.tri * 3 + (e.edge + 1) % 3]], verts[tri_idx[e.tri * 3 + e.edge]]);
        active = v_extract_x(v_dot3_x(v_cross3(n1, n2), edgeDir)) >= 0.f && cosA < 0.996195f; // convex, past cos(5 deg)
      }
    }
    if (active)
      for (uint32_t k = i; k < j; ++k)
        out_flags[edges[k].tri] |= (uint8_t)(1u << edges[k].edge);
    i = j;
  }
}

void soa4::computeEdgeFlags(uint8_t *tree, RootRef root, uint32_t verts_ofs, unsigned vert_count, vec3f bmin, vec3f inv_scale)
{
  const uint8_t *q8 = tree + verts_ofs;
  // Welded by vert21 word: the leaf-order renumber duplicates verts, and a rebuild from a chunk (a
  // bake) starts from those copies as source verts, so only the position tells the copies apart.
  HashedKeyMap<uint64_t, uint32_t, 0ULL, oa_hashmap_util::MumStepHash<uint64_t>> firstOf; // bit 63 is free: no key is the empty
                                                                                          // sentinel
  firstOf.reserve(vert_count);
  dag::Vector<uint32_t, framemem_allocator> canon(vert_count);
  for (unsigned i = 0; i < vert_count; ++i)
  {
    auto [slot, isNew] = firstOf.emplace_if_missing(*(const uint64_t *)(q8 + (size_t)i * 8u) | (1ull << 63));
    if (isNew)
      *slot = i;
    canon[i] = *slot;
  }
  struct Leaf
  {
    LeafRef ref;
    uint32_t firstTri, triCount;
  };
  // counted first: framemem regrowth would leave holes
  uint32_t leafCount = 0, triBound = 0;
  iterateLeafRefs(
    tree, root, [](vec3f, vec3f) { return true; },
    [&](vec3f, vec3f, LeafRef, const LeafLoc &l) {
      ++leafCount;
      triBound += quadLeafTriCount(leafFields(tree, l));
      return false;
    });
  dag::Vector<Leaf, framemem_allocator> leaves;
  dag::Vector<uint32_t, framemem_allocator> tris; // the leaf walk's triangle order, the order a reader expands
  leaves.reserve(leafCount);
  tris.reserve(triBound * 3u);
  iterateLeafRefs(
    tree, root, [](vec3f, vec3f) { return true; },
    [&](vec3f, vec3f, LeafRef ref, const LeafLoc &l) {
      const QuadLeafFields f = leafFields(tree, l);
      const uint32_t first = (uint32_t)tris.size() / 3u;
      // a leaf with verts outside the stream (a corrupt stream; the converter's no-hit placeholder,
      // whose base wraps) gets no triangles and zero flags
      uint32_t idx[12], n = 0; // a leaf holds at most four triangles
      bool ok = true;
      expandQuadLeafTris(f, ((uint32_t)l.bodyOfs + f.relBaseBytes - verts_ofs) / BVH_BLAS_VERT21_STRIDE,
        [&](uint32_t a, uint32_t b, uint32_t c) {
          ok &= a < vert_count && b < vert_count && c < vert_count;
          if (ok)
            idx[n++] = a, idx[n++] = b, idx[n++] = c;
        });
      for (uint32_t i = 0; ok && i < n; ++i)
        tris.push_back(canon[idx[i]]);
      leaves.push_back({ref, first, (uint32_t)tris.size() / 3u - first});
      return false;
    });
  // dequantized: what a Jolt build from the chunk sees
  dag::Vector<vec4f, framemem_allocator> verts(vert_count);
  for (unsigned i = 0; i < vert_count; ++i)
    verts[i] = v_madd(RayData::unpackVert21(q8 + (size_t)i * 8u), inv_scale, bmin);
  dag::Vector<uint8_t, framemem_allocator> triFlags(tris.size() / 3u, 0);
  active_edge_flags(tris.data(), (uint32_t)triFlags.size(), verts.data(), triFlags.data());
  for (const Leaf &l : leaves)
  {
    uint32_t bits = 0;
    for (uint32_t k = 0; k < l.triCount; ++k)
      bits |= (uint32_t)triFlags[l.firstTri + k] << (3u * k);
    *(uint16_t *)(tree + leafFlagsOfs(tree, l.ref)) = (uint16_t)bits;
  }
}
