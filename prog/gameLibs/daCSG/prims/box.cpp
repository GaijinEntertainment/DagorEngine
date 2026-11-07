// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "mesh/solidUtil.h"
#include "prims/prims.h"
#include <ska_hash_map/flat_hash_map2.hpp>
#include <EASTL/algorithm.h>
#include <math.h>

namespace dacsg
{
struct CubeFace
{
  uint8_t n, u, v;
  int8_t ns, us, vs;
};

// u x v = n; sides have v up
static const CubeFace cube_faces[6] = {
  {0, 2, 1, 1, -1, 1},
  {0, 2, 1, -1, 1, 1},
  {1, 0, 2, 1, 1, -1},
  {1, 0, 2, -1, 1, 1},
  {2, 0, 1, 1, 1, 1},
  {2, 0, 1, -1, -1, 1},
};

void cube_lattice(PrimBuilder &b, const Lattice &l)
{
  const uint32_t last[3] = {uint32_t(l.t[0].size() - 1), uint32_t(l.t[1].size() - 1), uint32_t(l.t[2].size() - 1)};
  ska::flat_hash_map<uint64_t, uint32_t, KeyHash> verts;
  auto vertex = [&](const uint32_t(&idx)[3]) {
    const auto slot = verts.emplace(idx[0] | uint64_t(idx[1]) << 21 | uint64_t(idx[2]) << 42, 0u);
    if (!slot.second)
      return slot.first->second;
    DPoint3 c, in, d;
    int moved = 0;
    for (int k = 0; k < 3; ++k)
    {
      c[k] = l.t[k][idx[k]];
      in[k] = eastl::clamp(c[k], -l.inner[k], l.inner[k]);
      d[k] = c[k] - in[k];
      moved += d[k] != 0;
    }
    return slot.first->second =
             b.vert(Point3(moved > 1 ? in + normalize(d) * l.r : c), l.r > 0 && moved ? Point3(normalize(d)) : Point3(0, 0, 0));
  };
  auto lat = [&](int axis, int sign, uint32_t i) { return sign > 0 ? i : last[axis] - i; };
  auto chart = [&](int axis, int sign, uint32_t i) {
    if (l.uvByIndex)
      return float(i) / last[axis];
    const dag::Vector<double> &t = l.t[axis];
    const double x = t[lat(axis, sign, i)];
    return float((sign > 0 ? x - t.front() : t.back() - x) / (t.back() - t.front()));
  };
  auto flat = [&](int axis, int sign, uint32_t i) {
    return fabs(l.t[axis][lat(axis, sign, i)]) <= l.inner[axis] && fabs(l.t[axis][lat(axis, sign, i + 1)]) <= l.inner[axis];
  };

  dag::Vector<uint32_t> grid;
  for (const CubeFace &f : cube_faces)
  {
    const uint32_t nu = last[f.u], nv = last[f.v];
    grid.resize((nu + 1) * (nv + 1));
    for (uint32_t j = 0; j <= nv; ++j)
      for (uint32_t i = 0; i <= nu; ++i)
      {
        uint32_t idx[3];
        idx[f.n] = f.ns > 0 ? last[f.n] : 0;
        idx[f.u] = lat(f.u, f.us, i);
        idx[f.v] = lat(f.v, f.vs, j);
        grid[j * (nu + 1) + i] = vertex(idx);
      }
    for (uint32_t j = 0; j < nv; ++j)
      for (uint32_t i = 0; i < nu; ++i)
      {
        const uint32_t smgr = flat(f.u, f.us, i) && flat(f.v, f.vs, j) ? 0 : SMOOTH;
        const float u0 = chart(f.u, f.us, i), u1 = chart(f.u, f.us, i + 1), v0 = chart(f.v, f.vs, j), v1 = chart(f.v, f.vs, j + 1);
        const uint32_t *row = &grid[j * (nu + 1) + i];
        b.quad(row[0], row[1], row[nu + 2], row[nu + 1], Point2(u0, v0), Point2(u1, v0), Point2(u1, v1), Point2(u0, v1), smgr);
      }
  }
}

static bool check_size(const float *s, eastl::string &err)
{
  return positive(s[0]) && positive(s[1]) && positive(s[2]) ? true : fail(err, "size must be positive");
}

bool make_box(const PrimArgs &a, PrimBuilder &b, eastl::string &err)
{
  if (!check_size(a.d.p, err))
    return false;
  Lattice l;
  for (int k = 0; k < 3; ++k)
  {
    l.inner[k] = 0.5 * a.d.p[k];
    l.t[k] = {-l.inner[k], l.inner[k]};
  }
  cube_lattice(b, l);
  return true;
}

bool make_rbox(const PrimArgs &a, PrimBuilder &b, eastl::string &err)
{
  const float *s = a.d.p, smallest = eastl::min(eastl::min(s[0], s[1]), s[2]);
  if (!check_size(s, err))
    return false;
  if (!positive(s[PrimDesc::RBOX_RADIUS]))
    return fail(err, "radius must be positive");
  if (2 * s[PrimDesc::RBOX_RADIUS] > smallest * (1 + 1e-6f))
    return fail(err, "radius is above half the smallest size");
  // a radius within the tolerance of half the smallest size is that half: the flats of that axis are gone, not a sliver
  const float r = 2 * s[PrimDesc::RBOX_RADIUS] < smallest * (1 - 1e-6f) ? s[PrimDesc::RBOX_RADIUS] : 0.5f * smallest;

  // At least seg segments per full turn
  const int q = eastl::max(1, int(a.seg + 7) / 8);
  Lattice l;
  l.r = r;
  for (int k = 0; k < 3; ++k)
  {
    const double h = 0.5 * s[k], e = h - r;
    l.inner[k] = e;
    dag::Vector<double> &t = l.t[k];
    t.resize(2 * q + 1 + (e > 0));
    for (int i = 0; i <= q; ++i)
    {
      const double off = i == q ? r : r * tan(PI_D / 4 * i / q);
      t[q - i] = -e - off;
      t[t.size() - 1 - q + i] = e + off;
    }
  }
  cube_lattice(b, l);
  return true;
}
} // namespace dacsg
