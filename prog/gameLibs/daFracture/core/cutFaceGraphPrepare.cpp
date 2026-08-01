// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/numeric.h>
#include <util/dag_stlqsort.h>
#include <generic/dag_relocatableFixedVector.h>
#include <daFracture/core/cutFaceFill.h>

#include "cutFaceGraph.h"
#include "vertexGrid.h"
#include "edgeGrid.h"
#include "cutMeshCommon.h"


namespace frx
{

static __forceinline bool line_line_intersect(Point2 p0, Point2 p1, Point2 q0, Point2 q1, float &t, float &u, Point2 &out_point)
{
  const float ax = p1.x - p0.x, ay = p1.y - p0.y;
  const float bx = q1.x - q0.x, by = q1.y - q0.y;
  const float d = ax * by - ay * bx;
  constexpr float SIN_PARALLEL = 1e-3f;
  if (d < 1e-3f)
    if (d * d < (SIN_PARALLEL * SIN_PARALLEL) * (ax * ax + ay * ay) * (bx * bx + by * by)) // length independed collinearity check
      return false;
  float tn = (q0.x - p0.x) * by - (q0.y - p0.y) * bx;
  float un = (q0.x - p0.x) * ay - (q0.y - p0.y) * ax;
  t = tn / d;
  u = un / d;
  out_point = (lerp(p0, p1, t) + lerp(q0, q1, u)) * 0.5f;
  return true;
}

// edge grid verification via intersection checksum
DAGOR_NOINLINE void verify_edge_grid(const CutFaceGraph &graph)
{
  EdgeGrid eGrid;
  eGrid.build(graph.edges.size(), 4, 0.1f, [&](const int ei) {
    const Point2 v0 = graph.verts[graph.edges[ei].v0];
    const Point2 v1 = graph.verts[graph.edges[ei].v1];
    const Point2 pMin(min(v0.x, v1.x) - 1e-2f, min(v0.y, v1.y) - 1e-2f);
    const Point2 pMax(max(v0.x, v1.x) + 1e-2f, max(v0.y, v1.y) + 1e-2f);
    return BBox2(pMin, pMax);
  });

  const auto segmentsIntersect = [&](int i, int j) -> bool {
    const auto &ei = graph.edges[i];
    const auto &ej = graph.edges[j];
    const Point2 i0 = graph.verts[ei.v0], i1 = graph.verts[ei.v1];
    const Point2 j0 = graph.verts[ej.v0], j1 = graph.verts[ej.v1];
    Point2 P;
    float t, u;
    if (!line_line_intersect(i0, i1, j0, j1, t, u, P))
      return false;
    return t >= 0.f && t <= 1.f && u >= 0.f && u <= 1.f;
  };

  int naiveCount = 0;
  for (int i = 0, ne = int(graph.edges.size()); i < ne; i++)
    for (int j = i + 1; j < ne; j++)
      if (segmentsIntersect(i, j))
        naiveCount++;

  int gridCount = 0;
  eGrid.iteratePotentialPairs([&](int i, int j) {
    if (segmentsIntersect(i, j))
      gridCount++;
  });

  G_ASSERTF(naiveCount == gridCount,
    "verify_edge_grid: naive intersection count %d != grid count %d (grid missed or duplicated pairs)", naiveCount, gridCount);
}


// --------------------------------------------------------------------------------------------
// PLANAR GRAPH PREPARE & BUILD - MAIN ALGORITHM
// --------------------------------------------------------------------------------------------


DAGOR_NOINLINE void prepare_planar_graph(DestrContext &ctx, const PlaneBasis &basis, CutFaceGraph &graph)
{
  G_UNUSED(ctx);
  G_UNUSED(basis);
  if (graph.edges.empty())
    return;

  constexpr float WELD_DIST = 1e-3f;
  constexpr float WELD_DIST_SQ = WELD_DIST * WELD_DIST;
  constexpr float EXTEND_DIST = 0.1f;
  constexpr float EXTEND_DIST_SQ = EXTEND_DIST * EXTEND_DIST;

  // NOTE: this flag should majorly not affect result (minus some jitter), it is purely for optimization purposes
  const bool isSegmentSoup = false;

  // STEP 1: build vertex grid
  VertexGrid vGrid;
  vGrid.build(graph.verts, 5 /* 32x32 */, isSegmentSoup ? 0.1f : EXTEND_DIST * 3.f);

  // STEP 2: weld vertices
  dag::Vector<int, framemem_allocator> vCanon;
  vCanon.resize(graph.verts.size());
  eastl::iota(vCanon.begin(), vCanon.end(), 0);
  const auto getVCanon = [&](int v) FORCE_INLINE_LAMBDA {
    while (vCanon[v] != v)
      v = vCanon.data()[v] = vCanon.data()[vCanon.data()[v]];
    return v;
  };
  const auto weld2verts = [&](int v1, int v2) FORCE_INLINE_LAMBDA {
    v1 = getVCanon(v1);
    v2 = getVCanon(v2);
    if (v1 != v2)
      vCanon[v2] = v1;
    return v1;
  };

  const auto canonizeEdges = [&]() FORCE_INLINE_LAMBDA {
    for (auto &e : graph.edges)
    {
      e.v0 = getVCanon(e.v0);
      e.v1 = getVCanon(e.v1);
    }
    graph.edges.erase(eastl::remove_if(graph.edges.begin(), graph.edges.end(), [](const auto &e) { return e.v0 == e.v1; }),
      graph.edges.end());
    stlsort::sort_branchless(graph.edges.begin(), graph.edges.end(),
      [](const auto &a, const auto &b) { return a.sortKey() < b.sortKey(); });
    graph.edges.erase(
      eastl::unique(graph.edges.begin(), graph.edges.end(), [](const auto &a, const auto &b) { return a.sortKey() == b.sortKey(); }),
      graph.edges.end());
  };

  if (isSegmentSoup)
  {
    for (int v = 0; v < int(graph.verts.size()); v++)
    {
      if (v != vCanon[v]) // already welded
        continue;
      const Point2 p = graph.verts[v];
      vGrid.queryBBox(BBox2(p, WELD_DIST * 2.f), [&](int v1) {
        v1 = getVCanon(v1);
        if (v == v1 || lengthSq(p - graph.verts[v1]) > WELD_DIST_SQ)
          return;
        vCanon[v1] = v;
      });
    }

    // cleanup duplicate and degenerate edges
    canonizeEdges();
  }
  else
  {
    // If case our data comes from actual mesh and has good topology (most connected edges already share a vertex)
    // we don't need separate weld step at all. The edge splitting step also handles vertex welding, just less efficiently
    // but when there are only few vertices to weld, it wins a lot to skip this step entirely
    // The only concern is STEP 4, which will produce degenerate edges for topologically disconnected but nearby vertices,
    // however it is fine, as they will be filtered out and welded on STEP 5 anyway
  }

  // STEP 3: fill vertex info for incoming/outgroing edges
  struct VertexInfo
  {
    bool incoming = false, outgoing = false;
  };
  dag::Vector<VertexInfo, framemem_allocator> vertInfo;
  vertInfo.resize(graph.verts.size());
  for (auto &e : graph.edges)
  {
    vertInfo[e.v0].outgoing = true;
    vertInfo[e.v1].incoming = true;
  }

  // STEP 4: search & connect pairs of dangling vertices + weld remaining vertices
  bool anyVerticesWelded = false;
  for (int v = 0; v < int(graph.verts.size()); v++)
  {
    // skip both non dangling and orphaned
    auto &__restrict vInfo = vertInfo[v];
    if (vInfo.incoming == vInfo.outgoing)
      continue;
    if (v != vCanon[v]) // already welded
      continue;
    const Point2 p = graph.verts[v];
    int found = -1;
    float foundDistSq = EXTEND_DIST_SQ;
    vGrid.queryBBox(BBox2(p, EXTEND_DIST * 2.f), [&](int v1) FORCE_INLINE_LAMBDA {
      v1 = getVCanon(v1);
      const auto &__restrict v1Info = vertInfo[v1];
      const float dSq = lengthSq(p - graph.verts[v1]);
      if (DAGOR_UNLIKELY(!isSegmentSoup && dSq < WELD_DIST_SQ && v1 != v)) // most vertices are already connected, thus slow path
      {
        vCanon[v1] = v;
        vInfo.incoming |= v1Info.incoming;
        vInfo.outgoing |= v1Info.outgoing;
        anyVerticesWelded = true;
        if (vInfo.incoming == vInfo.outgoing)
        {
          found = -1;
          foundDistSq = -1.f;
        }
        return;
      }
      // only pick other dangling, and only ones that match with us
      if (vInfo.outgoing == v1Info.outgoing || vInfo.incoming == v1Info.incoming)
        return;
      // weld very close verts in-place
      if (dSq >= foundDistSq)
        return;
      foundDistSq = dSq;
      found = v1;
    });

    // found other dangling - connect
    if (found != -1)
    {
      if (vertInfo[v].incoming) // v -> found
      {
        graph.edges.push_back({v, found});
        G_VERIFY(!eastl::exchange(vertInfo[v].outgoing, true));
        G_VERIFY(!eastl::exchange(vertInfo[found].incoming, true));
      }
      else // found -> v
      {
        graph.edges.push_back({found, v});
        G_VERIFY(!eastl::exchange(vertInfo[v].incoming, true));
        G_VERIFY(!eastl::exchange(vertInfo[found].outgoing, true));
      }
    }
  }

  if (!isSegmentSoup && anyVerticesWelded)
    canonizeEdges();

  // STEP 5: per-edge split state
  bool needEdgeFilteringPass = false;
  struct EdgeSplit
  {
    float t;
    int v;
  };
  struct EdgeSplitState
  {
    vec4f v01;
    dag::RelocatableFixedVector<EdgeSplit, 2, true, framemem_allocator> splits;
    float invLen;
    bool danglingV0;
    bool danglingV1;
  };
  dag::Vector<EdgeSplitState, framemem_allocator> edgeStates;
  {
    edgeStates.resize(graph.edges.size());
    int validEdgeCnt = 0;
    for (int ei = 0, ne = int(graph.edges.size()); ei < ne; ei++)
    {
      auto &e = graph.edges[validEdgeCnt] = graph.edges[ei];
      auto &st = edgeStates[validEdgeCnt];
      const Point2 v0 = graph.verts[e.v0], v1 = graph.verts[e.v1];
      float len = length(v1 - v0);
      if (len < WELD_DIST) // drop the edge
      {
        weld2verts(e.v0, e.v1);
        needEdgeFilteringPass = true;
        continue;
      }
      st.v01 = v_make_vec4f(v0.x, v0.y, v1.x, v1.y);
      st.invLen = 1.f / length(v1 - v0);
      st.danglingV0 = !vertInfo[e.v0].incoming;
      st.danglingV1 = !vertInfo[e.v1].outgoing;
      st.splits.push_back({0.f, e.v0});
      st.splits.push_back({1.f, e.v1});
      validEdgeCnt++;
    }
    graph.edges.resize(validEdgeCnt);
    edgeStates.resize(validEdgeCnt);
  }

  const auto insertSplitAt = [&](int e, float t, Point2 pos) FORCE_INLINE_LAMBDA -> EdgeSplit & {
    auto &splits = edgeStates[e].splits;
    int i = 0;
    for (; i < int(splits.size()); i++)
      if (t < splits[i].t)
        break;
    if (i > 0)
    {
      const int pv = splits[i - 1].v;
      if (lengthSq(pos - graph.verts[pv]) <= WELD_DIST_SQ)
        return splits[i - 1];
    }
    if (i < int(splits.size()))
    {
      const int sv = splits[i].v;
      if (lengthSq(pos - graph.verts[sv]) <= WELD_DIST_SQ)
        return splits[i];
    }
    return *splits.insert(splits.begin() + i, EdgeSplit{t, -1});
  };

  // STEP 6: prepare and fill edge grid
  EdgeGrid eGrid;
  eGrid.build(graph.edges.size(), 5, EXTEND_DIST * 3.f, [&](const int ei) FORCE_INLINE_LAMBDA {
    auto &st = edgeStates[ei];
    vec4f v01 = st.v01;
    vec4f v10 = v_perm_zwxy(v01);
    vec4f ext = v_splats(st.danglingV0 || st.danglingV1 ? EXTEND_DIST : WELD_DIST);
    vec4f bb = v_perm_xyab(v_sub(v_min(v01, v10), ext), v_add(v_max(v01, v10), ext));
    alignas(16) BBox2 bbox;
    v_st(&bbox, bb);
    return bbox;
  });

  // verify_edge_grid(graph);

  // STEP 7: Edge intersection and splitting + remaining vertex weld
  graph.verts.reserve(graph.verts.size() * 3 / 2);
  vCanon.reserve(graph.verts.capacity());
  eGrid.iteratePotentialPairs([&](const int ei, const int ej) FORCE_INLINE_LAMBDA {
    const auto ie = graph.edges[ei];
    const auto je = graph.edges[ej];
    auto &stI = edgeStates[ei];
    auto &stJ = edgeStates[ej];
    const bool anyDangling = stI.danglingV0 | stI.danglingV1 | stJ.danglingV0 | stJ.danglingV1;
    alignas(16) struct
    {
      Point2 i0, i1, j0, j1;
    } v;
    vec4f i01 = stI.v01;
    vec4f j01 = stJ.v01;
    v_st(&v.i0.x, i01);
    v_st(&v.j0.x, j01);

    // early out by bbox
    {
      vec4f bmin = v_min(v_perm_xyab(i01, j01), v_perm_zwcd(i01, j01)); // .xy = min(i0.xy, i1.xy), .zw = min(j0.xy, j1.xy)
      vec4f bmax = v_max(v_perm_xyab(i01, j01), v_perm_zwcd(i01, j01)); // .xy = max(i0.xy, i1.xy), .zw = max(j0.xy, j1.xy)
      vec4f bext = v_splats(anyDangling ? EXTEND_DIST : WELD_DIST);
      bmin = v_sub(bmin, bext);
      bmax = v_add(bmax, bext);
      if (v_signmask(v_cmp_lt(bmax, v_perm_zwxy(bmin))))
        return;
    }

    Point2 P;
    float ti, tj;
    if (!line_line_intersect(v.i0, v.i1, v.j0, v.j1, ti, tj, P))
    {
      const Point2 iDir = v.i1 - v.i0;
      // handle collinear
      const auto splitCollinearAt = [&](int e, Point2 e0, Point2 eDir, float invLenSqE, float weldSlackE, int fv) FORCE_INLINE_LAMBDA {
        const Point2 fpos = graph.verts[fv];
        const float t = ((fpos.x - e0.x) * eDir.x + (fpos.y - e0.y) * eDir.y) * invLenSqE;
        if (t <= weldSlackE) // at/near e's t=0 endpoint, or projects before e
        {
          if (lengthSq(fpos - graph.verts[graph.edges[e].v0]) < WELD_DIST_SQ)
          {
            weld2verts(graph.edges[e].v0, fv); // collinear end-to-end coincidence
            needEdgeFilteringPass = true;
          }
          return;
        }
        if (t >= 1.f - weldSlackE) // at/near e's t=1 endpoint, or projects past e
        {
          if (lengthSq(fpos - graph.verts[graph.edges[e].v1]) < WELD_DIST_SQ)
          {
            weld2verts(graph.edges[e].v1, fv);
            needEdgeFilteringPass = true;
          }
          return;
        }
        const float perpCross = (fpos.x - e0.x) * eDir.y - (fpos.y - e0.y) * eDir.x;
        if (perpCross * perpCross * invLenSqE > WELD_DIST_SQ)
          return; // fv is off e's line — not a valid collinear split point
        needEdgeFilteringPass = true;
        EdgeSplit &s = insertSplitAt(e, t, fpos);
        if (s.v == -1)
          s.v = fv;
        else if (s.v != fv)
          weld2verts(s.v, fv);
        // ctx.dbgDraw.drawPoint(basis.unProject(fpos), E3DCOLOR(255, 255, 255));
      };

      const Point2 jDir = v.j1 - v.j0;
      const float invLenSqI = sqr(stI.invLen), invLenSqJ = sqr(stJ.invLen);
      const float weldSlackI = WELD_DIST * stI.invLen, weldSlackJ = WELD_DIST * stJ.invLen;
      splitCollinearAt(ei, v.i0, iDir, invLenSqI, weldSlackI, je.v0);
      splitCollinearAt(ei, v.i0, iDir, invLenSqI, weldSlackI, je.v1);
      splitCollinearAt(ej, v.j0, jDir, invLenSqJ, weldSlackJ, ie.v0);
      splitCollinearAt(ej, v.j0, jDir, invLenSqJ, weldSlackJ, ie.v1);
      return;
    }
    else
    {
      if (ie.v0 == je.v0 || ie.v0 == je.v1 || ie.v1 == je.v0 || ie.v1 == je.v1)
        return;

      const float extTi = EXTEND_DIST * stI.invLen;
      const float extTj = EXTEND_DIST * stJ.invLen;
      const float tiLow = stI.danglingV0 ? -extTi : 0.f;
      const float tiHigh = stI.danglingV1 ? 1.f + extTi : 1.f;
      const float tjLow = stJ.danglingV0 ? -extTj : 0.f;
      const float tjHigh = stJ.danglingV1 ? 1.f + extTj : 1.f;

      const bool tiOK = (ti >= tiLow && ti <= tiHigh) || lengthSq(P - v.i0) < WELD_DIST_SQ || lengthSq(P - v.i1) < WELD_DIST_SQ;
      const bool tjOK = (tj >= tjLow && tj <= tjHigh) || lengthSq(P - v.j0) < WELD_DIST_SQ || lengthSq(P - v.j1) < WELD_DIST_SQ;
      if (!tiOK || !tjOK)
        return;

      EdgeSplit &splitI = insertSplitAt(ei, ti, P);
      EdgeSplit &splitJ = insertSplitAt(ej, tj, P);
      if (splitI.v != -1 && splitJ.v != -1)
      {
        if (splitI.v != splitJ.v)
        {
          weld2verts(splitI.v, splitJ.v);
          needEdgeFilteringPass = true;
        }
      }
      else if (splitI.v != -1)
      {
        splitJ.v = splitI.v;
        needEdgeFilteringPass = true;
      }
      else if (splitJ.v != -1)
      {
        splitI.v = splitJ.v;
        needEdgeFilteringPass = true;
      }
      else
      {
        const int newVert = graph.verts.size();
        graph.verts.push_back(P);
        vCanon.push_back(newVert);
        splitI.v = splitJ.v = newVert;
      }
    }
  });

  // STEP 9: rebuild edges from split chains. Each edge emits its own chain in its own direction.
  // Zero-length sub-edges (consecutive entries that ended up with the same canonical vertex) are
  // skipped inline. Collinear same-direction overlaps produce identical directed duplicates, so
  // when any collinear split happened we dedup (sort + unique), which preserves antiparallel pairs.
  dag::Vector<CutFaceGraph::Edge, framemem_allocator> newEdges;
  newEdges.reserve(graph.edges.size() * 2);
  for (auto &st : edgeStates)
  {
    for (auto &spl : st.splits)
      spl.v = getVCanon(spl.v);
    for (int i = 1, n = int(st.splits.size()); i < n; i++)
    {
      const int a = st.splits[i - 1].v;
      const int b = st.splits[i].v;
      if (a == b)
        continue;
      newEdges.push_back({a, b});
    }
  }
  graph.edges = eastl::move(newEdges);

  if (needEdgeFilteringPass)
  {
    stlsort::sort_branchless(graph.edges.begin(), graph.edges.end(),
      [](const auto &a, const auto &b) { return a.sortKey() < b.sortKey(); });
    graph.edges.erase(
      eastl::unique(graph.edges.begin(), graph.edges.end(), [](const auto &a, const auto &b) { return a.v0 == b.v0 && a.v1 == b.v1; }),
      graph.edges.end());
  }
}

// planarity / vertex-spacing verification
DAGOR_NOINLINE void verify_planar_graph(const CutFaceGraph &graph)
{
  constexpr float WELD_DIST = 1e-3f;
  constexpr float WELD_DIST_SQ = WELD_DIST * WELD_DIST;

  // Collect vertices actually referenced by surviving edges (orphan/dead slots in graph.verts are ignored).
  dag::Vector<int, framemem_allocator> usedVerts;
  usedVerts.reserve(graph.edges.size() * 2);
  for (const auto &e : graph.edges)
  {
    usedVerts.push_back(e.v0);
    usedVerts.push_back(e.v1);
  }
  eastl::sort(usedVerts.begin(), usedVerts.end());
  usedVerts.erase(eastl::unique(usedVerts.begin(), usedVerts.end()), usedVerts.end());

  // 1. every used vertex pair must be at least WELD_DIST apart
  for (int i = 0, n = int(usedVerts.size()); i < n; i++)
    for (int j = i + 1; j < n; j++)
    {
      const int a = usedVerts[i], b = usedVerts[j];
      G_VERIFYF(lengthSq(graph.verts[a] - graph.verts[b]) >= WELD_DIST_SQ,
        "verify_planar_graph: vertices %d and %d are within WELD_DIST", a, b);
    }

  // 2. no two edges may cross except at a shared endpoint vertex
  for (int i = 0, ne = int(graph.edges.size()); i < ne; i++)
    for (int j = i + 1; j < ne; j++)
    {
      const auto &ei = graph.edges[i];
      const auto &ej = graph.edges[j];
      const Point2 i0 = graph.verts[ei.v0], i1 = graph.verts[ei.v1];
      const Point2 j0 = graph.verts[ej.v0], j1 = graph.verts[ej.v1];

      Point2 P;
      float t, u;
      if (!line_line_intersect(i0, i1, j0, j1, t, u, P))
      {
        // parallel; only a problem if collinear AND overlapping
        const Point2 iDir = i1 - i0;
        const float invLenI = 1.f / length(iDir);
        const float perpCross = (j0.x - i0.x) * iDir.y - (j0.y - i0.y) * iDir.x;
        if (fabsf(perpCross) * invLenI > WELD_DIST)
          continue; // parallel but not collinear
        const float invLenSqI = sqr(invLenI);
        const float tj0 = ((j0.x - i0.x) * iDir.x + (j0.y - i0.y) * iDir.y) * invLenSqI;
        const float tj1 = ((j1.x - i0.x) * iDir.x + (j1.y - i0.y) * iDir.y) * invLenSqI;
        const float tLo = eastl::min(tj0, tj1), tHi = eastl::max(tj0, tj1);
        const float slack = WELD_DIST * invLenI;
        G_VERIFYF(tHi <= slack || tLo >= 1.f - slack,
          "verify_planar_graph: edges %d (%d->%d) and %d (%d->%d) are collinear and overlap", i, ei.v0, ei.v1, j, ej.v0, ej.v1);
        continue;
      }

      // Endpoint-spatial classification (works regardless of edge length, immune to t/u sign noise).
      const bool atI0 = lengthSq(P - i0) < WELD_DIST_SQ;
      const bool atI1 = lengthSq(P - i1) < WELD_DIST_SQ;
      const bool atJ0 = lengthSq(P - j0) < WELD_DIST_SQ;
      const bool atJ1 = lengthSq(P - j1) < WELD_DIST_SQ;
      const bool onI = atI0 || atI1 || (t > 0.f && t < 1.f);
      const bool onJ = atJ0 || atJ1 || (u > 0.f && u < 1.f);
      if (!onI || !onJ)
        continue; // line-line hit lies outside one of the segments

      const bool iAtEnd = atI0 || atI1;
      const bool jAtEnd = atJ0 || atJ1;
      if (iAtEnd && jAtEnd)
      {
        // Both segments meet at an endpoint each. The endpoint vertex ids must match.
        const int vi = atI0 ? ei.v0 : ei.v1;
        const int vj = atJ0 ? ej.v0 : ej.v1;
        G_VERIFYF(vi == vj,
          "verify_planar_graph: edges %d (%d->%d) and %d (%d->%d) meet at near-coincident but unmerged verts %d and %d", i, ei.v0,
          ei.v1, j, ej.v0, ej.v1, vi, vj);
      }
      else
      {
        G_VERIFYF(false, "verify_planar_graph: edges %d (%d->%d) and %d (%d->%d) cross at non-endpoint (t=%.4f, u=%.4f)", i, ei.v0,
          ei.v1, j, ej.v0, ej.v1, t, u);
      }
    }
}

} // namespace frx