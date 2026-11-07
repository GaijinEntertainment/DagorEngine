// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "mesh/solidUtil.h"
#include "prims/prims.h"
#include <math/dag_bounds3.h>
#include <EASTL/algorithm.h>
#include <EASTL/iterator.h>
#include <math.h>
#include <string.h>

namespace dacsg
{
Chart::Chart(const Point3 &n)
{
  axisU = normalize(fabsf(n.y) > 0.999f ? Point3(1, 0, 0) - n * n.x : Point3(0, 1, 0) % n);
  axisV = n % axisU;
}

void Chart::add(const Point3 &p)
{
  const Point2 q(p * axisU, p * axisV);
  lo = Point2(eastl::min(lo.x, q.x), eastl::min(lo.y, q.y));
  hi = Point2(eastl::max(hi.x, q.x), eastl::max(hi.y, q.y));
}

Point2 Chart::operator()(const Point3 &p) const
{
  const float u = (p * axisU - lo.x) * safeinv(hi.x - lo.x), v = (p * axisV - lo.y) * safeinv(hi.y - lo.y);
  return Point2(eastl::clamp(u, 0.f, 1.f), eastl::clamp(v, 0.f, 1.f));
}

uint32_t PrimBuilder::vert(const Point3 &p, const Point3 &n)
{
  m.pos.push_back(p);
  vertNormal.push_back(n);
  return m.pos.size() - 1;
}

void PrimBuilder::tri(uint32_t a, uint32_t b, uint32_t c, Point2 ta, Point2 tb, Point2 tc, uint32_t smgr, const Point3 (&n)[3])
{
  const uint32_t idx[3] = {a, b, c};
  m.tri.insert(m.tri.end(), idx, idx + 3);
  m.face.push_back(FaceInfo{prim, m.face.size(), smgr, 0, 0});
  const Point2 t[3] = {ta, tb, tc};
  uv.insert(uv.end(), t, t + 3);
  nrm.insert(nrm.end(), n, n + 3);
}

void PrimBuilder::quad(uint32_t a, uint32_t b, uint32_t c, uint32_t d, Point2 ta, Point2 tb, Point2 tc, Point2 td, uint32_t smgr,
  const Point3 (&n)[4])
{
  const DPoint3 pa(m.pos[a]), pb(m.pos[b]), pc(m.pos[c]), pd(m.pos[d]);
  if (((pb - pa) % (pc - pa)) * (pd - pa) <= 0)
  {
    tri(a, b, c, ta, tb, tc, smgr, {n[0], n[1], n[2]});
    tri(a, c, d, ta, tc, td, smgr, {n[0], n[2], n[3]});
  }
  else
  {
    tri(a, b, d, ta, tb, td, smgr, {n[0], n[1], n[3]});
    tri(b, c, d, tb, tc, td, smgr, {n[1], n[2], n[3]});
  }
}

void PrimBuilder::band(const Ring &a, const Ring &b, uint32_t segs, uint32_t smgr, const Chart *chart, dag::ConstSpan<Point2> dirs)
{
  auto vtx = [](const Ring &r, uint32_t i) { return r.count == 1 ? r.first : r.first + i % r.count; };
  auto tex = [&](const Ring &r, uint32_t i) {
    if (chart)
      return (*chart)(m.pos[vtx(r, i)]);
    return Point2((r.count == 1 ? i + 0.5f : float(i)) / segs, r.v);
  };
  // one normal per pole, along the axis, so an export keeps the pole one vertex
  auto nor = [&](const Ring &r, uint32_t i) {
    if (dirs.empty() || (r.n.x == 0 && r.n.y == 0))
      return Point3(0, 0, 0);
    if (r.count == 1)
      return Point3(0, r.n.y > 0 ? 1.f : r.n.y < 0 ? -1.f : 0.f, 0);
    return Point3(r.n.x * dirs[i % dirs.size()].x, r.n.y, r.n.x * dirs[i % dirs.size()].y);
  };
  for (uint32_t i = 0; i < segs; ++i)
  {
    if (a.count == 1)
      tri(vtx(a, i), vtx(b, i), vtx(b, i + 1), tex(a, i), tex(b, i), tex(b, i + 1), smgr, {nor(a, i), nor(b, i), nor(b, i + 1)});
    else if (b.count == 1)
      tri(vtx(a, i), vtx(b, i), vtx(a, i + 1), tex(a, i), tex(b, i), tex(a, i + 1), smgr, {nor(a, i), nor(b, i), nor(a, i + 1)});
    else
      quad(vtx(a, i), vtx(b, i), vtx(b, i + 1), vtx(a, i + 1), tex(a, i), tex(b, i), tex(b, i + 1), tex(a, i + 1), smgr,
        {nor(a, i), nor(b, i), nor(b, i + 1), nor(a, i + 1)});
  }
}

void PrimBuilder::finish()
{
  if (!m.findChannel(CH_UV))
  {
    G_ASSERT(uv.size() == m.face.size() * 3);
    Channel &c = m.addChannel(CH_UV, 2);
    memcpy(c.data.data(), uv.data(), uv.size() * sizeof(Point2));
  }
  auto set = [](const Point3 &n) { return lengthSq(n) > 0; };
  for (uint32_t c = 0; c < nrm.size(); ++c)
    if (!set(nrm[c]) && m.face[c / 3].smgr)
      nrm[c] = vertNormal[m.tri[c]];
  if (nrm.size() == m.face.size() * 3 && eastl::any_of(nrm.begin(), nrm.end(), set) && !m.findChannel(CH_NORMAL))
  {
    // in double: a float cross overflows
    const auto at = [&](uint32_t c) { return DPoint3(m.pos[m.tri[c]]); };
    for (uint32_t c = 0; c < nrm.size(); c += 3)
      if (!set(nrm[c]) || !set(nrm[c + 1]) || !set(nrm[c + 2]))
        nrm[c] = nrm[c + 1] = nrm[c + 2] = Point3(unit_or_zero((at(c + 1) - at(c)) % (at(c + 2) - at(c))));
    for (Point3 &n : nrm)
      n = normalize(n);
    memcpy(m.addChannel(CH_NORMAL, 3).data.data(), nrm.data(), nrm.size() * sizeof(Point3));
  }
  m.addDefaultChannels();
}

Point2 unit_dir(double deg)
{
  const double q = deg / 90;
  if (q == floor(q))
  {
    static const Point2 quarter[4] = {Point2(1, 0), Point2(0, 1), Point2(-1, 0), Point2(0, -1)};
    return quarter[(int(q) % 4 + 4) % 4];
  }
  const double rad = deg * (PI_D / 180);
  return Point2(float(cos(rad)), float(sin(rad)));
}

void lathe(PrimBuilder &b, dag::ConstSpan<ProfilePoint> prof, bool loop, uint32_t segs, double angle, dag::Vector<Ring> *rings_out)
{
  const bool full = angle >= 360;
  const uint32_t cols = full ? segs : segs + 1;
  dag::Vector<Point2> dir(cols);
  for (uint32_t i = 0; i < cols; ++i)
    dir[i] = unit_dir(full ? 360.0 * i / segs - 180 : angle * (double(i) / segs - 1));

  dag::Vector<Ring> rings;
  rings.reserve(prof.size());
  for (const ProfilePoint &pp : prof)
  {
    rings.push_back(Ring{b.m.pos.size(), pp.p.x == 0 ? 1u : cols, pp.v});
    if (pp.p.x == 0)
      b.vert(Point3(0, pp.p.y, 0));
    else
      for (const Point2 &d : dir)
        b.vert(Point3(pp.p.x * d.x, pp.p.y, pp.p.x * d.y));
  }

  const uint32_t n = prof.size(), bands = loop ? n : n - 1;
  auto seg = [&](uint32_t band) { return segment_normal(prof[band].p, prof[(band + 1) % n].p); };
  // along a smooth run the mean of the two segments, at a crease the band's own
  auto end_normal = [&](uint32_t band, uint32_t point, uint32_t other) {
    if (prof[point].n.x != 0 || prof[point].n.y != 0)
      return prof[point].n;
    return other < bands && !prof[other].cap && (prof[other].smgr & prof[band].smgr) ? normalize(seg(band) + seg(other)) : seg(band);
  };
  for (uint32_t i = 0; i < bands; ++i)
  {
    const uint32_t j = (i + 1) % n;
    Ring from = rings[i], next = rings[j];
    if (j == 0)
      next.v = 1;
    if (!prof[i].cap)
    {
      from.n = prof[i].smgr ? end_normal(i, i, i > 0 ? i - 1 : loop ? bands - 1 : bands) : Point2(0, 0);
      next.n = prof[i].smgr ? end_normal(i, j, j) : Point2(0, 0);
      b.band(from, next, segs, prof[i].smgr, nullptr, make_span_const(dir));
      continue;
    }
    Chart chart(Point3(0, prof[j].p.x > prof[i].p.x ? -1.f : 1.f, 0));
    for (const Ring *r : {&rings[i], &next})
      for (uint32_t k = 0; k < r->count; ++k)
        chart.add(b.m.pos[r->first + k]);
    b.band(rings[i], next, segs, 0, &chart);
  }
  if (rings_out)
    *rings_out = eastl::move(rings);
}

// The lowering calls the source with the world transform
static bool make_leaf(const PrimArgs &, PrimBuilder &, eastl::string &err)
{
  return fail(err, "a leaf tessellates through its source");
}

static const PrimFn prim_fns[] = {
  make_box,
  make_rbox,
  make_sphere,
  make_ellipsoid,
  make_icosphere,
  make_cubesphere,
  make_cylinder,
  make_cone,
  make_ccyl,
  make_capsule,
  make_torus,
  make_convex,
  make_extrude,
  make_revolve,
  make_tube,
  make_loft,
  make_leaf,
  make_mesh,
};
static_assert(eastl::size(prim_fns) == size_t(PrimType::Mesh) + 1);

// The largest radius a seg count turns around
static double largest_radius(const Graph &g, const PrimDesc &d)
{
  double r = 0;
  switch (d.type)
  {
    case PrimType::RBox: return d.p[PrimDesc::RBOX_RADIUS];
    case PrimType::Ellipsoid: return eastl::max(eastl::max(d.p[PrimDesc::RADIUS_X], d.p[PrimDesc::RADIUS_Y]), d.p[PrimDesc::RADIUS_Z]);
    case PrimType::Cone: return eastl::max(d.p[PrimDesc::RADIUS], d.p[PrimDesc::CONE_RADIUS_TOP]);
    case PrimType::Torus: return double(d.p[PrimDesc::RADIUS]) + d.p[PrimDesc::TORUS_RADIUS_MINOR];
    case PrimType::Revolve:
      for (const Point2 &q : g.points2(d.points))
        r = check_finite(q.x) ? eastl::max(r, double(fabsf(q.x))) : r;
      return r;
    default: return d.p[PrimDesc::RADIUS];
  }
}

// The fewest segments per turn whose chords on radius r sag at most sag
static uint32_t seg_for(double r, double sag)
{
  // a sag that 1 - sag / r rounds away turns by no angle: 256, with no division by 0
  return sag < r ? uint32_t(eastl::max(ceil(PI_D / eastl::max(acos(1 - sag / r), PI_D / 256)), 3.0)) : 3;
}

static bool fits_float(const Mesh &m, bool built, eastl::string &err)
{
  BBox3 box;
  for (const Point3 &p : m.pos)
    box += p;
  // an import with no faces is the empty solid, whatever positions it carries
  if (!m.tri.empty() && !(lengthSq(box.width()) > 1e-12f))
    return fail(err, "below the smallest size at this precision");
  for (uint32_t c = 0; c < m.tri.size(); c += 3)
  {
    const DPoint3 p0(m.pos[m.tri[c]]), p1(m.pos[m.tri[c + 1]]), p2(m.pos[m.tri[c + 2]]);
    const double sizeSq = max(max(lengthSq(p0), lengthSq(p1)), lengthSq(p2)), floorSq = sqr(8 * FLT_EPSILON) * sizeSq;
    if (!(sizeSq < 1e36))
      return fail(err, "beyond float range at this size");
    if (built && (!(lengthSq((p1 - p0) % (p2 - p0)) > 0) || lengthSq(p1 - p0) <= floorSq || lengthSq(p2 - p1) <= floorSq ||
                   lengthSq(p0 - p2) <= floorSq))
      return fail(err, "below float precision at this size");
  }
  return true;
}

static PrimArgs prim_args(const Graph &g, const PrimDesc &d, double sag, MeshLoader loader, bool check_only)
{
  // the radii meet comparisons: a parameter that is not finite, which the builder refuses, takes 3 segments
  const bool finite = eastl::all_of(eastl::begin(d.p), eastl::end(d.p), [](float x) { return check_finite(x); });
  const uint32_t seg = d.seg ? d.seg : finite ? seg_for(largest_radius(g, d), sag) : 3;
  const uint32_t segMinor = d.seg ? eastl::max(d.seg / 2u, 3u) : finite ? seg_for(d.p[PrimDesc::TORUS_RADIUS_MINOR], sag) : 3;
  return PrimArgs{g, d, seg, d.segMinor ? d.segMinor : segMinor, loader, check_only};
}

bool tessellate(const Graph &g, OpResult prim, double sag, Mesh &out, eastl::string &err, MeshLoader loader)
{
  out = Mesh();
  const PrimDesc &d = g.primDesc(prim);
  const PrimArgs args = prim_args(g, d, sag, loader, false);
  PrimBuilder b{out, prim.id};
  const bool made = prim_fns[int(d.type)](args, b, err);
  if (made)
    b.finish();
  if (!made || !fits_float(out, d.type != PrimType::Mesh, err))
  {
    err = g.getName(prim) + ": " + err;
    out = Mesh();
    return false;
  }
  return true;
}

bool check_prim(const Graph &g, OpResult prim, MeshLoader loader, eastl::string &err)
{
  Mesh none;
  // no sag: an automatic seg takes 3
  const PrimArgs args = prim_args(g, g.primDesc(prim), DBL_MAX, loader, true);
  PrimBuilder b{none, prim.id};
  if (prim_fns[int(args.d.type)](args, b, err) && fits_float(none, true, err))
    return true;
  err = g.getName(prim) + ": " + err;
  return false;
}
} // namespace dacsg
