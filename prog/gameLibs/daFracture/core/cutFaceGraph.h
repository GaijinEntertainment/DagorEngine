// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <memory/dag_framemem.h>
#include <dag/dag_vector.h>
#include <math/dag_Point2.h>
#include <math/dag_e3dColor.h>
#include <daFracture/core/destrMesh.h>
#include <daFracture/core/cutFaceFill.h>


namespace frx
{

struct CutFaceGraph
{
  dag::Vector<Point2, framemem_allocator> verts;
  dag::Vector<float, framemem_allocator> vertsH;

  struct Edge
  {
    int v0, v1;
    __forceinline uint64_t sortKey() const
    {
      G_STATIC_ASSERT(sizeof(Edge) == sizeof(uint64_t));
      return (const uint64_t &)*this;
    }
  };
  dag::Vector<Edge, framemem_allocator> edges;
  dag::Vector<float, framemem_allocator> edgeAngle;
  dag::Vector<float, framemem_allocator> edgeReverseAngle;
};


void prepare_planar_graph(DestrContext &ctx, const PlaneBasis &basis, CutFaceGraph &graph);
void verify_planar_graph(const CutFaceGraph &graph);


inline void dbg_draw_point(const DestrContext &ctx, const CutFaceGraph &graph, const PlaneBasis &basis, int v0, E3DCOLOR col)
{
  ctx.dbgDraw.drawPoint(basis.unProject(graph.verts[v0], ctx.dbgDraw.heightScale * graph.vertsH[v0]), col);
}

inline void dbg_draw_line(const DestrContext &ctx, const CutFaceGraph &graph, const PlaneBasis &basis, int v0, int v1, E3DCOLOR col)
{
  ctx.dbgDraw.drawLine(basis.unProject(graph.verts[v0], ctx.dbgDraw.heightScale * graph.vertsH[v0]),
    basis.unProject(graph.verts[v1], ctx.dbgDraw.heightScale * graph.vertsH[v1]), col);
}

inline void dbg_draw_arrow(const DestrContext &ctx, const CutFaceGraph &graph, const PlaneBasis &basis, int v0, int v1, E3DCOLOR col)
{
  ctx.dbgDraw.drawArrow(basis.unProject(graph.verts[v0], ctx.dbgDraw.heightScale * graph.vertsH[v0]),
    basis.unProject(graph.verts[v1], ctx.dbgDraw.heightScale * graph.vertsH[v1]), col);
}

} // namespace frx