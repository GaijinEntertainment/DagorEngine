// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_stlqsort.h>
#include <generic/dag_carray.h>
#include <daFracture/core/destrMesh.h>
#include "cutFaceGraph.h"
#include "cutMeshCommon.h"
#include "cutFaceBoundarySearch.h"


namespace frx
{

DAGOR_NOINLINE static void init_subgraph(CutFaceSubGraph &sg, const CutFaceGraph &graph)
{
  // init sccs
  sg.tarjanState.resize(graph.verts.size());
  sg.sccVerts.resize(graph.verts.size());
  sg.sccs.push_back({.vertsStart = 0, .vertsCnt = int(graph.verts.size())});
  eastl::iota(sg.sccVerts.begin(), sg.sccVerts.end(), 0);

  // init edges
  sg.activeEdges.resize(graph.edges.size(), true);
  sg.conSpan.resize(graph.verts.size());
  sg.connections.clear();
  sg.connections.resize(graph.edges.size() * 2);

  for (int ei = 0; ei < graph.edges.size(); ei++)
  {
    const auto &edge = graph.edges[ei];
    sg.conSpan[edge.v0].outgoingCnt++;
    sg.conSpan[edge.v1].incomingCnt++;
  }

  int curStart = 0;
  for (int i = 0; i < sg.conSpan.size(); i++)
  {
    auto &span = sg.conSpan[i];
    span.start = curStart;
    curStart += span.outgoingCnt + span.incomingCnt;
    span.outgoingCnt = span.incomingCnt;
    span.incomingCnt = 0;
  }

  for (int ei = 0; ei < graph.edges.size(); ei++)
  {
    const auto &edge = graph.edges[ei];
    auto &span0 = sg.conSpan[edge.v0];
    auto &span1 = sg.conSpan[edge.v1];
    sg.connections[span0.start + span0.outgoingCnt++] = {.edge = ei, .vert = edge.v1};
    sg.connections[span1.start + span1.incomingCnt++] = {.edge = ei, .vert = edge.v0};
  }

  for (auto &span : sg.conSpan)
    span.outgoingCnt -= span.incomingCnt;
}

// for given SCC id:
// - split it into one or more SCCs
// - remove all edges outside of SCCs
// - return begin, end indices of new SCCs range
DAGOR_NOINLINE static eastl::pair<int, int> split_sccs(const CutFaceGraph &, CutFaceSubGraph &sg, int scc_id)
{
  auto &state = sg.tarjanState;
  dag::Span<int> localVerts = sg.sccVertices(scc_id);
  for (int v : localVerts)
    state[v] = {};

  dag::Vector<int, framemem_allocator> stk;
  int timer = 0, numScc = 0;
  struct TarjanFrame
  {
    int v, ei;
  };
  dag::Vector<TarjanFrame, framemem_allocator> callStack;

  for (int root : localVerts)
  {
    if (state[root].idx >= 0 || sg.outgoing(root).empty())
      continue;
    state[root].idx = state[root].low = timer++;
    stk.push_back(root);
    state[root].onStack = true;
    callStack.push_back({root, 0});

    while (!callStack.empty())
    {
      auto &[cv, ei] = callStack.back();
      const auto cvOutgoing = sg.outgoing(cv);
      if (ei < int(cvOutgoing.size()))
      {
        int w = cvOutgoing[ei++].vert;
        if (state[w].idx < 0)
        {
          state[w].idx = state[w].low = timer++;
          stk.push_back(w);
          state[w].onStack = true;
          callStack.push_back({w, 0});
        }
        else if (state[w].onStack)
          state[cv].low = eastl::min(state[cv].low, state[w].low);
      }
      else
      {
        int doneV = cv;
        int doneLow = state[cv].low;
        callStack.pop_back();
        if (!callStack.empty())
          state[callStack.back().v].low = eastl::min(state[callStack.back().v].low, doneLow);
        if (doneLow == state[doneV].idx)
        {
          int scc = numScc++;
          int cnt = 0;
          int w;
          do
          {
            w = stk.back();
            stk.pop_back();
            state[w].onStack = false;
            state[w].sccId = scc;
            cnt++;
          } while (w != doneV);
          if (cnt == 1)
            state[doneV].sccId = -1;
        }
      }
    }
  }

  // partition by scc and disconnect edges
  stlsort::sort_branchless(localVerts.begin(), localVerts.end(), [&](int v1, int v2) { return state[v1].sccId < state[v2].sccId; });
  int prevSccId = -1;
  int newSccsOffset = sg.sccs.size();
  for (int i = sg.sccs[scc_id].vertsStart, ie = i + sg.sccs[scc_id].vertsCnt; i < ie; i++)
  {
    const int v = sg.sccVerts[i];
    const int curSccId = state[v].sccId;
    if (curSccId == -1)
      sg.removeEdgesIf(v, [&](const auto &, bool) { return true; });
    else
      sg.removeEdgesIf(v, [&](const auto &conn, bool) { return curSccId != state[conn.vert].sccId; });

    if (prevSccId != curSccId)
      sg.sccs.push_back().vertsStart = i;
    if (curSccId != -1)
      sg.sccs.back().vertsCnt++;
    prevSccId = curSccId;
  }

  sg.sccs[scc_id] = {}; // mark current one as empty
  return {newSccsOffset, int(sg.sccs.size())};
}

template <bool OuterBoundary>
DAGOR_NOINLINE static dag::Vector<int> find_boundary(const CutFaceGraph &graph, CutFaceSubGraph &sg, const PlaneBasis &, int scc_id)
{
  const auto sccVerts = sg.sccVertices(scc_id);
  if (sccVerts.empty())
    return {};

  int startV = sccVerts.front();
  for (int v : sccVerts)
    if (graph.verts[v].x < graph.verts[startV].x)
      startV = v;

  const auto getEdgeAngle = [&](const CutFaceSubGraph::Connection &conn, bool reverse) {
    return ((graph.edges[conn.edge].v1 == conn.vert) != reverse ? graph.edgeAngle : graph.edgeReverseAngle)[conn.edge];
  };

  // Pick most-downward edge from startV (min angle), considering both edge directions.
  CutFaceSubGraph::Connection startConn = {-1, -1};
  float startAngle = FLT_MAX;
  for (const auto &conn : sg.allEdges(startV))
  {
    const float a = getEdgeAngle(conn, /*reverse*/ false);
    if (a < startAngle)
    {
      startAngle = a;
      startConn = conn;
    }
  }
  G_ASSERT(startConn.vert >= 0);

  dag::Vector<int> boundary;
  boundary.push_back(startConn.vert);
  CutFaceSubGraph::Connection cur = startConn;

  // Walk using most-left-turn.
  for (int guard = 0; guard <= int(graph.edges.size()) + 1; guard++)
  {
    if (cur.vert == startV)
      break;

    float reverseAngle = getEdgeAngle(cur, /*reverse*/ true);
    CutFaceSubGraph::Connection bestNext = {-1, -1};
    float bestDa = OuterBoundary ? FLT_MAX : FLT_MIN;
    for (const auto &next : sg.allEdges(cur.vert))
    {
      float a = getEdgeAngle(next, /*reverse*/ false);
      float da = a - reverseAngle;
      if (da < 0.f)
        da += TWOPI;
      if constexpr (OuterBoundary)
      {
        if (da < 1e-4f)
          da = TWOPI - da;
        if (da < bestDa)
        {
          bestDa = da;
          bestNext = next;
        }
      }
      else
      {
        if (da > TWOPI - 1e-4f)
          da = TWOPI - da;
        if (da > bestDa)
        {
          bestDa = da;
          bestNext = next;
        }
      }
    }
    if (bestNext.vert < 0)
      break;
    boundary.push_back(bestNext.vert);
    cur = bestNext;
  }

  if (boundary.size() < 3 || cur.vert != startV)
    return {};

  return boundary;
}

DAGOR_NOINLINE dag::Vector<BoundaryLoop, framemem_allocator> boundary_loops_search(const DestrContext &ctx, const CutFaceGraph &graph,
  const PlaneBasis &basis)
{
  enum class SearchType : uint8_t
  {
    Inner,
    Outer,
    Both
  };
  struct WorkQueue
  {
    dag::Vector<int, framemem_allocator> sccsToProcess;
    CutFaceSubGraph subGraph;
  };
  // 0 - outer bound
  // 1 - inner bound
  carray<WorkQueue, 2> queues;
  CutFaceSubGraph dbgInitialSubGraph;

  // init only outer bounds search first
  {
    CutFaceSubGraph &sg = queues[0].subGraph;
    init_subgraph(sg, graph);
    const auto [sccsBegin, sccsEnd] = split_sccs(graph, sg, 0);
    queues[0].sccsToProcess.reserve(sccsEnd - sccsBegin);
    for (int nextScc = sccsBegin; nextScc < sccsEnd; nextScc++)
      if (sg.sccVertices(nextScc).size() >= 3)
        queues[0].sccsToProcess.push_back(nextScc);
    if (ctx.dbgDraw.drawCutEdgeGraph)
      dbgInitialSubGraph = sg;
  }
  bool complexSearchMode = false;
  // FIXME: The idea here is that we have mostly outer loops and we don't want to always run algorithm twice
  // to find both inner and outer loops. The difficulty here is determining where to switch to complexSearchMode
  // and not loose any boundaries
  if (true)
  {
    complexSearchMode = true;
    queues[1] = queues[0];
  }
  dag::Vector<BoundaryLoop, framemem_allocator> boundaries;

  while (!queues[0].sccsToProcess.empty() || !queues[1].sccsToProcess.empty())
  {
    int queueId = queues[0].sccsToProcess.empty() ? 1 : 0;
    auto &sccsToProcess = queues[queueId].sccsToProcess;
    CutFaceSubGraph &sg = queues[queueId].subGraph;
    const int sccId = sccsToProcess.back();
    const bool isOuterBoundary = queueId == 0;
    sccsToProcess.pop_back();
    auto boundary = find_boundary<true>(graph, sg, basis, sccId);
    // FIXME: isOuterBoundary ? find_boundary<true>(graph, sg, basis, sccId) : find_boundary<false>(graph, sg, basis, sccId);
    if (boundary.empty()) // failed to find any boundary - discard SCC
      continue;

    float area2 = 0.f;
    for (int k = 0; k < int(boundary.size()); k++)
    {
      Point2 a = graph.verts[boundary[k]];
      Point2 b = graph.verts[boundary[(k + 1) % int(boundary.size())]];
      area2 += a.x * b.y - b.x * a.y;
    }
    if ((area2 < 0.f) == isOuterBoundary)
    {
      eastl::reverse(boundary.begin(), boundary.end());
      area2 = -area2;
    }

    bool isComplete = true;
    for (int k = 0; k < int(boundary.size()); k++)
    {
      int v0 = boundary[k];
      int v1 = boundary[(k + 1) % int(boundary.size())];
      if (DAGOR_UNLIKELY(sg.findEdge(v0, v1) < 0))
      {
        isComplete = false;
        VERIFY_ALGORITHM(sg.removeEdge(v1, v0)); // there must be an opposite edge, that was traversed in an undirected graph
      }
    }

    // boundary found, split SCCs and continue
    if (isComplete)
    {
      for (int k = 0; k < int(boundary.size()); k++)
        sg.removeEdge(boundary[k], boundary[(k + 1) % int(boundary.size())]);

      auto &b = boundaries.push_back();
      b.verts = eastl::move(boundary);
      b.area2 = area2;
    }

    const auto [sccsBegin, sccsEnd] = split_sccs(graph, sg, sccId);
    for (int nextScc = sccsBegin; nextScc < sccsEnd; nextScc++)
      if (sg.sccVertices(nextScc).size() >= 3)
        sccsToProcess.push_back(nextScc);
  }

  for (auto &bound : boundaries)
  {
    bound.midpoint = Point2::ZERO;
    for (int v : bound.verts)
      bound.midpoint += graph.verts[v];
    bound.midpoint /= float(bound.verts.size());
  }

  // assign each boundary a direct parent (if it has one)
  for (int i = 0; i < boundaries.size(); i++)
  {
    auto &bound = boundaries[i];
    float closestX = FLT_MAX;
    for (int j = 0; j < int(boundaries.size()); j++)
    {
      if (i == j)
        continue;
      const auto &bound1 = boundaries[j];

      for (int n = 0; n < int(bound.verts.size()); n++)
      {
        Point2 pt = graph.verts[bound.verts[n]];
        bool valid = true;
        bool inside = false;
        float minX = FLT_MAX;
        for (int k = 0; k < int(bound1.verts.size()); k++)
        {
          if (DAGOR_UNLIKELY(bound1.verts[k] == bound.verts[n]))
          {
            valid = false;
            break;
          }
          Point2 p0 = graph.verts[bound1.verts[k]];
          Point2 p1 = graph.verts[bound1.verts[(k + 1) % int(bound1.verts.size())]];
          if ((p0.y <= pt.y && pt.y < p1.y) || (p1.y <= pt.y && pt.y < p0.y))
          {
            float t = (pt.y - p0.y) / (p1.y - p0.y);
            Point2 pCross = p0 + t * (p1 - p0);
            if (pt.x < pCross.x)
            {
              inside = !inside;
              inplace_min(minX, pCross.x);
            }
          }
        }
        if (valid && minX < closestX && inside)
        {
          // dbg_draw_arrow(basis.unProject(bound.midpoint), basis.unProject(bound1.midpoint), E3DCOLOR(255, 0, 255));
          closestX = minX;
          bound.parentId = j;
        }
        else if (valid)
          break;
      }
    }
  }

  // merge boundaries
  while (true)
  {
    constexpr int REMAP_TO_REMOVED = -2;
    bool anyMerged = false;

    for (auto &bound : boundaries)
    {
      int iterGuard = 0;
      int parentId = bound.parentId;
      while (parentId != -1)
      {
        parentId = boundaries[parentId].parentId;
        if (!VERIFY_ALGORITHM(iterGuard++ < int(boundaries.size())))
          bound.parentId = -1;
      }
    }
    for (auto &bound : boundaries)
    {
      if (bound.remapToId != -1) // skip root ones or merged/removed ones
        continue;
      if (bound.parentId == -1 && bound.area2 < 0.f) // remove inner bounds without parent
      {
        bound.remapToId = REMAP_TO_REMOVED;
        anyMerged = true;
        break;
      }
      if (bound.parentId == -1) // skip ones without parent
        continue;
      // remap parent
      int remappedParentId = bound.parentId;
      while (remappedParentId >= 0 && boundaries[remappedParentId].remapToId != -1)
        remappedParentId = boundaries[remappedParentId].remapToId;
      if (bound.parentId != remappedParentId)
      {
        bound.parentId = remappedParentId == REMAP_TO_REMOVED ? -1 : remappedParentId;
        anyMerged = true;
      }
      // remove boundaries where parent is same winding boundary
      if (bound.parentId != -1 && (bound.area2 > 0.f) == (boundaries[bound.parentId].area2 > 0.f))
      {
        bound.remapToId = bound.parentId;
        anyMerged = true;
        break;
      }
    }
    if (!anyMerged)
      break;
  }

  // filter out removed/merged loops
  {
    int newCnt = 0;
    for (auto &bound : boundaries)
    {
      if (bound.remapToId == -1)
        bound.remapToId = newCnt++;
      else
        bound.remapToId = -1;
    }
    for (auto &bound : boundaries)
      if (bound.parentId >= 0) // remap parent + drop negative area boundaries parents
        bound.parentId = boundaries[bound.parentId].area2 > 0.f ? boundaries[bound.parentId].remapToId : -1;
    for (auto &bound : boundaries)
    {
      if (bound.remapToId < 0)
        continue;
      if (&bound != &boundaries[bound.remapToId])
        boundaries[bound.remapToId] = eastl::move(bound);
    }
    boundaries.resize(newCnt);
  }

  if (ctx.dbgDraw.drawCutEdgeGraph)
  {
    dag::Vector<E3DCOLOR, framemem_allocator> edgeCol;
    edgeCol.resize(graph.edges.size());
    for (int i = 0, ie = int(graph.edges.size()); i < ie; i++)
      edgeCol[i] = dbgInitialSubGraph.activeEdges.test(i, false) ? E3DCOLOR(255, 255, 0) : E3DCOLOR(255, 0, 0);
    for (const auto &bound : boundaries)
    {
      const auto boundCopy = bound;
      const E3DCOLOR col = bound.parentId == -1 ? E3DCOLOR_MAKE(0, 255, 255, 255) // cyan:  outer (root) boundary
                                                : E3DCOLOR_MAKE(0, 255, 0, 255);  // green: inner (hole) boundary
      const int n = int(bound.verts.size());
      for (int k = 0; k < n; k++)
      {
        const int v0 = bound.verts[k], v1 = bound.verts[(k + 1) % n];
        for (int i = 0, ie = int(graph.edges.size()); i < ie; i++)
          if (graph.edges[i].v0 == v0 && graph.edges[i].v1 == v1)
          {
            edgeCol[i] = col;
            break;
          }
      }
    }
    for (int i = 0, ie = int(graph.edges.size()); i < ie; i++)
    {
      ctx.dbgDraw.drawArrow(basis.unProject(graph.verts[graph.edges[i].v0]), basis.unProject(graph.verts[graph.edges[i].v1]),
        edgeCol[i]);
    }
  }

  return boundaries;
}

} // namespace frx