// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daFracture/core/destrMesh.h>
#include "cutFaceGraph.h"
#include "cutFaceBoundarySearch.h"


namespace frx
{

// --------------------------------------------------------------------------------------------
// TRIANGULATION
// --------------------------------------------------------------------------------------------

template <typename EmitFn>
DAGOR_NOINLINE static void earcut_boundary_with_holes(const DestrContext &ctx, const CutFaceGraph &graph, const PlaneBasis &basis,
  dag::ConstSpan<BoundaryLoop> boundaries, int outerIdx, EmitFn &&emit_triangle)
{
  const BoundaryLoop &outer = boundaries[outerIdx];
  G_ASSERT(outer.verts.size() >= 3 && outer.area2 > 0.f);

  const E3DCOLOR colOuter = E3DCOLOR_MAKE(0, 255, 255, 255);  // cyan
  const E3DCOLOR colHole = E3DCOLOR_MAKE(0, 255, 0, 255);     // green
  const E3DCOLOR colBridge = E3DCOLOR_MAKE(255, 255, 0, 255); // yellow

  if (ctx.dbgDraw.drawBoundaryFill)
  {
    for (int i = 0, n = int(outer.verts.size()); i < n; i++)
      ctx.dbgDraw.drawArrow(basis.unProject(graph.verts[outer.verts[i]]), basis.unProject(graph.verts[outer.verts[(i + 1) % n]]),
        colOuter);
  }

  struct Node
  {
    int vertIdx;
    int prev, next;
    bool reflex;
  };
  dag::Vector<Node, framemem_allocator> nodes;

  // Pre-collect direct hole children (parentId == outerIdx, area2 < 0).
  dag::Vector<int, framemem_allocator> holeIds;
  size_t holeVertsTotal = 0;
  for (int i = 0; i < int(boundaries.size()); i++)
    if (boundaries[i].parentId == outerIdx && boundaries[i].area2 < 0.f && boundaries[i].verts.size() >= 3)
    {
      holeIds.push_back(i);
      holeVertsTotal += boundaries[i].verts.size();
    }
  nodes.reserve(outer.verts.size() + holeVertsTotal + 2 * holeIds.size());

  // (b-a) x (c-b)
  const auto crossN = [](Point2 a, Point2 b, Point2 c) { return (b.x - a.x) * (c.y - b.y) - (b.y - a.y) * (c.x - b.x); };
  // p inside CCW triangle (a,b,c), boundary inclusive.
  const auto pointInTri = [&](Point2 a, Point2 b, Point2 c, Point2 p) {
    return crossN(a, b, p) >= 0.f && crossN(b, c, p) >= 0.f && crossN(c, a, p) >= 0.f;
  };
  const auto reclassify = [&](int k) {
    Point2 a = graph.verts[nodes[nodes[k].prev].vertIdx];
    Point2 b = graph.verts[nodes[k].vertIdx];
    Point2 c = graph.verts[nodes[nodes[k].next].vertIdx];
    nodes[k].reflex = crossN(a, b, c) < 0.f;
  };

  const int outerStart = 0;
  for (int i = 0, n = int(outer.verts.size()); i < n; i++)
    nodes.push_back({outer.verts[i], (i - 1 + n) % n, (i + 1) % n, false});

  // Splice holes from rightmost first so earlier splices don't push later bridge candidates out of view.
  dag::Vector<float, framemem_allocator> holeMaxX(holeIds.size(), -FLT_MAX);
  for (int i = 0; i < int(holeIds.size()); i++)
    for (int v : boundaries[holeIds[i]].verts)
      inplace_max(holeMaxX[i], graph.verts[v].x);
  dag::Vector<int, framemem_allocator> holeOrder(holeIds.size());
  eastl::iota(holeOrder.begin(), holeOrder.end(), 0);
  eastl::sort(holeOrder.begin(), holeOrder.end(), [&](int a, int b) { return holeMaxX[a] > holeMaxX[b]; });

  for (int hOrd : holeOrder)
  {
    const auto &holeVerts = boundaries[holeIds[hOrd]].verts;
    const int holeN = int(holeVerts.size());
    const int holeStart = int(nodes.size());
    for (int i = 0; i < holeN; i++)
      nodes.push_back({holeVerts[i], holeStart + (i - 1 + holeN) % holeN, holeStart + (i + 1) % holeN, false});

    // hole boundary in green
    if (ctx.dbgDraw.drawBoundaryFill)
    {
      for (int i = 0; i < holeN; i++)
        ctx.dbgDraw.drawArrow(basis.unProject(graph.verts[holeVerts[i]]), basis.unProject(graph.verts[holeVerts[(i + 1) % holeN]]),
          colHole);
    }

    // Prefer fusing at a shared graph vertex with the current merged ring (avoids a degenerate zero-length bridge).
    int M = -1, H = -1;
    for (int i = 0; i < holeN && M < 0; i++)
    {
      const int hVi = holeVerts[i];
      int n = outerStart;
      do
      {
        if (nodes[n].vertIdx == hVi)
        {
          M = n;
          H = holeStart + i;
          break;
        }
        n = nodes[n].next;
      } while (n != outerStart);
    }
    const bool sharedFuse = (M >= 0);

    if (!sharedFuse)
    {
      // Bridge anchor = rightmost hole vertex.
      H = holeStart;
      for (int i = 1; i < holeN; i++)
        if (graph.verts[nodes[holeStart + i].vertIdx].x > graph.verts[nodes[H].vertIdx].x)
          H = holeStart + i;
      Point2 hp = graph.verts[nodes[H].vertIdx];
      const float hx = hp.x, hy = hp.y;

      // Cast horizontal ray rightward from H; pick the right endpoint of the closest straddling outer edge.
      float qx = FLT_MAX;
      int n = outerStart;
      do
      {
        Point2 a = graph.verts[nodes[n].vertIdx];
        Point2 b = graph.verts[nodes[nodes[n].next].vertIdx];
        if ((hy >= a.y) != (hy >= b.y))
        {
          const float x = a.x + (hy - a.y) / (b.y - a.y) * (b.x - a.x);
          if (x >= hx && x < qx)
          {
            qx = x;
            M = a.x < b.x ? n : nodes[n].next;
          }
        }
        n = nodes[n].next;
      } while (n != outerStart);
      if (M < 0)
        continue; // hole unreachable from ray; skip

      // Refine: any reflex outer vertex inside the visibility triangle (hp, M, (qx,hy)) blocks the
      // bridge. Pick the candidate with smallest tan(angle) to the ray.
      Point2 mp = graph.verts[nodes[M].vertIdx];
      float tanMin = FLT_MAX;
      const int stop = M;
      n = M;
      do
      {
        Point2 np = graph.verts[nodes[n].vertIdx];
        if (n != M && hx >= np.x && np.x >= mp.x && hx != np.x)
        {
          Point2 qhp(qx, hy);
          Point2 a, b, c;
          if (hy < mp.y)
          {
            a = hp;
            b = mp;
            c = qhp;
          }
          else
          {
            a = qhp;
            b = mp;
            c = hp;
          }
          if (pointInTri(a, b, c, np))
          {
            const float tang = fabsf(hy - np.y) / (hx - np.x);
            if (tang < tanMin)
            {
              M = n;
              mp = np;
              tanMin = tang;
            }
          }
        }
        n = nodes[n].next;
      } while (n != stop);
    }

    // bridge in yellow (only meaningful for non-shared splice — shared fuses are zero-length)
    if (ctx.dbgDraw.drawBoundaryFill)
    {
      if (!sharedFuse)
        ctx.dbgDraw.drawArrow(basis.unProject(graph.verts[nodes[M].vertIdx]), basis.unProject(graph.verts[nodes[H].vertIdx]),
          colBridge);
    }

    // Splice. outerStart stays in the merged ring either way.
    if (sharedFuse)
    {
      // Drop H (same vertex as M); link ... M -> H_next -> ... -> H_prev -> M_dup -> M_next ...
      const int H_prev = nodes[H].prev;
      const int H_next = nodes[H].next;
      const int M_next = nodes[M].next;
      const int M_dup = int(nodes.size());
      nodes.push_back({nodes[M].vertIdx, H_prev, M_next, false});
      nodes[M].next = H_next;
      nodes[H_next].prev = M;
      nodes[H_prev].next = M_dup;
      nodes[M_next].prev = M_dup;
    }
    else
    {
      // Standard bridge: ... M -> H -> H_next -> ... -> H_prev -> H_dup -> M_dup -> M_next ...
      const int H_prev = nodes[H].prev;
      const int M_next = nodes[M].next;
      const int H_dup = int(nodes.size());
      nodes.push_back({nodes[H].vertIdx, H_prev, -1, false});
      const int M_dup = int(nodes.size());
      nodes.push_back({nodes[M].vertIdx, H_dup, M_next, false});
      nodes[H_dup].next = M_dup;
      nodes[M].next = H;
      nodes[H].prev = M;
      nodes[H_prev].next = H_dup;
      nodes[M_next].prev = M_dup;
    }
  }

  // Classify reflex on the merged ring.
  int alive = 0;
  {
    int n = outerStart;
    do
    {
      reclassify(n);
      n = nodes[n].next;
      alive++;
    } while (n != outerStart);
  }

  int cur = outerStart;
  int stuck = 0;
  while (alive > 3 && stuck < alive * 2)
  {
    const int p = nodes[cur].prev;
    const int nx = nodes[cur].next;
    bool isEar = false;
    if (!nodes[cur].reflex)
    {
      Point2 a = graph.verts[nodes[p].vertIdx];
      Point2 b = graph.verts[nodes[cur].vertIdx];
      Point2 c = graph.verts[nodes[nx].vertIdx];
      if (crossN(a, b, c) > 0.f)
      {
        isEar = true;
        for (int k = nodes[nx].next; k != p; k = nodes[k].next)
        {
          if (!nodes[k].reflex)
            continue;
          // shared-vertex defense: a vertex coincident with a triangle corner shouldn't disqualify the ear
          if (nodes[k].vertIdx == nodes[p].vertIdx || nodes[k].vertIdx == nodes[cur].vertIdx || nodes[k].vertIdx == nodes[nx].vertIdx)
            continue;
          if (pointInTri(a, b, c, graph.verts[nodes[k].vertIdx]))
          {
            isEar = false;
            break;
          }
        }
      }
    }
    if (isEar)
    {
      emit_triangle(nodes[p].vertIdx, nodes[cur].vertIdx, nodes[nx].vertIdx);
      nodes[p].next = nx;
      nodes[nx].prev = p;
      reclassify(p);
      reclassify(nx);
      alive--;
      cur = nx;
      stuck = 0;
    }
    else
    {
      cur = nx;
      stuck++;
    }
  }
  if (alive == 3)
    emit_triangle(nodes[nodes[cur].prev].vertIdx, nodes[cur].vertIdx, nodes[nodes[cur].next].vertIdx);
}

} // namespace frx
