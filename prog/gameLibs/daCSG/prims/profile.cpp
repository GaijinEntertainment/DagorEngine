// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "mesh/bvh.h"
#include "prims/prims.h"
#include <EASTL/algorithm.h>
#include <float.h>
#include <math.h>

namespace dacsg
{
static bool on_segment(const Point2 &a, const Point2 &b, const Point2 &p)
{
  return orient2(dpoint2(a), dpoint2(b), dpoint2(p)) == 0 && eastl::min(a.x, b.x) <= p.x && p.x <= eastl::max(a.x, b.x) &&
         eastl::min(a.y, b.y) <= p.y && p.y <= eastl::max(a.y, b.y);
}

static bool segments_touch(const Point2 &a, const Point2 &b, const Point2 &c, const Point2 &d)
{
  const double d1 = orient2(dpoint2(c), dpoint2(d), dpoint2(a)), d2 = orient2(dpoint2(c), dpoint2(d), dpoint2(b)),
               d3 = orient2(dpoint2(a), dpoint2(b), dpoint2(c)), d4 = orient2(dpoint2(a), dpoint2(b), dpoint2(d));
  if (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) && ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0)))
    return true;
  return on_segment(c, d, a) || on_segment(c, d, b) || on_segment(a, b, c) || on_segment(a, b, d);
}

static const char *polyline_error(dag::ConstSpan<Point2> p, bool loop)
{
  const uint32_t n = p.size(), segs = loop ? n : n - 1;
  // past the float range fits_float keeps, a float square overflows
  for (const Point2 &q : p)
    if (!check_finite(q.x) || !check_finite(q.y))
      return "a point is not finite";
    else if (!(lengthSq(dpoint2(q)) < 1e36))
      return "a point is beyond float range at this size";
  dag::Vector<bbox3f> boxes(segs);
  for (uint32_t i = 0; i < segs; ++i)
  {
    const Point2 &a = p[i], &b = p[(i + 1) % n];
    if (a == b)
      return "a point repeats";
    boxes[i] = outward_box(DPoint3(min(a.x, b.x), min(a.y, b.y), 0), DPoint3(max(a.x, b.x), max(a.y, b.y), 0));
  }
  // only segments whose boxes meet can touch; a polyline coiled in one area caps the walk, as a tube path does
  Bvh bvh;
  bvh.build(boxes);
  dag::Vector<TriPair> pairs;
  const uint32_t most = 64 * segs + 65536;
  bvh.selfOverlapsUnsorted(pairs, most);
  if (pairs.size() > most)
    return "its segment boxes overlap in too many pairs: split it";
  for (const TriPair &q : pairs)
  {
    const uint32_t i = q.first, j = q.second;
    const Point2 &a = p[i], &b = p[(i + 1) % n], &c = p[j], &d = p[(j + 1) % n];
    if (j == i + 1 || (loop && i == 0 && j == n - 1))
    {
      const Point2 &s = j == i + 1 ? b : a, &e0 = j == i + 1 ? a : b, &e1 = j == i + 1 ? d : c;
      if (orient2(dpoint2(s), dpoint2(e0), dpoint2(e1)) == 0 && (e0 - s) * (e1 - s) > 0)
        return "it crosses itself";
    }
    else if (segments_touch(a, b, c, d))
      return "it crosses itself";
  }
  return nullptr;
}

// Creases above 30 deg get disjoint bits
static void smooth_groups(dag::ConstSpan<Point2> p, bool loop, dag::Vector<uint32_t> &smgr)
{
  const uint32_t n = p.size(), segs = loop ? n : n - 1;
  const float cos30 = 0.8660254f;
  auto crease = [&](uint32_t s) {
    const Point2 d0 = p[s] - p[(s + segs - 1) % segs], d1 = p[(s + 1) % n] - p[s];
    return d0 * d1 < cos30 * d0.length() * d1.length();
  };
  smgr.assign(segs, 1);
  uint32_t first = 0;
  while (loop && first < segs && !crease(first))
    ++first;
  if (loop && first == segs)
    return;
  uint32_t runs = 0, lastRun = 0;
  for (uint32_t k = 0; k < segs; ++k)
  {
    const uint32_t s = (first + k) % segs;
    const bool starts = k == 0 || crease(s);
    runs += starts;
    lastRun = starts ? k : lastRun;
    smgr[s] = runs % 2 ? 1 : 2;
  }
  if (!loop || runs % 2 == 0)
    return;
  if (runs > 1)
  {
    for (uint32_t k = lastRun; k < segs; ++k)
      smgr[(first + k) % segs] = 4;
    return;
  }
  // A lone run meets itself: 1.., 3, 2..
  for (uint32_t k = 0; k < segs; ++k)
    smgr[(first + k) % segs] = k * 2 + 1 == segs || k * 2 == segs ? 3 : k * 2 < segs ? 1 : 2;
}

static void lengths(dag::ConstSpan<Point2> p, bool loop, dag::Vector<float> &frac)
{
  const uint32_t n = p.size();
  dag::Vector<double> acc(n + 1, 0.0);
  for (uint32_t i = 0; i < n; ++i)
    acc[i + 1] = acc[i] + (i + 1 < n || loop ? length(p[(i + 1) % n] - p[i]) : 0);
  frac.resize(n + 1);
  // a profile of no length, which fits_float refuses: no 0 / 0
  const double total = acc[loop ? n : n - 1];
  for (uint32_t i = 0; i <= n; ++i)
    frac[i] = total >= DBL_MIN ? float(acc[i] / total) : 0;
}

static void make_ccw(dag::Vector<Point2> &p, bool seam_first)
{
  if (signed_area(make_span_const(p)) < 0)
    eastl::reverse(p.begin() + (seam_first ? 1 : 0), p.end());
}

bool make_extrude(const PrimArgs &a, PrimBuilder &b, eastl::string &err)
{
  const float h = a.d.p[PrimDesc::EXTRUDE_HEIGHT];
  if (!positive(h))
    return fail(err, "height must be positive");
  dag::Vector<Point2> p(a.g.points2(a.d.points).begin(), a.g.points2(a.d.points).end());
  if (p.size() < 3)
    return fail(err, "polygon has fewer than 3 points");
  if (const char *why = polyline_error(make_span_const(p), true))
    return fail(err, "polygon is invalid: %s", why);
  make_ccw(p, true);
  const uint32_t n = p.size(), first = b.m.pos.size();
  for (float y : {-0.5f * h, 0.5f * h})
    for (const Point2 &q : p)
      b.vert(Point3(q.x, y, q.y));

  dag::Vector<float> u;
  dag::Vector<uint32_t> smgr;
  lengths(make_span_const(p), true, u);
  smooth_groups(make_span_const(p), true, smgr);
  auto side = [&](uint32_t i, uint32_t other) {
    const Point2 s = segment_normal(p[i], p[(i + 1) % n]);
    const Point2 v = smgr[i] & smgr[other] ? normalize(s + segment_normal(p[other], p[(other + 1) % n])) : s;
    return Point3(v.x, 0, v.y);
  };
  for (uint32_t i = 0; i < n; ++i)
  {
    const uint32_t j = (i + 1) % n;
    const Point3 ni = side(i, (i + n - 1) % n), nj = side(i, j);
    b.quad(first + i, first + n + i, first + n + j, first + j, Point2(u[i], 0), Point2(u[i], 1), Point2(u[i + 1], 1),
      Point2(u[i + 1], 0), smgr[i], {ni, ni, nj, nj});
  }

  dag::Vector<uint32_t> tris;
  if (!triangulate_polygon(make_span_const(p), {}, tris, err))
    return false;
  for (int top = 0; top < 2; ++top)
  {
    const uint32_t base = first + top * n;
    b.cap(Point3(0, top ? 1.f : -1.f, 0), n, make_span_const(tris), top, [&](uint32_t k) { return base + k; });
  }
  return true;
}

bool make_revolve(const PrimArgs &a, PrimBuilder &b, eastl::string &err)
{
  const float angle = a.d.angleDeg;
  if (!check_finite(angle) || angle <= 0 || angle > 360 * (1 + 1e-6f))
    return fail(err, "angle must be above 0 and at most 360");
  dag::Vector<Point2> p(a.g.points2(a.d.points).begin(), a.g.points2(a.d.points).end());
  if (p.size() < 3)
    return fail(err, "profile has fewer than 3 points");
  // the snap to the axis scales with the radius the axis test reads, not with the height, and stays within the
  // tolerance's float floor there (8 steps, as auto_tolerance), so a closed profile past it keeps its hole
  float extent = 0;
  for (const Point2 &q : p)
    extent = eastl::max(extent, fabsf(q.x));
  const float snap = 8 * (nextafterf(extent, FLT_MAX) - extent);
  for (Point2 &q : p)
    if (fabsf(q.x) <= snap)
      q.x = 0;
  const bool loop = p.front().x != 0 || p.back().x != 0;
  for (uint32_t i = 0; i < p.size(); ++i)
  {
    const bool end = i == 0 || i + 1 == p.size();
    if (p[i].x < 0 || (p[i].x == 0 && (loop || !end)))
      return fail(err, loop ? "profile crosses or touches the axis; an open profile starts and ends on it"
                            : "profile crosses or touches the axis between its ends");
  }
  if (!loop && p.front().y == p.back().y)
    return fail(err, "profile ends meet on the axis");
  if (const char *why = polyline_error(make_span_const(p), loop))
    return fail(err, "profile is invalid: %s", why);
  make_ccw(p, loop);

  dag::Vector<float> v;
  dag::Vector<uint32_t> smgr;
  lengths(make_span_const(p), loop, v);
  smooth_groups(make_span_const(p), loop, smgr);
  dag::Vector<ProfilePoint> prof(p.size());
  for (uint32_t i = 0; i < p.size(); ++i)
    prof[i] = ProfilePoint{p[i], v[i], i < smgr.size() ? smgr[i] : 0, false};
  // an angle within the tolerance of 360, from either side, is a full turn: no pair of caps a step apart
  const bool full = angle >= 360 * (1 - 1e-6f);
  const uint32_t segs = full ? a.seg : eastl::max(1u, uint32_t(ceilf(a.seg * angle / 360)));
  dag::Vector<Ring> rings;
  lathe(b, make_span_const(prof), loop, segs, full ? 360 : angle, &rings);
  if (full)
    return true;

  dag::Vector<uint32_t> tris;
  if (!triangulate_polygon(make_span_const(p), {}, tris, err))
    return false;
  const Point2 end = unit_dir(-angle);
  const Point3 normals[2] = {Point3(end.y, 0, -end.x), Point3(0, 0, 1)};
  for (int side = 0; side < 2; ++side)
    b.cap(normals[side], rings.size(), make_span_const(tris), !side,
      [&](uint32_t k) { return rings[k].count == 1 ? rings[k].first : rings[k].first + (side ? segs : 0); });
  return true;
}

static double segment_distance(const DPoint3 &p0, const DPoint3 &p1, const DPoint3 &q0, const DPoint3 &q1)
{
  const DPoint3 d1 = p1 - p0, d2 = q1 - q0, r = p0 - q0;
  // the cross product keeps the denominator of near-parallel runs, which a * e - bb * bb cancels to zero
  const double a = d1 * d1, e = d2 * d2, f = d2 * r, c = d1 * r, bb = d1 * d2, den = lengthSq(d1 % d2);
  // any s fits parallel runs, as t then clamps and s follows
  double s = den > 0 ? eastl::clamp((bb * f - c * e) / den, 0.0, 1.0) : 0;
  double t = (bb * s + f) / e;
  if (t < 0 || t > 1)
  {
    t = eastl::clamp(t, 0.0, 1.0);
    s = eastl::clamp((t * bb - c) / a, 0.0, 1.0);
  }
  return length(p0 + d1 * s - q0 - d2 * t);
}

static DPoint3 rotate(const DPoint3 &v, const DPoint3 &axis, double c, double s)
{
  return v * c + (axis % v) * s + axis * ((axis * v) * (1 - c));
}

struct TubeRing
{
  DPoint3 center, n, t;
  double len;
};

bool make_tube(const PrimArgs &a, PrimBuilder &b, eastl::string &err)
{
  const float r = a.d.p[PrimDesc::RADIUS];
  const dag::ConstSpan<Point3> path = a.g.points3(a.d.points);
  if (!positive(r))
    return fail(err, "radius must be positive");
  if (path.size() < 2)
    return fail(err, "path has fewer than 2 points");
  for (const Point3 &q : path)
    if (!check_finite(q.x) || !check_finite(q.y) || !check_finite(q.z))
      return fail(err, "a path point is not finite");
  const uint32_t n = path.size();
  dag::Vector<DPoint3> d(n - 1);
  dag::Vector<double> len(n - 1);
  dag::Vector<bbox3f> boxes(n - 1);
  for (uint32_t i = 0; i + 1 < n; ++i)
  {
    len[i] = length(DPoint3(path[i + 1]) - DPoint3(path[i]));
    if (len[i] == 0)
      return fail(err, "a path point repeats");
    d[i] = (DPoint3(path[i + 1]) - DPoint3(path[i])) / len[i];
    const DPoint3 p0(path[i]), p1(path[i + 1]), grow(r, r, r);
    boxes[i] = outward_box(min(p0, p1) - grow, max(p0, p1) + grow);
  }
  // runs closer than 2 r have boxes grown by r that overlap
  Bvh bvh;
  bvh.build(boxes);
  dag::Vector<TriPair> pairs;
  // a path coiled in one volume overlaps in every pair of boxes: the walk is capped as validate caps the face walk
  const uint32_t most = 64 * (n - 1) + 65536;
  bvh.selfOverlapsUnsorted(pairs, most);
  if (pairs.size() > most)
    return fail(err, "path has over %u overlapping segment pairs: split it in the model", most);
  for (const TriPair &q : pairs)
    if (q.second > q.first + 1 && segment_distance(DPoint3(path[q.first]), DPoint3(path[q.first + 1]), DPoint3(path[q.second]),
                                    DPoint3(path[q.second + 1])) < 2.0 * r)
      return fail(err, "path overlaps itself: split it in the model");

  // Bend radius 2r unless segments are short
  dag::Vector<double> turn(n, 0.0), tangent(n, 0.0);
  for (uint32_t i = 1; i + 1 < n; ++i)
  {
    turn[i] = acos(eastl::clamp(d[i - 1] * d[i], -1.0, 1.0));
    if (turn[i] > PI_D - 1e-6)
      return fail(err, "path turns back on itself");
    // collinear segments are no bend, though their dot can round below 1; smaller turns add up along a path. A turn whose
    // dot rounds to 1 is none either: its tangent would be 0 / 0
    if (!(lengthSq(d[i - 1] % d[i]) > 0) || turn[i] == 0)
      continue;
    const double room = eastl::min(i > 1 ? len[i - 1] / 2 : len[i - 1], i + 2 < n ? len[i] / 2 : len[i]);
    tangent[i] = eastl::min(2.0 * r * tan(turn[i] / 2), room);
    if (tangent[i] / tan(turn[i] / 2) <= r)
      return fail(err, "bend radius is not above r at path point %u", i);
  }

  DPoint3 n0 = d[0] % DPoint3(0, 1, 0);
  if (lengthSq(n0) < 0.01)
    n0 = d[0] % DPoint3(1, 0, 0);
  dag::Vector<TubeRing> rings;
  rings.push_back(TubeRing{DPoint3(path[0]), normalize(n0), d[0], 0});
  // a ring within the float floor of the last one (the guard after the builder) is that ring, not a refusal
  auto add = [&](const DPoint3 &c, const DPoint3 &nn, const DPoint3 &t, double step) {
    if (step <= 8 * FLT_EPSILON * (length(c) + r))
      return;
    rings.push_back(TubeRing{c, nn, t, rings.back().len + step});
  };
  for (uint32_t i = 1; i + 1 < n; ++i)
  {
    if (tangent[i] == 0)
      continue;
    const DPoint3 start = DPoint3(path[i]) - d[i - 1] * tangent[i];
    add(start, rings.back().n, d[i - 1], length(start - rings.back().center));
    const DPoint3 axis = normalize(d[i - 1] % d[i]), toCenter = axis % d[i - 1];
    const double bend = tangent[i] / tan(turn[i] / 2);
    const DPoint3 center = start + toCenter * bend, nStart = rings.back().n;
    const uint32_t steps = eastl::max(1u, uint32_t(ceil(turn[i] * a.seg / PI_D)));
    for (uint32_t k = 1; k <= steps; ++k)
    {
      const double ang = turn[i] * k / steps, c = cos(ang), s = sin(ang);
      add(center + rotate(start - center, axis, c, s), rotate(nStart, axis, c, s), rotate(d[i - 1], axis, c, s),
        bend * turn[i] / steps);
    }
  }
  add(DPoint3(path[n - 1]), rings.back().n, d[n - 2], length(DPoint3(path[n - 1]) - rings.back().center));
  if (rings.size() < 2)
    return fail(err, "the path is shorter than float precision");

  const double total = rings.back().len;
  dag::Vector<Point2> dir(a.seg);
  for (uint32_t j = 0; j < a.seg; ++j)
    dir[j] = unit_dir(360.0 * j / a.seg);
  dag::Vector<Ring> ring(rings.size());
  for (uint32_t k = 0; k < rings.size(); ++k)
  {
    const TubeRing &tr = rings[k];
    const DPoint3 bin = tr.t % tr.n;
    ring[k] = Ring{b.m.pos.size(), a.seg, float(tr.len / total)};
    for (const Point2 &q : dir)
      b.vert(Point3(tr.center + (tr.n * q.x - bin * q.y) * r), Point3(normalize(tr.n * q.x - bin * q.y)));
  }
  for (uint32_t k = 0; k + 1 < rings.size(); ++k)
    b.band(ring[k], ring[k + 1], a.seg, SMOOTH);
  for (int end = 0; end < 2; ++end)
  {
    const TubeRing &tr = end ? rings.back() : rings.front();
    const Ring &edge = end ? ring.back() : ring.front();
    Chart chart(Point3(end ? tr.t : -tr.t));
    for (uint32_t j = 0; j < a.seg; ++j)
      chart.add(b.m.pos[edge.first + j]);
    const Ring pole{b.vert(Point3(tr.center)), 1, 0};
    if (end)
      b.band(edge, pole, a.seg, 0, &chart);
    else
      b.band(pole, edge, a.seg, 0, &chart);
  }
  return true;
}

// Seam corners: k == sections, i == n
struct LoftCorner
{
  uint32_t k, i;
};

bool make_loft(const PrimArgs &a, PrimBuilder &b, eastl::string &err)
{
  const dag::ConstSpan<Point3> pts = a.g.points3(a.d.points);
  const float sectionPoints = a.d.p[PrimDesc::LOFT_SECTION_POINTS];
  const bool ring = a.d.p[PrimDesc::LOFT_CLOSED] != 0;
  if (!(sectionPoints >= 3))
    return fail(err, "a section has fewer than 3 points");
  if (!(sectionPoints * (ring ? 3 : 2) <= pts.size()))
    return fail(err, ring ? "a closed loft has fewer than 3 sections" : "a loft has fewer than 2 sections");
  const uint32_t n = uint32_t(sectionPoints), sections = pts.size() / n, bands = ring ? sections : sections - 1;
  if (sections * n != pts.size())
    return fail(err, "the point count is not a multiple of the section size");
  for (const Point3 &q : pts)
    if (!check_finite(q.x) || !check_finite(q.y) || !check_finite(q.z))
      return fail(err, "a point is not finite");

  auto pos = [&](uint32_t k, uint32_t i) { return DPoint3(pts[(k % sections) * n + i % n]); };
  dag::Vector<DPoint3> center(sections + 1, DPoint3(0, 0, 0));
  for (uint32_t k = 0; k < sections; ++k)
    for (uint32_t i = 0; i < n; ++i)
    {
      if (pos(k, i) == pos(k, i + 1))
        return fail(err, "section %u: a point repeats", k);
      center[k] += pos(k, i) / n;
    }
  center[sections] = center[0];
  const DPoint3 p0 = pos(0, 0);
  DPoint3 e1(0, 0, 0), normal(0, 0, 0);
  for (const Point3 &q : pts)
    if (lengthSq(DPoint3(q) - p0) > lengthSq(e1))
      e1 = DPoint3(q) - p0;
  for (const Point3 &q : pts)
    if (lengthSq((DPoint3(q) - p0) % e1) > lengthSq(normal))
      normal = (DPoint3(q) - p0) % e1;
  // coplanar unless a point leaves the plane by more than the float guard's eight steps at its size, as its side edges must
  double off = -1;
  for (const Point3 &q : pts)
    off = eastl::max(off, fabs((DPoint3(q) - p0) * normal) - 8 * FLT_EPSILON * length(DPoint3(q)) * length(normal));
  if (!(off > 0))
    return fail(err, "the sections are coplanar");
  dag::Vector<double> v(bands + 1, 0.0);
  for (uint32_t k = 0; k < bands; ++k)
    v[k + 1] = v[k] + length(center[k + 1] - center[k]);
  const double vScale = v[bands] > 0 ? 1 / v[bands] : 0;

  // Start cap against the section order
  dag::Vector<LoftCorner> cap[2];
  DPoint3 capNormal[2];
  for (int end = 0; end < 2 && !ring; ++end)
  {
    const uint32_t k = end ? sections - 1 : 0;
    DPoint3 area(0, 0, 0);
    for (uint32_t i = 0; i < n; ++i)
      area += pos(k, i) % pos(k, i + 1);
    if (!(lengthSq(area) > 0))
      return fail(err, "section %u has no area", k);
    const Chart axes(Point3(unit_or_zero(area)));
    dag::Vector<DPoint2> flat(n);
    for (uint32_t i = 0; i < n; ++i)
      flat[i] = DPoint2(pos(k, i) * DPoint3(axes.axisU), pos(k, i) * DPoint3(axes.axisV));
    dag::Vector<uint32_t> tris;
    if (!triangulate_polygon(make_span_const(flat), {}, tris, err))
      return fail(err, "section %u cannot be capped: %s", k, eastl::string(err).c_str());
    for (uint32_t t = 0; t < tris.size(); t += 3)
    {
      const DPoint2 &p0 = flat[tris[t]], &p1 = flat[tris[t + 1]], &p2 = flat[tris[t + 2]];
      const bool alongSection = (p1.x - p0.x) * (p2.y - p0.y) - (p1.y - p0.y) * (p2.x - p0.x) > 0;
      const uint32_t swap = alongSection == bool(end) ? 0 : 1;
      for (uint32_t c : {0u, 1 + swap, 2 - swap})
        cap[end].push_back(LoftCorner{k, tris[t + c]});
    }
    capNormal[end] = unit_or_zero(end ? area : -area);
  }

  dag::Vector<LoftCorner> side;
  side.reserve(bands * n * 6);
  for (uint32_t k = 0; k < bands; ++k)
    for (uint32_t i = 0; i < n; ++i)
    {
      const LoftCorner a0{k, i}, a1{k, i + 1}, b0{k + 1, i}, b1{k + 1, i + 1};
      // the diagonal whose two triangles wind alike (a concave quad has one; the other folds), else the shorter
      const DPoint3 pa = pos(k, i), pb = pos(k, i + 1), pc = pos(k + 1, i + 1), pd = pos(k + 1, i);
      const bool acAlike = ((pb - pa) % (pc - pa)) * ((pc - pa) % (pd - pa)) > 0;
      const bool bdAlike = ((pb - pa) % (pd - pa)) * ((pc - pb) % (pd - pb)) > 0;
      const bool diagA0B1 = acAlike == bdAlike ? lengthSq(pc - pa) <= lengthSq(pd - pb) : acAlike;
      const LoftCorner quad[2][6] = {{a0, a1, b0, a1, b1, b0}, {a0, a1, b1, a0, b1, b0}};
      side.insert(side.end(), quad[diagA0B1], quad[diagA0B1] + 6);
    }
  double volume = 0;
  for (const dag::Vector<LoftCorner> *tris : {&side, &cap[0], &cap[1]})
    for (uint32_t t = 0; t < tris->size(); t += 3)
    {
      const LoftCorner *c = tris->data() + t;
      volume += pos(c[0].k, c[0].i) * (pos(c[1].k, c[1].i) % pos(c[2].k, c[2].i));
    }
  const bool flip = volume < 0;

  const uint32_t first = b.m.pos.size();
  for (const Point3 &q : pts)
    b.vert(q);
  auto emit = [&](const dag::Vector<LoftCorner> &tris, const Chart *chart, uint32_t smgr) {
    auto vtx = [&](const LoftCorner &c) { return first + (c.k % sections) * n + c.i % n; };
    auto tex = [&](const LoftCorner &c) { return chart ? (*chart)(b.m.pos[vtx(c)]) : Point2(float(c.i) / n, float(v[c.k] * vScale)); };
    for (uint32_t t = 0; t < tris.size(); t += 3)
    {
      const LoftCorner &c0 = tris[t], &c1 = tris[t + (flip ? 2 : 1)], &c2 = tris[t + (flip ? 1 : 2)];
      b.tri(vtx(c0), vtx(c1), vtx(c2), tex(c0), tex(c1), tex(c2), smgr);
    }
  };
  emit(side, nullptr, SMOOTH);
  for (int end = 0; end < 2 && !ring; ++end)
  {
    Chart chart(Point3(flip ? -capNormal[end] : capNormal[end]));
    for (uint32_t i = 0; i < n; ++i)
      chart.add(pts[(end ? sections - 1 : 0) * n + i]);
    emit(cap[end], &chart, 0);
  }
  return true;
}
} // namespace dacsg
