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

static __forceinline bool line_line_solve(float d, float tn, float un, float inv_len_sq_ab, Point2 p0, Point2 p1, Point2 q0, Point2 q1,
  float &t, float &u, Point2 &out_point)
{
  constexpr float SIN_PARALLEL = 1e-3f;
  if (d < 1e-3f)
    if (d * d * inv_len_sq_ab < SIN_PARALLEL * SIN_PARALLEL) // length independed collinearity check
      return false;
  t = tn / d;
  u = un / d;
  out_point = (lerp(p0, p1, t) + lerp(q0, q1, u)) * 0.5f;
  return true;
}

static __forceinline bool line_line_intersect(Point2 p0, Point2 p1, Point2 q0, Point2 q1, float &t, float &u, Point2 &out_point)
{
  const float ax = p1.x - p0.x, ay = p1.y - p0.y;
  const float bx = q1.x - q0.x, by = q1.y - q0.y;
  const float d = ax * by - ay * bx;
  const float tn = (q0.x - p0.x) * by - (q0.y - p0.y) * bx;
  const float un = (q0.x - p0.x) * ay - (q0.y - p0.y) * ax;
  return line_line_solve(d, tn, un, 1.f / ((ax * ax + ay * ay) * (bx * bx + by * by)), p0, p1, q0, q1, t, u, out_point);
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

#define PG_TRACE_ENABLED 0

#if PG_TRACE_ENABLED
  const bool TRACE_GRAPH = ctx.dbgDraw.drawCutEdgeGraph;
#define PG_TRACE(...)                \
  do                                 \
  {                                  \
    if (DAGOR_UNLIKELY(TRACE_GRAPH)) \
      debug("frxpg: " __VA_ARGS__);  \
  } while (0)
#else
  const bool TRACE_GRAPH = false;
#define PG_TRACE(...) \
  do                  \
  {                   \
  } while (0)
#endif

  PG_TRACE("prepare: %d verts, %d edges", int(graph.verts.size()), int(graph.edges.size()));

  constexpr float WELD_DIST = 1e-3f;
  constexpr float WELD_DIST_SQ = WELD_DIST * WELD_DIST;
  constexpr float EXTEND_DIST = 0.1f;
  constexpr float EXTEND_DIST_SQ = EXTEND_DIST * EXTEND_DIST;

  // gather close vertices during edge intersection and re-judge them in a separate pass, to avoid artifacts from vertex drift from
  // welding
  constexpr bool ENDPOINT_REJUDGE = true;
  constexpr float REJUDGE_BAND_SQ = 9.f * WELD_DIST_SQ; // (3*WELD)^2
  // extend edges from vertices, that have 1 incoming and 1 outgoing edge with almost opposite directions
  constexpr float SPIKE_MAX_DOT = -(1.f - 2e-3f);

  // NOTE: this flag should majorly not affect result (minus some jitter), it is purely for optimization purposes
  const bool isSegmentSoup = false;

  // STEP 1: build vertex grid
  VertexGrid vGrid;
  vGrid.build(graph.verts, 5 /* 32x32 */, isSegmentSoup ? 0.1f : EXTEND_DIST * 3.f);

  // STEP 2: weld vertices
  dag::Vector<int, framemem_allocator> vCanon;
  vCanon.resize(graph.verts.size());
  eastl::iota(vCanon.begin(), vCanon.end(), 0);
  const auto getVCanon = [&](int v) DAGOR_FORCE_INLINE_LAMBDA {
    while (vCanon[v] != v)
      v = vCanon.data()[v] = vCanon.data()[vCanon.data()[v]];
    return v;
  };
  bool needEdgeFilteringPass = false; // set by every graph mutation that can produce duplicate/degenerate edges
  const auto weld2verts = [&](int v1, int v2) DAGOR_FORCE_INLINE_LAMBDA {
    v1 = getVCanon(v1);
    v2 = getVCanon(v2);
    if (v1 != v2)
    {
      PG_TRACE("weld v%d(%.6f,%.6f) <- v%d(%.6f,%.6f) dist %.6f", v1, graph.verts[v1].x, graph.verts[v1].y, v2, graph.verts[v2].x,
        graph.verts[v2].y, length(graph.verts[v2] - graph.verts[v1]));
      vCanon[v2] = v1;
      needEdgeFilteringPass = true;
    }
    return v1;
  };

  const auto canonizeEdges = [&]() DAGOR_FORCE_INLINE_LAMBDA {
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

  // STEP 3: fill vertex info for incoming/outgoing edges (used by the STEP 4 pairing only;
  // the edge pass derives its own dangling classification at the freeze below)
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

  // STEP 4: propose connection edges for dangling vertices. Pure proposal pass over the pristine
  // STEP 3 flags: nothing is welded and no flags are updated, so every dangle sees the same
  // immutable world and connects to its true nearest complementary partner — order-independent and
  // immune to the masking that mutable flags caused. The rest of the pipeline normalizes the
  // proposals: mutual ones produce exact duplicate edges (deduped at STEP 9 via the pair-loop
  // duplicate early-out), sub-WELD ones collapse into welds in STEP 5, coincident tips merge via
  // STEP 7 endpoint welds, and every connect edge participates in the full STEP 7 splitting pass.
  for (int v = 0; v < int(graph.verts.size()); v++)
  {
    // skip both non dangling and orphaned
    const auto &__restrict vInfo = vertInfo[v];
    if (vInfo.incoming == vInfo.outgoing)
      continue;
    if (v != vCanon[v]) // already welded
      continue;
    const Point2 p = graph.verts[v];
    int found = -1;
    float foundDistSq = EXTEND_DIST_SQ;
    vGrid.queryBBox(BBox2(p, EXTEND_DIST * 2.f), [&](int v1) DAGOR_FORCE_INLINE_LAMBDA {
      v1 = getVCanon(v1);
      const auto &__restrict v1Info = vertInfo[v1];
      if (v1Info.incoming != v1Info.outgoing && v1 != v)
        PG_TRACE("step4   near dangling v%d %s dist %.6f", v1, v1Info.incoming ? "head(in)" : "tail(out)",
          length(p - graph.verts[v1]));
      // only pick other dangling, and only ones that match with us
      if (vInfo.outgoing == v1Info.outgoing || vInfo.incoming == v1Info.incoming)
        return;
      const float dSq = lengthSq(p - graph.verts[v1]);
      if (dSq >= foundDistSq)
        return;
      foundDistSq = dSq;
      found = v1;
    });

    if (found == -1)
    {
      PG_TRACE("step4 dangling v%d(%.6f,%.6f) %s: no complementary partner in reach", v, p.x, p.y,
        vInfo.incoming ? "head(in)" : "tail(out)");
      continue;
    }
    PG_TRACE("step4 connect dangling v%d and v%d dist %.6f (new edge %d)", v, found, sqrtf(foundDistSq), int(graph.edges.size()));
    if (vInfo.incoming) // head: v -> found
      graph.edges.push_back({v, found});
    else // tail: found -> v
      graph.edges.push_back({found, v});
  }

  // STEP 5: collapse sub-WELD edges to a fixpoint BEFORE freezing per-edge geometry below
  for (bool anyCollapsed = true; anyCollapsed;)
  {
    anyCollapsed = false;
    for (int i = 0; i < int(graph.edges.size());)
    {
      auto &e = graph.edges[i];
      const int a = getVCanon(e.v0), b = getVCanon(e.v1);
      if (a != b && lengthSq(graph.verts[a] - graph.verts[b]) >= WELD_DIST_SQ)
      {
        i++;
        continue;
      }
      if (a != b)
      {
        PG_TRACE("step5 collapse tiny edge v%d->v%d len %.6f", a, b, length(graph.verts[a] - graph.verts[b]));
        weld2verts(a, b);
        anyCollapsed = true;
      }
      e = graph.edges.back();
      graph.edges.pop_back();
    }
  }

  // per-edge split state
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
  struct VertexDirs
  {
    Point2 inDir = Point2(0.f, 0.f), outDir = Point2(0.f, 0.f); // normalized edge dir; valid when the count is exactly 1
    uint8_t inCnt = 0, outCnt = 0;                              // clamped at 2
    bool spike = false;
  };
  dag::Vector<VertexDirs, framemem_allocator> vertDirs;
  vertDirs.resize(graph.verts.size());
  dag::Vector<EdgeSplitState, framemem_allocator> edgeStates;
  edgeStates.resize(graph.edges.size());
  for (int ei = 0, ne = int(graph.edges.size()); ei < ne; ei++)
  {
    auto &e = graph.edges[ei];
    e.v0 = getVCanon(e.v0); // freeze geometry at canonical positions — matches what STEP 9 will emit
    e.v1 = getVCanon(e.v1);
    auto &st = edgeStates[ei];
    const Point2 v0 = graph.verts[e.v0], v1 = graph.verts[e.v1];
    st.v01 = v_make_vec4f(v0.x, v0.y, v1.x, v1.y);
    Point2 dir = v1 - v0;
    st.invLen = 1.f / length(dir);
    st.splits.push_back({0.f, e.v0});
    st.splits.push_back({1.f, e.v1});
    dir *= st.invLen;
    auto &d0 = vertDirs[e.v0];
    d0.outDir = dir;
    d0.outCnt++;
    auto &d1 = vertDirs[e.v1];
    d1.inDir = dir;
    d1.inCnt++;
  }

  // detect spike vertices and assign dangling flags to edge state
  if constexpr (SPIKE_MAX_DOT > -(1.f - 1e-7f))
    for (int v = 0, nv = int(vertDirs.size()); v < nv; v++)
    {
      auto &d = vertDirs[v];
      if (d.inCnt != 1 || d.outCnt != 1 || d.inDir * d.outDir > SPIKE_MAX_DOT)
        continue;
      d.spike = true;
      PG_TRACE("spike tip v%d(%.6f,%.6f) dot %.8f", v, graph.verts[v].x, graph.verts[v].y, d.inDir * d.outDir);
    }
  for (int ei = 0, ne = int(graph.edges.size()); ei < ne; ei++)
  {
    auto &st = edgeStates[ei];
    const auto &e = graph.edges[ei];
    st.danglingV0 = !vertDirs[e.v0].inCnt || vertDirs[e.v0].spike;
    st.danglingV1 = !vertDirs[e.v1].outCnt || vertDirs[e.v1].spike;
  }

  const auto insertSplitAt = [&](int e, float t, Point2 pos) DAGOR_FORCE_INLINE_LAMBDA -> EdgeSplit & {
    auto &splits = edgeStates[e].splits;
    int i = 0;
    for (; i < int(splits.size()); i++)
      if (t < splits[i].t)
        break;
    if (i > 0)
    {
      const int pv = splits[i - 1].v;
      if (lengthSq(pos - graph.verts[pv]) <= WELD_DIST_SQ)
      {
        PG_TRACE("split e%d t=%.4f (%.6f,%.6f) snap to prev v%d t=%.4f dist %.6f", e, t, pos.x, pos.y, pv, splits[i - 1].t,
          length(pos - graph.verts[pv]));
        return splits[i - 1];
      }
    }
    if (i < int(splits.size()))
    {
      const int sv = splits[i].v;
      if (lengthSq(pos - graph.verts[sv]) <= WELD_DIST_SQ)
      {
        PG_TRACE("split e%d t=%.4f (%.6f,%.6f) snap to next v%d t=%.4f dist %.6f", e, t, pos.x, pos.y, sv, splits[i].t,
          length(pos - graph.verts[sv]));
        return splits[i];
      }
    }
    PG_TRACE("split e%d t=%.4f insert at (%.6f,%.6f)", e, t, pos.x, pos.y);
    return *splits.insert(splits.begin() + i, EdgeSplit{t, -1});
  };

  // insert a split on edge e at param t and bind vertex fv to it (welding if a coincident split
  // vertex already sits there); shared tail of the STEP 7 endpoint test and the STEP 8 re-judge
  const auto adoptSplitVert = [&](int e, float t, int fv, Point2 fpos) DAGOR_FORCE_INLINE_LAMBDA {
    EdgeSplit &s = insertSplitAt(e, t, fpos);
    if (s.v == -1)
      s.v = fv;
    else if (s.v != fv)
      weld2verts(s.v, fv);
    needEdgeFilteringPass = true;
  };

  struct TRange
  {
    float low, high;
  };
  const auto extRange = [&](const EdgeSplitState &st) DAGOR_FORCE_INLINE_LAMBDA -> TRange {
    const float ext = EXTEND_DIST * st.invLen;
    return {st.danglingV0 ? -ext : 0.f, st.danglingV1 ? 1.f + ext : 1.f};
  };

  // STEP 6: prepare and fill edge grid
  EdgeGrid eGrid;
  eGrid.build(graph.edges.size(), 5, EXTEND_DIST * 3.f, [&](const int ei) DAGOR_FORCE_INLINE_LAMBDA {
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

  // list of points for re-judge pass
  struct EndpointWatch
  {
    int e, fv;
    float t;
  };
  dag::Vector<EndpointWatch, framemem_allocator> endpointWatch;

  eGrid.iteratePotentialPairs([&](const int ei, const int ej) DAGOR_FORCE_INLINE_LAMBDA {
    const auto ie = graph.edges[ei];
    const auto je = graph.edges[ej];
    PG_TRACE("pair e%d(v%d->v%d) x e%d(v%d->v%d)", ei, ie.v0, ie.v1, ej, je.v0, je.v1);
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

    // prepare cross & precheck if collinearity resolution is needed
    const float invLenSqI = sqr(stI.invLen), invLenSqJ = sqr(stJ.invLen);
    vec4f crosses; // {cross(D1, iDir), cross(D1, jDir), 0, cross(iDir, jDir)}
    int endpointSplitMask, sharedVertsMask;
    vec4f endpointPerpSqN; // perpCross^2 * invLenSq
    {
      const vec4f dif = v_sub(j01, i01);                                      // .xy = D1
      const vec4f dirs = v_sub(v_perm_zwcd(i01, j01), v_perm_xyab(i01, j01)); // (iDir.xy, jDir.xy)
      const vec4f dd = v_perm_xyab(dif, dirs);                                // (D1.x, D1.y, iDir.x, iDir.y)
      crosses = v_sub(v_mul(v_perm_xxzz(dd), v_perm_ywyw(dirs)), v_mul(v_perm_yyww(dd), v_perm_xzxz(dirs))); // (c0, c1, 0, d)
      const vec4f invLenSq = v_make_vec4f(invLenSqI, invLenSqI, invLenSqJ, invLenSqJ);
      const vec4f perp = v_sub(v_perm_xxyy(crosses), v_perm_zwzw(crosses)); // (c0, c0-d, c1, c1-d)
      const vec4f perpSqN = v_mul(v_mul(perp, perp), invLenSq);
      const vec4f active = v_cmp_ge(v_splats(ENDPOINT_REJUDGE ? REJUDGE_BAND_SQ : WELD_DIST_SQ), perpSqN);
      const vec4f ij = v_perm_xyab(v_cast_vec4f(v_ldui_half(&graph.edges[ei].v0)), v_cast_vec4f(v_ldui_half(&graph.edges[ej].v0)));
      const vec4i fvIds = v_cast_vec4i(v_perm_zwxy(ij)); // (je.v0, je.v1, ie.v0, ie.v1)
      const vec4f shared =
        v_cast_vec4f(v_ori(v_cmp_eqi(fvIds, v_cast_vec4i(v_perm_xxzz(ij))), v_cmp_eqi(fvIds, v_cast_vec4i(v_perm_yyww(ij)))));
      sharedVertsMask = v_signmask(shared);
      endpointSplitMask = v_signmask(v_andnot(shared, active));
      endpointPerpSqN = perpSqN;
    }

    if (DAGOR_UNLIKELY((sharedVertsMask & 3) == 3)) // {je.v0, je.v1} == {ie.v0, ie.v1}: identical or antiparallel span
    {
      needEdgeFilteringPass |= ie.v0 == je.v0; // exact directed duplicate — make sure the final dedup pass runs
      return;
    }

    // collinearity resolution
    if (DAGOR_UNLIKELY(endpointSplitMask != 0 || TRACE_GRAPH))
    {
      const auto splitAtEndpoint = [&](int e, Point2 e0, Point2 eDir, float invLenSqE, float weldSlackE, TRange tr, int fv,
                                     float perpSqN) DAGOR_FORCE_INLINE_LAMBDA {
        if (fv == graph.edges[e].v0 || fv == graph.edges[e].v1)
          return; // fv is e's own endpoint (chain neighbor)
        const Point2 fpos = graph.verts[fv];
        const float t = ((fpos.x - e0.x) * eDir.x + (fpos.y - e0.y) * eDir.y) * invLenSqE;
        if (t <= weldSlackE) // at/near e's t=0 endpoint, or projects before e
        {
          const float dSq = lengthSq(fpos - graph.verts[graph.edges[e].v0]);
          if (dSq < WELD_DIST_SQ)
            weld2verts(graph.edges[e].v0, fv);
          else if (perpSqN <= WELD_DIST_SQ && t >= tr.low)
          {
            PG_TRACE("  endpt extend back: e%d onto v%d at t=%.4f perp %.6f", e, fv, t, sqrtf(perpSqN));
            adoptSplitVert(e, t, fv, fpos);
          }
          else
          {
            if (ENDPOINT_REJUDGE && dSq < REJUDGE_BAND_SQ)
              endpointWatch.push_back({e, fv, t});
            if (TRACE_GRAPH && dSq < sqr(4.f * WELD_DIST))
              PG_TRACE("  endpt near-miss: v%d vs e%d v0(v%d) t=%.4f dist %.6f", fv, e, graph.edges[e].v0, t, sqrtf(dSq));
          }
          return;
        }
        if (t >= 1.f - weldSlackE)
        {
          const float dSq = lengthSq(fpos - graph.verts[graph.edges[e].v1]);
          if (dSq < WELD_DIST_SQ)
            weld2verts(graph.edges[e].v1, fv);
          else if (perpSqN <= WELD_DIST_SQ && t <= tr.high)
          {
            PG_TRACE("  endpt extend fwd: e%d onto v%d at t=%.4f perp %.6f", e, fv, t, sqrtf(perpSqN));
            adoptSplitVert(e, t, fv, fpos);
          }
          else
          {
            if (ENDPOINT_REJUDGE && dSq < REJUDGE_BAND_SQ)
              endpointWatch.push_back({e, fv, t});
            if (TRACE_GRAPH && dSq < sqr(4.f * WELD_DIST))
              PG_TRACE("  endpt near-miss: v%d vs e%d v1(v%d) t=%.4f dist %.6f", fv, e, graph.edges[e].v1, t, sqrtf(dSq));
          }
          return;
        }
        if (perpSqN > WELD_DIST_SQ)
        {
          if (ENDPOINT_REJUDGE && perpSqN < REJUDGE_BAND_SQ)
            endpointWatch.push_back({e, fv, t});
          if (TRACE_GRAPH && perpSqN < sqr(4.f * WELD_DIST))
            PG_TRACE("  endpt off-line: v%d vs e%d t=%.4f perp %.6f", fv, e, t, sqrtf(perpSqN));
          return;
        }
        PG_TRACE("  endpt T-split: v%d onto e%d at t=%.4f", fv, e, t);
        adoptSplitVert(e, t, fv, fpos);
        // ctx.dbgDraw.drawPoint(basis.unProject(fpos), E3DCOLOR(255, 255, 255));
      };

      alignas(16) Point4 perpSq;
      v_st(&perpSq, endpointPerpSqN);
      const Point2 iDir = v.i1 - v.i0, jDir = v.j1 - v.j0;
      const float weldSlackI = WELD_DIST * stI.invLen, weldSlackJ = WELD_DIST * stJ.invLen;
      const TRange trI = extRange(stI), trJ = extRange(stJ);
      splitAtEndpoint(ei, v.i0, iDir, invLenSqI, weldSlackI, trI, je.v0, perpSq[0]);
      splitAtEndpoint(ei, v.i0, iDir, invLenSqI, weldSlackI, trI, je.v1, perpSq[1]);
      splitAtEndpoint(ej, v.j0, jDir, invLenSqJ, weldSlackJ, trJ, ie.v0, perpSq[2]);
      splitAtEndpoint(ej, v.j0, jDir, invLenSqJ, weldSlackJ, trJ, ie.v1, perpSq[3]);
    }

    if (DAGOR_UNLIKELY(sharedVertsMask != 0)) // some verts are shared - no crossing resolution
    {
      PG_TRACE("  shared endpoint");
      return;
    }

    // cross resolution
    alignas(16) Point4 cr;
    v_st(&cr, crosses);
    float ti, tj;
    Point2 P;
    if (line_line_solve(cr.w, cr.y, cr.x, invLenSqI * invLenSqJ, v.i0, v.i1, v.j0, v.j1, ti, tj, P))
    {
      const TRange trI = extRange(stI), trJ = extRange(stJ);
      const bool tiOK = (ti >= trI.low && ti <= trI.high) || lengthSq(P - v.i0) < WELD_DIST_SQ || lengthSq(P - v.i1) < WELD_DIST_SQ;
      const bool tjOK = (tj >= trJ.low && tj <= trJ.high) || lengthSq(P - v.j0) < WELD_DIST_SQ || lengthSq(P - v.j1) < WELD_DIST_SQ;
      if (!tiOK || !tjOK)
      {
        PG_TRACE("  cross reject ti=%.4f(ok=%d) tj=%.4f(ok=%d) P=(%.6f,%.6f) dPi0=%.6f dPi1=%.6f dPj0=%.6f dPj1=%.6f", ti, int(tiOK),
          tj, int(tjOK), P.x, P.y, length(P - v.i0), length(P - v.i1), length(P - v.j0), length(P - v.j1));
        return;
      }

      EdgeSplit &splitI = insertSplitAt(ei, ti, P);
      EdgeSplit &splitJ = insertSplitAt(ej, tj, P);
      if (splitI.v != -1 && splitJ.v != -1)
      {
        if (splitI.v != splitJ.v)
          weld2verts(splitI.v, splitJ.v);
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
        if (!graph.vertsH.empty())
          graph.vertsH.push_back(lerp(graph.vertsH[ie.v0], graph.vertsH[ie.v1], ti));
        vCanon.push_back(newVert);
        splitI.v = splitJ.v = newVert;
      }
      PG_TRACE("  cross split ti=%.4f tj=%.4f P=(%.6f,%.6f) -> vI=%d vJ=%d", ti, tj, P.x, P.y, splitI.v, splitJ.v);
    }
  });

  // STEP 8: re-judge the watched borderline rejections against the chain geometry STEP 9 will
  // actually emit (split-vert canonical positions), while the split lists are still mutable.
  // No-op when ENDPOINT_REJUDGE is off — nothing gets recorded into the watch-list then.
  // A vertex cleared at [WELD, 3*WELD) from an edge's frozen original line can be sub-WELD from a
  // kinked sub-segment; re-running the endpoint test locally heals those T-junctions/welds.
  // Actions here create at most second-level kinks (band covers one level of stacking) — deeper
  // stacks are left to verify_planar_graph.
  if (ENDPOINT_REJUDGE)
    for (const auto &w : endpointWatch)
    {
      const int fv = getVCanon(w.fv);
      const Point2 fpos = graph.verts[fv];
      auto &splits = edgeStates[w.e].splits;
      int hi = 1;
      while (hi < int(splits.size()) - 1 && splits[hi].t < w.t)
        hi++;
      for (int attempt = 0; attempt < 2; attempt++)
      {
        const int a = getVCanon(splits[hi - 1].v), b = getVCanon(splits[hi].v);
        if (fv == a || fv == b)
          break;
        const Point2 pa = graph.verts[a], pb = graph.verts[b];
        const Point2 dir = pb - pa;
        const float lenSq = lengthSq(dir);
        if (lenSq < WELD_DIST_SQ)
        {
          if (lengthSq(fpos - pa) < WELD_DIST_SQ)
          {
            PG_TRACE("rejudge weld v%d <- v%d (collapsed seg on e%d)", a, fv, w.e);
            weld2verts(a, fv);
          }
          break;
        }
        const float t = ((fpos.x - pa.x) * dir.x + (fpos.y - pa.y) * dir.y) / lenSq;
        const float slackE = WELD_DIST / sqrtf(lenSq);
        if (t > slackE && t < 1.f - slackE)
        {
          const float perpCross = (fpos.x - pa.x) * dir.y - (fpos.y - pa.y) * dir.x;
          if (perpCross * perpCross <= WELD_DIST_SQ * lenSq)
          {
            PG_TRACE("rejudge T-split: v%d onto e%d seg t %.4f..%.4f", fv, w.e, splits[hi - 1].t, splits[hi].t);
            adoptSplitVert(w.e, lerp(splits[hi - 1].t, splits[hi].t, t), fv, fpos);
          }
          break;
        }
        if (t <= slackE && lengthSq(fpos - pa) < WELD_DIST_SQ)
        {
          PG_TRACE("rejudge weld v%d <- v%d (e%d seg start)", a, fv, w.e);
          weld2verts(a, fv);
          break;
        }
        if (t >= 1.f - slackE && lengthSq(fpos - pb) < WELD_DIST_SQ)
        {
          PG_TRACE("rejudge weld v%d <- v%d (e%d seg end)", b, fv, w.e);
          weld2verts(b, fv);
          break;
        }
        const int nextHi = t <= slackE ? hi - 1 : hi + 1;
        if (nextHi < 1 || nextHi > int(splits.size()) - 1)
          break;
        hi = nextHi;
      }
    }

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
      PG_TRACE("emit orig e%d sub %d: v%d->v%d (t %.4f..%.4f)", int(&st - edgeStates.data()), int(newEdges.size()), a, b,
        st.splits[i - 1].t, st.splits[i].t);
      newEdges.push_back({a, b});
    }
  }
  graph.edges = eastl::move(newEdges);

  if (needEdgeFilteringPass)
  {
    stlsort::sort_branchless(graph.edges.begin(), graph.edges.end(),
      [](const auto &a, const auto &b) { return a.sortKey() < b.sortKey(); });
    [[maybe_unused]] const int beforeDedup = int(graph.edges.size());
    graph.edges.erase(
      eastl::unique(graph.edges.begin(), graph.edges.end(), [](const auto &a, const auto &b) { return a.v0 == b.v0 && a.v1 == b.v1; }),
      graph.edges.end());
    PG_TRACE("dedup removed %d of %d edges", beforeDedup - int(graph.edges.size()), beforeDedup);
  }

  if (DAGOR_UNLIKELY(TRACE_GRAPH))
    for (int i = 0, ne = int(graph.edges.size()); i < ne; i++)
    {
      const auto &e = graph.edges[i];
      const Point2 p0 = graph.verts[e.v0], p1 = graph.verts[e.v1];
      debug("frxpg: final e%d: v%d(%.6f,%.6f) -> v%d(%.6f,%.6f) len %.6f", i, e.v0, p0.x, p0.y, e.v1, p1.x, p1.y, length(p1 - p0));
    }

#undef PG_TRACE
#undef PG_TRACE_ENABLED
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
      if (ei.v0 == ej.v1 && ei.v1 == ej.v0)
        continue;
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
        if (t > 0.f && t < 1.f && u > 0.f && u < 1.f)
        {
          const int vi = atI0 ? ei.v0 : ei.v1;
          const int vj = atJ0 ? ej.v0 : ej.v1;
          G_VERIFYF(vi == vj,
            "verify_planar_graph: edges %d (%d->%d) and %d (%d->%d) cross at near-coincident but unmerged verts %d and %d "
            "(t=%.4f, u=%.4f)",
            i, ei.v0, ei.v1, j, ej.v0, ej.v1, vi, vj, t, u);
        }
      }
      else
      {
        G_VERIFYF(false,
          "verify_planar_graph: edges %d (%d->%d) and %d (%d->%d) cross at non-endpoint (t=%.4f, u=%.4f) P=(%.6f,%.6f) "
          "i0=(%.6f,%.6f) i1=(%.6f,%.6f) lenI=%.6f j0=(%.6f,%.6f) j1=(%.6f,%.6f) lenJ=%.6f dPi0=%.6f dPi1=%.6f dPj0=%.6f dPj1=%.6f",
          i, ei.v0, ei.v1, j, ej.v0, ej.v1, t, u, P.x, P.y, i0.x, i0.y, i1.x, i1.y, length(i1 - i0), j0.x, j0.y, j1.x, j1.y,
          length(j1 - j0), length(P - i0), length(P - i1), length(P - j0), length(P - j1));
      }
    }
}

} // namespace frx