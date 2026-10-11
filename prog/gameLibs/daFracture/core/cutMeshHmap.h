// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <memory/dag_framemem.h>
#include <dag/dag_vectorMap.h>
#include <dag/dag_vectorSet.h>
#include <math/dag_mathUtils.h>
#include <math/dag_bits.h>
#include <math/integer/dag_IPoint4.h>
#include <generic/dag_relocatableFixedVector.h>
#include <util/dag_stlqsort.h>
#include <ska_hash_map/flat_hash_map2.hpp>

#include <daFracture/core/cutFaceFill.h>
#include "cutMeshCommon.h"


namespace frx
{

// Takes a mesh, plane and a heightfield, and slices the mesh in two parts above and below the heightfield.
// Heightfield type must have:
//   float sample(Point2) const;  - sample the heightfield at point on plane
//   float cellSize;              - grid cell size, distances below it are sliced linearly
//   float maxHeight;             - sample result must stay in [0, maxHeight] range
//   Point2 gridOrigin;           - grid origin offset, usually zero
//
// High level algorithm overview:
// 1. Vertex prepare & coarse pass
// - Sample heightfield for all vertices, store height of each vertex relative to heightfield
// - Gather faces, that intersect heightfield band [0, maxHeight], other faces classified in place. Intersecting faces passed to fine
// pass
// - NOTE: A tie-break keeps |height| >= eps for every vertex, so no vertex/edge ever lies exactly on the cut - several later
// invariants rely on this
// 2. Face is projected on the cut plane and measured
// - If all projected edges are below cellSize, face is cut trivially based on height values of its vertices (either sliced in 2 parts
// or emitted fully)
// - Cut edges at this step stored in the edge hashmap with a single root
// 3. For large faces, each edge is processed and stored in edge hashmap, hashmap ensures each edge is processed once and roots are
// shared
// - Long edges perform a woo-style DDA walk to find all their intersections with a grid and search roots between them
// - Short edges also do a walk, but contain 1 root max based on their samples on their ends, to be consistent with small face path
// 4. Large faces fast path:
// - No roots - emit whole face at one side
// - Exactly 2 edges with 1 root each and 2 roots in total, that are projected closer than cellSize - trivial cut as step 2
// 5. Full large face cutting is prepared
// - Cell array for face is allocated, each cell contains height sample (lazy) + up to 3 per-edge segment references
// - All roots are sorted in winding order and assigned to cells
// 6. Cut chains are traced through triangle:
// - Roots are walked in winding order, each new root starts a grid walk and traces cut to some other root through cells
// - When cell is walked, a lightweight transient scaffolding helper geometry is produced, for this all grid intersection and grid
// corners are walked in order to form a triangle fan (cell-vs-triangle is always convex)
// - This triangle fan is then traced through to determine single entry and exit point, both points are either a root on edge, or
// emitted as a vertex inside the face
// - When cell walk hits other root, it pushes it to the stack, when opposite root is hit, chain is closed, and a vertex stack is used
// to feed the whole part into earcut algorithm, to triangulate the polygon and emit it to either side
// - On failure face is bailed and emitted as a whole and partial state is rolled back
// 7. Both trivial slices and chain walk also write cuts to CutFaceData to feed into cut face fill algorithm (for solid materials
// only)
// 8. After all faces are cut, finalize pass runs:
// - All cut vertices are projected and written to CutFaceData
// - All cut vertices are reindexed and pushed to both up & down meshes

template <typename HmapImplT>
DAGOR_NOINLINE static void cut_mesh_hmap_impl(const DestrContext &ctx, const DestrMesh &mesh, DestrMesh &up_mesh, DestrMesh &down_mesh,
  CutFaceData &cut_face_data, HmapImplT &hmap)
{
  const plane3f cut_plane = v_ldu(&cut_face_data.basis.plane.n.x);

  constexpr bool DBG_DRAW = false;
  constexpr bool DBG_PROJECT = true;
  const auto dbgProj = [&](Point3 p) DAGOR_FORCE_INLINE_LAMBDA -> Point3 {
    if constexpr (DBG_PROJECT)
    {
      alignas(16) float pl[4];
      v_st(pl, cut_plane);
      const Point3 n(pl[0], pl[1], pl[2]);
      return p - n * ((n * p + pl[3]) / (n * n));
    }
    return p;
  };
  [[maybe_unused]] const auto dbgLine = [&](Point3 a, Point3 b, E3DCOLOR c)
                                          DAGOR_FORCE_INLINE_LAMBDA { ctx.dbgDraw.drawLine(dbgProj(a), dbgProj(b), c); };
  [[maybe_unused]] const auto dbgPoint = [&](Point3 p, E3DCOLOR c) DAGOR_FORCE_INLINE_LAMBDA { ctx.dbgDraw.drawPoint(dbgProj(p), c); };
  bool dbgFaceSolid = false; // used for debug draw in lambdas

  const float max_dist = hmap.maxHeight;
  vec4f basisU = v_ldu(&cut_face_data.basis.U.x);
  vec4f basisV = v_ldu(&cut_face_data.basis.V.x);
  const auto projectUV = [&](vec4f vert) DAGOR_FORCE_INLINE_LAMBDA {
    vec4f p2 = v_perm_xaxa(v_dot3_x(vert, basisU), v_dot3_x(vert, basisV));
    return Point2(v_extract_x(p2), v_extract_y(p2));
  };
  const auto projectUVv = [&](vec4f vert)
                            DAGOR_FORCE_INLINE_LAMBDA { return v_perm_xaxa(v_dot3_x(vert, basisU), v_dot3_x(vert, basisV)); };
  vec4f vCellSize = v_splats(hmap.cellSize);
  vec4f invCellSize = v_splats(1.f / hmap.cellSize);
  const vec4f gridOriginUV = v_make_vec4f(hmap.gridOrigin.x, hmap.gridOrigin.y, hmap.gridOrigin.x, hmap.gridOrigin.y);
  const auto uvToCell = [&](vec4f uv) DAGOR_FORCE_INLINE_LAMBDA { return v_mul(v_sub(uv, gridOriginUV), invCellSize); };
  const float eps = 1e-5f * max(hmap.cellSize, max_dist);

  // reserve face arrays
  dag::Vector<DestrMesh::Face, framemem_allocator> candidateFacesVec;
  const uint32_t upMeshFacesStart = up_mesh.faces.size();
  const uint32_t downMeshFacesStart = down_mesh.faces.size();
  const uint32_t upMeshVertsStart = up_mesh.verts.size();
  const uint32_t downMeshVertsStart = down_mesh.verts.size();

  // precompute distances of vertices to plane
  const DestrMesh::Vertex *__restrict verts = mesh.verts.data();
  dag::Vector<float, framemem_allocator> vertDistVec;
  dag::Vector<uint8_t, framemem_allocator> vertClassifyVec;
  vertDistVec.resize_noinit(mesh.verts.size());
  vertClassifyVec.resize_noinit(mesh.verts.size());
  float *__restrict vertDist = vertDistVec.data();
  uint8_t *__restrict vertClassify = vertClassifyVec.data();

  // repack vertices, that can be classified to be in one part of the mesh in advance
  dag::Vector<uint32_t, framemem_allocator> old2newUp, old2newDown;
  old2newUp.resize_noinit(mesh.verts.size());
  old2newDown.resize_noinit(mesh.verts.size());
  uint32_t *__restrict old2newUpPtr = old2newUp.data();
  uint32_t *__restrict old2newDownPtr = old2newDown.data();

  {
    up_mesh.verts.resize_noinit(upMeshVertsStart + mesh.verts.size());
    down_mesh.verts.resize_noinit(downMeshVertsStart + mesh.verts.size());
    DestrMesh::Vertex *__restrict upVertsPtr = up_mesh.verts.data() + upMeshVertsStart;
    DestrMesh::Vertex *__restrict downVertsPtr = down_mesh.verts.data() + downMeshVertsStart;
    auto *upVertsStart = up_mesh.verts.data(); // absolute base, so old2new matches reindexVert's v.size()
    auto *downVertsStart = down_mesh.verts.data();
    for (int i = 0, ie = mesh.verts.size(); i != ie; i++)
    {
      const float d = vertDist[i] = v_extract_x(v_plane_dist_x(cut_plane, v_ld(&verts[i].pos.x)));
      *upVertsPtr = *downVertsPtr = verts[i];
      const uint8_t down = uint8_t(d < -1e-6f);
      const uint8_t up = uint8_t(d > max_dist + 1e-6f);
      old2newUpPtr[i] = up ? upVertsPtr - upVertsStart : uint32_t(-1);
      old2newDownPtr[i] = down ? downVertsPtr - downVertsStart : uint32_t(-1);
      upVertsPtr += up;
      downVertsPtr += down;
      vertClassify[i] = up + (down << 1); // 0 for center, 1 for up, 2 for down
    }
    {
      for (int i = 0, ie = mesh.verts.size(); i != ie; i++)
      {
        const float s = vertDist[i] - hmap.sample(projectUV(v_ld(&verts[i].pos.x)));
        vertDist[i] = fabsf(s) < eps ? (s < 0.f ? -eps : eps) : s; // never 0 (+0 -> +eps)
      }
    }
    up_mesh.verts.resize(upVertsPtr - up_mesh.verts.data());
    down_mesh.verts.resize(downVertsPtr - down_mesh.verts.data());
  }

  // coarse cull pass
  {
    up_mesh.faces.resize_noinit(upMeshFacesStart + mesh.faces.size());
    down_mesh.faces.resize_noinit(downMeshFacesStart + mesh.faces.size());
    candidateFacesVec.resize_noinit(mesh.faces.size());
    DestrMesh::Face *__restrict upFacesPtr = up_mesh.faces.data() + upMeshFacesStart;
    DestrMesh::Face *__restrict downFacesPtr = down_mesh.faces.data() + downMeshFacesStart;
    DestrMesh::Face *__restrict candidateFacesPtr = candidateFacesVec.data();
    for (const auto &face : mesh.faces)
    {
      uint8_t c0 = vertClassify[face.idx.data()[0]];
      uint8_t c1 = vertClassify[face.idx.data()[1]];
      uint8_t c2 = vertClassify[face.idx.data()[2]];
      uint8_t c012 = c0 & c1 & c2;
      if ((c0 & c1 & c2) == 0)
        *candidateFacesPtr++ = face;
      else
      {
        const bool isDown = c012 & 2;
        DestrMesh::Face *__restrict &facesPtr = isDown ? downFacesPtr : upFacesPtr;
        DestrMesh::Face &f = *facesPtr++;
        uint32_t *remap = isDown ? old2newDownPtr : old2newUpPtr;
        f = face;
        f.idx.data()[0] = remap[f.idx.data()[0]];
        f.idx.data()[1] = remap[f.idx.data()[1]];
        f.idx.data()[2] = remap[f.idx.data()[2]];
      }
    }
    up_mesh.faces.resize(upFacesPtr - up_mesh.faces.data());
    down_mesh.faces.resize(downFacesPtr - down_mesh.faces.data());
    candidateFacesVec.resize(candidateFacesPtr - candidateFacesVec.data());
  }

  const uint32_t upMeshCutFacesStart = up_mesh.faces.size();
  const uint32_t downMeshCutFacesStart = down_mesh.faces.size();

  // fine pass edge data prepare

  struct EdgePt
  {
    float s;
    uint32_t rootIdx : 31, isMainBit : 1;
  };
  dag::Vector<EdgePt, framemem_allocator> edgePts;
  // 16 bytes
  struct EdgeData
  {
    enum : uint32_t
    {
      NO_ROOT = 0x3FFFFFFFu // ICE workaround
    };
    EdgeData() :
      firstRootIdx(NO_ROOT), firstRootBeforeFirstPt(0), exactlyOneRoot(0), gridX(0), xDir(0), gridY(0), yDir(0), offset(-1), cnt(-1)
    {}
    uint32_t firstRootIdx : 30, firstRootBeforeFirstPt : 1, exactlyOneRoot : 1;
    int16_t gridX : 15, xDir : 1, gridY : 15, yDir : 1;
    uint32_t offset, cnt;
  };
  constexpr uint32_t NO_ROOT = EdgeData::NO_ROOT;
  ska::flat_hash_map<uint64_t, EdgeData, eastl::hash<uint64_t>, eastl::equal_to<uint64_t>, framemem_allocator> edgeData;

  constexpr uint32_t CUT_VERT_FLAG = 0x20000000u;
  dag::Vector<DestrMesh::Vertex, framemem_allocator> cutVerts;

  const float maxSeg = hmap.cellSize;
  const float invL = 1.f / hmap.cellSize;

  const auto makeEdgeKey = [](uint32_t i1, uint32_t i2)
                             DAGOR_FORCE_INLINE_LAMBDA { return i1 < i2 ? (uint64_t(i1) << 32) | i2 : (uint64_t(i2) << 32) | i1; };
  const auto reindexVert = [&](bool up, uint32_t idx) DAGOR_FORCE_INLINE_LAMBDA {
    uint32_t &__restrict remap = (up ? old2newUpPtr : old2newDownPtr)[idx];
    if (remap == uint32_t(-1))
    {
      auto &v = (up ? up_mesh : down_mesh).verts;
      remap = v.size();
      v.push_back(verts[idx]);
    }
    return remap;
  };

  cut_face_data.edges.reserve(cut_face_data.edges.size() + candidateFacesVec.size());
  cutVerts.reserve(candidateFacesVec.size());
  edgeData.reserve(candidateFacesVec.size() * 2);
  edgePts.reserve(candidateFacesVec.size());

  const auto makeRootUnique = [&](uint32_t a, uint32_t b, float t) DAGOR_FORCE_INLINE_LAMBDA {
    const uint32_t id = CUT_VERT_FLAG | cutVerts.size();
    cutVerts.push_back_noinit() = lerp_vertex_data(verts[a], verts[b], t);
    return id;
  };

  // full edge prepare, DDA and stuff
  const auto prepareEdgeFull = [&](uint32_t i0, uint32_t i1, bool skinny) DAGOR_NOINLINE_LAMBDA -> EdgeData {
    const uint32_t a = i0 < i1 ? i0 : i1, b = i0 < i1 ? i1 : i0;
    EdgeData &__restrict edge = edgeData[makeEdgeKey(a, b)];
    if (edge.offset != -1)
      return edge; // edge is ready
    const vec4f pa = v_ld(&verts[a].pos.x), pb = v_ld(&verts[b].pos.x);
    const vec4f uva = projectUVv(pa), uvb = projectUVv(pb);
    const vec4f uvda = v_perm_xyab(uva, v_plane_dist_x(cut_plane, pa));
    const vec4f uvdb = v_perm_xyab(uvb, v_plane_dist_x(cut_plane, pb));
    const vec4f p2a = uvToCell(uva), p2b = uvToCell(uvb);
    const vec4i p2ai = v_cvt_floori(p2a), p2bi = v_cvt_floori(p2b);
    // init dir and base cell pos
    alignas(16) IPoint4 gridOfsAndSign;
    v_sti(&gridOfsAndSign, v_permi_xyab(p2ai, v_subi(p2bi, p2ai)));
    edge.gridX = gridOfsAndSign.x;
    edge.gridY = gridOfsAndSign.y;
    edge.xDir = gridOfsAndSign.z < 0;
    edge.yDir = gridOfsAndSign.w < 0;
    edge.offset = edgePts.size();
    int edgePtPos = edgePts.size();
    // check next root
    float lastS = vertDist[a], lastT = 0.f;
    const auto chkRoot = [&](float t, float s) DAGOR_FORCE_INLINE_LAMBDA -> void {
      if (DAGOR_UNLIKELY(lastT == t))
        return;
      if (DAGOR_UNLIKELY((lastS < 0.f) != (s < 0.f))) // root push - slow path
      {
        const float r = lastT + (t - lastT) * lastS / (lastS - s);
        const uint32_t rootIdx = skinny && edge.firstRootIdx != NO_ROOT ? edge.firstRootIdx : makeRootUnique(a, b, r);
        if (DBG_DRAW && dbgFaceSolid)
          dbgPoint(cutVerts[rootIdx & ~CUT_VERT_FLAG].pos, skinny ? E3DCOLOR(255, 96, 32) : E3DCOLOR(255, 32, 32));
        // skinny edges already have firstRootIdx set
        edge.exactlyOneRoot = edge.firstRootIdx == NO_ROOT || skinny;
        if (edge.firstRootIdx == NO_ROOT)
          edge.firstRootIdx = rootIdx;
        if (edgePtPos == int(edge.offset))
          edge.firstRootBeforeFirstPt = true;
        else
          edgePts.data()[edgePtPos - 1].rootIdx = rootIdx;
      }
      lastT = t;
      lastS = s;
    };
    // push next point
    const auto pushPt = [&](vec4f tttt, bool main_axis) DAGOR_FORCE_INLINE_LAMBDA {
      Point3_vec4 uvd;
      v_st(&uvd, v_lerp_vec4f(tttt, uvda, uvdb));
      const float t = v_extract_x(tttt);
      float s = skinny ? lerp(vertDist[a], vertDist[b], t) : uvd.z - hmap.sample(Point2(uvd.x, uvd.y));
      if (DAGOR_UNLIKELY(fabsf(s) < eps))
        s = s < 0.f ? -eps : eps; // tie-break
      chkRoot(t, s);
      edgePts.data()[edgePtPos++] = EdgePt{.s = s, .rootIdx = NO_ROOT, .isMainBit = uint32_t(main_axis)};
    };
    // DDA setup
    const vec4f edgeV = v_sub(p2b, p2a);
    const vec4f edgeVabs = v_abs(edgeV);
    const vec4f tstep = v_div(V_C_ONE, edgeVabs);
    const vec4f frac = v_sub(p2a, v_floor(p2a));
    vec4f tmax = v_mul(v_sel(v_sub(V_C_ONE, frac), frac, edgeV), tstep);
    tmax = v_sel(tmax, v_splats(FLT_MAX), v_cmp_eq(edgeVabs, v_zero()));
    // DDA loop
    int cntX = abs(gridOfsAndSign.z), cntY = abs(gridOfsAndSign.w);
    edgePts.resize_noinit(edgePts.size() + (cntX + cntY));
    const vec4f stepX = v_and(tstep, (vec4f)V_CI_MASK1000), stepY = v_and(tstep, (vec4f)V_CI_MASK0100);
    while (cntX | cntY)
    {
      const bool selX = (cntY == 0) | ((cntX != 0) & v_test_vec_x_le(tmax, v_splat_y(tmax)));
      cntX -= selX;
      cntY -= !selX;
      const vec4f vselX = v_cast_vec4f(v_splatsi(-int(selX)));
      pushPt(v_sel(v_splat_y(tmax), v_splat_x(tmax), vselX), selX);
      tmax = v_add(tmax, v_sel(stepY, stepX, vselX));
    }
    // last span
    G_ASSERT(edgePtPos == edgePts.size());
    edge.cnt = edgePtPos - edge.offset;
    chkRoot(1.f, vertDist[b]);
    return edge;
  };

  // prepare edge for small triangle, this edge must be skinny (small triangles are ones with all skinny edges)
  const auto prepareEdgeSmallTriRoot = [&](uint32_t i0, uint32_t i1, float t) DAGOR_FORCE_INLINE_LAMBDA -> uint32_t {
    const uint32_t a = i0 < i1 ? i0 : i1, b = i0 < i1 ? i1 : i0;
    EdgeData &edge = edgeData[makeEdgeKey(a, b)];
    if (edge.cnt != -1)
      return edge.firstRootIdx; // edge is ready for small tri (only root inited)
    edge.cnt = 0;
    edge.firstRootIdx = makeRootUnique(i0, i1, t);
    if (DBG_DRAW && dbgFaceSolid)
      dbgPoint(cutVerts[edge.firstRootIdx & ~CUT_VERT_FLAG].pos, E3DCOLOR(255, 96, 32)); // root: skinny-edge
    return edge.firstRootIdx;
  };

  const auto emitWhole = [&](const DestrMesh::Face &face, bool toUp) DAGOR_FORCE_INLINE_LAMBDA {
    DestrMesh::Face &f = (toUp ? up_mesh : down_mesh).faces.push_back_noinit();
    f.idx[0] = reindexVert(toUp, face.idx.data()[0]);
    f.idx[1] = reindexVert(toUp, face.idx.data()[1]);
    f.idx[2] = reindexVert(toUp, face.idx.data()[2]);
    f.mat = face.mat;
  };

  const auto trivialTriangleSplit = [&](uint32_t idx0, uint32_t idx1, uint32_t idx2, uint32_t cutIdx0, uint32_t cutIdx1, bool up,
                                      uint16_t mat, bool solid) DAGOR_FORCE_INLINE_LAMBDA {
    idx0 = reindexVert(up, idx0);
    idx1 = reindexVert(!up, idx1);
    idx2 = reindexVert(up, idx2);
    auto &__restrict quadFaces = up ? up_mesh.faces : down_mesh.faces; // v0/v2 side: 2 tris
    auto &__restrict loneFaces = up ? down_mesh.faces : up_mesh.faces; // v1 side: 1 tri
    DestrMesh::Face &fl = loneFaces.push_back_noinit();
    quadFaces.resize_noinit(quadFaces.size() + 2);
    DestrMesh::Face *fq = quadFaces.data() + quadFaces.size() - 2;
    fl.idx[0] = cutIdx0;
    fl.idx[1] = idx1;
    fl.idx[2] = cutIdx1;
    fl.mat = mat; // lone:  cut0, v1, cut1
    fq[0].idx[0] = cutIdx0;
    fq[0].idx[1] = cutIdx1;
    fq[0].idx[2] = idx0;
    fq[0].mat = mat; // quad1: cut0, cut1, v0
    fq[1].idx[0] = idx0;
    fq[1].idx[1] = cutIdx1;
    fq[1].idx[2] = idx2;
    fq[1].mat = mat; // quad2: v0, cut1, v2

    // write cut indices
    if (solid)
    {
      auto &cutEdge = cut_face_data.edges.push_back_noinit();
      cutEdge.a = (!up ? cutIdx0 : cutIdx1) & ~CUT_VERT_FLAG;
      cutEdge.b = (!up ? cutIdx1 : cutIdx0) & ~CUT_VERT_FLAG;
    }
  };

  const auto doFaceSplit = [&](const DestrMesh::Face face) DAGOR_FORCE_INLINE_LAMBDA {
    uint32_t idx0 = face.idx.data()[0];
    uint32_t idx1 = face.idx.data()[1];
    uint32_t idx2 = face.idx.data()[2];
    vec4f d = v_make_vec3f(vertDist[idx0], vertDist[idx1], vertDist[idx2]);
    // NO zero-snap: vertDist is guaranteed to never be zero by tie-break during prepare
    vec4f dRot = v_perm_yzxw(d);
    const unsigned intersectMask = v_signmask(v_xor(d, dRot)) & 7; // bit0 = i01, bit1 = i12, bit2 = i20
    const unsigned intersectCnt = dag::popcount(intersectMask);

    // never-zero two-valued signs change an EVEN number of times around a 3-cycle: intersectCnt is 0 or 2
    if (intersectCnt != 2)
    {
      FRX_CHECK_FAILURE(intersectCnt != 0);
      emitWhole(face, !(v_signmask(d) & 1));
      return;
    }

    // rotate so v0-v1 and v1-v2 are 2 intersections
    vec4i idx = v_make_vec3i(idx0, idx1, idx2);
    if (intersectMask == 0b110)
    {
      d = v_perm_yzxw(d);
      dRot = v_perm_yzxw(dRot);
      idx = v_cast_vec4i(v_perm_yzxw(v_cast_vec4f(idx)));
    }
    if (intersectMask == 0b101)
    {
      d = v_perm_zxyw(d);
      dRot = v_perm_zxyw(dRot);
      idx = v_cast_vec4i(v_perm_zxyw(v_cast_vec4f(idx)));
    }
    vec4f t = v_div(d, v_sub(d, dRot)); // .x = t01, .y = t12, .z = t20

    // pick unique cut indices
    idx0 = v_extract_xi(idx);
    idx1 = v_extract_yi(idx);
    idx2 = v_extract_zi(idx);
    uint32_t cutIdx0 = prepareEdgeSmallTriRoot(idx0, idx1, v_extract_x(t));
    uint32_t cutIdx1 = prepareEdgeSmallTriRoot(idx1, idx2, v_extract_y(t));

    // store cut faces: lone vertex v1 -> 1 triangle on its side; quad (v0,v2) -> 2 triangles on the other
    const bool up = !(v_signmask(d) & 1); // v0 (and the quad) is on the positive (up_mesh) side
    const bool solid = ctx.materials[face.mat].isSolid;
    trivialTriangleSplit(idx0, idx1, idx2, cutIdx0, cutIdx1, up, face.mat, solid);
    if (DBG_DRAW && solid)
      dbgLine(cutVerts[cutIdx0 & ~CUT_VERT_FLAG].pos, cutVerts[cutIdx1 & ~CUT_VERT_FLAG].pos, E3DCOLOR(0, 255, 0)); // debug:
                                                                                                                    // small-face split
                                                                                                                    // chord
  };

  // large triangle cutting by scaffolding and chain trace

  struct CellEntry
  {
    uint32_t segPack;
    float cornerS;
  };
  struct FaceEdgeRoot
  {
    union
    {
      uint32_t lIdx; // (e << 16) | idx
      struct
      {
        uint16_t idx, e;
      };
    };
    int16_t cellX, cellY;
    uint32_t cellIdx;
    uint32_t rootIdx : 31, isEdgeFwd : 1;
  };
  struct CellStepByBorder
  {
    int16_t x, y;
    uint32_t i;
    int8_t opposite;
  };
  struct FaceGrid // large face info for grid cut
  {
    uint32_t vertIdx[3];            // face corner indices into mesh verts
    Point2 cornerUv[3];             // face corners projected to cut plane UV
    alignas(16) float planeDist[4]; // corner distances to cut plane (d, NOT the s in vertDist); [3] unused, pads aligned load
    EdgeData edge[3];               // by-value copies, edgeData map may rehash during prep
    bool edgeFwd[3];                // face edge e runs canonical-forward: vertIdx[e] < vertIdx[(e + 1) % 3]
    vec4f barySub, baryMult;        // uvToBarycentric constants
    alignas(16) IPoint4 ofsSz;      // xy = cell bbox origin, zw = bbox size + 2 (far pad row/col for corner reads only)
    CellStepByBorder stepByBorder[4];
    dag::Vector<CellEntry, framemem_allocator> cells;
    dag::Vector<FaceEdgeRoot, framemem_allocator> edgeRoots; // roots sorted in face winding order
  };
  FaceGrid fg = {};
  const auto uvToBarycentric = [&](vec4f uv) DAGOR_FORCE_INLINE_LAMBDA -> vec4f {
    vec4f w = v_mul(v_perm_xyxy(v_sub(uv, fg.barySub)), fg.baryMult);
    vec4f w0w1 = v_sub(v_perm_xzxz(w), v_perm_ywyw(w)); // (w0, w1, w0, w1)
    return v_perm_xyab(w0w1, v_splat_y(v_sub(V_C_ONE, v_add(w0w1, v_splat_x(w0w1)))));
  };
  const auto uvToVert = [&](vec4f uv) DAGOR_FORCE_INLINE_LAMBDA -> DestrMesh::Vertex {
    const vec4f w = uvToBarycentric(uv);
    DestrMesh::Vertex v;
    lerp_vertex_data_barycentric(v, verts[fg.vertIdx[0]], verts[fg.vertIdx[1]], verts[fg.vertIdx[2]], v_splat_x(w), v_splat_y(w),
      v_splat_z(w));
    return v;
  };

  // prepare large triangle, allocate cells, fill edge cells and build sorted root list
  const auto buildCellArray = [&]() DAGOR_NOINLINE_LAMBDA {
    {
      vec4i cellMin = v_zeroi(), cellMax = v_zeroi();
      for (int k = 0; k < 3; k++)
      {
        const uint32_t vi = fg.vertIdx[k];
        const vec4f p = v_ld(&verts[vi].pos.x);
        fg.planeDist[k] = v_extract_x(v_plane_dist_x(cut_plane, p));
        const vec4i c = v_cvt_floori(uvToCell(v_ldu(&fg.cornerUv[k].x)));
        cellMin = k ? v_mini(cellMin, c) : c;
        cellMax = k ? v_maxi(cellMax, c) : c;
      }
      v_sti(&fg.ofsSz, v_permi_xyab(cellMin, v_addi(v_subi(cellMax, cellMin), v_splatsi(2))));

      // init barycentric
      vec4f A = v_ldu(&fg.cornerUv[0].x);
      vec4f B = v_ldu(&fg.cornerUv[1].x);
      vec4f C = v_ldu(&fg.cornerUv[2].x);
      vec4f den = v_mul(v_sub(A, C), v_perm_yxwz(v_sub(B, C)));
      den = v_sub(den, v_splat_y(den));
      vec4f invDen = v_remove_not_finite(v_rcp_x(den));
      fg.barySub = C;
      fg.baryMult = v_perm_yxwz(v_perm_xyab(v_sub(B, C), v_sub(C, A)));
      fg.baryMult = v_mul(fg.baryMult, v_splat_x(invDen));

      fg.stepByBorder[0] = CellStepByBorder{-1, 0, uint32_t(-1), 2};
      fg.stepByBorder[1] = CellStepByBorder{0, 1, uint32_t(fg.ofsSz.z), 3};
      fg.stepByBorder[2] = CellStepByBorder{1, 0, uint32_t(1), 0};
      fg.stepByBorder[3] = CellStepByBorder{0, -1, uint32_t(-fg.ofsSz.z), 1};
    }

    const auto cellIdxOf = [&](int i, int j) DAGOR_FORCE_INLINE_LAMBDA { return (i - fg.ofsSz.x) + (j - fg.ofsSz.y) * fg.ofsSz.z; };
    fg.cells.resize_noinit(fg.ofsSz.z * fg.ofsSz.w);
    mem_set_ff(fg.cells);
    fg.edgeRoots.resize_noinit(fg.edge[0].cnt + fg.edge[1].cnt + fg.edge[2].cnt + 3);
    FaceEdgeRoot *__restrict roots = fg.edgeRoots.data();
    for (int e = 0; e < 3; e++)
    {
      const EdgeData edge = fg.edge[e];
      G_ASSERT(edge.cnt < 1023);
      const uint32_t shift = 10u * e;
      const uint32_t clearMask = ~(1023u << shift);
      const uint32_t cellStartIdx = cellIdxOf(edge.gridX, edge.gridY);
      uint32_t cellIdx = cellStartIdx;
      const int32_t stepX = edge.xDir ? -1 : 1, stepY = edge.yDir ? -fg.ofsSz.z : fg.ofsSz.z;
      const int16_t cStepX = stepX, cStepY = edge.yDir ? -1 : 1;
      const EdgePt *__restrict pts = edgePts.data() + edge.offset;
      const bool fwd = fg.vertIdx[e] < fg.vertIdx[(e + 1) % 3];
      fg.edgeFwd[e] = fwd;
      int16_t cellX = edge.gridX, cellY = edge.gridY;
      uint32_t nextSpanRootIdx = edge.firstRootBeforeFirstPt ? edge.firstRootIdx : NO_ROOT;
      for (uint32_t i = 0;; i++, pts++)
      {
        CellEntry &ce = fg.cells[cellIdx];
        if (nextSpanRootIdx != NO_ROOT)
          *roots++ = FaceEdgeRoot{.lIdx = uint32_t((e << 16) | i),
            .cellX = cellX,
            .cellY = cellY,
            .cellIdx = cellIdx,
            .rootIdx = nextSpanRootIdx,
            .isEdgeFwd = uint32_t(fwd)};
        ce.segPack = (ce.segPack & clearMask) | (i << shift);
        if (i == edge.cnt)
          break;
        cellIdx += pts->isMainBit ? stepX : stepY;
        (pts->isMainBit ? cellX : cellY) += pts->isMainBit ? cStepX : cStepY;
        nextSpanRootIdx = pts->rootIdx;
      }
    }
    const auto cmpRoot = [&](const FaceEdgeRoot &a, const FaceEdgeRoot &b) DAGOR_FORCE_INLINE_LAMBDA {
      bool e = a.e < b.e;
      bool idx = a.isEdgeFwd ? a.idx < b.idx : b.idx < a.idx;
      return a.e == b.e ? idx : e;
    };
    fg.edgeRoots.resize(roots - fg.edgeRoots.data());
    if (DAGOR_LIKELY(fg.edgeRoots.size() == 2)) // most common case, full sort is almost never needed
    {
      if (cmpRoot(fg.edgeRoots.data()[1], fg.edgeRoots.data()[0]))
        eastl::swap(fg.edgeRoots.data()[0], fg.edgeRoots.data()[1]);
    }
    else
      stlsort::sort(fg.edgeRoots.begin(), fg.edgeRoots.end(), cmpRoot);
  };

  struct ChainWalkState // -V730
  {
    int16_t cellX, cellY;
    uint32_t cellIdx;
    int8_t e;
    uint16_t idx;
    uint32_t prevCutVert;
    bool poisoned = false; // bail out
  };
  const auto buildAndTraverseCell = [&](ChainWalkState &__restrict state, bool face_uv_ccw, bool dbg) DAGOR_NOINLINE_LAMBDA {
    CellEntry &cell = fg.cells[state.cellIdx];
    const auto getCornerS = [&](uint32_t c) DAGOR_FORCE_INLINE_LAMBDA -> float {
      const uint32_t ofs = c == 0 ? 0u : c == 1 ? uint32_t(fg.ofsSz.z) : c == 2 ? uint32_t(fg.ofsSz.z) + 1u : 1u;
      CellEntry &ce = fg.cells[state.cellIdx + ofs];
      if ((uint32_t &)ce.cornerS != ~uint32_t(0))
        return ce.cornerS;
      const vec4f uv =
        v_madd(v_make_vec4f(int(state.cellX) + (c >= 2), int(state.cellY) + (c == 1 || c == 2), 0.f, 0.f), vCellSize, gridOriginUV);
      const float s =
        v_extract_x(v_dot3_x(v_ld(&fg.planeDist[0]), uvToBarycentric(uv))) - hmap.sample(Point2(v_extract_x(uv), v_extract_y(uv)));
      return ce.cornerS = fabsf(s) < eps ? (s < 0.f ? -eps : eps) : s; // tie-break
    };
    constexpr uint32_t NO_BORDER = uint32_t(-1);
    const auto getCrossingBorder = [&](const EdgeData &edge, uint32_t idx, bool after_cell) DAGOR_FORCE_INLINE_LAMBDA {
      if (idx >= edge.cnt)
        return NO_BORDER;
      EdgePt pt = edgePts[edge.offset + idx];
      const uint32_t axisBit = pt.isMainBit;
      const uint32_t sideBit = bool(pt.isMainBit ? edge.xDir : edge.yDir) == after_cell; // 0 = low border, 1 = high
      return axisBit ? (sideBit << 1) : ((sideBit << 1) ^ 3);
    };
    struct ScaffoldVert
    {
      enum : uint8_t
      {
        CORNER,
        CROSSING,
        FACE_VERT
      };
      // e >= 0 - face edge (chord), e < 0 - cell border (code = -e-1)
      int8_t e;
      uint8_t kind : 2;
      uint8_t isMainAxis : 1;
      uint8_t srcEdge : 2;
      uint16_t idx;
      float s;
    };
    // stores triangle fan v0-v1-v2, v0-v2-v3, ... v0-v[N-2]-v[N-1]
    dag::RelocatableFixedVector<ScaffoldVert, 15, false> scaffoldVerts;
    int entry = -1;
    const auto pushRim = [&](const ScaffoldVert &v) DAGOR_FORCE_INLINE_LAMBDA {
      if (v.e == state.e)
        entry = int(scaffoldVerts.size());
      scaffoldVerts.push_back(v);
    };
    const auto scaffoldToUv = [&](const ScaffoldVert &sv, uint32_t border) DAGOR_NOINLINE_LAMBDA -> vec4f {
      if (sv.kind == ScaffoldVert::FACE_VERT)
        return v_ldu(&fg.cornerUv[sv.e].x);
      if (sv.kind == ScaffoldVert::CORNER)
        return v_madd(v_make_vec4f(int(state.cellX) + (sv.idx >= 2), int(state.cellY) + (sv.idx == 1 || sv.idx == 2), 0.f, 0.f),
          vCellSize, gridOriginUV);
      const uint32_t e = sv.e >= 0 ? uint32_t(sv.e) : uint32_t(sv.srcEdge);
      const bool fwd = fg.edgeFwd[e];
      const Point2 uvA = fwd ? fg.cornerUv[e] : fg.cornerUv[(e + 1) % 3];
      const Point2 uvB = fwd ? fg.cornerUv[(e + 1) % 3] : fg.cornerUv[e];
      const bool mainAxis = sv.isMainAxis;
      const int line = mainAxis ? int(state.cellX) + int(border == 2) : int(state.cellY) + int(border == 1);
      const float o = mainAxis ? hmap.gridOrigin.x : hmap.gridOrigin.y;
      const float ca = ((mainAxis ? uvA.x : uvA.y) - o) * invL;
      const float cb = ((mainAxis ? uvB.x : uvB.y) - o) * invL;
      const float t = (float(line) - ca) / (cb - ca);
      const float lineUv = o + line * hmap.cellSize;
      return mainAxis ? v_make_vec4f(lineUv, lerp(uvA.y, uvB.y, t), 0.f, 0.f) : v_make_vec4f(lerp(uvA.x, uvB.x, t), lineUv, 0.f, 0.f);
    };
    if (cell.segPack == ~0u)
    {
      if (FRX_CHECK_FAILURE(state.e >= 0))
      {
        state.poisoned = true;
        return true;
      }
      for (uint32_t corner = 0; corner < 4; corner++)
        pushRim(
          ScaffoldVert{.e = int8_t(-int(corner) - 1), .kind = ScaffoldVert::CORNER, .idx = uint16_t(corner), .s = getCornerS(corner)});
    }
    else
    {
      uint32_t lastExit = NO_BORDER, firstEntry = NO_BORDER;
      const uint32_t cornerWalkDir = face_uv_ccw ? -1 : 1;
      const uint32_t cornerWalkBias = face_uv_ccw ? 0 : 1;
      const uint32_t cornerOutBias = face_uv_ccw ? 3 : 0;
      for (uint32_t e = 0, segPack = cell.segPack; e < 3; e++, segPack >>= 10u)
      {
        uint32_t idx = segPack & 1023u;
        if (idx == 1023u)
          continue;
        EdgeData edge = fg.edge[e];
        const bool fwd = fg.edgeFwd[e];
        const uint32_t cA = getCrossingBorder(edge, idx - 1, true);
        const uint32_t cB = getCrossingBorder(edge, idx, false);
        const uint32_t entryCorner = fwd ? cA : cB;
        const uint32_t exitCorner = fwd ? cB : cA;
        const uint32_t entryIdx = fwd ? idx - 1 : idx;
        const uint32_t exitIdx = fwd ? idx : idx - 1;
        if (lastExit != NO_BORDER && entryCorner != NO_BORDER)
        {
          for (uint32_t corner = (lastExit + cornerWalkBias) & 3, stop = (entryCorner + cornerWalkBias) & 3; corner != stop;
               corner = (corner + cornerWalkDir) & 3)
            pushRim(ScaffoldVert{.e = int8_t(-int((corner + cornerOutBias) & 3) - 1),
              .kind = ScaffoldVert::CORNER,
              .idx = uint16_t(corner),
              .s = getCornerS(corner)});
        }
        if (entryCorner != NO_BORDER)
        {
          const EdgePt entryPt = edgePts[edge.offset + entryIdx];
          pushRim(ScaffoldVert{.e = int8_t(e),
            .kind = ScaffoldVert::CROSSING,
            .isMainAxis = uint8_t(entryPt.isMainBit),
            .srcEdge = uint8_t(e),
            .idx = uint16_t(idx),
            .s = entryPt.s});
        }
        else
          pushRim(ScaffoldVert{.e = int8_t(e), .kind = ScaffoldVert::FACE_VERT, .idx = uint16_t(idx), .s = vertDist[fg.vertIdx[e]]});
        if (exitCorner != NO_BORDER)
        {
          const EdgePt exitPt = edgePts[edge.offset + exitIdx];
          pushRim(ScaffoldVert{.e = int8_t(-int(exitCorner) - 1),
            .kind = ScaffoldVert::CROSSING,
            .isMainAxis = uint8_t(exitPt.isMainBit),
            .srcEdge = uint8_t(e),
            .s = exitPt.s});
        }
        firstEntry = firstEntry == NO_BORDER ? entryCorner : firstEntry;
        lastExit = exitCorner;
      }
      if (lastExit != NO_BORDER && firstEntry != NO_BORDER)
        for (uint32_t corner = (lastExit + cornerWalkBias) & 3, stop = (firstEntry + cornerWalkBias) & 3; corner != stop;
             corner = (corner + cornerWalkDir) & 3)
          pushRim(ScaffoldVert{.e = int8_t(-int((corner + cornerOutBias) & 3) - 1),
            .kind = ScaffoldVert::CORNER,
            .idx = uint16_t(corner),
            .s = getCornerS(corner)});
    }
    if (DBG_DRAW && dbg)
    {
      const auto rimPos = [&](const ScaffoldVert &sv) -> Point3 {
        uint32_t border = 0;
        if (sv.e < 0)
          border = uint32_t(-sv.e) - 1;
        else if (sv.kind == ScaffoldVert::CROSSING)
          border = getCrossingBorder(fg.edge[sv.e], fg.edgeFwd[sv.e] ? uint32_t(sv.idx) - 1 : uint32_t(sv.idx), fg.edgeFwd[sv.e]);
        return uvToVert(scaffoldToUv(sv, border)).pos;
      };
      Point3 pos[15];
      const int nv = int(scaffoldVerts.size());
      for (int i = 0; i < nv; i++)
        pos[i] = rimPos(scaffoldVerts[i]);
      for (int i = 0; i < nv; i++)
        dbgLine(pos[i], pos[i + 1 == nv ? 0 : i + 1], E3DCOLOR(150, 150, 0));
      for (int i = 2; i + 1 < nv; i++)
        dbgLine(pos[0], pos[i], E3DCOLOR(70, 0, 70));
    }
    const int n = int(scaffoldVerts.size());
    if (FRX_CHECK_FAILURE(entry < 0))
    {
      state.poisoned = true; // can't find entry cell segment
      return true;
    }
    const float sEntry = scaffoldVerts[entry].s;
    if (FRX_CHECK_FAILURE((sEntry < 0.f) == (scaffoldVerts[entry + 1 == n ? 0 : entry + 1].s < 0.f)))
    {
      state.poisoned = true; // entered segment has no sign change (no root)
      return true;
    }
    // walk scaffold triangle fan
    int walk = entry;
    int walkPrev = walk;
    int exit;
    if ((sEntry < 0.f) == (scaffoldVerts[0].s < 0.f))
    {
      do
      {
        walkPrev = eastl::exchange(walk, walk + 1 == n ? 0 : walk + 1);
      } while ((sEntry < 0.f) != (scaffoldVerts[walk].s < 0.f) && walk != entry);
      exit = walkPrev;
    }
    else
    {
      do
      {
        walkPrev = eastl::exchange(walk, walk == 0 ? n - 1 : walk - 1);
      } while ((sEntry < 0.f) == (scaffoldVerts[walk].s < 0.f) && walk != entry);
      exit = walk;
    }
    if (scaffoldVerts[exit].e < 0)
    {
      const uint32_t border = uint32_t(-scaffoldVerts[exit].e) - 1;
      const ScaffoldVert &A = scaffoldVerts[exit];
      const ScaffoldVert &B = scaffoldVerts[exit + 1 == n ? 0 : exit + 1];
      const float t = A.s / (A.s - B.s);
      const uint32_t newVert = CUT_VERT_FLAG | uint32_t(cutVerts.size());
      cutVerts.push_back(uvToVert(v_lerp_vec4f(v_splats(t), scaffoldToUv(A, border), scaffoldToUv(B, border))));
      if (DBG_DRAW && dbg)
        dbgPoint(cutVerts.back().pos, E3DCOLOR(255, 32, 128));
      state.prevCutVert = newVert;
      CellStepByBorder step = fg.stepByBorder[border];
      state.cellX += step.x;
      state.cellY += step.y;
      state.cellIdx += step.i;
      state.e = -step.opposite - 1;
      if (FRX_CHECK_FAILURE(state.cellX < fg.ofsSz.x || state.cellX > fg.ofsSz.x + fg.ofsSz.z - 2 || state.cellY < fg.ofsSz.y ||
                            state.cellY > fg.ofsSz.y + fg.ofsSz.w - 2))
      {
        state.poisoned = true; // out of bounds
        return true;
      }
      return false; // continue chain
    }
    else
    {
      state.e = scaffoldVerts[exit].e;
      state.idx = scaffoldVerts[exit].idx;
      const EdgeData &exitEdge = fg.edge[state.e];
      const uint32_t root = state.idx == 0 ? (exitEdge.firstRootBeforeFirstPt ? uint32_t(exitEdge.firstRootIdx) : NO_ROOT)
                                           : uint32_t(edgePts[exitEdge.offset + state.idx - 1].rootIdx);
      state.poisoned |= FRX_CHECK_FAILURE(root == NO_ROOT); // no root at edge segment we've arrived to
      state.prevCutVert = root;
      return true; // end chain
    }
  };

  struct ChainVert
  {
    uint32_t v;
    Point2 uv;
  };
  dag::Vector<ChainVert, framemem_allocator> chainVerts;
  struct Chain
  {
    uint32_t vBegin, vEnd;
    uint32_t lBegin, lEnd;
  };
  dag::Vector<Chain, framemem_allocator> chainsStack;

  // earcut and emit polygon
  dag::Vector<int, framemem_allocator> ecNxt, ecPrv;
  const auto emitRegion = [&](int from, int to, bool up, uint16_t mat, bool face_uv_ccw) DAGOR_NOINLINE_LAMBDA {
    const int n = to - from;
    if (n < 3)
      return;
    const ChainVert *__restrict rv = chainVerts.data() + from;
    const auto cr = [](Point2 a, Point2 b, Point2 c) { return (b.x - a.x) * (c.y - b.y) - (b.y - a.y) * (c.x - b.x); };
    const auto inside = [&](Point2 a, Point2 b, Point2 c, Point2 p) {
      return cr(a, b, p) >= 0.f && cr(b, c, p) >= 0.f && cr(c, a, p) >= 0.f;
    };
    const auto mapVert = [&](uint32_t tag, bool up)
                           DAGOR_FORCE_INLINE_LAMBDA { return (tag & CUT_VERT_FLAG) ? tag : reindexVert(up, tag); };
    const auto emitTri = [&](int a, int b, int c) {
      // match each triangle's UV winding to the face's (ear-clip output can be reversed on non-convex regions)
      const Point2 pa = rv[a].uv, pb = rv[b].uv, pc = rv[c].uv;
      const bool triCcw = (pb.x - pa.x) * (pc.y - pa.y) - (pb.y - pa.y) * (pc.x - pa.x) > 0.f;
      const int j = triCcw == face_uv_ccw ? b : c, k = triCcw == face_uv_ccw ? c : b;
      DestrMesh::Face &f = (up ? up_mesh : down_mesh).faces.push_back_noinit();
      f.idx[0] = mapVert(rv[a].v, up);
      f.idx[1] = mapVert(rv[j].v, up);
      f.idx[2] = mapVert(rv[k].v, up);
      f.mat = mat;
    };
    float area2 = 0.f;
    for (int i = 0, j = n - 1; i < n; j = i++)
      area2 += rv[j].uv.x * rv[i].uv.y - rv[i].uv.x * rv[j].uv.y;
    ecNxt.clear();
    ecNxt.resize(n);
    ecPrv.clear();
    ecPrv.resize(n);
    for (int i = 0; i < n; i++)
    {
      ecNxt[i] = area2 >= 0.f ? (i + 1) % n : (i - 1 + n) % n;
      ecPrv[i] = area2 >= 0.f ? (i - 1 + n) % n : (i + 1) % n;
    }
    int alive = n, cur = 0, stuck = 0;
    bool fwd = true;
    while (alive > 3 && stuck < alive * 2)
    {
      const int p = ecPrv[cur], nx = ecNxt[cur];
      const Point2 a = rv[p].uv, b = rv[cur].uv, c = rv[nx].uv;
      bool ear = cr(a, b, c) > 0.f;
      if (ear)
        for (int k = ecNxt[nx]; k != p; k = ecNxt[k])
          if (inside(a, b, c, rv[k].uv))
          {
            ear = false;
            break;
          }
      if (ear)
      {
        emitTri(p, cur, nx);
        ecNxt[p] = nx;
        ecPrv[nx] = p;
        alive--;
        stuck = 0;
        fwd = !fwd;
        cur = fwd ? nx : p;
      }
      else
      {
        cur = fwd ? nx : p;
        stuck++;
      }
    }
    if (alive == 3)
      emitTri(ecPrv[cur], cur, ecNxt[cur]);
  };

  const auto traceChains = [&](bool face_uv_ccw, bool solid, uint16_t mat) DAGOR_NOINLINE_LAMBDA {
    chainsStack.clear();
    chainVerts.clear();
    const bool baseSign = vertDist[fg.vertIdx[0]] < 0.f;
    bool curSign = baseSign;
    FaceEdgeRoot *roots = fg.edgeRoots.data();
    FaceEdgeRoot *rootsEnd = fg.edgeRoots.end();
    int e = 0;
    while (true)
    {
      for (; (roots == rootsEnd || e <= roots->e) && e < 3; e++)
        chainVerts.push_back(ChainVert{.v = fg.vertIdx[e], .uv = fg.cornerUv[e]});
      if (roots == rootsEnd)
        break;
      if (!chainsStack.empty() && chainsStack.back().lEnd == roots->lIdx)
      {
        Chain chain = chainsStack.back();
        eastl::reverse(chainVerts.begin() + chain.vEnd, chainVerts.end());
        emitRegion(chain.vBegin, int(chainVerts.size()), /*up*/ !curSign, mat, face_uv_ccw);
        chainVerts.resize(chain.vEnd);
        chainsStack.pop_back();
      }
      else
      {
        const auto rootUv = [&](uint32_t id)
                              DAGOR_FORCE_INLINE_LAMBDA { return projectUV(v_ldu(&cutVerts[id & ~CUT_VERT_FLAG].pos.x)); };
        // trace new chain
        ChainWalkState state;
        state.cellX = roots->cellX;
        state.cellY = roots->cellY;
        state.cellIdx = roots->cellIdx;
        state.prevCutVert = roots->rootIdx; // push to chain
        Chain chain = Chain{.vBegin = chainVerts.size(), .lBegin = roots->lIdx};
        chainVerts.push_back({.v = state.prevCutVert, .uv = rootUv(state.prevCutVert)});
        state.e = roots->e;
        state.idx = roots->idx;
        int guard = 0, maxIterGuard = fg.cells.size() * 2 + 8;
        for (; guard < maxIterGuard; guard++)
        {
          const bool end = buildAndTraverseCell(state, face_uv_ccw, solid);
          if (state.poisoned)
            goto bail;
          chainVerts.push_back({.v = state.prevCutVert, .uv = rootUv(state.prevCutVert)});
          if (solid)
          {
            auto &cutEdge = cut_face_data.edges.push_back_noinit();
            const int cutI = chainVerts.size() - 2;
            const bool cw = !curSign;
            cutEdge.a = (cw ? chainVerts[cutI + 1] : chainVerts[cutI]).v & ~CUT_VERT_FLAG;
            cutEdge.b = (cw ? chainVerts[cutI] : chainVerts[cutI + 1]).v & ~CUT_VERT_FLAG;
            if (DBG_DRAW)
              dbgLine(cutVerts[cutEdge.a].pos, cutVerts[cutEdge.b].pos, E3DCOLOR(0, 128, 255));
          }
          if (end)
            break;
        }
        if (FRX_CHECK_FAILURE(guard >= maxIterGuard))
          goto bail;
        G_ASSERT(state.e >= 0);
        chain.lEnd = (state.e << 16u) | state.idx;
        chain.vEnd = chainVerts.size();
        chainsStack.push_back(chain);
      }
      roots++;
      curSign = !curSign;
    }
    if (FRX_CHECK_FAILURE(!chainsStack.empty()))
      goto bail;
    if (FRX_CHECK_FAILURE(curSign != baseSign))
      goto bail;
    emitRegion(0, int(chainVerts.size()), /*up*/ !baseSign, mat, face_uv_ccw);
    return true;
  bail:
    // algorithm failed
    return false;
  };

  // main face cut loop

  vec4f maxEdgeLenSqV = v_splats(maxSeg * maxSeg);
  for (const auto face : candidateFacesVec)
  {
    if (DBG_DRAW)
      dbgFaceSolid = ctx.materials[face.mat].isSolid;
    const uint32_t i0 = face.idx.data()[0], i1 = face.idx.data()[1], i2 = face.idx.data()[2];
    const vec4f uv0 = projectUVv(v_ld(&verts[i0].pos.x));
    const vec4f uv1 = projectUVv(v_ld(&verts[i1].pos.x));
    const vec4f uv2 = projectUVv(v_ld(&verts[i2].pos.x));
    vec4f len01_12sq = v_sub(v_perm_xyab(uv0, uv1), v_perm_xyab(uv1, uv2));
    len01_12sq = v_mul(len01_12sq, len01_12sq);
    len01_12sq = v_add(len01_12sq, v_perm_yxwz(len01_12sq));
    vec4f len20sq = v_sub(uv2, uv0);
    len20sq = v_mul(len20sq, len20sq);
    len20sq = v_add(len20sq, v_perm_yxwz(len20sq));
    vec4f edgeLenSq = v_perm_xzac(len01_12sq, len20sq);
    const unsigned skinnyEdgeMask = v_signmask(v_cmp_le(edgeLenSq, maxEdgeLenSqV));
    if ((skinnyEdgeMask & 7) == 7)
    {
      doFaceSplit(face);
      continue;
    }
    fg.edge[0] = prepareEdgeFull(i0, i1, skinnyEdgeMask & 1);
    fg.edge[1] = prepareEdgeFull(i1, i2, skinnyEdgeMask & 2);
    fg.edge[2] = prepareEdgeFull(i2, i0, skinnyEdgeMask & 4);
    if (fg.edge[0].firstRootIdx == NO_ROOT && fg.edge[1].firstRootIdx == NO_ROOT && fg.edge[2].firstRootIdx == NO_ROOT)
    {
      emitWhole(face, vertDist[i0] > 0.f);
      continue;
    }
    const bool solid = ctx.materials[face.mat].isSolid;
    // check for fast path - only 2 roots that are close enough - perform trivial cut even on large triangle
    if (DAGOR_LIKELY(fg.edge[0].exactlyOneRoot + fg.edge[1].exactlyOneRoot + fg.edge[2].exactlyOneRoot == 2))
    {
      uint32_t idx0, idx1, idx2, cutIdx0, cutIdx1, rootlessRoot;
      if (!fg.edge[0].exactlyOneRoot) // e0 rootless: lone = i2, rotate left
      {
        idx0 = i1;
        idx1 = i2;
        idx2 = i0;
        cutIdx0 = fg.edge[1].firstRootIdx;
        cutIdx1 = fg.edge[2].firstRootIdx;
        rootlessRoot = fg.edge[0].firstRootIdx;
      }
      else if (!fg.edge[2].exactlyOneRoot) // e2 rootless: lone = i1, no rotation
      {
        idx0 = i0, idx1 = i1, idx2 = i2;
        cutIdx0 = fg.edge[0].firstRootIdx;
        cutIdx1 = fg.edge[1].firstRootIdx;
        rootlessRoot = fg.edge[2].firstRootIdx;
      }
      else // e1 rootless: lone = i0, rotate right
      {
        idx0 = i2;
        idx1 = i0;
        idx2 = i1;
        cutIdx0 = fg.edge[2].firstRootIdx;
        cutIdx1 = fg.edge[0].firstRootIdx;
        rootlessRoot = fg.edge[1].firstRootIdx;
      }
      if (DAGOR_LIKELY((rootlessRoot == NO_ROOT) &
                       v_test_vec_x_lt(v_length2_sq_x(v_sub(projectUVv(v_ldu(&cutVerts[cutIdx0 & ~CUT_VERT_FLAG].pos.x)),
                                         projectUVv(v_ldu(&cutVerts[cutIdx1 & ~CUT_VERT_FLAG].pos.x)))),
                         maxEdgeLenSqV)))
      {
        trivialTriangleSplit(idx0, idx1, idx2, cutIdx0, cutIdx1, vertDist[idx0] >= 0.f, face.mat, solid);

        if (DBG_DRAW && solid)
          dbgLine(cutVerts[cutIdx0 & ~CUT_VERT_FLAG].pos, cutVerts[cutIdx1 & ~CUT_VERT_FLAG].pos, E3DCOLOR(0, 255, 128));
        continue;
      }
    }
    fg.vertIdx[0] = i0, fg.vertIdx[1] = i1, fg.vertIdx[2] = i2;
    fg.cornerUv[0] = Point2(v_extract_x(uv0), v_extract_y(uv0));
    fg.cornerUv[1] = Point2(v_extract_x(uv1), v_extract_y(uv1));
    fg.cornerUv[2] = Point2(v_extract_x(uv2), v_extract_y(uv2));
    buildCellArray();
    const vec4f windingCheck = v_mul(v_sub(uv1, uv0), v_perm_yxwz(v_sub(uv2, uv0)));
    const bool faceUvCcw = v_extract_x(windingCheck) > v_extract_y(windingCheck);
    const uint32_t upFacesBefore = up_mesh.faces.size(), downFacesBefore = down_mesh.faces.size(),
                   edgeCntBefore = cut_face_data.edges.size();
    if (DAGOR_UNLIKELY(!traceChains(faceUvCcw, solid, face.mat)))
    {
      // trace chains algorithm failed and bailed - emit whole face
      up_mesh.faces.resize(upFacesBefore);
      down_mesh.faces.resize(downFacesBefore);
      cut_face_data.edges.resize(edgeCntBefore);
      emitWhole(face, int(vertDist[i0] > 0.f) + int(vertDist[i1] > 0.f) + int(vertDist[i2] > 0.f) >= 2);
    }
  }

  // flush cut vertices, project them on the plane
  {
    mat33f basis;
    basis.col0 = v_ldu(&cut_face_data.basis.U.x);
    basis.col1 = v_ldu(&cut_face_data.basis.V.x);
    basis.col2 = v_ldu(&cut_face_data.basis.plane.n.x);

    mat33f invBasis;
    v_mat33_orthonormal_inverse(invBasis, basis);
    const DestrMesh::Vertex *__restrict cutVertsPtr = cutVerts.data();
    cut_face_data.verts.reserve(cut_face_data.verts.size() + cutVerts.size());
    for (int i = 0, ie = cutVerts.size(); i != ie; i++)
    {
      const vec4f wp = v_ld(&cutVertsPtr[i].pos.x);
      vec4f v = v_mat33_mul_vec3(invBasis, wp);
      const float h = v_extract_x(v_plane_dist_x(cut_plane, wp));
      v = v_perm_xycw(v, v_splats(h));
      v_st(&cut_face_data.verts.push_back_noinit(), v_perm_xyzd(v, v_zero()));
    }
  }

  const auto finalizeCutFaces = [&](DestrMesh &dst_mesh, uint32_t cut_faces_start) DAGOR_FORCE_INLINE_LAMBDA {
    DestrMesh::Face *__restrict faces = dst_mesh.faces.data();
    const uint32_t facesEnd = dst_mesh.faces.size();
    const uint32_t cutOfs = dst_mesh.verts.size();
    for (uint32_t i = cut_faces_start; i != facesEnd; i++)
      for (uint32_t &idx : faces[i].idx)
        if (idx & CUT_VERT_FLAG)
          idx = (idx & ~CUT_VERT_FLAG) + cutOfs;
    dst_mesh.verts.resize_noinit(cutOfs + cutVerts.size());
    memcpy(dst_mesh.verts.data() + cutOfs, cutVerts.data(), sizeof(DestrMesh::Vertex) * cutVerts.size());
  };
  finalizeCutFaces(up_mesh, upMeshCutFacesStart);
  finalizeCutFaces(down_mesh, downMeshCutFacesStart);
}

} // namespace frx
