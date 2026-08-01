// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <cstdint>
#include <EASTL/bitvector.h>
#include <EASTL/numeric.h>
#include <EASTL/algorithm.h>
#include <math/dag_Point3.h>
#include <math/dag_e3dColor.h>
#include <math/dag_plane3.h>
#include <math/dag_bounds2.h>
#include <vecmath/dag_vecMath.h>

#include <util/dag_stlqsort.h>
#include <dag/dag_vector.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <generic/dag_relocatableFixedVector.h>
#include <memory/dag_framemem.h>

#include <daFracture/core/destrMesh.h>
#include <daFracture/core/cutFaceFill.h>
#include "cutMeshCommon.h"
#include "cutFaceGraph.h"
#include "cutFaceBoundarySearch.h"
#include "cutFaceTriangulatePlanar.h"


namespace frx
{

DAGOR_NOINLINE void fill_cut_faces(DestrContext &ctx, const CutFaceData &cut_data, DestrMesh *up_mesh, DestrMesh *down_mesh,
  uint16_t cut_face_mat)
{
  FRAMEMEM_REGION;

  PlaneBasis basis = cut_data.basis;
  CutFaceGraph graph;

  if (ctx.dbgDraw.drawCutFaceBasis)
  {
    ctx.dbgDraw.drawArrow(basis.unProject(Point2(0, 0)), basis.unProject(Point2(1, 0)), E3DCOLOR(255, 0, 0));
    ctx.dbgDraw.drawArrow(basis.unProject(Point2(0, 0)), basis.unProject(Point2(0, 1)), E3DCOLOR(0, 255, 0));
  }

  // drop topology info to verify algorithm is robust and produces same result
  const bool dbgForceSegmentSoup = false;
  if (dbgForceSegmentSoup)
  {
    for (const auto &e : cut_data.edges)
    {
      graph.edges.push_back({.v0 = int(graph.verts.size()), .v1 = int(graph.verts.size() + 1)});
      v_stu_half(&graph.verts.push_back_noinit(), cut_data.verts[e.a]);
      v_stu_half(&graph.verts.push_back_noinit(), cut_data.verts[e.b]);
    }
  }
  else
  {
    graph.edges.reserve(cut_data.edges.size());
    graph.verts.reserve(cut_data.verts.size());
    for (const auto &v : cut_data.verts)
      v_stu_half(&graph.verts.push_back_noinit(), v);
    for (const auto &e : cut_data.edges)
      graph.edges.push_back({.v0 = int(e.a), .v1 = int(e.b)});
  }

  if (ctx.dbgDraw.drawCutSegments)
  {
    for (const auto &e : graph.edges)
      ctx.dbgDraw.drawArrow(basis.unProject(graph.verts[e.v0]), basis.unProject(graph.verts[e.v1]), E3DCOLOR(255, 255, 128));
  }

  // preprocess segment soup & build graph
  prepare_planar_graph(ctx, basis, graph);
  if (false)
    verify_planar_graph(graph);

  // prepare edge angles
  graph.edgeAngle.resize(graph.edges.size());
  graph.edgeReverseAngle.resize(graph.edges.size());
  for (int i = 0, ie = int(graph.edges.size()); i < ie; i++)
  {
    Point2 d = graph.verts[graph.edges[i].v1] - graph.verts[graph.edges[i].v0];
    const float angle = atan2f(d.y, d.x);
    graph.edgeAngle[i] = angle;
    graph.edgeReverseAngle[i] = angle > 0.f ? angle - PI : angle + PI;
  }

  // search boundaries
  auto boundaries = boundary_loops_search(ctx, graph, basis);

  {
    // prepare vertices for triangulation
    struct VertPair
    {
      int up = -1, down = -1;
      bool valid() const { return up != -1; }
    };
    dag::Vector<VertPair, framemem_allocator> boundaryMeshVerts;
    boundaryMeshVerts.resize(graph.verts.size());
    for (const BoundaryLoop &boundary : boundaries)
    {
      for (int vi : boundary.verts)
      {
        VertPair &vPair = boundaryMeshVerts[vi];
        if (vPair.valid())
          continue;

        DestrMesh::Vertex vd;
        vd.pos = basis.unProject(graph.verts[vi]);
        vd.tc = graph.verts[vi];

        vPair.up = up_mesh ? up_mesh->verts.size() : 0; // validate
        if (up_mesh)
        {
          vd.norm = -basis.plane.n;
          up_mesh->verts.push_back(vd);
        }
        vPair.down = down_mesh ? down_mesh->verts.size() : 0; // validate
        if (down_mesh)
        {
          vd.norm = basis.plane.n;
          down_mesh->verts.push_back(vd);
        }
      }
    }

    // triangulate each outer boundary and its inner holes
    for (int i = 0; i < boundaries.size(); i++)
    {
      if (boundaries[i].parentId != -1)
        continue;
      if (boundaries[i].area2 < 1e-6f)
        continue;
      earcut_boundary_with_holes(ctx, graph, basis, boundaries, i, [&](int va, int vb, int vc) {
        if (DAGOR_UNLIKELY(ctx.dbgDraw.drawBoundaryTriangles))
        {
          Point3 a = basis.unProject(graph.verts[va]);
          Point3 b = basis.unProject(graph.verts[vb]);
          Point3 c = basis.unProject(graph.verts[vc]);
          ctx.dbgDraw.drawLine(a, b, E3DCOLOR(0, 255, 255));
          ctx.dbgDraw.drawLine(b, c, E3DCOLOR(0, 255, 255));
          ctx.dbgDraw.drawLine(c, a, E3DCOLOR(0, 255, 255));
        }
        const VertPair &a = boundaryMeshVerts[va];
        const VertPair &b = boundaryMeshVerts[vb];
        const VertPair &c = boundaryMeshVerts[vc];
        G_ASSERT(a.valid() && b.valid() && c.valid());
        if (up_mesh)
          up_mesh->faces.push_back(DestrMesh::Face{.idx = {uint32_t(a.up), uint32_t(b.up), uint32_t(c.up)}, .mat = cut_face_mat});
        if (down_mesh)
          down_mesh->faces.push_back(
            DestrMesh::Face{.idx = {uint32_t(a.down), uint32_t(c.down), uint32_t(b.down)}, .mat = cut_face_mat});
      });
    }
  }
}

} // namespace frx
