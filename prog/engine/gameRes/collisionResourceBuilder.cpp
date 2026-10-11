// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameRes/collisionResourceBuilder.h>
#include <gameRes/collResStream.h>
#include "collisionGameResInternal.h"
#include <stdio.h>
#include <stdlib.h>
#include <math/dag_plane3.h>
#include <gameRes/dag_legacySceneRay.h>
#include <scene/dag_physMat.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_chainedMemIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_oodleIo.h>
#include <ioSys/dag_btagCompr.h>
#include <generic/dag_sort.h>
#include <generic/dag_staticTab.h>
#include <generic/dag_tab.h>
#include <memory/dag_genMemAlloc.h>
#include <EASTL/algorithm.h>
#include <EASTL/sort.h>
#include <EASTL/utility.h>
#include <debug/dag_debug.h>
#include <daBVH/dag_bvhBuild.h>
#include <daBVH/dag_swBLAS_soa4.h>
#include <daBVH/swCommon.h>
#include <daBVH/dag_quadBLASBuilder.h>
#include <daBVH/dag_swBLAS_soa4Convert.h>
#include <daBVH/swBLASLeafDefs.hlsli>
#include <util/dag_string.h>

using PoseMeta = CollisionResourceInstance::PoseMeta;
using NodeBlasChunkHeader = CollisionResource::NodeBlasChunkHeader;

static constexpr uint8_t POSE_CLASS_BITS =
  CollisionNode::IDENT | CollisionNode::TRANSLATE | CollisionNode::ORTHONORMALIZED | CollisionNode::ORTHOUNIFORM;

static constexpr int MAX_16BIT_INDEXED_VERTS = 0x10000;

// The one migration rule for a legacy stream (v0 and v1), whose sphere carries its own center: a
// SPHERE node takes that sphere as its geometry, so its box becomes the sphere's box, a BOX node's
// sphere is a function of its box alone in whatever frame the load keeps it, and every other node
// folds the center into a radius around the box center. The exporter's zero-vert marker (r < 0) stays.
static void collres_fold_legacy_sphere(CollisionResourceNodeType type, BBox3 &box, float &radius, const Point3 &c, float r)
{
  if (r < 0.f)
    radius = r;
  else if (type == COLLISION_NODE_TYPE_SPHERE)
  {
    box = BBox3(BSphere3(c, r));
    radius = r;
  }
  else if (type == COLLISION_NODE_TYPE_BOX)
    radius = box.width().length() * 0.5f; // a legacy stream stores this box authored-frame and the fit node-local
  else
    radius = length(c - box.center()) + r;
}

// The node sphere's radius: around the box center, over the points.
template <class P>
static float radius_around(const Point3 &center, dag::ConstSpan<P> points)
{
  float r2 = 0.f;
  for (const P &p : points)
    inplace_max(r2, lengthSq(p - center));
  return sqrtf(r2);
}
// Row 3 defined as {0,0,0,1}: the bake's full plane inverse reads the w lanes.
static inline mat44f authored_tm44(const TMatrix &tm)
{
  mat44f m;
  v_mat44_make_from_43cu(m, tm.array);
  return m;
}

CollisionResourceBuilder::CollisionResourceBuilder()
{
  vFullBBox.bmin = vFullBBox.bmax = v_zero();
  vBoundingSphere = v_zero();
}

uint32_t CollisionResourceBuilder::landName(const char *name)
{
  const CollresNamePlan plan = collres_plan_name((uint32_t)names.size(), name);
  names.resize(plan.grownPoolSize);
  collres_land_name(names.data(), plan, name);
  return plan.nameOfs;
}

int CollisionResourceBuilder::newNode(const char *name)
{
  const uint32_t nameOfs = landName(name);
  const int idx = (int)nodes.size();
  CollisionNode &n = nodes.push_back();
  n.nameOfs = nameOfs;
  n.nodeIndex = (uint16_t)idx;
  authoredTm.push_back(TMatrix::IDENT);
  bindMeta.push_back().behaviorFlags = n.behaviorFlags; // the live flags start as the authored ones
  geom.push_back();
  if (collisionFlags & COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID)
    relGeomTm.push_back(TMatrix::IDENT);
  return idx;
}

int CollisionResourceBuilder::addSphereNode(const char *name, int16_t phys_mat_id, const BSphere3 &bsphere)
{
  const int idx = newNode(name);
  CollisionNode &n = nodes[idx];
  n.physMatId = sanitize_builder_phys_mat(phys_mat_id, name);
  n.type = COLLISION_NODE_TYPE_SPHERE;
  n.flags = CollisionNode::IDENT;
  n.radiusAroundBoxCenter = bsphere.r;
  n.modelBBox = BBox3(bsphere);
  return idx;
}

int CollisionResourceBuilder::addBoxNode(const char *name, int16_t phys_mat_id, const BBox3 &bbox)
{
  const int idx = newNode(name);
  CollisionNode &n = nodes[idx];
  n.physMatId = sanitize_builder_phys_mat(phys_mat_id, name);
  n.type = COLLISION_NODE_TYPE_BOX;
  n.flags = CollisionNode::IDENT;
  n.modelBBox = bbox;
  n.radiusAroundBoxCenter = bbox.width().length() / 2.f;
  return idx;
}

int CollisionResourceBuilder::addCapsuleNode(const char *name, int16_t phys_mat_id, const Point3 &p0, const Point3 &p1, float radius)
{
  G_ASSERTF_RETURN(capsules.size() < 0x10000u, -1, "capsule pool overflow: %d capsules, node <%s>", (int)capsules.size(),
    name); // capsuleIndex is 16-bit
  const int idx = newNode(name);
  CollisionNode &n = nodes[idx];
  n.physMatId = sanitize_builder_phys_mat(phys_mat_id, name);
  n.type = COLLISION_NODE_TYPE_CAPSULE;
  n.flags = CollisionNode::IDENT;
  n.modelBBox = (BBox3(BSphere3(p0, radius)) += BSphere3(p1, radius));
  n.radiusAroundBoxCenter = (p0 - p1).length() / 2.f + radius;
  n.capsuleIndex = (uint16_t)capsules.size();
  capsules.push_back(Capsule(p0, p1, radius));
  return idx;
}

int CollisionResourceBuilder::addMeshNode(const char *name, int16_t phys_mat_id, const TMatrix &tm, const BBox3 &bbox,
  dag::ConstSpan<Point3_vec4> verts, dag::ConstSpan<uint32_t> indices, uint16_t behavior_flags, uint8_t flags)
{
  G_ASSERTF_RETURN(verts.size() > 2 && indices.size() > 2 && (indices.size() % 3) == 0, -1,
    "addMeshNode: malformed mesh geometry: verts.size=%d (need >2), indices.size=%d (need >2 and %%3==0)", (int)verts.size(),
    (int)indices.size());
  const int idx = newNode(name);
  CollisionNode &n = nodes[idx];
  n.physMatId = sanitize_builder_phys_mat(phys_mat_id, name);
  n.type = COLLISION_NODE_TYPE_MESH;
  n.flags = flags;
  n.behaviorFlags = behavior_flags;
  bindMeta[idx].behaviorFlags = behavior_flags;
  {
    mat44f vTm;
    v_mat44_make_from_43cu_unsafe(vTm, tm.array);
    const float len0sq = tm.getcol(0).lengthSq();
    const float len1sq = tm.getcol(1).lengthSq();
    const float len2sq = tm.getcol(2).lengthSq();
    const float maxLenSq = max(len0sq, max(len1sq, len2sq));
    // Squared column lengths overflow for large finite scales and FTZ-underflow to zero below
    // ~1e-19; the spectral norm normalizes internally, so a valid basis always stamps a finite
    // nonzero conservative scale.
    setAuthoredNodeTm(idx, vTm, flags,
      (maxLenSq >= FLT_MIN && maxLenSq <= FLT_MAX) ? sqrtf(maxLenSq) : CollisionResource::mat33SpectralNorm(vTm));
  }
  n.modelBBox = bbox;
  n.radiusAroundBoxCenter = radius_around(bbox.center(), verts);
  geom[idx].vertsOfs = (uint32_t)rawVerts.size();
  geom[idx].indicesOfs = (uint32_t)rawIndices.size();
  n.verticesCount = (uint32_t)verts.size();
  n.indicesCount = (uint32_t)indices.size();
  rawVerts.insert(rawVerts.end(), verts.begin(), verts.end());
  rawIndices.insert(rawIndices.end(), indices.begin(), indices.end());
  return idx;
}

int CollisionResourceBuilder::addMeshNode(const char *name, int16_t phys_mat_id, const TMatrix &tm, const BBox3 &bbox,
  dag::ConstSpan<Point3_vec4> verts, dag::ConstSpan<uint16_t> indices, uint16_t behavior_flags, uint8_t flags)
{
  dag::Vector<uint32_t> idx32(indices.size());
  for (int i = 0, e = (int)indices.size(); i < e; ++i)
    idx32[i] = indices[i];
  return addMeshNode(name, phys_mat_id, tm, bbox, verts, dag::ConstSpan<uint32_t>(idx32.data(), idx32.size()), behavior_flags, flags);
}

int CollisionResourceBuilder::addConvexNode(const char *name, int16_t phys_mat_id, const TMatrix &tm, const BBox3 &bbox,
  dag::ConstSpan<Point3_vec4> verts, dag::ConstSpan<uint16_t> indices, dag::ConstSpan<plane3f> convex_planes, uint16_t behavior_flags,
  uint8_t flags)
{
  // A CONVEX node with planesCount == 0 is malformed (convex code paths iterate planesCount
  // directly), so reject an empty plane span before the type retag.
  G_ASSERTF_RETURN(!convex_planes.empty(), -1, "addConvexNode: convex_planes must be non-empty");
  G_ASSERTF_RETURN(convexPlanes.size() + convex_planes.size() < 0x10000u, -1, "convex plane buffer overflow: %d + %d > 65536",
    (int)convexPlanes.size(), (int)convex_planes.size());
  const int idx = addMeshNode(name, phys_mat_id, tm, bbox, verts, indices, behavior_flags, flags);
  if (idx < 0)
    return idx;
  CollisionNode &n = nodes[idx];
  n.type = COLLISION_NODE_TYPE_CONVEX;
  n.planesOfs = (uint16_t)convexPlanes.size();
  n.planesCount = (uint16_t)convex_planes.size();
  convexPlanes.insert(convexPlanes.end(), convex_planes.begin(), convex_planes.end());
  return idx;
}

// node bounds both merges keep: vert21 step, chunk leaf base, 16-bit export indices
static constexpr float MAX_MESH_NODE_EXTENT = 4096.f;
static constexpr uint32_t MAX_MESH_NODE_FACES = 256u << 10;
static constexpr uint32_t MAX_MESH_NODE_VERTS = 65530;
static constexpr float SPLIT_FINE_FACE_EXTENT = 512.f;
static constexpr float SPLIT_COARSE_MIN_EDGE = 1.f; // stays apart at a coarse cell's cm vert21 step
// a cell plus the overhang of faces up to 512 m fits the extent
static constexpr float SPLIT_FINE_CELL = MAX_MESH_NODE_EXTENT - 2 * SPLIT_FINE_FACE_EXTENT;
static constexpr float SPLIT_COARSE_CELL = 65536.f;
static constexpr float SPLIT_MAX_CELL_COORD = float((1 << 21) - 1);

namespace
{
struct SplitWorkspace
{
  dag::Vector<vec4f> centroid; // w: the face's cell size
  dag::Vector<uint64_t> cellKey;
  dag::Vector<uint32_t> order;
  dag::Vector<eastl::pair<uint32_t, uint32_t>> parts;
  dag::Vector<uint32_t> vertSeenGen, vertNewIdx;
  uint32_t gen = 0;
};

struct PartStats
{
  bbox3f box;
  uint32_t distinctVerts;
};

PartStats split_part_stats(SplitWorkspace &ws, dag::ConstSpan<Point3_vec4> verts, dag::ConstSpan<uint32_t> indices, uint32_t begin,
  uint32_t end)
{
  ++ws.gen;
  PartStats s;
  v_bbox3_init_empty(s.box);
  s.distinctVerts = 0;
  for (uint32_t i = begin; i < end; ++i)
    for (int k = 0; k < 3; ++k)
    {
      const uint32_t v = indices[ws.order[i] * 3u + k];
      if (ws.vertSeenGen[v] != ws.gen)
      {
        ws.vertSeenGen[v] = ws.gen;
        ++s.distinctVerts;
        v_bbox3_add_pt(s.box, v_ld(&verts[v].x));
      }
    }
  return s;
}
} // namespace

int CollisionResourceBuilder::addSplitMeshNodes(dag::ConstSpan<uint16_t> class_flags, dag::ConstSpan<Point3_vec4> verts,
  dag::ConstSpan<uint32_t> indices, dag::ConstSpan<int16_t> face_pmid, dag::ConstSpan<uint8_t> face_class)
{
  const uint32_t srcFaces = (uint32_t)indices.size() / 3u;
  G_ASSERTF_RETURN(!class_flags.empty() && indices.size() % 3u == 0 && (face_pmid.empty() || face_pmid.size() == srcFaces) &&
                     (face_class.empty() || face_class.size() == srcFaces),
    -1, "addSplitMeshNodes: %d classes, %d indices, %d face_pmid and %d face_class entries", (int)class_flags.size(),
    (int)indices.size(), (int)face_pmid.size(), (int)face_class.size());
  auto classOf = [&](uint32_t f) -> uint32_t { return face_class.empty() ? 0u : face_class[f]; };
  auto pmidOf = [&](uint32_t f) -> int16_t { return face_pmid.empty() ? (int16_t)PHYSMAT_DEFAULT : face_pmid[f]; };
  auto keyOf = [&](uint32_t f) { return (classOf(f) << 16) | (uint16_t)pmidOf(f); };

  SplitWorkspace ws;
  ws.centroid.resize(srcFaces);
  ws.order.reserve(srcFaces);
  uint32_t badFaces = 0;
  vec3f origin = V_C_MAX_VAL;
  for (uint32_t f = 0; f < srcFaces; ++f)
  {
    const uint32_t *fi = &indices[f * 3u];
    if (fi[0] >= verts.size() || fi[1] >= verts.size() || fi[2] >= verts.size() || fi[0] == fi[1] || fi[1] == fi[2] ||
        fi[0] == fi[2] || classOf(f) >= class_flags.size())
    {
      ++badFaces;
      continue;
    }
    const vec3f v0 = v_ld(&verts[fi[0]].x), v1 = v_ld(&verts[fi[1]].x), v2 = v_ld(&verts[fi[2]].x);
    if (!v_test_xyz_finite(v0) || !v_test_xyz_finite(v1) || !v_test_xyz_finite(v2))
    {
      ++badFaces;
      continue;
    }
    const vec3f faceMin = v_min(v0, v_min(v1, v2));
    const float faceExtent = v_extract_x(v_hmax3(v_sub(v_max(v0, v_max(v1, v2)), faceMin)));
    const float shortestEdgeSq =
      v_extract_x(v_min(v_length3_sq_x(v_sub(v1, v0)), v_min(v_length3_sq_x(v_sub(v2, v1)), v_length3_sq_x(v_sub(v0, v2)))));
    const bool coarse = faceExtent > SPLIT_FINE_FACE_EXTENT && shortestEdgeSq >= SPLIT_COARSE_MIN_EDGE * SPLIT_COARSE_MIN_EDGE;
    const float cell = coarse ? SPLIT_COARSE_CELL : SPLIT_FINE_CELL;
    ws.centroid[f] = v_perm_xyzd(v_mul(v_add(v0, v_add(v1, v2)), v_splats(1.f / 3.f)), v_splats(cell));
    origin = v_min(origin, faceMin);
    ws.order.push_back(f);
  }
  if (badFaces)
    logerr("addSplitMeshNodes: dropped %u faces (bad indices, an unknown class, or non-finite verts)", badFaces);
  if (ws.order.empty())
    return 0;
  ws.vertSeenGen.assign(verts.size(), 0u);

  ws.cellKey.resize(srcFaces);
  for (uint32_t f : ws.order)
  {
    const vec4f c = ws.centroid[f];
    const vec4f coord = v_min(v_max(v_floor(v_div(v_sub(c, origin), v_splat_w(c))), v_zero()), v_splats(SPLIT_MAX_CELL_COORD));
    const vec4i ci = v_cvt_vec4i(coord);
    ws.cellKey[f] = (uint64_t(v_extract_w(c) > SPLIT_FINE_CELL) << 63) | (uint64_t(v_extract_xi(ci)) << 42) |
                    (uint64_t(v_extract_yi(ci)) << 21) | uint64_t(v_extract_zi(ci));
  }
  stlsort::sort(ws.order.begin(), ws.order.end(),
    [&](uint32_t a, uint32_t b) { return ws.cellKey[a] != ws.cellKey[b] ? ws.cellKey[a] < ws.cellKey[b] : a < b; });

  dag::Vector<eastl::pair<uint32_t, uint32_t>> stack;
  for (uint32_t end = (uint32_t)ws.order.size(); end > 0;)
  {
    uint32_t begin = end - 1;
    while (begin > 0 && ws.cellKey[ws.order[begin - 1]] == ws.cellKey[ws.order[end - 1]])
      --begin;
    stack.push_back({begin, end}); // the first cell pops first
    end = begin;
  }
  while (!stack.empty())
  {
    const uint32_t begin = stack.back().first, end = stack.back().second;
    stack.pop_back();
    const uint32_t count = end - begin;
    const PartStats stats = split_part_stats(ws, verts, indices, begin, end);
    if (count > MAX_MESH_NODE_FACES || stats.distinctVerts > MAX_MESH_NODE_VERTS)
    {
      alignas(16) float ext[4];
      v_st(ext, v_bbox3_size(stats.box));
      const int axis = ext[0] >= ext[1] ? (ext[0] >= ext[2] ? 0 : 2) : (ext[1] >= ext[2] ? 1 : 2);
      alignas(16) float mid[4];
      v_st(mid, v_bbox3_center(stats.box));
      const float midV = mid[axis];
      const vec4f *cs = ws.centroid.data();
      auto along = [&](uint32_t f) {
        alignas(16) float c[4];
        v_st(c, cs[f]);
        return c[axis];
      };
      uint32_t *ordBegin = ws.order.data() + begin, *ordEnd = ws.order.data() + end;
      uint32_t *split = eastl::partition(ordBegin, ordEnd, [&](uint32_t f) { return along(f) < midV; });
      if (split == ordBegin || split == ordEnd)
      {
        split = ordBegin + count / 2;
        eastl::nth_element(ordBegin, split, ordEnd, [&](uint32_t a, uint32_t b) { return along(a) < along(b); });
      }
      const uint32_t splitAt = begin + (uint32_t)(split - ordBegin);
      stack.push_back({splitAt, end}); // left emits first: deterministic names
      stack.push_back({begin, splitAt});
      continue;
    }
    stlsort::sort(ws.order.data() + begin, ws.order.data() + end,
      [&](uint32_t a, uint32_t b) { return keyOf(a) != keyOf(b) ? keyOf(a) < keyOf(b) : a < b; });
    ws.parts.push_back({begin, end});
  }

  auto runEnd = [&](uint32_t gb, uint32_t end) {
    uint32_t ge = gb + 1;
    while (ge < end && keyOf(ws.order[ge]) == keyOf(ws.order[gb]))
      ++ge;
    return ge;
  };
  uint32_t runs = 0;
  for (const auto &part : ws.parts)
    for (uint32_t gb = part.first; gb < part.second; gb = runEnd(gb, part.second))
      ++runs;
  if (nodes.size() + runs > CollisionResource::MAX_COLLISION_NODES)
  {
    logerr("addSplitMeshNodes: %u nodes would pass the %u-node limit; nothing added", (unsigned)(nodes.size() + runs),
      (unsigned)CollisionResource::MAX_COLLISION_NODES);
    return -1;
  }

  ws.vertNewIdx.resize(verts.size());
  dag::Vector<Point3_vec4> partVerts;
  dag::Vector<uint32_t> partIdx;
  int addedNodes = 0;
  for (const auto &part : ws.parts)
    for (uint32_t gb = part.first; gb < part.second;)
    {
      const uint32_t ge = runEnd(gb, part.second);
      ++ws.gen;
      partVerts.clear();
      partIdx.clear();
      bbox3f box;
      v_bbox3_init_empty(box);
      for (uint32_t i = gb; i < ge; ++i)
        for (int k = 0; k < 3; ++k)
        {
          const uint32_t v = indices[ws.order[i] * 3u + k];
          if (ws.vertSeenGen[v] != ws.gen)
          {
            ws.vertSeenGen[v] = ws.gen;
            ws.vertNewIdx[v] = (uint32_t)partVerts.size();
            Point3_vec4 &pv = partVerts.push_back();
            pv = verts[v];
            pv.resv = 1.0f;
            v_bbox3_add_pt(box, v_ld(&verts[v].x));
          }
          partIdx.push_back(ws.vertNewIdx[v]);
        }
      BBox3 bb;
      v_stu_bbox3(bb, box);
      const String name(0, "part%03u", (unsigned)nodes.size());
      addMeshNode(name, pmidOf(ws.order[gb]), TMatrix::IDENT, bb, make_span_const(partVerts), make_span_const(partIdx),
        class_flags[classOf(ws.order[gb])], CollisionNode::IDENT | CollisionNode::ORTHONORMALIZED);
      ++addedNodes;
      gb = ge;
    }
  return addedNodes;
}

CollisionResource *build_static_collision_frt(const StaticSceneRayTracer &frt, dag::ConstSpan<uint8_t> face_pmid, const char *name)
{
  CollisionResourceBuilder builder;
  const int parts = builder.addStaticCollisionFrt(frt, face_pmid);
  if (parts <= 0)
  {
    logerr("frt->collres %s: %d faces gave no static collision (%d parts)", name, frt.getFacesCount(), parts);
    return nullptr;
  }
  builder.recomputeBounds();
  builder.collapse(name);
  builder.collisionFlags |= COLLISION_RES_FLAG_BLAS_TWO_SIDED; // the FRT walks cull nothing
  CollisionResource *res = builder.build(name);
  debug("frt->collres %s: %d faces -> %d parts -> %d nodes", name, frt.getFacesCount(), parts, (int)res->getAllNodes().size());
  return res;
}

bool load_frt_block_as_collision(IGenLoad &crd, CollisionResource *&out_res)
{
  out_res = nullptr;
  Tab<uint8_t> pmid(tmpmem);
  StaticSceneRayTracer *frt = load_frt_block(crd, pmid);
  if (!frt)
    return false;
  if (frt->getFacesCount() > 0)
    out_res = build_static_collision_frt(*frt, make_span_const(pmid), crd.getTargetName());
  frt->destroy();
  return true;
}

int CollisionResourceBuilder::addStaticCollisionFrt(const StaticSceneRayTracer &frt, dag::ConstSpan<uint8_t> face_pmid)
{
  const uint32_t faces = (uint32_t)frt.getFacesCount();
  G_ASSERTF_RETURN(face_pmid.empty() || face_pmid.size() == faces, -1, "addStaticCollisionFrt: %u faces, %d face_pmid entries", faces,
    (int)face_pmid.size());
  dag::Vector<uint8_t> faceClass(faces, 1);
  for (int i = 0, e = frt.getFaceIndicesCount(); i < e; ++i) // face flags live only in these entries
  {
    const SceneRayI24F8 fi = frt.faceIndices(i);
    if (fi.index < faces && !(fi.flags & StaticSceneRayTracer::USER_INVISIBLE) && (fi.flags & StaticSceneRayTracer::CULL_BOTH))
      faceClass[fi.index] = 0;
  }
  dag::Vector<int16_t> pmid(faces);
  for (uint32_t f = 0; f < faces; ++f)
    pmid[f] = (int16_t)(faceClass[f] ? PHYSMAT_INVALID : face_pmid.empty() ? PHYSMAT_DEFAULT : face_pmid[f]);
  static const uint16_t classes[] = {CollisionNode::TRACEABLE | CollisionNode::PHYS_COLLIDABLE, CollisionNode::PHYS_COLLIDABLE};
  return addSplitMeshNodes(make_span_const(classes, 2), dag::ConstSpan<Point3_vec4>(&frt.verts(0), frt.getVertsCount()),
    dag::ConstSpan<uint32_t>(frt.faces(0).v, faces * 3), make_span_const(pmid), make_span_const(faceClass));
}

void CollisionResourceBuilder::replaceNodeGeometry(int node_index, dag::ConstSpan<Point3> verts, dag::ConstSpan<uint32_t> indices)
{
  G_ASSERT_RETURN((uint32_t)node_index < nodes.size(), );
  CollisionNode &n = nodes[node_index];
  G_ASSERTF_RETURN(collres_is_mesh_list_node(n.type), , "replaceNodeGeometry: node %d is a primitive, its frame is not raw geometry",
    node_index);
  NodeGeom &ng = geom[node_index];
  ng.vertsOfs = (uint32_t)rawVerts.size();
  ng.indicesOfs = (uint32_t)rawIndices.size();
  ng.faceUserOfs = NodeGeom::NO_FACE_USERS;
  n.verticesCount = (uint32_t)verts.size();
  n.indicesCount = (uint32_t)indices.size();
  if (indices.empty())
  {
    n.verticesCount = 0;
    return;
  }
  rawVerts.reserve(rawVerts.size() + verts.size());
  // The box and the sphere follow the verts: the weld moves survivors onto vert21 cell centers, so a
  // vert can land up to half a cell outside the source box.
  BBox3 nodeBox;
  for (const Point3 &v : verts)
  {
    Point3_vec4 &vv = rawVerts.push_back();
    vv.x = v.x;
    vv.y = v.y;
    vv.z = v.z;
    vv.resv = 1.0f;
    nodeBox += v;
  }
  rawIndices.insert(rawIndices.end(), indices.begin(), indices.end());
  n.modelBBox = nodeBox;
  n.radiusAroundBoxCenter = radius_around(nodeBox.center(), verts);
}

void CollisionResourceBuilder::eraseNode(int node_index)
{
  G_ASSERT_RETURN((uint32_t)node_index < nodes.size(), );
  nodes.erase(nodes.begin() + node_index);
  authoredTm.erase(authoredTm.begin() + node_index);
  bindMeta.erase(bindMeta.begin() + node_index);
  geom.erase(geom.begin() + node_index);
  if ((uint32_t)node_index < relGeomTm.size())
    relGeomTm.erase(relGeomTm.begin() + node_index);
  for (int i = node_index, e = (int)nodes.size(); i < e; ++i)
    nodes[i].nodeIndex = (uint16_t)i;
}

int CollisionResourceBuilder::nodePhysMatId(int node_index, int palette_index) const
{
  const CollisionNode &n = nodes[node_index];
  if (n.physMatId >= 0)
    return palette_index == 0 ? (int)n.physMatId : PHYSMAT_INVALID;
  if (n.physMatId == PHYSMAT_INVALID)
    return PHYSMAT_INVALID;
  const uint32_t ofs = CollisionResource::nodePhysMatSliceOfs(n.physMatId);
  if (ofs >= physMatPool.size() || (uint32_t)palette_index >= (uint32_t)physMatPool[ofs])
    return PHYSMAT_INVALID;
  const uint32_t at = ofs + 1u + (uint32_t)palette_index;
  return at < physMatPool.size() ? (int)(int16_t)physMatPool[at] : PHYSMAT_INVALID;
}

int CollisionResourceBuilder::nodePhysMatCount(int node_index) const
{
  const CollisionNode &n = nodes[node_index];
  if (n.physMatId == PHYSMAT_INVALID)
    return 0;
  if (n.physMatId >= 0)
    return 1;
  const uint32_t ofs = CollisionResource::nodePhysMatSliceOfs(n.physMatId);
  if (ofs >= physMatPool.size())
    return 0;
  uint32_t count = physMatPool[ofs];
  const uint32_t tail = (uint32_t)physMatPool.size() - ofs - 1u;
  if (count > tail)
    count = tail;
  if (count > CollisionResource::MAX_NODE_PHYS_MATS)
    count = CollisionResource::MAX_NODE_PHYS_MATS;
  return (int)count;
}

// A stored slice never moves or renumbers.
bool CollisionResourceBuilder::setNodePhysMats(int node_index, dag::ConstSpan<int> phys_mat_ids, bool coverage_order)
{
  if ((uint32_t)node_index >= nodes.size())
    return false;
  CollisionNode &n = nodes[node_index];

  uint16_t mats[CollisionResource::MAX_NODE_PHYS_MATS];
  uint32_t count = 0;
  for (int id : phys_mat_ids)
  {
    // An id that is not a valid inline one would become a pool reference on the way in (-5 reads back as slice offset 3; past-int16
    // truncates into the same range): refuse the whole set.
    if (id < PHYSMAT_INVALID || (int)(int16_t)id != id)
    {
      logerr("collision node #%d: physmat id %d is not storable (needs PHYSMAT_INVALID or a non-negative id fitting int16)",
        node_index, id);
      return false;
    }
    const uint16_t m = (uint16_t)(int16_t)id;
    uint32_t at = 0;
    if (coverage_order)
      for (; at < count && mats[at] < m; ++at) // kept ascending so a duplicate shows up in this same pass
        ;
    else
      for (; at < count && mats[at] != m; ++at) // caller's order kept; scan the whole set for the duplicate
        ;
    if (at < count && mats[at] == m)
      continue;
    if (count == CollisionResource::MAX_NODE_PHYS_MATS)
    {
      logerr("collision node #%d: more than %u distinct materials (%d given); the leaf field cannot index them", node_index,
        CollisionResource::MAX_NODE_PHYS_MATS, (int)phys_mat_ids.size());
      return false;
    }
    for (uint32_t i = count; i > at; --i)
      mats[i] = mats[i - 1];
    mats[at] = m;
    ++count;
  }

  if (count <= 1)
  {
    // One material is the node's own field, and none is PHYSMAT_INVALID: neither takes a slice, so such a node keeps unstamped (value
    // 0) leaves and their short bodies.
    n.physMatId = count ? (int16_t)mats[0] : (int16_t)PHYSMAT_INVALID;
    return true;
  }

  // Most-covering material first, ties by ascending id; weights are resource-wide so equal sets still share a slice.
  // Index 0 is what unstamped leaves read and the only stamped value that keeps a short body, so the dominant id owns it.
  if (coverage_order)
  {
    uint32_t covered[CollisionResource::MAX_NODE_PHYS_MATS] = {};
    for (const CollisionNode &node : nodes)
    {
      const uint32_t w = node.indicesCount ? node.indicesCount / 3u : 1u; // a node without faces still counts once
      for (int i = 0, e = nodePhysMatCount(node.nodeIndex); i < e; ++i)
      {
        const uint16_t m = (uint16_t)(int16_t)nodePhysMatId(node.nodeIndex, i);
        for (uint32_t k = 0; k < count; ++k)
          if (mats[k] == m)
          {
            covered[k] += w;
            break;
          }
      }
    }
    for (uint32_t i = 1; i < count; ++i) // insertion sort, stable, so equal coverage keeps the id order above
    {
      const uint16_t m = mats[i];
      const uint32_t w = covered[i];
      uint32_t at = i;
      for (; at > 0 && covered[at - 1] < w; --at)
      {
        mats[at] = mats[at - 1];
        covered[at] = covered[at - 1];
      }
      mats[at] = m;
      covered[at] = w;
    }
  }

  for (uint32_t ofs = 0; ofs + 1u < physMatPool.size(); ofs += 1u + physMatPool[ofs])
    if (physMatPool[ofs] == count && memcmp(&physMatPool[ofs + 1u], mats, count * sizeof(uint16_t)) == 0)
    {
      n.physMatId = (int16_t)(-(int)ofs - 2);
      return true;
    }

  const uint32_t ofs = (uint32_t)physMatPool.size();
  if (ofs > CollisionResource::MAX_PHYS_MAT_POOL_OFS)
  {
    logerr("collision resource material pool is full (%u entries); node #%d keeps its old material", ofs, node_index);
    return false;
  }
  physMatPool.resize(ofs + 1u + count);
  physMatPool[ofs] = (uint16_t)count;
  memcpy(physMatPool.data() + ofs + 1u, mats, count * sizeof(uint16_t));
  n.physMatId = (int16_t)(-(int)ofs - 2);
  return true;
}

void CollisionResourceBuilder::setAuthoredNodeTm(int node_index, mat44f_cref tm, uint8_t class_flags, float max_scale)
{
  G_ASSERT_RETURN((uint32_t)node_index < nodes.size(), );
  v_mat_43cu_from_mat44(authoredTm[node_index].array, tm);
  PoseMeta &pm = bindMeta[node_index];
  pm = PoseMeta(); // node slots are reused (legacy drop path): no stale status bits
  // Every load/build path sets the authored flags before this call: seed the live value here.
  pm.behaviorFlags = nodes[node_index].behaviorFlags;
  // Serialized stamps are data: a NaN/Inf/denormal scale passes every matrix gate below yet
  // collapses or explodes the sphere culls that square it. Recompute out-of-band stamps.
  if (!(max_scale >= FLT_MIN && max_scale <= 1.f / FLT_MIN))
    max_scale = CollisionResource::mat33SpectralNorm(tm);
  pm.maxTmScale = max_scale;
  pm.setPoseIdentity(v_mat44_is_ident43(tm));
  pm.flags = class_flags & POSE_CLASS_BITS;
  // Capsule geometry is never exporter-baked, so epsilon-identity motion must still apply.
  if ((pm.flags & CollisionNode::IDENT) && nodes[node_index].type == COLLISION_NODE_TYPE_CAPSULE && !v_mat44_is_ident43(tm))
    pm.flags = (pm.flags & ~CollisionNode::IDENT) | CollisionNode::TRANSLATE;
  // Authored mirrored placements remain valid; only singular placements are hidden.
  // Any non-finite affine component (translation included) is unrealizable. Per lane, not a
  // column sum: finite components can overflow the intermediate sum near the scale bound.
  const bool tmFinite = v_test_mat43_finite(tm);
  // Scale-free gate on the matrix itself, not the serialized scale (a retained bake
  // serializes its effective scale); evaluated unconditionally so the log never reads an
  // unwritten ndet.
  float ndet;
  const bool detOk = CollisionResource::relativeDetAboveFloor(tm, ndet);
  bool traceOk = tmFinite && detOk;
  if (traceOk)
  {
    // Inverse-based dispatch needs the whole inverse finite: near-bound translations and
    // huge-scale cofactors overflow it while the forward placement stays representable.
    mat44f inv;
    v_mat44_inverse43(inv, tm);
    traceOk = v_test_mat43_finite(inv);
  }
  pm.setTraceable(traceOk);
  pm.setComposable(traceOk); // authored mirrors are load-traceable, so composable follows
  // Per occurrence (not once): affected content must be enumerable from the log.
  if (DAGOR_UNLIKELY(!pm.isTraceable()))
    logwarn("collision: singular authored tm (ndet=%g) on node %d <%s>; hidden from the pose mirror", ndet, node_index,
      nodeName(node_index));
  // Non-uniform primitive placement is supported conservatively but remains a content error.
  else if (DAGOR_UNLIKELY(pm.flags == 0 && (nodes[node_index].type == COLLISION_NODE_TYPE_SPHERE ||
                                             nodes[node_index].type == COLLISION_NODE_TYPE_CAPSULE)))
    LOGWARN_ONCE("collision: non-uniform authored tm on %s node %d traces as an ellipsoid",
      nodes[node_index].type == COLLISION_NODE_TYPE_SPHERE ? "sphere" : "capsule", node_index);
}

void CollisionResourceBuilder::unbakePrimNode(CollisionNode &n, const TMatrix &tm, CollisionResourceInstance::PoseMeta &pm)
{
  if (pm.flags & CollisionNode::IDENT)
    return; // nothing baked beyond an eps-identity; dispatch treats IDENT T as a no-op
  if (!pm.isTraceable())
  {
    // A singular authored primitive must remain in its baked frame.
    pm.setGeometryBaked(true);
    return;
  }
  // Zero-vert marker: bounds are empty; corner-mapping the inverted box would explode it into
  // a huge phantom that accessors and a later re-export preserve.
  if (n.radiusAroundBoxCenter < 0.f)
    return;
  float sphereRadDivisor = 1.f;
  if (n.type == COLLISION_NODE_TYPE_SPHERE)
  {
    // A non-conformal placement cannot un-bake the exporter's vert-fit sphere (any scalar
    // division under-recovers): it keeps the baked frame as a retained, poseable bake with an
    // identity effective bind frame. Conformality is checked scale-relatively on a
    // max-element-normalized basis (squared lengths overflow/underflow at extreme scales, and
    // an absolute divisor floor would corrupt tiny valid placements).
    const Point3 c0 = tm.getcol(0), c1 = tm.getcol(1), c2 = tm.getcol(2);
    float m = 0.f;
    for (int a = 0; a < 3; a++)
    {
      const Point3 c = tm.getcol(a);
      m = max(m, max(fabsf(c.x), max(fabsf(c.y), fabsf(c.z))));
    }
    if (!(m >= FLT_MIN && m <= 1.f / FLT_MIN))
    {
      // Outside the invertible normal-float band there is no recoverable local frame.
      pm.setGeometryBaked(true);
      pm.setRetainedBake(true);
      pm.flags = CollisionNode::IDENT | CollisionNode::ORTHONORMALIZED; // effective bind frame is identity
      pm.maxTmScale = 1.f;
      return;
    }
    const Point3 n0 = c0 / m, n1 = c1 / m, n2 = c2 / m;
    const float l0 = lengthSq(n0), l1 = lengthSq(n1), l2 = lengthSq(n2);
    const float d01 = fabsf(n0 * n1), d02 = fabsf(n0 * n2), d12 = fabsf(n1 * n2);
    bool conformal = pm.flags != 0;
    if (conformal && !(pm.flags & (CollisionNode::IDENT | CollisionNode::TRANSLATE)))
    {
      const float s2 = max(l0, max(l1, l2));
      const float relTol = 2e-3f * s2;
      conformal = fabsf(l0 - s2) <= relTol && fabsf(l1 - s2) <= relTol && fabsf(l2 - s2) <= relTol && d01 <= relTol && d02 <= relTol &&
                  d12 <= relTol;
    }
    if (!conformal)
    {
      pm.setGeometryBaked(true);
      pm.setRetainedBake(true);
      pm.flags = CollisionNode::IDENT | CollisionNode::ORTHONORMALIZED; // effective bind frame is identity
      pm.maxTmScale = 1.f;
      return;
    }
    // The baked fit can sit along the smallest stretch: divide by the Gershgorin sigma_min
    // lower bound; the conformality band caps the overshoot.
    const float nSigmaMin = sqrtf(max(min(l0 - d01 - d02, min(l1 - d01 - d12, l2 - d02 - d12)), 0.f));
    sphereRadDivisor = max(m * nSigmaMin, FLT_MIN);
  }
  // Always the full inverse: ORTHONORMALIZED is an epsilon class, and the transpose shortcut
  // compounds its scale error into the stored geometry on every load-export cycle.
  const TMatrix itm = collres_inverse(tm);
  BBox3 localBox;
  for (int k = 0; k < 8; k++)
    localBox += itm * n.modelBBox.point(k);
  // An exact identity basis, not the eps TRANSLATE class: a 5e-4 basis slack still expands the
  // corner-mapped box, and skipping the refit would leave the narrow phase outside its culls.
  const bool exactIdentityBasis =
    tm.getcol(0) == Point3(1, 0, 0) && tm.getcol(1) == Point3(0, 1, 0) && tm.getcol(2) == Point3(0, 0, 1);
  if (!exactIdentityBasis)
  {
    // Rotated or skewed AABBs can only be un-baked conservatively.
    BBox3 recomposed;
    for (int k = 0; k < 8; k++)
      recomposed += tm * localBox.point(k);
    const float eps = 1e-3f * n.modelBBox.width().length() + 1e-5f;
    if (n.type == COLLISION_NODE_TYPE_BOX &&
        ((recomposed[0] - n.modelBBox[0]).length() > eps || (recomposed[1] - n.modelBBox[1]).length() > eps))
      LOGWARN_ONCE("collision: rotated/skewed authored placement on a baked box node; local bounds are conservative");
  }
  const Point3 bakedSphereC = n.bsphereCenter(); // the exporter-baked sphere, read before the box moves
  n.modelBBox = localBox;
  // A BOX node's sphere is a function of its box alone, so take it from the box this un-bake just
  // settled: a legacy stream stores the box in the authored frame and the sphere node-local, and the
  // fold across those two frames would otherwise leave the placement in the radius. A conservative
  // corner-mapped box also outgrows the exporter's vert fit, and every sphere cull must cover what
  // the box narrow phase tests.
  if (n.type == COLLISION_NODE_TYPE_BOX && n.radiusAroundBoxCenter >= 0.f)
    n.radiusAroundBoxCenter = localBox.width().length() * 0.5f;
  if (n.type == COLLISION_NODE_TYPE_SPHERE)
  {
    // Only sphere bounding spheres are exporter-baked (verts * wtm); box bounding
    // spheres accumulate raw node-local verts at export, so they need no un-bake.
    const Point3 c = itm * bakedSphereC;
    if (n.radiusAroundBoxCenter >= 0.f)
    {
      // Conformal classes only reach here (general sphere placements stay baked at entry).
      n.radiusAroundBoxCenter /= sphereRadDivisor;
      // Rebuild exact sphere bounds after the conservative corner transform.
      const Point3 r3(n.radiusAroundBoxCenter, n.radiusAroundBoxCenter, n.radiusAroundBoxCenter);
      n.modelBBox[0] = c - r3;
      n.modelBBox[1] = c + r3;
    }
  }
}

void CollisionResourceBuilder::setNodeTm(int node_index, const TMatrix &tm)
{
  G_ASSERT_RETURN((uint32_t)node_index < nodes.size(), );
  mat44f vTm;
  v_mat44_make_from_43cu_unsafe(vTm, tm.array);
  float maxScale = 1.f;
  const uint8_t flags = CollisionResource::classifyNodeTmFlags(vTm, maxScale);
  // the node's class bits always match the authored tm: accessors and the geometry tm gate on them
  nodes[node_index].flags = (nodes[node_index].flags & ~POSE_CLASS_BITS) | flags;
  setAuthoredNodeTm(node_index, vTm, flags, maxScale);
}

void CollisionResourceBuilder::setRelGeomNodeTm(int node_index, const TMatrix &tm)
{
  G_ASSERT_RETURN((uint32_t)node_index < nodes.size(), );
  if (relGeomTm.size() < nodes.size()) // new slots start identity
    relGeomTm.resize(nodes.size(), TMatrix::IDENT);
  relGeomTm[node_index] = tm;
  collisionFlags |= COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID;
}

// Live IDENT means exact identity; loader epsilon classes are not dispatch no-ops.
// Widens the bind meta scale to the conservative spectral bound; for shear-class authored
// nodes nodeMaxTmScale, the exporter weld eps and the serialized scale follow it.
void CollisionResourceBuilder::stampPoseScales()
{
  for (int i = 0, e = (int)nodes.size(); i < e; ++i)
  {
    PoseMeta &pm = bindMeta[i];
    // A retained bake's public scale describes its identity effective frame; reclassifying the
    // unchanged authored matrix would leak the authored scale into it.
    if (pm.isGeometryBaked())
      continue;
    const bool shearLike = pm.flags == 0;
    // The column max under-reads every non-bit-exact uniform basis (slack ~eps/(2*s^2)):
    // restamp with the spectral norm; exact axis-aligned bases keep their bit-exact column max.
    const bool slackUniform = (pm.flags & CollisionNode::ORTHOUNIFORM) != 0 && (pm.flags & CollisionNode::ORTHONORMALIZED) == 0 &&
                              !is_exact_axis_aligned_basis(authoredTm[i]);
    // ORTHONORMALIZED is an epsilon class (IDENT included): the column max can under-read the
    // stretch by the classifier tolerance; Gershgorin over the tolerance volume gives
    // sigma^2 <= 1 + 3e-3, so 1.0015 covers it. Bit-exact rigid bases keep their exact stamp.
    if ((pm.flags & CollisionNode::ORTHONORMALIZED) && !is_exact_rigid_basis(authoredTm[i]))
      pm.maxTmScale = max(pm.maxTmScale, 1.0015f);
    if (!shearLike && !slackUniform)
      continue;
    const mat44f vTm = authored_tm44(authoredTm[i]);
    // No margin on the uniform-class restamp (a true uniform's spectral norm is its scale,
    // and consumers pin the exact stamp); the shear class keeps the persisted margin.
    pm.maxTmScale = max(pm.maxTmScale, mat33_spectral_norm(vTm) * (shearLike ? 1.0002f : 1.f));
    // The widened scale raises the relative determinant floor: re-gate traceability so a
    // near-singular shear cannot stay traceable on the strength of its pre-widen column max.
    // Same normalized gate as setAuthoredNodeTm: raw det * s^3 overflows for large finite bases.
    if (pm.isTraceable())
    {
      float ndet;
      pm.setTraceable(CollisionResource::relativeDetAboveFloor(vTm, ndet));
      if (!pm.isTraceable())
        pm.setComposable(false); // effectively singular: the stored value is the right report
    }
  }
}

// Stored geometry -> resource space at bind (the resource's geometryTmFromPosed with the
// authored placement as the pose): a singular bake stays raw, an authored-frame consumer
// (eps-IDENT prim, retained bake) composes the identity, the rest the placement.
TMatrix CollisionResourceBuilder::geometryTm(int node_index) const
{
  const PoseMeta &pm = bindMeta[node_index];
  if (pm.isGeometryBaked() && !pm.isRetainedBake())
    return TMatrix::IDENT;
  if (!CollisionResource::usesAuthoredFrame(nodes[node_index], pm))
    return authoredTm[node_index];
  return TMatrix::IDENT;
}

BBox3 CollisionResourceBuilder::nodeBBox(int node_index) const
{
  const CollisionNode &n = nodes[node_index];
  const PoseMeta &pm = bindMeta[node_index];
  if (n.type != COLLISION_NODE_TYPE_BOX && n.type != COLLISION_NODE_TYPE_SPHERE)
    return n.modelBBox;
  // IDENT and singular authored primitives retain exporter-baked bounds; r < 0 is the exporter's
  // zero-vert marker (composing it would corner-map the inverted box into a phantom placement);
  // a non-composable pose must not compose (NaN/Inf bounds would leak).
  if ((pm.flags & CollisionNode::IDENT) || (pm.isGeometryBaked() && !pm.isRetainedBake()) || n.radiusAroundBoxCenter < 0 ||
      !pm.isComposable())
    return n.modelBBox;
  const TMatrix tm = geometryTm(node_index);
  if (n.type == COLLISION_NODE_TYPE_SPHERE) // exact: corner-mapping would inflate under rotation
    return composed_sphere_box(tm, n.bsphereCenter(), n.radiusAroundBoxCenter);
  BBox3 out;
  for (int k = 0; k < 8; k++)
    out += tm * n.modelBBox.point(k);
  return out;
}

// Capsule, box and sphere primitives store their shape data outside verts and indices, so they are
// out of scope here.
bool CollisionResourceBuilder::bakeNodeTransform(int node_index)
{
  if ((uint32_t)node_index >= nodes.size())
    return false;
  CollisionNode &m = nodes[node_index];
  if (m.type != COLLISION_NODE_TYPE_CONVEX && m.type != COLLISION_NODE_TYPE_MESH)
    return false;
  if ((bindMeta[node_index].flags & (CollisionNode::IDENT | CollisionNode::TRANSLATE)) == CollisionNode::IDENT || m.indicesCount == 0)
    return false;
  // Singular authored placements remain hidden and unbaked: inverting one would feed invalid
  // convex planes and the identity restamp below would revive the node as traceable.
  if (!bindMeta[node_index].isTraceable())
    return false;
  Point3_vec4 *vbase = rawVerts.data() + geom[node_index].vertsOfs;
  uint32_t *ibase = rawIndices.data() + geom[node_index].indicesOfs;
  const uint32_t mVCount = (uint32_t)m.verticesCount;
  const mat44f nodeTm = authored_tm44(authoredTm[node_index]);
  bbox3f box;
  v_bbox3_init_empty(box);
  for (vec4f *__restrict verts = (vec4f *)(void *)vbase, *ve = verts + mVCount; verts != ve; ++verts)
  {
    *verts = v_mat44_mul_vec3p(nodeTm, *verts);
    v_bbox3_add_pt(box, *verts);
  }
  vec4f vSphereC = v_bbox3_center(box), sphereRad2 = v_zero();
  for (vec4f *__restrict verts = (vec4f *)(void *)vbase, *ve = verts + mVCount; verts != ve; ++verts)
    sphereRad2 = v_max(sphereRad2, v_length3_sq_x(v_sub(vSphereC, *verts)));

  mat44f N, TN;
  v_mat44_inverse(N, nodeTm);
  v_mat44_transpose(TN, N);
  plane3f *__restrict base = convexPlanes.data() + m.planesOfs;
  for (plane3f *__restrict planes = base, *pe = base + m.planesCount; planes != pe; ++planes)
    *planes = v_mat44_mul_vec4(TN, *planes);

  v_stu_bbox3(m.modelBBox, box);
  m.radiusAroundBoxCenter = v_extract_x(v_sqrt_x(sphereRad2));

  const float tmDet = v_extract_x(v_dot3_x(nodeTm.col0, v_cross3(nodeTm.col1, nodeTm.col2)));
  if (tmDet < 0.f) // swap indices order
    for (uint32_t i = 0, e = m.indicesCount; i + 2 < e; i += 3)
      eastl::swap(ibase[i + 0], ibase[i + 2]);
  m.flags = CollisionNode::IDENT | (m.flags & (~CollisionNode::TRANSLATE));
  m.flags = CollisionNode::ORTHONORMALIZED | (m.flags & (~CollisionNode::ORTHOUNIFORM));
  mat44f identTm;
  v_mat44_ident(identTm);
  setAuthoredNodeTm(node_index, identTm, m.flags, 1.f);
  return true;
}

bool CollisionResourceBuilder::bakeMirroredNodes()
{
  bool baked = false;
  for (int i = 0, e = (int)nodes.size(); i < e; ++i)
    if ((nodes[i].type == COLLISION_NODE_TYPE_MESH || nodes[i].type == COLLISION_NODE_TYPE_CONVEX) && authoredTm[i].det() < 0.f)
      baked |= bakeNodeTransform(i);
  return baked;
}

void CollisionResourceBuilder::collapse(const char *res_name)
{
  collisionFlags |= COLLISION_RES_FLAG_OPTIMIZED;
  mergeMeshNodes(res_name, /*fuse_materials*/ false);
}

// most resources hold a handful of nodes; a level's static collision spills to tmpmem
static constexpr size_t NODE_KEYS_INPLACE = 8;

void CollisionResourceBuilder::sortNodes()
{
  for (CollisionNode &n : nodes)
    n.insideOfNode = CollisionNode::INVALID_IDX; // the containment pass below only sets
  // Preserve legacy sort and containment frames for primitives.
  auto nodeSortBox = [this](const CollisionNode &n, int meta_index) -> BBox3 {
    if (n.type != COLLISION_NODE_TYPE_BOX && n.type != COLLISION_NODE_TYPE_SPHERE)
      return n.modelBBox;
    const bool geometryInModelSpace = (bindMeta[meta_index].flags & CollisionNode::IDENT) || bindMeta[meta_index].isGeometryBaked();
    if (geometryInModelSpace)
      return n.modelBBox;
    const TMatrix tm = geometryTm(meta_index);
    if (n.type == COLLISION_NODE_TYPE_SPHERE)
      return composed_sphere_box(tm, n.bsphereCenter(), n.radiusAroundBoxCenter); // legacy tight key
    BBox3 out;
    for (int k = 0; k < 8; k++)
      out += tm * n.modelBBox.point(k);
    return out;
  };
  // Keys precomputed per node: element moves invalidate nodeIndex-based lookup during the sort, and a
  // size key stored once keeps the comparator a strict weak order (a length recomputed per call rounds
  // differently under FP contraction). Vector boxes: a 16 B load of a BBox3 at a heap array's end over-reads.
  // The loop below writes at n.nodeIndex, so it fills every slot only while nodeIndex is a
  // permutation of [0, size) - which every mutation keeps - and the resize can skip the init.
  dag::RelocatableFixedVector<bbox3f, NODE_KEYS_INPLACE, true, TmpmemAlloc, uint32_t, false> sortKeys;
  dag::RelocatableFixedVector<float, NODE_KEYS_INPLACE, true, TmpmemAlloc, uint32_t, false> sizeKeys;
  sortKeys.resize((uint32_t)nodes.size());
  sizeKeys.resize((uint32_t)nodes.size());
  for (const CollisionNode &n : nodes)
  {
    sortKeys[n.nodeIndex] = v_ldu_bbox3(nodeSortBox(n, n.nodeIndex));
    sizeKeys[n.nodeIndex] = v_extract_x(v_length3_sq_x(v_bbox3_size(sortKeys[n.nodeIndex])));
  }
  const char *namesData = names.empty() ? "" : names.data();
  stlsort::sort(nodes.begin(), nodes.end(), [namesData, &sizeKeys](const CollisionNode &left, const CollisionNode &right) {
    const float szL = sizeKeys[left.nodeIndex], szR = sizeKeys[right.nodeIndex];
    if (szL == szR)
      return strcmp(namesData + left.nameOfs, namesData + right.nameOfs) > 0;
    return szL > szR; // larger first
  });

  // Keep every nodeIndex-parallel array in the new order.
  {
    const bool hasRelTms = !relGeomTm.empty();
    G_ASSERTF_RETURN(!hasRelTms || relGeomTm.size() == nodes.size(), , "relGeomTm.size()=%d nodes.size()=%d", (int)relGeomTm.size(),
      (int)nodes.size());
    G_ASSERTF_RETURN(authoredTm.size() == nodes.size() && bindMeta.size() == nodes.size() && geom.size() == nodes.size(), ,
      "authored %d bind %d geom %d vs %d nodes", (int)authoredTm.size(), (int)bindMeta.size(), (int)geom.size(), (int)nodes.size());
    dag::Vector<TMatrix> sortedAuthoredTm(nodes.size()), sortedRelTm(hasRelTms ? nodes.size() : 0);
    dag::Vector<PoseMeta> sortedMeta(nodes.size());
    dag::Vector<NodeGeom> sortedGeom(nodes.size());
    for (size_t nodeNo = 0; nodeNo < nodes.size(); nodeNo++)
    {
      const uint16_t nodeIdx = nodes[nodeNo].nodeIndex;
      sortedAuthoredTm[nodeNo] = authoredTm[nodeIdx];
      sortedMeta[nodeNo] = bindMeta[nodeIdx];
      sortedGeom[nodeNo] = geom[nodeIdx];
      if (hasRelTms)
        sortedRelTm[nodeNo] = relGeomTm[nodeIdx];
    }
    authoredTm = eastl::move(sortedAuthoredTm);
    bindMeta = eastl::move(sortedMeta);
    geom = eastl::move(sortedGeom);
    if (hasRelTms)
      relGeomTm = eastl::move(sortedRelTm);
  }

  // Containment reads the same precomputed keys (cached values; nodeIndex is re-stamped after).
  for (size_t nodeNo1 = 0; nodeNo1 < nodes.size(); nodeNo1++)
  {
    const bbox3f bbox1 = sortKeys[nodes[nodeNo1].nodeIndex];
    const float bboxLenSq = sizeKeys[nodes[nodeNo1].nodeIndex];
    for (size_t nodeNo2 = nodeNo1 + 1; nodeNo2 < nodes.size(); nodeNo2++)
    {
      bbox3f bbox2 = sortKeys[nodes[nodeNo2].nodeIndex];
      if (v_bbox3_test_box_inside(bbox1, bbox2)) // bbox2 inside of bbox1.
      {
        uint16_t n2ins = nodes[nodeNo2].insideOfNode;
        if (n2ins == CollisionNode::INVALID_IDX || bboxLenSq < sizeKeys[nodes[n2ins].nodeIndex])
          nodes[nodeNo2].insideOfNode = (uint16_t)nodeNo1;
      }
    }
  }
  for (size_t i = 0; i < nodes.size(); i++)
    nodes[i].nodeIndex = (uint16_t)i;
}

// An empty total (point-only or fully dropped) keeps the loaded bounds.
void CollisionResourceBuilder::recomputeBounds()
{
  auto forEachResourcePoint = [&](auto &&pt) {
    for (const CollisionNode &n : nodes)
    {
      if (n.type == COLLISION_NODE_TYPE_MESH || n.type == COLLISION_NODE_TYPE_CONVEX)
      {
        if (n.indicesCount == 0)
          continue;
        const TMatrix &tm = authoredTm[n.nodeIndex];
        const Point3_vec4 *nv = rawVerts.data() + geom[n.nodeIndex].vertsOfs;
        for (uint32_t i = 0, e = (uint32_t)n.verticesCount; i < e; ++i)
          pt(tm * Point3(nv[i].x, nv[i].y, nv[i].z));
      }
      else if (!n.modelBBox.isempty())
      {
        if (n.type == COLLISION_NODE_TYPE_CAPSULE)
        {
          const TMatrix &tm = authoredTm[n.nodeIndex];
          for (int k = 0; k < 8; k++)
            pt(tm * n.modelBBox.point(k));
        }
        else
        {
          const BBox3 nodeBox = nodeBBox(n.nodeIndex);
          for (int k = 0; k < 8; k++)
            pt(nodeBox.point(k));
        }
      }
    }
  };

  BBox3 total;
  forEachResourcePoint([&](const Point3 &p) { total += p; });
  if (total.isempty())
    return;

  boundingBox = total;
  v_bbox3_init(vFullBBox, v_ldu(&total[0].x));
  v_bbox3_add_pt(vFullBBox, v_ldu(&total[1].x));
  vFullBBox.bmin = v_perm_xyzd(vFullBBox.bmin, v_zero());
  vFullBBox.bmax = v_perm_xyzd(vFullBBox.bmax, v_zero());

  const Point3 center = total.center();
  float r2 = 0.f;
  forEachResourcePoint([&](const Point3 &p) { r2 = max(r2, lengthSq(p - center)); });
  vBoundingSphere = v_make_vec4f(center.x, center.y, center.z, r2);
  boundingSphereRad = sqrtf(r2);
  // The full box bounds the traced root box from above, so a verdict here holds at load.
  collisionFlags = CollisionResource::meshNodesCoverRootBox(vFullBBox, make_span_const(nodes), make_span_const(bindMeta))
                     ? collisionFlags | COLLISION_RES_FLAG_NODES_COVER_ROOT_BOX
                     : collisionFlags & ~COLLISION_RES_FLAG_NODES_COVER_ROOT_BOX;
}

// Bucket mesh nodes, merge each bucket's geometry into its first node, delete the rest, rebuild the
// raw arrays in node order and re-stamp every nodeIndex-parallel structure. With fuse_materials a
// bucket spans materials; each fused survivor's per-face material indices land in rawFaceUser.
void CollisionResourceBuilder::mergeMeshNodes(const char *res_name, bool fuse_materials)
{
  if (!res_name || !*res_name)
    res_name = "?";
  bool anyBaked = false;
  for (int i = 0, e = (int)nodes.size(); i < e; ++i)
    anyBaked |= bakeNodeTransform(i);
  // One bucket, either shape, discriminated by fusable: a fusable bucket matches on behaviorFlags and collects mats; a non-fusable one
  // matches on (setKey, isPhysCollidable) and reads no mats.
  struct BucketMeta
  {
    bool isPhysCollidable;
    // Part of the non-fused key: a PHYS_COLLIDABLE-only node must never share a bucket with a traceable one, or the survivor's flags
    // would change what the merged geometry answers.
    bool isTraceable;
    uint16_t behaviorFlags; // fuse buckets match on all behaviour flags
    bool fusable;           // minted by an own-material node; a set-minted bucket takes no own-material joiner
    // The raw physMatId of the minting node: equal values mean equal material sets, since a set is either inline or a deduped slice.
    // Only meaningful without fusion, where a bucket is one set.
    int setKey;
    StaticTab<int, CollisionResource::MAX_NODE_PHYS_MATS - 1> mats;
    BBox3 box;
    uint32_t faces;
    uint32_t verts;
  };
  Tab<Tab<uint16_t>> meshNodesByMat(tmpmem);
  Tab<BucketMeta> bucketMeta(tmpmem); // one entry per meshNodesByMat bucket
  meshNodesByMat.reserve(nodes.size());
  bucketMeta.reserve(nodes.size());
  uint32_t boundsRefusals = 0;

  for (const CollisionNode &m : nodes)
  {
    if (!collres_is_mesh_list_node(m.type))
      continue;
    const bool isTraceable = m.checkBehaviorFlags(CollisionNode::TRACEABLE);
    const bool isPhysCollidable = m.checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE);
    // A PHYS_COLLIDABLE-only mesh node buckets for the fuse only: the export collapse keeps it apart, so cooked shapes do not change.
    const bool bucketable = isTraceable || (fuse_materials && isPhysCollidable);
    const bool isMesh = m.type == COLLISION_NODE_TYPE_MESH;
    const bool hasRawSlice = m.hasGeometry();                   // a degenerate-dropped node has none
    const bool poseValid = bindMeta[m.nodeIndex].isTraceable(); // singular or mirrored hides, both behaviour classes
    if (isMesh && bucketable && hasRawSlice && poseValid)
    {
      // A node already holding a set keeps the old rule -- it cannot be folded into a bucket whose per-face indices are built from
      // single-material sources.
      const bool ownMaterial = fuse_materials && m.physMatId >= PHYSMAT_INVALID;
      const int nodeMat = nodePhysMatId(m.nodeIndex, 0);
      intptr_t bucket = -1;
      const uint32_t nodeFaces = m.indicesCount / 3u;
      const uint32_t nodeVerts = m.verticesCount;
      // Without fusion this reproduces the old key exactly: a bucket then holds one material and one behaviour class, so
      // equal-material nodes still meet and unlike ones never do.
      for (intptr_t b = 0; b < (intptr_t)meshNodesByMat.size(); ++b)
      {
        BucketMeta &bm = bucketMeta[b];
        bool known = true;
        if (!ownMaterial)
        {
          if (bm.isPhysCollidable != isPhysCollidable || bm.isTraceable != isTraceable || bm.setKey != (int)m.physMatId)
            continue;
        }
        else
        {
          if (bm.behaviorFlags != m.behaviorFlags || !bm.fusable)
            continue;
          known = find_value_idx(bm.mats, nodeMat) != -1;
          if (!known && bm.mats.size() + 1 > (int)CollisionResource::MAX_NODE_PHYS_MATS - 1)
            continue; // full: this node opens a fresh bucket of the same behaviour class
        }
        BBox3 grown = bm.box;
        grown += m.modelBBox;
        const Point3 w = grown.width();
        if (max(w.x, max(w.y, w.z)) > MAX_MESH_NODE_EXTENT || bm.faces + nodeFaces > MAX_MESH_NODE_FACES ||
            bm.verts + nodeVerts > MAX_MESH_NODE_VERTS)
        {
          ++boundsRefusals;
          continue;
        }
        bucket = b;
        bm.box = grown;
        bm.faces += nodeFaces;
        bm.verts += nodeVerts;
        if (!known)
          bm.mats.push_back(nodeMat);
        break;
      }
      if (bucket == -1)
      {
        Tab<uint16_t> bucketNodes(tmpmem);
        BucketMeta meta{
          isPhysCollidable, isTraceable, m.behaviorFlags, ownMaterial, (int)m.physMatId, {}, m.modelBBox, nodeFaces, nodeVerts};
        if (ownMaterial)
          meta.mats.push_back(nodeMat);
        bucket = meshNodesByMat.size();
        meshNodesByMat.push_back(bucketNodes);
        bucketMeta.push_back(meta);
      }
      meshNodesByMat[size_t(bucket)].push_back(m.nodeIndex);
    }
  }

  // Per-bucket merged data, keyed by target nodeIndex (= nodes[0]->nodeIndex). The eventual rebuild
  // walks the node list; for each surviving node we look up a possibly-pending bucket merge here.
  struct BucketMerge
  {
    uint16_t targetNodeIndex;
    dag::Vector<Point3_vec4> verts;
    dag::Vector<uint16_t> indices;
    // One entry per face when the bucket fused across materials, or when a same-set bucket carried
    // users: the face's index into the survivor's material set. Empty otherwise: unstamped leaves.
    dag::Vector<uint8_t> faceUser;
  };
  if (boundsRefusals)
    debug("CollisionResource: the node bounds refused %u joins, %d buckets res <%s>", boundsRefusals, (int)meshNodesByMat.size(),
      res_name);
  // Nothing to merge -- the common OPTIMIZED load, whose exporter collapse already merged what it could: skip the merge machinery,
  // the rebuild and, when nothing baked, the quadratic containment sort -- an all-IDENT asset's serialized order
  // and insideOfNode are the exporter's own sort output, still valid.
  bool anyMerge = false;
  for (const Tab<uint16_t> &bucketNodes : meshNodesByMat)
    anyMerge = anyMerge || bucketNodes.size() > 1;
  if (!anyMerge)
  {
    // A bake rewrote modelBBox -- the mesh sort key and containment frame -- so order and insideOfNode must be re-derived.
    if (anyBaked)
      sortNodes();
    stampPoseScales();
    return;
  }

  // Taken before any union install, while every source still owns one material.
  dag::Vector<eastl::pair<uint16_t, uint32_t>> coverageSnapshot;
  bool anyUnionToOrder = false;
  for (const BucketMeta &bm : bucketMeta)
    anyUnionToOrder = anyUnionToOrder || bm.mats.size() > 1;
  if (anyUnionToOrder)
    for (const CollisionNode &node : nodes)
    {
      if (node.physMatId < PHYSMAT_INVALID)
        continue;
      const int matId = nodePhysMatId(node.nodeIndex, 0);
      if (matId == PHYSMAT_INVALID)
        continue;
      const uint32_t w = node.indicesCount ? node.indicesCount / 3u : 1u;
      bool known = false;
      for (auto &e : coverageSnapshot)
        if ((int)(int16_t)e.first == matId)
        {
          e.second += w;
          known = true;
          break;
        }
      if (!known)
        coverageSnapshot.push_back({(uint16_t)(int16_t)matId, w});
    }
  auto snapshotWeight = [&](int mat) -> uint32_t {
    for (const auto &e : coverageSnapshot)
      if ((int)(int16_t)e.first == mat)
        return e.second;
    return 0;
  };

  dag::Vector<int> srcMats;
  srcMats.reserve(nodes.size());
  dag::Vector<int> orderedMats;
  orderedMats.reserve(CollisionResource::MAX_NODE_PHYS_MATS);
  dag::Vector<BucketMerge> bucketMerges;
  bucketMerges.reserve(meshNodesByMat.size());
  dag::RelocatableFixedVector<int, 256> nodesToRemove;
  nodesToRemove.reserve(nodes.size());

  for (intptr_t bucketIdx = 0; bucketIdx < (intptr_t)meshNodesByMat.size(); ++bucketIdx)
  {
    const Tab<uint16_t> &bucketNodes = meshNodesByMat[bucketIdx];
    const uint16_t targetIdx = bucketNodes[0];
    uint64_t v_total = 0, i_total = 0;
    bool anyUsers = false;
    for (uint16_t ni : bucketNodes)
    {
      // Bucketed nodes are mesh nodes with geometry, so verticesCount is meaningful.
      v_total += (uint32_t)nodes[ni].verticesCount;
      i_total += nodes[ni].indicesCount;
      anyUsers |= geom[ni].hasFaceUsers();
    }
    if (v_total > MAX_MESH_NODE_VERTS)
    {
      // Too large to merge into one 16-bit-indexed node. With per-node BLAS storage each node
      // becomes its own uint32 chunk, so keep them all unmerged instead of dropping nodes[1..].
      debug("CollisionResource: bucket too large (%llu verts), keeping %d nodes unmerged res <%s>", (unsigned long long)v_total,
        (int)bucketNodes.size(), res_name);
      continue;
    }
    if (v_total == 0)
      continue;

    BucketMerge bm;
    bm.targetNodeIndex = targetIdx;
    bm.verts.reserve((size_t)v_total);
    bm.indices.reserve((size_t)i_total);

    // The survivor takes the union as its set here, before the geometry moves.
    dag::ConstSpan<int> fusedMats = make_span_const(bucketMeta[bucketIdx].mats);
    const bool fused = fusedMats.size() > 1;
    // The survivor is a source too, so capture every source's own material first.
    if (fused)
    {
      srcMats.clear();
      for (uint16_t ni : bucketNodes)
        srcMats.push_back(nodePhysMatId(ni, 0));
      // The union in snapshot coverage order (ties by ascending id, PHYSMAT_INVALID last, matching the live sort's tie rule),
      // installed order-preserving.
      orderedMats.clear();
      orderedMats.insert(orderedMats.end(), fusedMats.begin(), fusedMats.end());
      for (int i = 1; i < (int)orderedMats.size(); ++i)
      {
        const int m = orderedMats[i];
        const uint32_t w = snapshotWeight(m);
        int at = i;
        for (; at > 0 && (snapshotWeight(orderedMats[at - 1]) < w || (snapshotWeight(orderedMats[at - 1]) == w &&
                                                                       (uint16_t)(int16_t)orderedMats[at - 1] > (uint16_t)(int16_t)m));
             --at)
          orderedMats[at] = orderedMats[at - 1];
        orderedMats[at] = m;
      }
      if (!setNodePhysMats(targetIdx, dag::ConstSpan<int>(orderedMats.data(), orderedMats.size()), /*coverage_order*/ false))
      {
        // Cannot encode the union -- only a full pool can refuse it, since the bucket guard above keeps every union under the count
        // cap. Keep the bucket unmerged rather than fuse nodes whose materials the leaves could not tell apart.
        debug("CollisionResource: bucket of %d materials not encodable, keeping %d nodes unmerged res <%s>", (int)fusedMats.size(),
          (int)bucketNodes.size(), res_name);
        continue;
      }
    }
    if (fused || anyUsers)
      bm.faceUser.reserve((size_t)(i_total / 3u));

    CollisionNode &targetNode = nodes[targetIdx];
    BBox3 bbox;
    uint16_t v_off = 0;
    for (int srcIdxInBucket = 0; srcIdxInBucket < bucketNodes.size(); ++srcIdxInBucket)
    {
      const CollisionNode &m = nodes[bucketNodes[srcIdxInBucket]];
      const NodeGeom &mg = geom[bucketNodes[srcIdxInBucket]];
      const Point3_vec4 *srcVerts = rawVerts.data() + mg.vertsOfs;
      const uint32_t *srcIdx = rawIndices.data() + mg.indicesOfs;
      const uint32_t mVCount = (uint32_t)m.verticesCount;
      for (uint32_t i = 0, e = mVCount; i < e; ++i)
      {
        bm.verts.push_back(srcVerts[i]);
        bbox += srcVerts[i];
      }
      for (uint32_t i = 0, e = m.indicesCount; i < e; ++i)
        bm.indices.push_back(uint16_t(srcIdx[i] + v_off));
      v_off = (uint16_t)(v_off + mVCount);
      if (fused)
      {
        // Every face of this source carries its node's one material; find its index in the survivor's coverage-ordered set once and
        // stamp it per face.
        const int srcMat = srcMats[srcIdxInBucket];
        uint8_t userIdx = 0;
        for (int k = 0, e = nodePhysMatCount(targetIdx); k < e; ++k)
          if (nodePhysMatId(targetIdx, k) == srcMat)
          {
            userIdx = (uint8_t)k;
            break;
          }
        for (uint32_t f = 0, fe = m.indicesCount / 3u; f < fe; ++f)
          bm.faceUser.push_back(userIdx);
      }
      else if (anyUsers)
      {
        // Same-set sources: their faces keep their own users (index 0, the dominant, for a source without any).
        const dag::ConstSpan<uint8_t> users = nodeFaceUsers(bucketNodes[srcIdxInBucket]);
        for (uint32_t f = 0, fe = m.indicesCount / 3u; f < fe; ++f)
          bm.faceUser.push_back(users.empty() ? 0 : users[f]);
      }
    }

    targetNode.modelBBox = bbox;
    targetNode.radiusAroundBoxCenter = radius_around(bbox.center(), make_span_const(bm.verts));
    targetNode.flags |= targetNode.IDENT;
    targetNode.flags &= ~targetNode.TRANSLATE;
    {
      mat44f identTm;
      v_mat44_ident(identTm);
      setAuthoredNodeTm(targetIdx, identTm, targetNode.flags, 1.f);
    }

    bucketMerges.push_back(eastl::move(bm));

    // The merged geometry now lives in nodes[0]; drop the now-redundant bucket members. Too-large buckets
    // took the early continue above and keep all their nodes (each becomes a per-node BLAS chunk).
    for (int i = 1; i < bucketNodes.size(); ++i)
      nodesToRemove.push_back(bucketNodes[i]);
  }

  // Compact the node list: drop merged-away nodes, and every nodeIndex-parallel array with them. Surviving
  // nodes keep their (now-stale) raw offsets; they are re-stamped during the rebuild pass below.
  {
    dag::Vector<CollisionNode> newNodes;
    dag::Vector<TMatrix> newAuthoredTm, newRelTm;
    dag::Vector<PoseMeta> newMeta;
    dag::Vector<NodeGeom> newGeom;
    const bool hasRelTms = !relGeomTm.empty();
    newNodes.reserve(nodes.size());
    newAuthoredTm.reserve(nodes.size());
    newMeta.reserve(nodes.size());
    newGeom.reserve(nodes.size());
    for (CollisionNode &node : nodes)
    {
      if (find_value_idx(nodesToRemove, node.nodeIndex) != -1)
        continue;
      const uint16_t oldIdx = node.nodeIndex;
      node.insideOfNode = CollisionNode::INVALID_IDX;
      node.nodeIndex = (uint16_t)newNodes.size();
      newNodes.push_back(node);
      newAuthoredTm.push_back(authoredTm[oldIdx]);
      newMeta.push_back(bindMeta[oldIdx]);
      newGeom.push_back(geom[oldIdx]);
      if (hasRelTms)
        newRelTm.push_back(relGeomTm[oldIdx]);
      // the merge is keyed by the pre-compaction index; re-key it to the survivor's new slot
      for (BucketMerge &bm : bucketMerges)
        if (bm.targetNodeIndex == oldIdx)
          bm.targetNodeIndex = node.nodeIndex;
    }
    nodes = eastl::move(newNodes);
    authoredTm = eastl::move(newAuthoredTm);
    bindMeta = eastl::move(newMeta);
    geom = eastl::move(newGeom);
    if (hasRelTms)
      relGeomTm = eastl::move(newRelTm);
  }

  // Rebuild the raw arrays in node order: for each surviving mesh/convex node, append either the
  // bucket-merged data (if the node is a merge target) or the existing slice from the old arrays.
  // Stamp fresh offsets/counts on the node.
  dag::Vector<Point3_vec4> newVerts;
  dag::Vector<uint32_t> newIndices;
  dag::Vector<uint8_t> newFaceUser;
  uint64_t totalV = 0, totalI = 0;
  for (const CollisionNode &n : nodes)
    if (n.hasGeometry()) // verticesCount means nothing for empty/non-mesh nodes
    {
      totalV += (uint32_t)n.verticesCount;
      totalI += n.indicesCount;
    }
  for (const BucketMerge &bm : bucketMerges)
  {
    totalV += bm.verts.size();
    totalI += bm.indices.size();
  }
  newVerts.reserve((size_t)totalV);
  newIndices.reserve((size_t)totalI);
  for (CollisionNode &n : nodes)
  {
    if (!collres_is_mesh_list_node(n.type))
      continue;
    NodeGeom &ng = geom[n.nodeIndex];
    const BucketMerge *bm = nullptr;
    for (const BucketMerge &candidate : bucketMerges)
      if (candidate.targetNodeIndex == n.nodeIndex)
      {
        bm = &candidate;
        break;
      }
    if (bm)
    {
      ng.vertsOfs = (uint32_t)newVerts.size();
      n.verticesCount = (uint32_t)bm->verts.size();
      ng.indicesOfs = (uint32_t)newIndices.size();
      n.indicesCount = (uint32_t)bm->indices.size();
      newVerts.insert(newVerts.end(), bm->verts.begin(), bm->verts.end());
      newIndices.insert(newIndices.end(), bm->indices.begin(), bm->indices.end());
      ng.faceUserOfs = NodeGeom::NO_FACE_USERS;
      if (!bm->faceUser.empty())
      {
        ng.faceUserOfs = (uint32_t)newFaceUser.size();
        newFaceUser.insert(newFaceUser.end(), bm->faceUser.begin(), bm->faceUser.end());
      }
    }
    else if (n.hasGeometry())
    {
      const uint32_t newVOfs = (uint32_t)newVerts.size();
      const uint32_t newIOfs = (uint32_t)newIndices.size();
      const Point3_vec4 *srcV = rawVerts.data() + ng.vertsOfs;
      const uint32_t *srcI = rawIndices.data() + ng.indicesOfs;
      newVerts.insert(newVerts.end(), srcV, srcV + (uint32_t)n.verticesCount);
      newIndices.insert(newIndices.end(), srcI, srcI + n.indicesCount);
      ng.vertsOfs = newVOfs;
      ng.indicesOfs = newIOfs;
      if (ng.hasFaceUsers())
      {
        const dag::ConstSpan<uint8_t> srcU = nodeFaceUsers(n.nodeIndex);
        ng.faceUserOfs = (uint32_t)newFaceUser.size();
        newFaceUser.insert(newFaceUser.end(), srcU.begin(), srcU.end());
      }
    }
  }
  rawVerts = eastl::move(newVerts);
  rawIndices = eastl::move(newIndices);
  rawFaceUser = eastl::move(newFaceUser);
  // The merged geometry is read out by here: release it before the sort, which is quadratic in
  // the node count and wants the memory more than this staging does.
  bucketMerges.clear();
  bucketMerges.shrink_to_fit();

  sortNodes();
  stampPoseScales();
}

template <typename T>
static inline void readTab(IGenLoad &cb, T &tab)
{
  int s = cb.readInt();
  tab.resize(0);
  if (s)
  {
    reserve_and_resize(tab, s);
    cb.read(tab.data(), data_size(tab));
  }
}

static inline auto load_frt16(IGenLoad &cb) { return StaticSceneRayTracerT<uint16_t>::load(cb); }

// Legacy on-disk bits in CollisionNode::flags signaling that the per-node mesh data was written
// as an offset into the FRT vertex/face dump rather than as raw data. The runtime no longer keeps
// these flags; the reader decodes the FRT slice into the raw arrays and clears the bits.
static constexpr uint8_t LEGACY_FLAG_VERTICES_ARE_REFS = 64;
static constexpr uint8_t LEGACY_FLAG_INDICES_ARE_REFS = 128;

bool CollisionResourceBuilder::loadLegacy(IGenLoad &crd, unsigned label, const char *res_name, int (*resolve_phmat)(const char *))
{
  G_ASSERTF_RETURN((label & COLLRES_STREAM_MAGIC_MASK) == COLLRES_STREAM_MAGIC, false, "Invalid collision resource: 0x%8X", label);
  if (!res_name)
    res_name = "?";
  return (label & COLLRES_STREAM_VERSION_MASK) == 0 ? loadV0(crd, res_name, resolve_phmat) : loadV1(crd, res_name, resolve_phmat);
}

bool CollisionResourceBuilder::loadV1(IGenLoad &zcrd, const char *res_name, int (*resolve_phmat)(const char *))
{
  zcrd.read(&vFullBBox, sizeof(vFullBBox));
  zcrd.read(&vBoundingSphere, sizeof(vBoundingSphere));
  zcrd.read(&boundingBox, sizeof(boundingBox));
  zcrd.readReal(boundingSphereRad);
  collisionFlags = zcrd.readInt();

  // Legacy FRT blocks on disk: drained into local unique_ptrs (released at the end of this function), so
  // nothing holds them beyond the read. Consulted only by LEGACY_FLAG_VERTICES_ARE_REFS /
  // LEGACY_FLAG_INDICES_ARE_REFS node entries to source mesh vertex/index data.
  using LegacyFRT = const StaticSceneRayTracerT<uint16_t>;
  eastl::unique_ptr<LegacyFRT, DestroyDeleter<LegacyFRT>> legacyTraceFRT, legacyCollFRT;
  if (collisionFlags & COLLISION_RES_FLAG_HAS_TRACE_FRT)
    legacyTraceFRT.reset(load_frt16(zcrd));
  if ((collisionFlags & COLLISION_RES_FLAG_HAS_COLL_FRT) && !(collisionFlags & COLLISION_RES_FLAG_REUSE_TRACE_FRT))
    legacyCollFRT.reset(load_frt16(zcrd));

  const uint32_t numNodes = (uint32_t)zcrd.readInt();
  nodes.resize(numNodes);
  authoredTm.resize(numNodes, TMatrix::IDENT);
  bindMeta.resize(numNodes);
  geom.resize(numNodes);
  relGeomTm.resize((collisionFlags & COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID) ? numNodes : 0u, TMatrix::IDENT);
  String tmp_node_name, tmp_str;
  dag::RelocatableFixedVector<plane3f, 16, true, framemem_allocator> stagedPlanes;
  dag::Vector<uint16_t> idx16; // one node's 16-bit slice, resized per node like the strings above
  for (uint32_t nodeIdx = 0; nodeIdx < numNodes; ++nodeIdx)
  {
    CollisionNode &n = nodes[nodeIdx];
    n.nodeIndex = (uint16_t)nodeIdx;
    zcrd.readString(tmp_node_name);
    n.nameOfs = landName(tmp_node_name.c_str());
    zcrd.readString(tmp_str);
    n.physMatId = resolve_phmat ? resolve_phmat(tmp_str.str()) : PhysMat::getMaterialId(tmp_str.str());

    zcrd.read(&n.modelBBox, sizeof(n.modelBBox));
    BSphere3 tmpSph;
    zcrd.read(&tmpSph, sizeof(BSphere3));
    n.behaviorFlags = zcrd.readIntP<2>();
    n.flags = zcrd.readIntP<1>();
    n.type = (CollisionResourceNodeType)zcrd.readIntP<1>();
    if (n.type >= NUM_COLLISION_NODE_TYPES) // the type picks the record layout below; nothing past it can be read
    {
      logerr("collision node <%s> of res <%s>: unknown node type %d, stream refused", tmp_node_name.c_str(), res_name, (int)n.type);
      return false;
    }
    collres_fold_legacy_sphere(n.type, n.modelBBox, n.radiusAroundBoxCenter, tmpSph.c, tmpSph.r);
    // Preserve serialized pose metadata verbatim.
    TMatrix nodeAuthoredTm;
    float authoredMaxScale;
    zcrd.readReal(authoredMaxScale);
    zcrd.read(&nodeAuthoredTm, sizeof(nodeAuthoredTm));
    n.insideOfNode = zcrd.readIntP<2>();

    {
      uint16_t planesCnt = (uint16_t)zcrd.readIntP<2>();
      stagedPlanes.resize(planesCnt);
      if (planesCnt)
        zcrd.read(stagedPlanes.data(), planesCnt * sizeof(plane3f));
    }

    if (int cnt = zcrd.readInt())
    {
      if ((uint32_t)cnt > (uint32_t)MAX_16BIT_INDEXED_VERTS)
        DAG_FATAL("Mesh vertex count %d > %d in node <%s> of res <%s>", cnt, MAX_16BIT_INDEXED_VERTS, tmp_node_name.c_str(), res_name);
      const uint32_t prev = (uint32_t)rawVerts.size();
      geom[nodeIdx].vertsOfs = prev;
      n.verticesCount = (uint32_t)cnt;
      rawVerts.resize(prev + cnt);
      if (n.flags & LEGACY_FLAG_VERTICES_ARE_REFS)
      {
        int ofs = zcrd.readInt();
        const auto &legacyFRT = (ofs & 0x40000000) ? legacyCollFRT : legacyTraceFRT;
        memcpy(rawVerts.data() + prev, &legacyFRT->verts(ofs & 0xFFFFFF), cnt * sizeof(Point3_vec4)); //-V780
      }
      else
        zcrd.read(rawVerts.data() + prev, cnt * sizeof(Point3_vec4));
    }

    if (int cnt = zcrd.readInt())
    {
      // cnt comes straight from the pack. A negative value sign-extends to a huge size_t in the
      // idx16 allocation below; a non-multiple-of-3 is not a triangle list. The upper bound is a pure
      // allocation guard against a corrupt count, not a format limit: index count is faces*3 and is
      // unrelated to the 65536 vertex bound, and the exporter writes idxs.size() uncapped, so the
      // bound must clear any node a real cook can produce or the pack fails to round-trip.
      if (cnt < 0 || (cnt % 3) != 0 || (uint32_t)cnt > 0x4000000u)
        DAG_FATAL("Malformed index count %d in node <%s> of res <%s>", cnt, tmp_node_name.c_str(), res_name);
      const uint32_t prev = (uint32_t)rawIndices.size();
      geom[nodeIdx].indicesOfs = prev;
      n.indicesCount = (uint32_t)cnt;
      rawIndices.resize(prev + cnt);
      // The pack stores 16-bit node-local indices; the raw indices are uint32 (per-node BLAS chunks
      // may dup past 65536 verts), so read the 16-bit slice into a temp and widen element-wise.
      idx16.resize((size_t)cnt);
      if (n.flags & LEGACY_FLAG_INDICES_ARE_REFS)
      {
        int ofs = zcrd.readInt();
        const auto &legacyFRT = (ofs & 0x40000000) ? legacyCollFRT : legacyTraceFRT;
        memcpy(idx16.data(), (uint16_t *)legacyFRT->faces(0).v + (ofs & 0xFFFFFF), cnt * sizeof(uint16_t));
      }
      else
        zcrd.read(idx16.data(), cnt * sizeof(uint16_t));
      uint32_t *dst = rawIndices.data() + prev;
      for (int i = 0; i < cnt; ++i)
        dst[i] = idx16[i];
    }
    // Mask down to the transform class: the legacy-ref markers are consumed above, and no other
    // persisted bit may leak into the runtime flags (TRACE_TWO_SIDED etc. are stamped at the landing).
    n.flags &= POSE_CLASS_BITS;
    {
      mat44f vAuthoredTm;
      v_mat44_make_from_43cu_unsafe(vAuthoredTm, nodeAuthoredTm.array);
      setAuthoredNodeTm((int)nodeIdx, vAuthoredTm, n.flags, authoredMaxScale);
      if (n.type == COLLISION_NODE_TYPE_BOX || n.type == COLLISION_NODE_TYPE_SPHERE)
        unbakePrimNode(n, nodeAuthoredTm, bindMeta[nodeIdx]);
    }
    if (n.type == COLLISION_NODE_TYPE_CAPSULE)
    {
      // Capsule geometry remains node-local; T carries its placement.
      Capsule c;
      if (DAGOR_UNLIKELY(n.radiusAroundBoxCenter < 0.f))
      {
        // Zero-vert marker: Capsule::set on the inverted empty box would fabricate a huge
        // phantom segment; store a degenerate capsule and keep the node out of tracing.
        c.set(Point3(0, 0, 0), Point3(0, 0, 0), 0.f);
        bindMeta[nodeIdx].setTraceable(false);
      }
      else
        c.set(n.modelBBox);
      if (DAGOR_UNLIKELY(capsules.size() >= 0x10000u)) // capsuleIndex is 16-bit
        DAG_FATAL("capsule pool overflow: %d capsules at node <%s>", (int)capsules.size(), tmp_node_name.c_str());
      n.capsuleIndex = (uint16_t)capsules.size();
      capsules.push_back(c);
    }

    if (!stagedPlanes.empty())
    {
      const size_t prevSize = convexPlanes.size();
      // planesOfs is uint16_t; offsets above 65535 would wrap on cast and read the wrong slice
      // back. Bail out with a fatal error rather than silently corrupting the resource.
      if (prevSize > eastl::numeric_limits<uint16_t>::max())
        DAG_FATAL("Convex planes total %u exceeds uint16_t offset limit in node <%s> of res <%s>", (unsigned)prevSize,
          tmp_node_name.c_str(), res_name);
      n.planesOfs = (uint16_t)prevSize;
      n.planesCount = (uint16_t)stagedPlanes.size();
      convexPlanes.insert(convexPlanes.end(), stagedPlanes.begin(), stagedPlanes.end());
    }
  }
  if (collisionFlags & COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID)
    zcrd.read(relGeomTm.data(), data_size(relGeomTm));
  stampPoseScales();
  return true;
}

bool CollisionResourceBuilder::loadV0(IGenLoad &_cb, const char *res_name, int (*resolve_phmat)(const char *))
{
  int version = _cb.readInt();
  bool hasMaterialData = version >= 0x20150115;
  bool hasCollisionFlags = version >= 0x20180510;
  if (version != 0x20200300 && version != 0x20180510 && version != 0x20160120 && version != 0x20150115 && version != 0x20080925)
    DAG_FATAL("Invalid collision resource version %#08X", version);

  BSphere3 resSphere;
  _cb.beginBlock();
  _cb.read(&resSphere, sizeof(BSphere3));
  _cb.endBlock();

  unsigned int blockFlags = 0;
  const int blockSize = _cb.beginBlock(&blockFlags);
  struct MemoryChainedData *unpacked_data = nullptr;

  if (blockFlags == btag_compr::ZSTD)
  {
    MemorySaveCB cwrUnpack(clamp((blockSize * 4 + 0xFFF) & ~0xFFF, 2 << 10, 64 << 10));
    zstd_decompress_data(cwrUnpack, _cb, blockSize);
    unpacked_data = cwrUnpack.takeMem();
  }
  else if (blockFlags == btag_compr::OODLE)
  {
    int decompSize = _cb.readInt();
    MemorySaveCB cwrUnpack(eastl::min(decompSize, 64 << 10));
    oodle_decompress_data(cwrUnpack, _cb, blockSize - sizeof(int), decompSize);
    unpacked_data = cwrUnpack.takeMem();
  }

  MemoryLoadCB crd(unpacked_data, true);
  IGenLoad &cb = blockFlags != btag_compr::NONE ? static_cast<IGenLoad &>(crd) : _cb;

  beginRawNodes(hasCollisionFlags ? (uint32_t)cb.readInt() : collisionFlags, resSphere);
  const unsigned int numNodes = cb.readInt();
  String nodeName, matName;
  dag::RelocatableFixedVector<int, 4096> tempIndices;
  dag::RelocatableFixedVector<Point3, 1024> tempVertices;
  SmallTab<Plane3, TmpmemAlloc> tempConvexPlanes;
  for (unsigned int nodeNo = 0; nodeNo < numNodes; nodeNo++)
  {
    RawNode n;
    cb.readString(nodeName);
    if (hasMaterialData)
      cb.readString(matName);
    const uint32_t typeAndBehFlags = cb.readInt();
    n.type = (CollisionResourceNodeType)(typeAndBehFlags & 0xFFu);
    if (n.type >= NUM_COLLISION_NODE_TYPES) // the type picks the record layout below; nothing past it can be read
    {
      logerr("collision node <%s> of res <%s>: unknown node type %d, stream refused", nodeName.c_str(), res_name, (int)n.type);
      return false;
    }
    if ((collisionFlags & COLLISION_RES_FLAG_HAS_BEHAVIOUR_FLAGS) == COLLISION_RES_FLAG_HAS_BEHAVIOUR_FLAGS)
      n.behaviorFlags = (uint16_t)((typeAndBehFlags & 0xFF00) | cb.readIntP<1>());
    cb.read(&n.tm, sizeof(TMatrix));
    if (collisionFlags & COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID)
      cb.read(&n.relGeomTm, sizeof(TMatrix));
    cb.read(&n.sphere, sizeof(BSphere3));
    cb.read(&n.box, sizeof(BBox3));
    if (n.type == COLLISION_NODE_TYPE_CONVEX)
      readTab(cb, tempConvexPlanes);
    else
      tempConvexPlanes.clear();
    readTab(cb, tempVertices);
    readTab(cb, tempIndices);
    n.name = nodeName.c_str();
    n.physMat = hasMaterialData ? matName.c_str() : nullptr;
    n.planes = make_span_const(tempConvexPlanes);
    n.verts = make_span_const(tempVertices);
    n.indices = make_span_const(tempIndices);
    addRawNode(n, res_name, resolve_phmat);
  }
  _cb.endBlock();
  endRawNodes();
  return true;
}

void CollisionResourceBuilder::beginRawNodes(uint32_t collision_flags, const BSphere3 &res_sphere)
{
  collisionFlags = collision_flags;
  vBoundingSphere = v_perm_xyzd(v_ldu(&res_sphere.c.x), v_splats(res_sphere.r2));
  boundingSphereRad = res_sphere.r;
  v_bbox3_init_empty(vFullBBox); // the union of the node boxes
}

void CollisionResourceBuilder::addRawNode(const RawNode &src, const char *res_name, int (*resolve_phmat)(const char *))
{
  const int nodeNo = newNode(src.name);
  CollisionNode &node = nodes[nodeNo];
  if (src.physMat)
    node.physMatId = resolve_phmat ? resolve_phmat(src.physMat) : PhysMat::getMaterialId(src.physMat);
  else
    node.physMatId = PHYSMAT_INVALID;
  node.type = src.type;
  if ((collisionFlags & COLLISION_RES_FLAG_HAS_BEHAVIOUR_FLAGS) == COLLISION_RES_FLAG_HAS_BEHAVIOUR_FLAGS)
    node.behaviorFlags = src.behaviorFlags;

  if (collisionFlags & COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID)
    relGeomTm[nodeNo] = src.relGeomTm;
  mat44f vNodeTm;
  v_mat44_make_from_43cu_unsafe(vNodeTm, src.tm.array);
  float authoredMaxScale;
  node.flags = CollisionResource::classifyNodeTmFlags(vNodeTm, authoredMaxScale);
  setAuthoredNodeTm(nodeNo, vNodeTm, node.flags, authoredMaxScale);

  node.modelBBox = src.box;
  if (src.verts.size() > MAX_16BIT_INDEXED_VERTS)
    DAG_FATAL("Mesh vertexes count %i > %i in node <%s> of res <%s>", (int)src.verts.size(), MAX_16BIT_INDEXED_VERTS, src.name,
      res_name);

  if (!src.indices.empty() && !src.verts.empty())
  {
    bbox3f vertBox;
    v_bbox3_init_empty(vertBox);
    for (int i = 0, e = src.verts.size(); i < e; i++) // the last vertex may end the heap buffer: no 16 B load there
      v_bbox3_add_pt(vertBox, i + 1 < e ? v_ldu(&src.verts[i].x) : v_ldu_p3_safe(&src.verts[i].x));

    v_stu_bbox3(node.modelBBox, vertBox);
  }
  // Fold against the box the builder KEEPS: the stored one is the exporter's world-frame box, and a
  // geometry node's was just replaced by the node-local vert box, so folding earlier would add the
  // authored placement to the radius.
  collres_fold_legacy_sphere(node.type, node.modelBBox, node.radiusAroundBoxCenter, src.sphere.c, src.sphere.r);
  bbox3f nodeBBox = v_ldu_bbox3(node.modelBBox); // a SPHERE node's box is now its sphere's

  if (!v_bbox3_is_empty(nodeBBox))
  {
    if (node.type == COLLISION_NODE_TYPE_MESH || node.type == COLLISION_NODE_TYPE_CONVEX || node.type == COLLISION_NODE_TYPE_CAPSULE)
      v_bbox3_init(nodeBBox, vNodeTm, nodeBBox);
    v_bbox3_add_box(vFullBBox, nodeBBox);
  }

  if (node.modelBBox.lim[0].x > node.modelBBox.lim[1].x) // Particle View 01, isempty() throws float exception.
  {
    eraseNode(nodeNo); // its name stays in the pool
    return;
  }

  if (!src.verts.empty())
  {
    const uint32_t prev = (uint32_t)rawVerts.size();
    geom[nodeNo].vertsOfs = prev;
    node.verticesCount = (uint32_t)src.verts.size();
    rawVerts.resize(prev + src.verts.size());
    Point3_vec4 *dst = rawVerts.data() + prev;
    for (int i = 0; i < src.verts.size(); ++i)
    {
      dst[i] = src.verts[i];
      dst[i].resv = 1.0f;
    }
  }
  if (!src.indices.empty() && !src.verts.empty())
  {
    const uint32_t prev = (uint32_t)rawIndices.size();
    geom[nodeNo].indicesOfs = prev;
    rawIndices.resize(prev + src.indices.size());
    uint32_t *dst = rawIndices.data() + prev;
    // Drop faces with any out-of-range vertex index here, not at the chunk build: the exporter
    // indexes the raw slices unchecked. A signed cast of a negative index wraps to a huge
    // unsigned, so the unsigned compare catches both negatives and overflow.
    const unsigned vcount = (unsigned)src.verts.size();
    uint32_t kept = 0, droppedFaces = 0;
    for (int i = 0; i + 2 < src.indices.size(); i += 3) // rotate (0,1,2)->(0,2,1)
    {
      if ((unsigned)src.indices[i] >= vcount || (unsigned)src.indices[i + 1] >= vcount || (unsigned)src.indices[i + 2] >= vcount)
      {
        ++droppedFaces;
        continue;
      }
      dst[kept + 0] = (uint32_t)src.indices[i + 0];
      dst[kept + 2] = (uint32_t)src.indices[i + 1];
      dst[kept + 1] = (uint32_t)src.indices[i + 2];
      kept += 3;
    }
    const uint32_t trailing = (uint32_t)(src.indices.size() % 3);
    if (droppedFaces)
      logerr("collision node <%s> of res <%s>: %u faces reference an out-of-range vertex; dropped", src.name, res_name, droppedFaces);
    if (trailing)
      logerr("collision node <%s> of res <%s>: %u trailing indices (not a whole face); dropped", src.name, res_name, trailing);
    rawIndices.resize(prev + kept);
    node.indicesCount = kept;
  }

  if (node.type == COLLISION_NODE_TYPE_BOX || node.type == COLLISION_NODE_TYPE_SPHERE)
    unbakePrimNode(node, src.tm, bindMeta[nodeNo]);
  if (node.type == COLLISION_NODE_TYPE_CAPSULE)
  {
    // node-local, no bake: T carries the placement (see the v1 reader)
    Capsule c;
    if (DAGOR_UNLIKELY(node.radiusAroundBoxCenter < 0.f))
    {
      // Zero-vert marker: see the v1 reader.
      c.set(Point3(0, 0, 0), Point3(0, 0, 0), 0.f);
      bindMeta[nodeNo].setTraceable(false);
    }
    else
      c.set(node.modelBBox);
    if (DAGOR_UNLIKELY(capsules.size() >= 0x10000u)) // capsuleIndex is 16-bit
      DAG_FATAL("capsule pool overflow: %d capsules at node <%s>", (int)capsules.size(), src.name);
    node.capsuleIndex = (uint16_t)capsules.size();
    capsules.push_back(c);
  }
  else if (node.type == COLLISION_NODE_TYPE_CONVEX && !src.planes.empty())
  {
    size_t prevSize = convexPlanes.size();
    if (prevSize > eastl::numeric_limits<uint16_t>::max())
      DAG_FATAL("Convex planes total %u exceeds uint16_t offset limit in node <%s> of res <%s>", (unsigned)prevSize, src.name,
        res_name);
    node.planesOfs = (uint16_t)prevSize;
    node.planesCount = (uint16_t)src.planes.size();
    convexPlanes.resize(prevSize + src.planes.size());
    for (int i = 0; i < src.planes.size(); ++i)
    {
      Plane3 p = src.planes[i];
      p.normalize();
      convexPlanes[prevSize + i] = v_ldu(&p.n.x);
    }
  }
}

void CollisionResourceBuilder::endRawNodes()
{
  v_stu_bbox3(boundingBox, vFullBBox);
  sortNodes();
  stampPoseScales();
}

void CollisionResourceBuilder::fromResource(const CollisionResource &res)
{
  const dag::ConstSpan<CollisionNode> src = res.getAllNodes();
  nodes.assign(src.begin(), src.end());
  authoredTm.assign(res.data->authoredNodeTm().begin(), res.data->authoredNodeTm().end());
  relGeomTm.assign(res.data->relGeomNodeTms().begin(), res.data->relGeomNodeTms().end());
  bindMeta = res.defaultInstance.poseMeta; // the live flags become the bind flags
  physMatPool.assign(res.data->physMatPool().begin(), res.data->physMatPool().end());
  capsules.assign(res.data->capsules().begin(), res.data->capsules().end());
  convexPlanes.assign(res.data->convexPlanes().begin(), res.data->convexPlanes().end());
  names.assign(res.data->names().begin(), res.data->names().end());
  vFullBBox = res.vFullBBox;
  vBoundingSphere = res.vBoundingSphere;
  boundingBox = res.boundingBox;
  boundingSphereRad = res.boundingSphereRad;
  collisionFlags = res.collisionFlags;
  geom.clear();
  geom.resize(nodes.size());
  rawVerts.clear();
  rawIndices.clear();
  rawFaceUser.clear();
  for (CollisionNode &n : nodes)
  {
    const int i = n.nodeIndex;
    const bool geometry = collres_is_mesh_list_node(n.type) && n.hasGeometry();
    // The posed state is this rebuild's authored baseline, and the landing stamps its chunks from
    // it, so the authored flags follow the live ones rather than keeping the source's. Deliberate:
    // re-seeding no longer returns to the pre-rule state, which the source resource still holds.
    n.behaviorFlags = bindMeta[i].behaviorFlags;
    // the built-state fields are derived again at the landing
    n.nodeBlasOfs = ~0u;
    n.tlasLeafLoc = ~0u;
    n.geomNodeId = dag::Index16();
    if (!geometry)
    {
      n.verticesCount = 0;
      continue;
    }
    geom[i].vertsOfs = (uint32_t)rawVerts.size();
    geom[i].indicesOfs = (uint32_t)rawIndices.size();
    rawVerts.resize(rawVerts.size() + n.verticesCount);
    Point3_vec4 *dst = rawVerts.data() + geom[i].vertsOfs;
    res.iterateNodeVerts(i, [&](int k, vec4f v) { v_st(&dst[k].x, v_perm_xyzd(v, V_C_ONE)); });
    // A set-holding node's per-face materials live only in its leaves: carry them, or the re-chunk
    // would answer index 0 for every face.
    if (res.isNodeOwnMaterialAuthority(n))
      res.walkNodeChunkLeavesForFaces(src[i], [&](int, uint32_t i0, uint32_t i1, uint32_t i2) {
        rawIndices.push_back(i0);
        rawIndices.push_back(i1);
        rawIndices.push_back(i2);
      });
    else
    {
      geom[i].faceUserOfs = (uint32_t)rawFaceUser.size();
      res.walkNodeChunkLeavesForFaces</*WithUser*/ true>(src[i], [&](int, uint32_t i0, uint32_t i1, uint32_t i2, uint32_t user) {
        rawIndices.push_back(i0);
        rawIndices.push_back(i1);
        rawIndices.push_back(i2);
        rawFaceUser.push_back((uint8_t)user);
      });
    }
    n.indicesCount = (uint32_t)rawIndices.size() - geom[i].indicesOfs;
  }
}

// One set per node-list walk, kept apart from the chunk_store output.
struct CollisionResourceBuilder::NodeBlasBuildScratch
{
  dag::Vector<vec4f> vertsSrc, vertsOpt, qVerts;
  dag::Vector<Point3_vec4> packSrc;
  dag::Vector<unsigned> localIdx;
  dag::Vector<uint8_t> stackless, soa4, vertsTmp;
};

// Pack one node's raw slice as a vert21 stream and stamp the chunk header's frames: the exact pack
// scale (the bit-exact q-space trace transform) and the decode scale and origin.
// Per node, not a resource-level box: with a resource frame, a small Jolt-fed node inside a large
// resource would quantize on a grid orders of magnitude coarser than the per-node-referenced-bounds
// grid validateVerticesForJolt cleared at export, merging verts that Jolt's own quantization then
// rejects as degenerate.
static void pack_node_verts21(uint8_t *verts21, const Point3_vec4 *verts, uint32_t count, CollisionResource::NodeBlasChunkHeader &hdr)
{
  bbox3f bb;
  v_bbox3_init_empty(bb);
  for (uint32_t i = 0; i < count; ++i)
    v_bbox3_add_pt(bb, v_ld(&verts[i].x));
  const vec3f safeSize = v_max(v_bbox3_size(bb), v_splats(build_bvh::blas_size_eps));
  const vec3f scale = v_div(v_splats(65535.f), safeSize);
  const vec3f qOfs = v_neg(v_mul(bb.bmin, scale));
  v_stu_p3(hdr.scale, scale);
  v_stu_p3(hdr.invScale, v_rcp(scale)); // exact IEEE division (v_div(1, x)), same as blasInvScale
  v_stu_p3(hdr.bmin, bb.bmin);
  for (uint32_t i = 0; i < count; ++i)
    build_bvh::packVert21(verts21 + (size_t)i * 8u, v_madd(v_ld(&verts[i].x), scale, qOfs));
}

// A quad-BLAS over one node's triangles, the node's vert21 stream moved inside the chunk, so the
// tree is the net memory cost. Built in the stream's own quantized space (writeQuadBVH2 is fed the
// unpacked q values with scale=1 and ofs=0), so inner-node boxes are exact integer bounds.
void CollisionResourceBuilder::buildNodeBlasChunks(dag::Span<CollisionNode> built, dag::Vector<uint8_t> &chunks)
{
  G_STATIC_ASSERT(sizeof(NodeBlasChunkHeader) == 48); // a multiple of 8: the vert21 stream stays 8-aligned
  // Every mesh/convex node gets a per-node BLAS chunk -- no size floor, so the owning vertex
  // storage is always a chunk, never a bare vert21 stream.
  NodeBlasBuildScratch scratch;
  for (CollisionNode &node : built)
  {
    node.nodeBlasOfs = ~0u;
    if (!collres_is_mesh_list_node(node.type) || !node.hasGeometry())
      continue;
    const NodeGeom &ng = geom[node.nodeIndex];
    const uint8_t *faceUser = nodeFaceUsers(node.nodeIndex).data(); // null without users
    const uint32_t at = (uint32_t)chunks.size();
    if (buildOneNodeBlasChunk(node, rawVerts.data() + ng.vertsOfs, (unsigned)node.verticesCount, rawIndices.data() + ng.indicesOfs,
          (unsigned)node.indicesCount, scratch, chunks, faceUser))
      node.nodeBlasOfs = at;
    else
      node.indicesCount = 0; // rejected chunk (reason logerr'd by the builder): no collision surface
  }
}

// The SAH-leaf renumber and the QUAD_O over-spread dup run on a local copy of nodeIdx; each false
// return logerrs its reason here. False only if the node is degenerate (no buildable quad prims);
// the post-dup vert count is not capped (indices are uint32).
bool CollisionResourceBuilder::buildOneNodeBlasChunk(CollisionNode &node, const Point3_vec4 *nodeVerts, unsigned nodeVertCount,
  const uint32_t *nodeIdx, unsigned nodeIdxCount, NodeBlasBuildScratch &scratch, dag::Vector<uint8_t> &chunk_store,
  const uint8_t *face_user)
{
  dag::Vector<vec4f> &nodeVertsSrc = scratch.vertsSrc, &nodeVertsOpt = scratch.vertsOpt, &qVerts = scratch.qVerts;
  dag::Vector<Point3_vec4> &packSrc = scratch.packSrc;
  dag::Vector<uint8_t> &stkTmp = scratch.stackless, &soaOut = scratch.soa4, &vertsTmp = scratch.vertsTmp;
  const char *name = nodeName(node.nodeIndex);
  // SAH-leaf renumber + QUAD_O over-spread dup: everything stays inside this node's index space,
  // so the dup guard and the signed 13-bit quad-leaf offsets behave identically to the combined buildBLAS flatten.
  dag::Vector<unsigned> &localIdx = scratch.localIdx;
  localIdx.resize_noinit((size_t)nodeIdxCount); // the copy below writes every element
  for (unsigned i = 0; i < nodeIdxCount; ++i)
    localIdx[i] = (unsigned)nodeIdx[i];
  // Defensive: a malformed asset can hold an index past the node's vert block; leafOrderVertexFetch
  // would read srcVerts and write its renumber table out of bounds. Drop the node.
  for (unsigned i = 0; i < nodeIdxCount; ++i)
    if (localIdx[i] >= nodeVertCount)
    {
      logerr("collision node <%s>#%u: source index out of range (>= %u verts); dropping", name, (unsigned)node.nodeIndex,
        nodeVertCount);
      return false;
    }
  nodeVertsSrc.resize_noinit(nodeVertCount); // the load below writes every element
  for (unsigned i = 0; i < nodeVertCount; ++i)
    nodeVertsSrc[i] = v_ld(&nodeVerts[i].x);
  // SAH-leaf-order renumber + shared window-block over-spread dup (build_bvh): leafOrderVertexFetch
  // runs its own SAH partition (no vertex-cache pre-pass), matching vert order to the leaf grouping.
  const unsigned postDup =
    build_bvh::leafOrderVertexFetch(localIdx.data(), nodeIdxCount, nodeVertsSrc.data(), nodeVertCount, nodeVertsOpt);

  packSrc.resize_noinit(postDup); // the store below writes every element
  for (unsigned i = 0; i < postDup; ++i)
    v_st(&packSrc[i].x, nodeVertsOpt[i]);
  // No 65536 cap: the indices are uint32 and the leaf encoding addresses each vert by absolute byte
  // offset (the signed 13-bit leaf field only bounds the dup'd intra-leaf spread, not the vert count), so a
  // heavily-dup'd chunk past 65536 verts writes its connectivity back without truncation.

  // Pack the stream (it computes the per-node frame into the chunk header), then unpack q values for
  // the tree build.
  NodeBlasChunkHeader hdr;
  vertsTmp.resize_noinit((size_t)postDup * 8u); // packVert21 writes all 8 bytes of every vert
  pack_node_verts21(vertsTmp.data(), packSrc.data(), postDup, hdr);
  qVerts.resize_noinit(postDup); // the unpack below writes every element
  for (unsigned i = 0; i < postDup; ++i)
    qVerts[i] = RayData::unpackVert21(vertsTmp.data() + (size_t)i * 8u);

  Tab<build_bvh::QuadPrim> prims;
  int qc = 0, sc = 0;
  // Leaf user bits index the owning node's material set. A fused node reaches here with its
  // per-face values; a multi-material node assembled without them lands unstamped, and
  // answers its set's first material -- log that.
  if (node.physMatId < PHYSMAT_INVALID && !face_user)
    LOGERR_ONCE("collision node <%s>#%u carries %d materials, but the per-node BLAS chunk has no per-face material to stamp", name,
      (unsigned)node.nodeIndex, nodePhysMatCount(node.nodeIndex));
  build_bvh::buildQuadPrims(prims, qc, sc, localIdx.data(), (int)(nodeIdxCount / 3u), qVerts.data(), face_user);
  if (prims.empty())
  {
    logerr("collision node <%s>#%u: no buildable geometry (degenerate); dropping", name, (unsigned)node.nodeIndex);
    return false;
  }
  // Emitted face count is known here (quad = 2 tris, single = 1; degenerate faces were dropped). Stamp
  // the node's cached count so consumers reading indicesCount/3 match the leaf walk without re-deriving.
  node.indicesCount = (uint32_t)(qc * 2 + sc) * 3u;
  // Pair quads into double-quad leaves (up to 4 tris/leaf). Single node, so pairing is
  // unconstrained: every vert belongs to this node and no vert_group is needed.
  dag::Vector<build_bvh::DoubleQuadPrim> dqPrims;
  build_bvh::buildDoubleQuadPrims(dqPrims, prims.data(), (int)prims.size(), qVerts.data());
  dag::Vector<bbox3f> primBoxes(dqPrims.size());
  build_bvh::addDoubleQuadPrimitivesAABBList(primBoxes.data(), dqPrims.data(), (int)dqPrims.size(), qVerts.data());
  Tab<bbox3f> bvhNodes;
  int maxDepth = 0;
  build_bvh::create_bvh_node_sah(bvhNodes, primBoxes.data(), (uint32_t)dqPrims.size(), 4, maxDepth);
  const int treeBytes = build_bvh::calcBLASTreeBytes((int)bvhNodes.size(), (int)dqPrims.size());
  // Pad the tree region up to 8 before the vert21 stream so it is 8-aligned; nodeVerts21Ptr / the
  // chunk trace recompute this from hdr->treeBytes.
  const uint32_t alignedTree = CollisionResource::alignVert21StreamOfs((uint32_t)treeBytes);

  // Unsigned 24-bit leaf-base gate (see packQuadA): past QUAD_BASE_BYTE_MAX it emits degenerate
  // no-hit leaves, so traces would silently miss those triangles. Reject the node rather than
  // write a corrupt chunk.
  if ((int64_t)alignedTree + (int64_t)(postDup - 1) * 8 > (int64_t)QUAD_BASE_BYTE_MAX)
  {
    logerr("collision node <%s>#%u: per-node BLAS span (tree %d B + %u verts) exceeds the unsigned 24-bit leaf base range; dropping",
      name, (unsigned)node.nodeIndex, treeBytes, postDup);
    return false;
  }
  if (!soa4_tree_fits_leaf_refs(treeBytes))
  {
    logerr("collision node <%s>#%u: per-node BLAS tree (%d B) exceeds the SoA4 LeafRef 32 MB node offset range; dropping", name,
      (unsigned)node.nodeIndex, treeBytes);
    return false;
  }

  // The bare vert21 stream is the converter's vert region, so its short-body gate and the
  // deserializer's agree byte for byte.
  stkTmp.assign((size_t)alignedTree + vertsTmp.size(), 0);
  int dataOffset = 0;
  // q-space identity frame; the vert21 stream sits past the 8-padded tree, inside the region the
  // trace walk addresses from the tree base.
  build_bvh::writeDoubleQuadBVH2(stkTmp.data(), bvhNodes.data(), dqPrims.data(), V_C_ONE, v_zero(), /*vertDataOfs*/ (int)alignedTree,
    0, 0, dataOffset);
  G_ASSERTF(dataOffset == treeBytes, "node BLAS chunk: tree wrote %d bytes, expected %d", dataOffset, treeBytes);
  memcpy(stkTmp.data() + alignedTree, vertsTmp.data(), vertsTmp.size());

  // a Jolt shape is the only flags reader: a node it can never become skips the tails and the pass
  const bool withFlags = (node.behaviorFlags & CollisionNode::PHYS_COLLIDABLE) != 0;
  const soa4::ConvertResult cr =
    soa4::buildFromStackless(stkTmp.data(), 0, treeBytes, (int)alignedTree, (int)vertsTmp.size(), soaOut, withFlags);
  if (!cr.valid())
  {
    logerr("collision node <%s>#%u: SoA4 conversion failed (tree %d B); dropping", name, (unsigned)node.nodeIndex, treeBytes);
    return false;
  }
  if (withFlags) // Jolt's active edges, into the tree's flags words
    soa4::computeEdgeFlags(soaOut.data(), cr.root, (uint32_t)cr.vertsOfs, postDup, v_ldu_p3(hdr.bmin), v_ldu_p3(hdr.invScale));

  hdr.treeBytes = (uint32_t)cr.treeBytes;
  hdr.rootRef = cr.root;
  hdr.flags = withFlags ? NodeBlasChunkHeader::HAS_EDGE_FLAGS : 0;
  const uint32_t chunkOfs = (uint32_t)chunk_store.size();
  chunk_store.resize(chunkOfs + sizeof(NodeBlasChunkHeader) + soaOut.size(), 0);
  uint8_t *chunk = chunk_store.data() + chunkOfs;
  memcpy(chunk, &hdr, sizeof(NodeBlasChunkHeader));
  memcpy(chunk + sizeof(NodeBlasChunkHeader), soaOut.data(), soaOut.size());

  // The reordered connectivity (localIdx) is baked into the chunk tree above; the chunk is the
  // node's storage from here on.
  node.verticesCount = (uint32_t)postDup;
  return true;
}

// IDENT mesh with the behavior bit and geometry: the class the legacy two-sided FRT covered.
bool CollisionResourceBuilder::isNodeEligibleForTwoSided(const CollisionNode &n, uint8_t behavior_flag) const
{
  return n.type == COLLISION_NODE_TYPE_MESH && n.checkBehaviorFlags(behavior_flag) &&
         (bindMeta[n.nodeIndex].flags & CollisionNode::IDENT) && n.indicesCount > 0;
}

// TRACE_TWO_SIDED rides FRT-replacement eligibility, not storage: the legacy two-sided FRT
// covered the eligible nodes, so those trace two-sided in every ray walk (all-hits, closest,
// any-hit). One bit answers both behaviour filters: a node whose two modes disagree (a legacy
// asset with one HAS_*_FRT bit) stays clear and keeps CullCCW under both. Reuse means the
// traceable mode serves the collidable filter too, so only it applies.
void CollisionResourceBuilder::stampTwoSidedNodes(dag::Span<CollisionNode> built)
{
  // REUSE_TRACE_FRT from the node sets (every mesh node's TRACEABLE and PHYS_COLLIDABLE bits match):
  // the persisted bit is not trusted, and the collidable operand routes by it. Assign (set and
  // clear), never OR-only: a stale bit can arrive set off disk.
  bool coll_and_trace_equals = true;
  for (const CollisionNode &node : built)
    if (collres_is_mesh_list_node(node.type) &&
        node.checkBehaviorFlags(CollisionNode::TRACEABLE) != node.checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
    {
      coll_and_trace_equals = false;
      break;
    }
  if (coll_and_trace_equals)
    collisionFlags |= COLLISION_RES_FLAG_REUSE_TRACE_FRT;
  else
    collisionFlags &= ~COLLISION_RES_FLAG_REUSE_TRACE_FRT;
  // The persisted cull-mode markers (BLAS_TWO_SIDED, legacy HAS_*_FRT), one per behavior.
  const bool traceTwoSided = (collisionFlags & (COLLISION_RES_FLAG_BLAS_TWO_SIDED | COLLISION_RES_FLAG_HAS_TRACE_FRT)) != 0;
  const bool collTwoSided = (collisionFlags & (COLLISION_RES_FLAG_BLAS_TWO_SIDED | COLLISION_RES_FLAG_HAS_COLL_FRT)) != 0;
  for (CollisionNode &n : built)
  {
    n.flags &= ~CollisionNode::TRACE_TWO_SIDED;
    const bool viaTraceable = isNodeEligibleForTwoSided(n, CollisionNode::TRACEABLE);
    const bool viaCollidable =
      isNodeEligibleForTwoSided(n, CollisionNode::PHYS_COLLIDABLE) && !(collisionFlags & COLLISION_RES_FLAG_REUSE_TRACE_FRT);
    if ((viaTraceable || viaCollidable) && (!viaTraceable || traceTwoSided) && (!viaCollidable || collTwoSided))
      n.flags |= CollisionNode::TRACE_TWO_SIDED;
  }
}

bool CollisionResourceBuilder::land(CollisionResource &dst, const char *res_name)
{
  if (!res_name)
    res_name = "?";
  G_ASSERTF_RETURN(authoredTm.size() == nodes.size() && bindMeta.size() == nodes.size() && geom.size() == nodes.size() &&
                     (relGeomTm.empty() || relGeomTm.size() == nodes.size()),
    false, "collision res <%s>: the builder's arrays are not node-parallel", res_name);
  if (nodes.size() > CollisionResource::MAX_COLLISION_NODES)
  {
    logerr("collision res <%s>: %u nodes exceed the %u-node limit; the resource stays empty", res_name, (unsigned)nodes.size(),
      (unsigned)CollisionResource::MAX_COLLISION_NODES);
    return false;
  }
#if DAGOR_DBGLEVEL > 0
  // The live seed is the authored value at land: a producer that wrote one of the two public
  // stores and not the other lands a dispatch disagreeing with the stamped chunk storage.
  for (uint32_t i = 0, e = (uint32_t)nodes.size(); i < e; ++i)
    G_ASSERTF(bindMeta[i].behaviorFlags == nodes[i].behaviorFlags,
      "collision res <%s>: node %u live behavior flags 0x%04X != authored 0x%04X", res_name, i, (unsigned)bindMeta[i].behaviorFlags,
      (unsigned)nodes[i].behaviorFlags);
#endif
  // A producer that skipped recomputeBounds lands bounds nothing traces through: say so.
  bool anyBounded = false;
  for (const CollisionNode &n : nodes)
    anyBounded |= n.type != COLLISION_NODE_TYPE_POINTS && (!collres_is_mesh_list_node(n.type) || n.hasGeometry());
  if (anyBounded && boundingBox.isempty())
    logerr("collision res <%s>: empty bounds over %u nodes, the producer skipped recomputeBounds", res_name, (unsigned)nodes.size());
  if (relGeomTm.empty())
    collisionFlags &= ~COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID;
  else
    collisionFlags |= COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID;
  stampPoseScales();
  // The fuse: an OPTIMIZED (collapsed) resource merges its survivors by behavior across materials
  // into set-holding nodes whose leaves carry the face's material.
  if (collisionFlags & COLLISION_RES_FLAG_OPTIMIZED)
    mergeMeshNodes(res_name, /*fuse_materials*/ true);
  const uint32_t nodeCountAfterFuse = (uint32_t)nodes.size();

  // The chunk builds land their counts and offsets on a copy, so they leave the raw counts alone.
  dag::Vector<CollisionNode> built(nodes.begin(), nodes.end());
  dag::Vector<uint8_t> chunks;
  buildNodeBlasChunks(make_span(built), chunks);
  stampTwoSidedNodes(make_span(built));

  // inverse(authored) for the slots that compose through it (eps-IDENT box/sphere prims, retained
  // bakes); the array is emitted only when a node needs it, identity elsewhere.
  dag::Vector<TMatrix> itm;
  bool wantItm = false;
  for (uint32_t i = 0; i < nodeCountAfterFuse && !wantItm; ++i)
    wantItm = CollisionResource::usesAuthoredFrame(built[i], bindMeta[i]);
  if (wantItm)
  {
    itm.resize(nodeCountAfterFuse, TMatrix::IDENT);
    for (uint32_t i = 0; i < nodeCountAfterFuse; ++i)
      if (CollisionResource::usesAuthoredFrame(built[i], bindMeta[i]))
      {
        const mat44f a = authored_tm44(authoredTm[i]);
        mat44f inv;
        v_mat44_inverse43(inv, a);
        v_mat_43cu_from_mat44(itm[i].array, inv);
      }
  }

  // The bind trace sphere: the resource sphere extended over composed epsilon-IDENT mesh frames and
  // over un-baked rotated/skewed boxes, whose conservative local box recomposes beyond it; the
  // whole-resource boxes (rendinst reads vFullBBox) widen over those boxes too.
  Point3_vec4 bsphC;
  v_stu_p3(&bsphC.x, vBoundingSphere);
  float bindR2 = v_extract_w(vBoundingSphere);
  for (const CollisionNode &n : built)
  {
    const PoseMeta &pm = bindMeta[n.nodeIndex];
    if (n.type == COLLISION_NODE_TYPE_BOX)
    {
      // Mirror unbakePrimNode's conservative branch: exact-frame (IDENT/TRANSLATE/baked) boxes
      // recompose inside the serialized sphere.
      if ((pm.flags & (CollisionNode::IDENT | CollisionNode::TRANSLATE)) || pm.isGeometryBaked())
        continue;
      const TMatrix &authored = authoredTm[n.nodeIndex];
      for (int k = 0; k < 8; k++)
      {
        const Point3 p = authored * n.modelBBox.point(k);
        bindR2 = max(bindR2, lengthSq(p - *(const Point3 *)&bsphC.x));
        boundingBox += p;
        v_bbox3_add_pt(vFullBBox, v_ldu_p3_safe(&p.x));
      }
      continue;
    }
    if (!collres_is_mesh_list_node(n.type) || !(pm.flags & CollisionNode::IDENT))
      continue;
    const TMatrix &authored = authoredTm[n.nodeIndex];
    // bit-compare intended: a false mismatch only widens the bind sphere conservatively
    if (memcmp(&authored, &TMatrix::IDENT, sizeof(TMatrix)) == 0) //-V1014
      continue;
    for (int k = 0; k < 8; k++)
      bindR2 = max(bindR2, lengthSq(authored * n.modelBBox.point(k) - *(const Point3 *)&bsphC.x));
  }

  using Data = CollisionResource::Data;
  uint32_t counts[Data::ARRAY_COUNT] = {};
  counts[Data::NODES] = counts[Data::AUTHORED_TM] = nodeCountAfterFuse;
  counts[Data::AUTHORED_ITM] = (uint32_t)itm.size();
  counts[Data::REL_GEOM_TMS] = (uint32_t)relGeomTm.size();
  counts[Data::NODE_BLAS] = (uint32_t)chunks.size();
  counts[Data::TLAS] = CollisionResource::tlasReserveBytes(nodeCountAfterFuse);
  counts[Data::PHYS_MAT_POOL] = (uint32_t)physMatPool.size();
  counts[Data::CAPSULES] = (uint32_t)capsules.size();
  counts[Data::CONVEX_PLANES] = (uint32_t)convexPlanes.size();
  counts[Data::NAMES] = (uint32_t)names.size();
  counts[Data::NODE_ORDER] = nodeCountAfterFuse ? Data::LIST_BOUNDS + nodeCountAfterFuse : 0;
  dst.data = Data::build(counts);
  dst.data->nodeBlasBuildId = 1; // generation 0 is left to refuse a forged ref
  collres_copy_into(dst.data->allNodesList(), built.data());
  collres_copy_into(dst.data->authoredNodeTm(), authoredTm.data());
  collres_copy_into(dst.data->authoredNodeItm(), itm.data());
  collres_copy_into(dst.data->relGeomNodeTms(), relGeomTm.data());
  collres_copy_into(dst.data->nodeBlasData(), chunks.data());
  collres_copy_into(dst.data->physMatPool(), physMatPool.data());
  collres_copy_into(dst.data->capsules(), capsules.data());
  collres_copy_into(dst.data->convexPlanes(), convexPlanes.data());
  collres_copy_into(dst.data->names(), names.data());
  dst.defaultInstance.poseMeta = bindMeta;
  // The lanes past the box are zero, as a loaded resource carries them; an empty resource lands the
  // empty box.
  if (boundingBox.isempty())
    v_bbox3_init_empty(dst.vFullBBox);
  else
    dst.vFullBBox = vFullBBox;
  dst.vFullBBox.bmin = v_perm_xyzd(dst.vFullBBox.bmin, v_zero());
  dst.vFullBBox.bmax = v_perm_xyzd(dst.vFullBBox.bmax, v_zero());
  dst.vBoundingSphere = vBoundingSphere;
  dst.vBindTraceSphere = v_perm_xyzd(vBoundingSphere, v_splats(bindR2));
  dst.boundingBox = boundingBox;
  dst.setBoundingSphereRad(boundingSphereRad);
  dst.collisionFlags = collisionFlags;
  dst.finishLoad(res_name);
  return true;
}

bool CollisionResourceBuilder::write(IGenSave &cwr, const char *res_name, const char *(*mat_name)(int id))
{
  CollisionResource tmp;
  return land(tmp, res_name) && tmp.write(cwr, mat_name);
}

// The name map of an in-process stream: the id itself, so no PhysMat table is consulted. Private
// to build, the only producer of names in this form and the only consumer of them.
static const char *collres_synthetic_mat_name(int id)
{
  static thread_local char buf[16];
  snprintf(buf, sizeof(buf), "#%d", id);
  return buf;
}

static int collres_synthetic_resolve_phmat(const char *name) { return name[0] == '#' ? atoi(name + 1) : PHYSMAT_DEFAULT; }

CollisionResource *CollisionResourceBuilder::build(const char *res_name, void *inplace_mem)
{
  DynamicMemGeneralSaveCB mem(tmpmem, 0, 64 << 10);
  mem.writeInt(COLLRES_STREAM_MAGIC | COLLRES_STREAM_VERSION);
  mem.beginBlock();
  write(mem, res_name, collres_synthetic_mat_name); // a refusal is logged; the truncated body loads as an empty resource
  mem.endBlock(btag_compr::NONE);
  InPlaceMemLoadCB crd(mem.data(), (int)mem.size());
  CollisionResource *res = inplace_mem ? new (inplace_mem, _NEW_INPLACE)
                                           CollisionResource(crd, -1, res_name, collres_synthetic_resolve_phmat)
                                       : new CollisionResource(crd, -1, res_name, collres_synthetic_resolve_phmat);
  // An in-process resource serves bind-pose consumers at once; the loader leaves the default
  // instance's clone to its first pose write.
  if (res->hasAllNodesTLAS())
    res->defaultInstance.refitAllTlasLeaves();
  return res;
}
