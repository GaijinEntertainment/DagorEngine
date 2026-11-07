// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "mesh/solidUtil.h"
#include "prims/prims.h"
#include <ska_hash_map/flat_hash_map2.hpp>
#include <EASTL/algorithm.h>
#include <math.h>

namespace dacsg
{
static void lat_long(PrimBuilder &b, const Point3 &radii, uint32_t seg)
{
  const uint32_t stacks = eastl::max(2u, (seg + 1) / 2), first = b.m.pos.size(), firstCorner = b.nrm.size();
  dag::Vector<ProfilePoint> prof(stacks + 1);
  for (uint32_t j = 0; j <= stacks; ++j)
    prof[j] = ProfilePoint{unit_dir(180.0 * j / stacks - 90), float(j) / stacks, SMOOTH, false, unit_dir(180.0 * j / stacks - 90)};
  lathe(b, prof, false, seg, 360);
  for (uint32_t i = first; i < b.m.pos.size(); ++i)
    b.m.pos[i] = Point3(b.m.pos[i].x * radii.x, b.m.pos[i].y * radii.y, b.m.pos[i].z * radii.z);
  for (uint32_t i = firstCorner; i < b.nrm.size(); ++i)
    b.nrm[i] = Point3(unit_or_zero(DPoint3(b.nrm[i].x / double(radii.x), b.nrm[i].y / double(radii.y), b.nrm[i].z / double(radii.z))));
}

bool make_sphere(const PrimArgs &a, PrimBuilder &b, eastl::string &err)
{
  const float r = a.d.p[PrimDesc::RADIUS];
  if (!positive(r))
    return fail(err, "radius must be positive");
  lat_long(b, Point3(r, r, r), a.seg);
  return true;
}

bool make_ellipsoid(const PrimArgs &a, PrimBuilder &b, eastl::string &err)
{
  const Point3 radii(a.d.p[PrimDesc::RADIUS_X], a.d.p[PrimDesc::RADIUS_Y], a.d.p[PrimDesc::RADIUS_Z]);
  if (!positive(radii.x) || !positive(radii.y) || !positive(radii.z))
    return fail(err, "radii must be positive");
  lat_long(b, radii, a.seg);
  return true;
}


struct IcoMesh
{
  dag::Vector<DPoint3> v;
  dag::Vector<uint32_t> tri;
  ska::flat_hash_map<uint64_t, uint32_t, KeyHash> mid;

  uint32_t midpoint(uint32_t a, uint32_t b, bool project)
  {
    const DPoint3 p = (v[a] + v[b]) * 0.5;
    const auto slot = mid.emplace(edge_key(a, b), uint32_t(v.size()));
    if (slot.second)
      v.push_back(project ? normalize(p) : p);
    return slot.first->second;
  }
};

// z-symmetric: a seam-crossing face has a seam vertex
static void icosahedron(IcoMesh &m)
{
  const double y = 1 / sqrt(5.0), rho = 2 * y;
  auto ring = [&](double deg, double h) {
    const double rad = fabs(deg) * (PI_D / 180), z = deg == 0 || fabs(deg) == 180 ? 0 : rho * sin(rad);
    m.v.push_back(DPoint3(rho * cos(rad), h, deg < 0 ? -z : z));
  };
  m.v.push_back(DPoint3(0, 1, 0));
  for (int k = 0; k < 5; ++k)
    ring(180 - 72 * k, y);
  for (int k = 0; k < 5; ++k)
    ring(144 - 72 * k, -y);
  m.v.push_back(DPoint3(0, -1, 0));
  for (uint32_t k = 0; k < 5; ++k)
  {
    const uint32_t u0 = 1 + k, u1 = 1 + (k + 1) % 5, l0 = 6 + k, l1 = 6 + (k + 1) % 5;
    const uint32_t t[12] = {0, u0, u1, u0, l0, u1, u1, l0, l1, 11, l1, l0};
    m.tri.insert(m.tri.end(), t, t + 12);
  }
  for (uint32_t i = 0; i < m.tri.size(); i += 3)
  {
    const DPoint3 &p0 = m.v[m.tri[i]], &p1 = m.v[m.tri[i + 1]], &p2 = m.v[m.tri[i + 2]];
    if (((p1 - p0) % (p2 - p0)) * (p0 + p1 + p2) < 0)
      eastl::swap(m.tri[i + 1], m.tri[i + 2]);
  }
}

static void subdivide(IcoMesh &m)
{
  dag::Vector<uint32_t> out;
  out.reserve(m.tri.size() * 4);
  m.mid.clear();
  for (uint32_t i = 0; i < m.tri.size(); i += 3)
  {
    const uint32_t a = m.tri[i], b = m.tri[i + 1], c = m.tri[i + 2];
    const uint32_t ab = m.midpoint(a, b, true), bc = m.midpoint(b, c, true), ca = m.midpoint(c, a, true);
    const uint32_t t[12] = {a, ab, ca, ab, b, bc, ca, bc, c, ab, bc, ca};
    out.insert(out.end(), t, t + 12);
  }
  m.tri = eastl::move(out);
}

static void split_seam(IcoMesh &m)
{
  m.mid.clear();
  const uint32_t faces = m.tri.size() / 3;
  for (uint32_t f = 0; f < faces; ++f)
  {
    uint32_t *t = &m.tri[f * 3];
    int k = 0;
    while (k < 3 && m.v[t[k]].z != 0)
      ++k;
    const DPoint3 &pa = m.v[t[(k + 1) % 3]], &pb = m.v[t[(k + 2) % 3]];
    if (k == 3 || (pa.z > 0) == (pb.z > 0) || pa.z == 0 || pb.z == 0 || pa.x + pb.x >= 0)
    {
      G_ASSERT(k < 3 || ((m.v[t[0]].z > 0) == (m.v[t[1]].z > 0) && (m.v[t[1]].z > 0) == (m.v[t[2]].z > 0)));
      continue;
    }
    const uint32_t v0 = t[k], a = t[(k + 1) % 3], b = t[(k + 2) % 3], s = m.midpoint(a, b, false);
    t[0] = v0;
    t[1] = a;
    t[2] = s;
    const uint32_t extra[3] = {v0, s, b};
    m.tri.insert(m.tri.end(), extra, extra + 3);
  }
}

bool make_icosphere(const PrimArgs &a, PrimBuilder &b, eastl::string &err)
{
  const float r = a.d.p[PrimDesc::RADIUS];
  if (!positive(r))
    return fail(err, "radius must be positive");
  IcoMesh m;
  icosahedron(m);
  for (int i = 0; i < a.d.level; ++i)
    subdivide(m);
  split_seam(m);

  const uint32_t first = b.m.pos.size();
  dag::Vector<Point2, EASTLAllocatorType, false> uv(m.v.size());
  for (uint32_t v = 0; v < m.v.size(); ++v)
  {
    const DPoint3 d = normalize(m.v[v]);
    b.vert(Point3(m.v[v] * r), Point3(d));
    uv[v] = Point2(float(atan2(d.z, d.x) / (2 * PI_D) + 0.5), float(asin(eastl::clamp(d.y, -1.0, 1.0)) / PI_D + 0.5));
  }
  for (uint32_t i = 0; i < m.tri.size(); i += 3)
  {
    Point2 t[3];
    int pole = -1;
    bool positiveSide = false;
    for (int k = 0; k < 3; ++k)
    {
      const DPoint3 &d = m.v[m.tri[i + k]];
      t[k] = uv[m.tri[i + k]];
      pole = d.x == 0 && d.z == 0 ? k : pole;
      positiveSide |= d.z > 0;
    }
    for (int k = 0; k < 3; ++k)
    {
      const DPoint3 &d = m.v[m.tri[i + k]];
      if (d.z == 0 && d.x < 0)
        t[k].x = positiveSide ? 1.f : 0.f;
    }
    if (pole >= 0)
      t[pole].x = (t[(pole + 1) % 3].x + t[(pole + 2) % 3].x) * 0.5f;
    b.tri(first + m.tri[i], first + m.tri[i + 1], first + m.tri[i + 2], t[0], t[1], t[2], SMOOTH);
  }
  return true;
}

bool make_cubesphere(const PrimArgs &a, PrimBuilder &b, eastl::string &err)
{
  const float r = a.d.p[PrimDesc::RADIUS];
  if (!positive(r))
    return fail(err, "radius must be positive");
  // At least seg segments around the equator
  const uint32_t n = eastl::max(1u, (a.seg + 3) / 4);
  Lattice l;
  l.r = r;
  l.uvByIndex = true;
  for (int k = 0; k < 3; ++k)
  {
    l.t[k].resize(n + 1);
    for (uint32_t i = 0; i <= n; ++i)
      l.t[k][i] = i == 0 ? -r : i == n ? r : r * tan(PI_D * (2.0 * i - n) / (4.0 * n));
  }
  cube_lattice(b, l);
  return true;
}

bool make_capsule(const PrimArgs &a, PrimBuilder &b, eastl::string &err)
{
  const float h = a.d.p[PrimDesc::HEIGHT];
  if (!positive(a.d.p[PrimDesc::RADIUS]) || !positive(h))
    return fail(err, "radius and length must be positive");
  if (h < 2 * a.d.p[PrimDesc::RADIUS] * (1 - 1e-6f))
    return fail(err, "length is below 2 r");
  // a radius within the tolerance of h / 2, from either side, is h / 2: the poles span h
  const float r = 0.5 * h - a.d.p[PrimDesc::RADIUS] > 1e-6 * h ? a.d.p[PrimDesc::RADIUS] : 0.5f * h;
  const double c = 0.5 * h - r, len = PI_D * r + 2 * c;
  const uint32_t q = eastl::max(1u, (a.seg + 3) / 4);
  dag::Vector<ProfilePoint> prof;
  for (int dome = 0; dome < 2; ++dome)
    for (uint32_t j = dome && c == 0 ? 1 : 0; j <= q; ++j)
    {
      const double lat = 90.0 * j / q - (dome ? 0 : 90);
      const Point2 d = unit_dir(lat);
      const double y = dome ? c : -c, arc = r * (lat + 90) * (PI_D / 180) + (dome ? 2 * c : 0);
      prof.push_back(ProfilePoint{Point2(r * d.x, float(y + r * d.y)), float(arc / len), SMOOTH, false, d});
    }
  lathe(b, prof, false, a.seg, 360);
  return true;
}
} // namespace dacsg
