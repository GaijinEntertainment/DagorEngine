// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/bitvector.h>
#include <EASTL/numeric.h>
#include <generic/dag_span.h>
#include "cutFaceGraph.h"


namespace frx
{

// allows quick edge/vertex removal and traversal, but not adding new ones
struct CutFaceSubGraph
{
  eastl::bitvector<framemem_allocator> activeEdges;

  struct VertConSpan
  {
    // span into connections vector
    // incoming connections [start; start + incomingCnt)
    // outgoing connections [start + incomingCnt; start + incomingCnt + outgoingCnt)
    int start = 0;
    int16_t incomingCnt = 0, outgoingCnt = 0;
  };
  dag::Vector<VertConSpan, framemem_allocator> conSpan; // parallel to CutFaceGraph::verts
  struct Connection                                     // stores indices in CutFaceGraph
  {
    int edge;
    int vert;
  };
  dag::Vector<Connection, framemem_allocator> connections;

  struct Scc
  {
    int vertsStart = 0, vertsCnt = 0;
    bool empty() const { return vertsCnt == 0; }
  };
  dag::Vector<Scc, framemem_allocator> sccs;
  dag::Vector<int, framemem_allocator> sccVerts;
  struct TarjanState
  {
    int idx = -1;
    int low = 0;
    int sccId : 31 = -1;
    bool onStack : 1 = false;
  };
  dag::Vector<TarjanState, framemem_allocator> tarjanState;

  dag::ConstSpan<Connection> outgoing(int v) const
  {
    const auto &s = conSpan[v];
    return dag::ConstSpan<Connection>(connections.data() + s.start + s.incomingCnt, s.outgoingCnt);
  }

  dag::ConstSpan<Connection> incoming(int v) const
  {
    const auto &s = conSpan[v];
    return dag::ConstSpan<Connection>(connections.data() + s.start, s.incomingCnt);
  }

  dag::ConstSpan<Connection> allEdges(int v) const
  {
    const auto &s = conSpan[v];
    return dag::ConstSpan<Connection>(connections.data() + s.start, s.outgoingCnt + s.incomingCnt);
  }

  dag::Span<int> sccVertices(int scc_id)
  {
    auto &scc = sccs[scc_id];
    return dag::Span<int>(sccVerts.data() + scc.vertsStart, scc.vertsCnt);
  }

  bool removeEdge(int v0, int v1)
  {
    // outgoing
    bool removed = false;
    {
      VertConSpan &span = conSpan[v0];
      for (int i = span.start + span.incomingCnt, ie = i + span.outgoingCnt; i < ie; i++)
        if (connections[i].vert == v1)
        {
          activeEdges.set(connections[i].edge, false);
          connections[i] = connections[ie - 1];
          span.outgoingCnt--;
          removed = true;
          break;
        }
    }
    // incoming
    {
      VertConSpan &span = conSpan[v1];
      for (int i = span.start, ie = i + span.incomingCnt; i < ie; i++)
        if (connections[i].vert == v0)
        {
          activeEdges.set(connections[i].edge, false);
          connections[i] = connections[ie - 1];
          connections[ie - 1] = connections[span.start + span.incomingCnt + span.outgoingCnt - 1];
          span.incomingCnt--;
          break;
        }
    }
    return removed;
  }

  int findEdge(int v0, int v1) const
  {
    const VertConSpan &span = conSpan[v0];
    for (int i = span.start + span.incomingCnt, ie = i + span.outgoingCnt; i < ie; i++)
      if (connections[i].vert == v1)
        return connections[i].edge;
    return -1;
  }

  template <typename Fn>
  void removeEdgesIf(int v, Fn &&fn)
  {
    VertConSpan &span = conSpan[v];
    int newIncomingEnd = span.start;
    for (int i = span.start, ie = i + span.incomingCnt; i < ie; i++)
      if (!fn(connections[i], /* outgoing */ false))
        connections[newIncomingEnd++] = connections[i];
      else
        activeEdges.set(connections[i].edge, false);
    int newOutgoungEnd = newIncomingEnd;
    for (int i = span.start + span.incomingCnt, ie = i + span.outgoingCnt; i < ie; i++)
      if (!fn(connections[i], /* outgoing */ true))
        connections[newOutgoungEnd++] = connections[i];
      else
        activeEdges.set(connections[i].edge, false);
    span.incomingCnt = newIncomingEnd - span.start;
    span.outgoingCnt = newOutgoungEnd - newIncomingEnd;
  }
};

struct BoundaryLoop
{
  dag::Vector<int, framemem_allocator> verts;
  Point2 midpoint;
  float area2 = 0.f;
  int parentId = -1;
  int remapToId = -1;
};

dag::Vector<BoundaryLoop, framemem_allocator> boundary_loops_search(const DestrContext &ctx, const CutFaceGraph &graph,
  const PlaneBasis &basis);

} // namespace frx