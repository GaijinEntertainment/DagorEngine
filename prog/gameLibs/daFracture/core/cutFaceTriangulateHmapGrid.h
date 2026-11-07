// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stlqsort.h>
#include <daFracture/core/destrMesh.h>

#include "cutMeshCommon.h"
#include "cutFaceGraph.h"
#include "cutFaceBoundarySearch.h"


namespace frx
{

template <typename EmitTriFn>
static void earcut_simple(dag::ConstSpan<int> handle, dag::ConstSpan<Point2> pos, const DestrContext &ctx, const CutFaceGraph &graph,
  const PlaneBasis &basis, EmitTriFn &&emit_triangle)
{
  const int n = int(handle.size());
  if (n < 3)
    return;
  const auto crossN = [](Point2 a, Point2 b, Point2 c) { return (b.x - a.x) * (c.y - b.y) - (b.y - a.y) * (c.x - b.x); };
  const auto pointInTri = [&](Point2 a, Point2 b, Point2 c, Point2 p) {
    return crossN(a, b, p) >= 0.f && crossN(b, c, p) >= 0.f && crossN(c, a, p) >= 0.f;
  };

  // signed area * 2 to pick the CCW traversal direction
  float area2 = 0.f;
  for (int i = 0, j = n - 1; i < n; j = i++)
    area2 += pos[j].x * pos[i].y - pos[i].x * pos[j].y;
  dag::Vector<int, framemem_allocator> nxt(n), prv(n);
  for (int i = 0; i < n; i++)
  {
    nxt[i] = area2 >= 0.f ? (i + 1) % n : (i - 1 + n) % n;
    prv[i] = area2 >= 0.f ? (i - 1 + n) % n : (i + 1) % n;
  }

  const auto emitT = [&](int a, int b, int c) {
    if (DAGOR_UNLIKELY(ctx.dbgDraw.drawCutFaceTriangles))
    {
      dbg_draw_line(ctx, graph, basis, handle[a], handle[b], E3DCOLOR(0, 255, 255));
      dbg_draw_line(ctx, graph, basis, handle[b], handle[c], E3DCOLOR(0, 255, 255));
      dbg_draw_line(ctx, graph, basis, handle[c], handle[a], E3DCOLOR(0, 255, 255));
    }
    emit_triangle(handle[a], handle[b], handle[c]);
  };

  int alive = n, cur = 0, stuck = 0;
  bool fwd = true;
  while (alive > 3 && stuck < alive * 2)
  {
    const int p = prv[cur], nx = nxt[cur];
    const Point2 a = pos[p], b = pos[cur], c = pos[nx];
    bool isEar = crossN(a, b, c) > 0.f;
    if (isEar)
      for (int k = nxt[nx]; k != p; k = nxt[k])
        if (pointInTri(a, b, c, pos[k]))
        {
          isEar = false;
          break;
        }
    if (isEar)
    {
      emitT(p, cur, nx);
      nxt[p] = nx;
      prv[nx] = p;
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
    emitT(prv[cur], cur, nxt[cur]);
}


template <typename EmitTriFn>
static void earcut_vertical_sweep(dag::ConstSpan<int> handle, dag::ConstSpan<Point2> pos, const DestrContext &ctx,
  const CutFaceGraph &graph, const PlaneBasis &basis, EmitTriFn &&emit_triangle)
{
  const int n = int(handle.size());
  if (n < 3)
    return;
  const auto crossN = [](Point2 a, Point2 b, Point2 c) { return (b.x - a.x) * (c.y - b.y) - (b.y - a.y) * (c.x - b.x); };
  const auto pointInTri = [&](Point2 a, Point2 b, Point2 c, Point2 p) {
    return crossN(a, b, p) >= 0.f && crossN(b, c, p) >= 0.f && crossN(c, a, p) >= 0.f;
  };

  float area2 = 0.f;
  for (int i = 0, j = n - 1; i < n; j = i++)
    area2 += pos[j].x * pos[i].y - pos[i].x * pos[j].y;
  dag::Vector<int, framemem_allocator> nxt(n), prv(n);
  dag::Vector<uint8_t, framemem_allocator> alive;
  alive.resize(n, 1);
  for (int i = 0; i < n; i++)
  {
    nxt[i] = area2 >= 0.f ? (i + 1) % n : (i - 1 + n) % n;
    prv[i] = area2 >= 0.f ? (i - 1 + n) % n : (i + 1) % n;
  }

  const auto emitT = [&](int a, int b, int c) {
    if (DAGOR_UNLIKELY(ctx.dbgDraw.drawCutFaceTriangles))
    {
      dbg_draw_line(ctx, graph, basis, handle[a], handle[b], E3DCOLOR(0, 255, 255));
      dbg_draw_line(ctx, graph, basis, handle[b], handle[c], E3DCOLOR(0, 255, 255));
      dbg_draw_line(ctx, graph, basis, handle[c], handle[a], E3DCOLOR(0, 255, 255));
    }
    emit_triangle(handle[a], handle[b], handle[c]);
  };

  const auto isDegenAt = [&](int cur) {
    const Point2 a = pos[prv[cur]], b = pos[cur], c = pos[nxt[cur]];
    const float ux = b.x - a.x, uy = b.y - a.y, vx = c.x - b.x, vy = c.y - b.y;
    const float u2 = ux * ux + uy * uy, v2 = vx * vx + vy * vy;
    if (u2 * v2 == 0.f)
      return true;
    constexpr float SIN2_EPS = 0.0025f;
    const float cr = ux * vy - uy * vx;
    return cr * cr <= SIN2_EPS * u2 * v2 && ux * vx + uy * vy < 0.f;
  };

  const auto isEarAt = [&](int cur) {
    const int p = prv[cur], nx = nxt[cur];
    const Point2 a = pos[p], b = pos[cur], c = pos[nx];
    if (crossN(a, b, c) <= 0.f)
      return false;
    for (int k = nxt[nx]; k != p; k = nxt[k])
    {
      if (pointInTri(a, b, c, pos[k]))
        return false;
    }
    return true;
  };

  int remaining = n, guard = 0;
  bool bailed = false;
  while (remaining > 3 && guard++ < n * n + 16)
  {
    int bestCur = -1;
    float bestY = FLT_MAX;
    for (int cur = 0; cur < n; cur++)
      if (alive[cur] && pos[cur].y < bestY && isEarAt(cur))
      {
        bestY = pos[cur].y;
        bestCur = cur;
      }
    if (DAGOR_UNLIKELY(bestCur < 0)) // slow path - no ear found, search for degenerate triangles
    {
      int cut = -1;
      for (int cur = 0; cur < n && cut < 0; cur++)
        if (alive[cur] && isDegenAt(cur))
          cut = cur;
      if (cut >= 0)
      {
        const int p = prv[cut], nx = nxt[cut];
        nxt[p] = nx;
        prv[nx] = p;
        alive[cut] = 0;
        remaining--;
        continue;
      }
      // no degenerate triangle found
      int c0 = 0;
      while (!alive[c0])
        c0++;
      float rem2 = 0.f;
      for (int i = 0, v = c0, pv = prv[c0]; i < remaining; i++, pv = v, v = nxt[v])
        rem2 += pos[pv].x * pos[v].y - pos[v].x * pos[pv].y;
      const bool uncovered = rem2 > fabsf(area2) * 0.01f + 1e-7f;
      if (uncovered)
        for (int i = 0, v = c0; i < remaining; i++, v = nxt[v])
          FRX_LOG_FAILURE("earcut stuck vert %d/%d: idx=%d handle=%d pos=(%.7f, %.7f)", i, remaining, v, handle[v], pos[v].x,
            pos[v].y);
      FRX_CHECK_FAILURE(uncovered,
        "earcut_vertical_sweep: no valid ear with %d/%d verts left, dropping area2=%f of %f -- see stuck-vert dump in log", remaining,
        n, rem2, area2);
      bailed = true;
      break; // drop the remainder
    }
    const int p = prv[bestCur], nx = nxt[bestCur];
    emitT(p, bestCur, nx);
    nxt[p] = nx;
    prv[nx] = p;
    alive[bestCur] = 0;
    remaining--;
  }
  FRX_CHECK_FAILURE(remaining != 3 && !bailed, "earcut_vertical_sweep: guard exhausted with %d/%d verts left", remaining, n);
  if (remaining == 3)
  {
    int cur = 0;
    while (!alive[cur])
      cur++;
    emitT(prv[cur], cur, nxt[cur]);
  }
}


// Algorithm that takes a boundary from the cut face processor and a heightfield, and triangulates it, inserting enough vertices to
// sample the heightfield at target cellSize
//
// NOTE: In current implementation, inner hole boundaries are ignored (intentional simplification) - the face is filled solid
//
// High level algorithm overview:
// 1. Walk boundary vertices, snapping vertices within eps of a vertical gridline exactly onto it, and insert additional vertices for:
// - edges split at their crossings with vertical grid lines
// - long on-line vertical edges (walls) subdivided to the required cell size
// 2. Vertices on grid lines are gathered and sorted per line; each carries flags for its prev and next vertex position relative to the
// grid line
// 3. Interior spans are gathered per line from the on-line vertices by a parity sweep (per side of the line)
// 4. Spans interior on both sides are subdivided to the required cell size, these vertices sample the heightfield to determine their
// height
// 5. Resulting stripe faces are walked and assembled (ring order plus per-line vertex order decide every step) and each closed face is
// triangulated with a vertical sweep earcut
// - Vertical sweep on subdivided vertical stripe ensures no long triangles
//
// Requirements (violations are detected and drop only the affected face):
// 1. No repeating vertices in a boundary loop
// - satisfied by the boundary search splitting pass
// 2. No two vertices at distances below 2 eps, and no vertex on another edge's interior
// - satisfied by planar graph prepare, whose weld distance is greater than 2 eps of this function

template <typename HmapT, typename EmitVertFn, typename EmitTriFn>
DAGOR_NOINLINE static void heightmap_fill_stripes(const DestrContext &ctx, const CutFaceGraph &graph, const PlaneBasis &basis,
  dag::ConstSpan<BoundaryLoop> boundaries, int outerIdx, HmapT &hmap, EmitVertFn &&emit_vertex, EmitTriFn &&emit_triangle)
{
  const BoundaryLoop &outer = boundaries[outerIdx];
  G_ASSERT(outer.verts.size() >= 3 && outer.area2 > 0.f);

  const float eps = 0.49e-3f; // IMPORTANT: less than half of vertex weld distance in prepare_planar_graph

  constexpr bool USE_GRID_ORIGIN = true;
  const float cell = hmap.cellSize;
  const float ox = USE_GRID_ORIGIN ? hmap.gridOrigin.x : 0.f;
  const auto lineX = [&](int k) { return ox + k * cell; };
  const auto snapX = [&](float x) {
    if (cell <= 1e-5f)
      return x;
    const float xk = lineX(int(floorf((x - ox) / cell + 0.5f)));
    return fabsf(x - xk) < eps ? xk : x;
  };
  const auto onLineK = [&](float x, int &k) {
    k = int(floorf((x - ox) / cell + 0.5f));
    return x == lineX(k);
  };

  dag::Vector<int, framemem_allocator> H;
  dag::Vector<Point2, framemem_allocator> P;
  const auto addNode = [&](int h, Point2 p) {
    p.x = snapX(p.x);
    H.push_back(h);
    P.push_back(p);
    return int(H.size()) - 1;
  };
  const auto addVertImpl = [&](Point3 p) { return addNode(emit_vertex(p), Point2::xy(p)); };
  const auto addVertLerp = [&](int va, int vb, float t) {
    return addVertImpl(lerp(Point3::xyV(graph.verts[va], graph.vertsH[va]), Point3::xyV(graph.verts[vb], graph.vertsH[vb]), t));
  };
  const auto addVertHmap = [&](Point2 p2d) { return addVertImpl(Point3::xyV(p2d, hmap.sample(p2d))); };

  // no grid - plain earcut of the outer ring
  if (cell <= 1e-5f)
  {
    dag::Vector<int, framemem_allocator> rh;
    dag::Vector<Point2, framemem_allocator> rp;
    for (int v : outer.verts)
    {
      rh.push_back(v);
      rp.push_back(graph.verts[v]);
    }
    earcut_simple(rh, rp, ctx, graph, basis, emit_triangle);
    return;
  }

  dag::Vector<int, framemem_allocator> ring; // node indices, boundary order (CCW), cyclic
  const int on = int(outer.verts.size());
  ring.reserve(on * 2);
  for (int i = 0; i < on; i++)
  {
    const int va = outer.verts[i], vb = outer.verts[(i + 1) % on];
    Point2 A = graph.verts[va], B = graph.verts[vb];
    A.x = snapX(A.x);
    B.x = snapX(B.x);
    ring.push_back(addNode(va, A));
    if (A.x != B.x)
    {
      const float xlo = eastl::min(A.x, B.x), xhi = eastl::max(A.x, B.x);
      const int k0 = int(ceilf((xlo - ox) / cell - 1e-3f)), k1 = int(floorf((xhi - ox) / cell + 1e-3f));
      const bool fwd = A.x < B.x;
      for (int k = fwd ? k0 : k1; fwd ? k <= k1 : k >= k0; k += fwd ? 1 : -1)
      {
        const float xk = lineX(k);
        if (xk <= xlo || xk >= xhi)
          continue;
        ring.push_back(addVertLerp(va, vb, (xk - A.x) / (B.x - A.x)));
      }
    }
    else
    {
      int k;
      if (onLineK(A.x, k) && fabsf(B.y - A.y) > cell) // vertical wall on a line -> subdivide for hmap res
      {
        const int segs = eastl::max(1, int(ceilf(fabsf(B.y - A.y) / cell)));
        for (int s = 1; s < segs; s++)
          ring.push_back(addVertLerp(va, vb, float(s) / float(segs)));
      }
    }
  }
  const int RN = int(ring.size());
  G_ASSERT(RN == int(H.size())); // nodes were appended in ring order: node id == ring position

  struct LV
  {
    int k;
    float y;
    int node;
    uint8_t rt, lt;
  };
  dag::Vector<LV, framemem_allocator> lv;
  for (int i = 0; i < RN; i++)
  {
    const Point2 p = P[ring[i]];
    int k;
    if (!onLineK(p.x, k))
      continue;
    const float x0 = lineX(k);
    const Point2 pa = P[ring[(i - 1 + RN) % RN]], pb = P[ring[(i + 1) % RN]];
    const uint8_t rt = uint8_t(int(pa.x > x0) ^ int(pb.x > x0));
    const uint8_t lt = uint8_t(int(pa.x < x0) ^ int(pb.x < x0));
    lv.push_back({k, p.y, ring[i], rt, lt});
  }
  stlsort::sort(lv.begin(), lv.end(), [](const LV &a, const LV &b) { return a.k != b.k ? a.k < b.k : a.y < b.y; });

  // vertex-simple boundary + WELD spacing make every (k, y) unique: lv index == on-line vert index
  const int MN = int(lv.size());

  dag::Vector<uint8_t, framemem_allocator> spanBits; // span above vert m: bit0 = interior on +x side, bit1 = on -x side
  dag::Vector<int, framemem_allocator> spanVertStart, spanVertCnt;
  spanBits.resize(MN, 0);
  spanVertStart.resize(MN, 0);
  spanVertCnt.resize(MN, 0);
  for (int mi = 0; mi < MN;)
  {
    const int k = lv[mi].k;
    bool rin = false, lin = false;
    int m = mi;
    for (; m < MN && lv[m].k == k; m++)
    {
      rin ^= (lv[m].rt != 0);
      lin ^= (lv[m].lt != 0);
      if (m + 1 < MN && lv[m + 1].k == k)
        spanBits[m] = uint8_t(int(rin) | (int(lin) << 1));
    }
    FRX_CHECK_FAILURE(rin || lin, "fill_stripes: parity not closed on line k=%d (rin=%d lin=%d) -- missed crossing", k, int(rin),
      int(lin));
    mi = m;
  }
  for (int m = 0; m < MN; m++)
  {
    if (spanBits[m] != 3)
      continue;
    const Point2 p0 = P[lv[m].node], p1 = P[lv[m + 1].node];
    const int segs = eastl::max(1, int(ceilf((p1.y - p0.y) / cell)));
    spanVertStart[m] = int(H.size());
    for (int s = 1; s < segs; s++)
      addVertHmap(lerp(p0, p1, float(s) / float(segs)));
    spanVertCnt[m] = int(H.size()) - spanVertStart[m];
    if (DAGOR_UNLIKELY(ctx.dbgDraw.drawBoundaryFill))
      ctx.dbgDraw.drawLine(basis.unProject(p0, ctx.dbgDraw.heightScale * graph.vertsH[H[lv[m].node]]),
        basis.unProject(p1, ctx.dbgDraw.heightScale * graph.vertsH[H[lv[m + 1].node]]), E3DCOLOR_MAKE(255, 0, 255, 255));
  }

  dag::Vector<int, framemem_allocator> edgeCol, mIdx;
  edgeCol.resize(RN);
  mIdx.resize(RN, -1);
  for (int m = 0; m < MN; m++)
    mIdx[lv[m].node] = m;
  for (int i = 0; i < RN; i++)
  {
    const Point2 A = P[ring[i]], B = P[ring[(i + 1) % RN]];
    int ka, kb;
    if (onLineK(A.x, ka) && onLineK(B.x, kb) && ka == kb)
      edgeCol[i] = B.y < A.y ? ka : ka - 1; // interior left of CCW travel: down-going wall -> right column
    else
      edgeCol[i] = int(floorf(((A.x + B.x) * 0.5f - ox) / cell));
  }
  bool anyTransition = false;
  for (int i = 0; i < RN; i++)
    anyTransition |= edgeCol[i] != edgeCol[(i + 1) % RN];

  dag::Vector<int, framemem_allocator> faceNodes, faceH;
  dag::Vector<Point2, framemem_allocator> faceP;
  const auto emitFace = [&]() {
    const int fn = int(faceNodes.size());
    if (fn < 3)
      return;
    float a2 = 0.f;
    for (int a = 0, b = fn - 1; a < fn; b = a++)
    {
      const Point2 pb = P[faceNodes[b]], pa = P[faceNodes[a]];
      a2 += pb.x * pa.y - pa.x * pb.y;
    }
    if (a2 <= 0.f)
    {
      FRX_CHECK_FAILURE(a2 <= -0.1f * cell * cell, "fill_stripes: inverted face, area2=%f", a2);
      return;
    }
    faceH.clear();
    faceP.clear();
    for (int nidx : faceNodes)
    {
      faceH.push_back(H[nidx]);
      faceP.push_back(P[nidx]);
    }
    earcut_vertical_sweep(faceH, faceP, ctx, graph, basis, emit_triangle);
  };

  // chord = a double-sided span; chordDone bit0 = right face done (traversed down), bit1 = left (up)
  dag::Vector<uint8_t, framemem_allocator> chordDone, consumed;
  chordDone.resize(MN, 0);
  consumed.resize(RN, 0);
  const int guardMax = 4 * int(H.size()) + 64;

  const auto chordAt = [&](int j, bool down) {
    const int s = down ? j - 1 : j;
    return s >= 0 && s + 1 < MN && lv[s].k == lv[s + 1].k && spanBits[s] == 3 ? s : -1;
  };

  const auto traverseChord = [&](int s, bool down) {
    FRX_CHECK_FAILURE((chordDone[s] & (down ? 1 : 2)) != 0, "fill_stripes: chord %d traversed twice (down=%d)", s, int(down));
    chordDone[s] |= down ? 1 : 2;
    faceNodes.push_back(lv[down ? s + 1 : s].node);
    if (down)
      for (int t = spanVertStart[s] + spanVertCnt[s] - 1; t >= spanVertStart[s]; t--)
        faceNodes.push_back(t);
    else
      for (int t = spanVertStart[s], te = t + spanVertCnt[s]; t < te; t++)
        faceNodes.push_back(t);
    return down ? s : s + 1;
  };

  const auto walkFace = [&](int sx, bool seedDown) {
    const int c = seedDown ? lv[sx].k : lv[sx].k - 1;
    faceNodes.clear();
    int guard = 0;
    bool ok = true, closed = false, pinch = false;
    int j = traverseChord(sx, seedDown);
    while (ok && !closed) // -V560
    {
      int runEdge = -1;
      if (!pinch && edgeCol[lv[j].node] == c && !consumed[lv[j].node])
        runEdge = lv[j].node;
      pinch = false;
      if (runEdge >= 0)
      {
        int cur = runEdge;
        while (true)
        {
          if (FRX_CHECK_FAILURE(consumed[cur] || ++guard > guardMax, "fill_stripes: ring run of col %d broke at edge %d (consumed=%d)",
                c, cur, int(consumed[cur])))
          {
            ok = false;
            break;
          }
          consumed[cur] = 1;
          faceNodes.push_back(ring[cur]);
          cur = (cur + 1) % RN;
          if (edgeCol[cur] != c)
            break;
          const int jj = mIdx[cur];
          if (jj >= 0 && (lv[jj].k == c || lv[jj].k == c + 1) && chordAt(jj, lv[jj].k == c) >= 0)
          {
            pinch = true;
            break;
          }
        }
        if (!ok)
          break;
        j = mIdx[cur];
        if (FRX_CHECK_FAILURE(j < 0 || (lv[j].k != c && lv[j].k != c + 1),
              "fill_stripes: run of col %d stopped at node %d (%f,%f) not on its lines", c, cur, P[cur].x, P[cur].y))
        {
          ok = false;
          break;
        }
        continue;
      }
      const bool down = lv[j].k == c;
      const int s = chordAt(j, down);
      if (s == sx && down == seedDown)
      {
        closed = true;
        break;
      }
      if (DAGOR_UNLIKELY(s < 0))
      {
        FRX_LOG_FAILURE("fill_stripes stuck: col=%d seed=%d seedDown=%d down=%d cell=%.7f", c, sx, int(seedDown), int(down), cell);
        for (int q = eastl::max(0, j - 2); q <= eastl::min(MN - 1, j + 2); q++)
          if (lv[q].k == lv[j].k)
            FRX_LOG_FAILURE("fill_stripes stuck vert[%d]%s: node=%d handle=%d pos=(%.7f,%.7f) bitsAbove=%d", q, q == j ? " <-cur" : "",
              lv[q].node, H[lv[q].node], P[lv[q].node].x, P[lv[q].node].y, int(spanBits[q]));
        FRX_LOG_FAILURE("fill_stripes stuck: in-edge col=%d, out-edge col=%d, consumed=%d", edgeCol[(lv[j].node - 1 + RN) % RN],
          edgeCol[lv[j].node], int(consumed[lv[j].node]));
        ok = false;
        break;
      }
      if (FRX_CHECK_FAILURE(++guard > guardMax, "fill_stripes: face walk of col %d exceeded guard at vert %d", c, lv[j].node))
      {
        ok = false;
        break;
      }
      j = traverseChord(s, down);
    }
    if (ok && closed)
      emitFace();
  };

  bool anyChord = false;
  for (int s = 0; s < MN; s++)
    anyChord |= spanBits[s] == 3;
  if (!anyChord)
  {
    FRX_CHECK_FAILURE(anyTransition, "fill_stripes: ring spans multiple columns but produced no chords");
    faceNodes = ring;
    emitFace();
  }
  else
  {
    for (int s = 0; s < MN; s++)
    {
      if (spanBits[s] != 3)
        continue;
      if (!(chordDone[s] & 1))
        walkFace(s, true);
      if (!(chordDone[s] & 2))
        walkFace(s, false);
    }
#if FRX_VERIFY_LOG
    for (int i = 0; i < RN; i++)
      if (!consumed[i])
        FRX_LOG_FAILURE("fill_stripes: ring edge %d (col %d, node %d->%d) not consumed by any face", i, edgeCol[i], i, (i + 1) % RN);
#endif
  }

  if (DAGOR_UNLIKELY(ctx.dbgDraw.drawBoundaryFill))
    for (int i = 0; i < RN; i++)
      ctx.dbgDraw.drawArrow(basis.unProject(P[ring[i]], ctx.dbgDraw.heightScale * graph.vertsH[H[ring[i]]]),
        basis.unProject(P[ring[(i + 1) % RN]], ctx.dbgDraw.heightScale * graph.vertsH[H[ring[(i + 1) % RN]]]),
        E3DCOLOR_MAKE(0, 255, 255, 255));
}

} // namespace frx