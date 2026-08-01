// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <memory/dag_framemem.h>
#include <dag/dag_vector.h>
#include <math/dag_Point2.h>
#include <daFracture/core/cutFaceFill.h>


namespace frx
{

struct CutFaceGraph
{
  dag::Vector<Point2, framemem_allocator> verts;

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

} // namespace frx