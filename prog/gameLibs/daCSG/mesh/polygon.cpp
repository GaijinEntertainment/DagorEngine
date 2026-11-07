// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "mesh/polygon.h"
#include <EASTL/sort.h>
#include <float.h>
#include <math.h>

namespace dacsg
{
double orient2(const DPoint2 &a, const DPoint2 &b, const DPoint2 &c) { return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x); }

bool inside_loop(dag::ConstSpan<DPoint2> l, const DPoint2 &p)
{
  bool in = false;
  for (uint32_t i = 0, j = l.size() - 1; i < l.size(); j = i++)
    if ((l[i].y > p.y) != (l[j].y > p.y) && p.x < (l[j].x - l[i].x) * (p.y - l[i].y) / (l[j].y - l[i].y) + l[i].x)
      in = !in;
  return in;
}

// relative to the first point, so a loop far from the origin keeps the precision of its own size
template <typename P>
static double shoelace(dag::ConstSpan<P> pts)
{
  double a = 0;
  for (uint32_t i = 0, j = pts.size() - 1; i < pts.size(); j = i++)
  {
    const double ux = double(pts[j].x) - pts[0].x, uy = double(pts[j].y) - pts[0].y;
    const double vx = double(pts[i].x) - pts[0].x, vy = double(pts[i].y) - pts[0].y;
    a += ux * vy - vx * uy;
  }
  return a * 0.5;
}

double signed_area(dag::ConstSpan<Point2> pts) { return shoelace(pts); }

namespace
{
struct EarNode
{
  DPoint2 p;
  uint32_t idx, prev, next;
};

bool in_triangle(const DPoint2 &a, const DPoint2 &b, const DPoint2 &c, const DPoint2 &p)
{
  const double s0 = orient2(a, b, p), s1 = orient2(b, c, p), s2 = orient2(c, a, p);
  return (s0 >= 0 && s1 >= 0 && s2 >= 0) || (s0 <= 0 && s1 <= 0 && s2 <= 0);
}

// Outer loop with bridged holes, CCW
struct EarRing
{
  dag::Vector<EarNode> n;
  double mirror = 1;

  double turn(uint32_t i) const { return orient2(n[n[i].prev].p, n[i].p, n[n[i].next].p); }

  uint32_t addLoop(dag::ConstSpan<DPoint2> pts, uint32_t base, bool keep_order)
  {
    const uint32_t first = n.size(), cnt = pts.size();
    for (uint32_t k = 0; k < cnt; ++k)
    {
      const uint32_t src = keep_order ? k : cnt - 1 - k;
      const DPoint2 p(pts[src].x * mirror, pts[src].y);
      n.push_back(EarNode{p, base + src, first + (k + cnt - 1) % cnt, first + (k + 1) % cnt});
    }
    return first;
  }

  bool locallyInside(uint32_t r, const DPoint2 &m) const
  {
    const DPoint2 &p = n[r].p, e0 = n[n[r].prev].p - p, e1 = n[n[r].next].p - p, d = m - p;
    const double c1 = e1.x * d.y - e1.y * d.x, c0 = d.x * e0.y - d.y * e0.x;
    return turn(r) > 0 ? c1 >= 0 && c0 >= 0 : c1 >= 0 || c0 >= 0;
  }

  void split(uint32_t a, uint32_t b)
  {
    const EarNode na = n[a], nb = n[b];
    const uint32_t a2 = n.size(), b2 = a2 + 1;
    n.push_back(na);
    n.push_back(nb);
    n[a].next = b;
    n[b].prev = a;
    n[a2].next = na.next;
    n[na.next].prev = a2;
    n[b2].next = a2;
    n[a2].prev = b2;
    n[nb.prev].next = b2;
    n[b2].prev = nb.prev;
  }

  // Eberly's hole bridge, ray toward +x
  bool bridge(uint32_t hole, uint32_t outer)
  {
    uint32_t m = hole;
    for (uint32_t i = n[hole].next; i != hole; i = n[i].next)
      if (n[i].p.x > n[m].p.x || (n[i].p.x == n[m].p.x && n[i].p.y < n[m].p.y))
        m = i;
    const DPoint2 mp = n[m].p;
    double hitX = DBL_MAX;
    uint32_t p = ~0u, i = outer;
    do
    {
      const DPoint2 &a = n[i].p, &b = n[n[i].next].p;
      if (a.y <= mp.y && mp.y <= b.y && a.y != b.y)
      {
        const double x = a.x + (mp.y - a.y) * (b.x - a.x) / (b.y - a.y);
        if (x >= mp.x && x < hitX)
        {
          hitX = x;
          p = mp.y == a.y ? i : mp.y == b.y ? n[i].next : a.x > b.x ? i : n[i].next;
        }
      }
      i = n[i].next;
    } while (i != outer);
    if (p == ~0u)
      return false;

    const DPoint2 hit(hitX, mp.y), pp = n[p].p;
    uint32_t best = p;
    double bestTan = DBL_MAX, bestDist = DBL_MAX;
    do
    {
      const DPoint2 &q = n[i].p;
      if (q == pp || (q.x > mp.x && in_triangle(mp, hit, pp, q)))
      {
        const double tan = q.x > mp.x ? fabs(q.y - mp.y) / (q.x - mp.x) : DBL_MAX, dist = lengthSq(q - mp);
        if (locallyInside(i, mp) && (tan < bestTan || (tan == bestTan && dist < bestDist)))
        {
          best = i;
          bestTan = tan;
          bestDist = dist;
        }
      }
      i = n[i].next;
    } while (i != outer);
    // a hole that touches the loop at that vertex joins the ring there: a bridge of no length would stall the clip
    if (n[best].p == mp)
    {
      const uint32_t bestNext = n[best].next;
      n[best].next = n[m].next;
      n[n[m].next].prev = best;
      n[m].next = bestNext;
      n[bestNext].prev = m;
      return true;
    }
    split(best, m);
    return true;
  }

  // a corner within 1e-9 rad of straight is no ear: its triangle would be flat
  bool isEar(uint32_t b) const
  {
    const uint32_t a = n[b].prev, c = n[b].next;
    const DPoint2 &pa = n[a].p, &pb = n[b].p, &pc = n[c].p;
    if (orient2(pa, pb, pc) <= 1e-9 * length(pa - pb) * length(pc - pb))
      return false;
    for (uint32_t i = n[c].next; i != a; i = n[i].next)
    {
      const DPoint2 &q = n[i].p;
      // a corner's copy from a bridge, or a second visit of a corner's point, blocks no ear
      const bool copy = n[i].idx == n[a].idx || n[i].idx == n[b].idx || n[i].idx == n[c].idx || q == pa || q == pb || q == pc;
      if (!copy && orient2(pa, pb, q) >= 0 && orient2(pb, pc, q) >= 0 && orient2(pc, pa, q) >= 0)
        return false;
    }
    return true;
  }

  // A zero-area loop clips to flat triangles, which the caller's simplify removes
  uint32_t flattest(uint32_t cur) const
  {
    uint32_t best = cur;
    double bestRatio = DBL_MAX;
    for (uint32_t i = cur;;)
    {
      const DPoint2 &p = n[i].p;
      const double scale = length(n[n[i].prev].p - p) * length(n[n[i].next].p - p);
      const double ratio = scale > 0 ? fabs(turn(i)) / scale : 0;
      if (ratio < bestRatio)
      {
        bestRatio = ratio;
        best = i;
      }
      i = n[i].next;
      if (i == cur)
        return best;
    }
  }

  // twice the signed area of the ring at cur, relative to its point
  double ringArea2(uint32_t cur) const
  {
    const DPoint2 &o = n[cur].p;
    double a = 0;
    for (uint32_t i = cur;;)
    {
      const DPoint2 p = n[i].p - o, q = n[n[i].next].p - o;
      a += p.x * q.y - q.x * p.y;
      i = n[i].next;
      if (i == cur)
        return a;
    }
  }

  // An ear that leaves a flat rest is taken only when nothing else is left: a vertex on a straight run then
  // joins a vertex off the run instead of making a zero-area triangle
  bool clip(uint32_t cur, uint32_t count, dag::Vector<uint32_t> &tris, bool force)
  {
    tris.reserve((count - 2) * 3);
    double left = ringArea2(cur);
    const double flatBound = 1e-9 * fabs(left);
    for (uint32_t stall = 0; count > 3;)
    {
      const bool ear = isEar(cur) && left - turn(cur) > flatBound;
      if (!ear && stall <= count)
      {
        ++stall;
        cur = n[cur].next;
        continue;
      }
      if (!ear && !force)
        return false;
      if (!ear)
        cur = flattest(cur);
      const uint32_t a = n[cur].prev, c = n[cur].next;
      const uint32_t t[3] = {n[a].idx, n[cur].idx, n[c].idx};
      tris.insert(tris.end(), t, t + 3);
      left -= turn(cur);
      n[a].next = c;
      n[c].prev = a;
      --count;
      // skipping one vertex avoids sliver fans
      cur = n[c].next;
      stall = 0;
    }
    if (turn(cur) <= 0 && !force)
      return false;
    const uint32_t t[3] = {n[n[cur].prev].idx, n[cur].idx, n[n[cur].next].idx};
    tris.insert(tris.end(), t, t + 3);
    return true;
  }
};

void to_double(dag::ConstSpan<Point2> src, dag::Vector<DPoint2> &dst)
{
  dst.resize(src.size());
  for (uint32_t i = 0; i < src.size(); ++i)
    dst[i] = DPoint2(src[i].x, src[i].y);
}
} // namespace

bool triangulate_polygon(dag::ConstSpan<DPoint2> outer, dag::ConstSpan<dag::ConstSpan<DPoint2>> holes, dag::Vector<uint32_t> &tris,
  eastl::string &err, bool force)
{
  tris.clear();
  uint32_t total = outer.size();
  bool tooSmall = outer.size() < 3;
  for (const dag::ConstSpan<DPoint2> &h : holes)
  {
    tooSmall |= h.size() < 3;
    total += h.size() + 2;
  }
  if (tooSmall)
  {
    err = "polygon or hole has fewer than 3 points";
    return false;
  }
  const double outerArea = shoelace(outer);
  if (outerArea == 0 && !force)
  {
    err = "polygon has zero area";
    return false;
  }

  static thread_local EarRing r; // reused per thread: no allocation per call
  r.n.clear();
  r.n.reserve(total);
  // the forced caller gives loops in face order, so a flat loop with a noisy area sign keeps it; a clockwise one
  // clips mirrored, so its ears turn the loop's way; a hole turns against the mirrored loop
  r.mirror = force && outerArea < 0 ? -1 : 1;
  const uint32_t start = r.addLoop(outer, 0, force || outerArea >= 0);
  dag::Vector<eastl::pair<double, uint32_t>> order;
  uint32_t base = outer.size();
  for (const dag::ConstSpan<DPoint2> &h : holes)
  {
    const uint32_t first = r.addLoop(h, base, shoelace(h) * r.mirror < 0);
    // the bridges run toward +x in the mirrored ring, so the hole nearest that side joins first
    double maxX = -DBL_MAX;
    for (const DPoint2 &p : h)
      maxX = eastl::max(maxX, p.x * r.mirror);
    order.push_back(eastl::make_pair(maxX, first));
    base += h.size();
  }
  eastl::sort(order.begin(), order.end(), [](const auto &a, const auto &b) { return a.first > b.first; });
  for (const auto &o : order)
    if (!r.bridge(o.second, start))
    {
      err = "a hole is outside the polygon";
      return false;
    }
  if (!r.clip(start, r.n.size(), tris, force))
  {
    tris.clear();
    err = "polygon crosses itself or is degenerate";
    return false;
  }
  return true;
}

bool triangulate_polygon(dag::ConstSpan<Point2> outer, dag::ConstSpan<dag::ConstSpan<Point2>> holes, dag::Vector<uint32_t> &tris,
  eastl::string &err)
{
  dag::Vector<DPoint2> outerD;
  to_double(outer, outerD);
  dag::Vector<dag::Vector<DPoint2>> holesD(holes.size());
  dag::Vector<dag::ConstSpan<DPoint2>> spans(holes.size());
  for (uint32_t i = 0; i < holes.size(); ++i)
  {
    to_double(holes[i], holesD[i]);
    spans[i] = make_span_const(holesD[i]);
  }
  return triangulate_polygon(make_span_const(outerD), make_span_const(spans), tris, err);
}
} // namespace dacsg
