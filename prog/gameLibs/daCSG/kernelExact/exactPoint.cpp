// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "kernelExact/exactPoint.h"

namespace dacsg::exact
{
// zero for a degenerate triangle
static void tri_normal(const GridPoint &a, const GridPoint &b, const GridPoint &c, BigInt<2> n[3])
{
  int64_t e1[3], e2[3];
  for (int i = 0; i < 3; ++i)
  {
    e1[i] = b.c[i] - a.c[i];
    e2[i] = c.c[i] - a.c[i];
  }
  for (int i = 0; i < 3; ++i)
  {
    const int j = (i + 1) % 3, k = (i + 2) % 3;
    n[i] = mul64(e1[j], e2[k]) - mul64(e1[k], e2[j]);
  }
}

void cross(const BigInt<2> a[3], const BigInt<2> b[3], BigInt<3> out[3])
{
  for (int i = 0; i < 3; ++i)
  {
    const int j = (i + 1) % 3, k = (i + 2) % 3;
    out[i] = mul<3>(a[j], b[k]) - mul<3>(a[k], b[j]);
  }
}

template <int N>
static int largest(const BigInt<N> v[3])
{
  int axis = 0;
  for (int i = 1; i < 3; ++i)
    if (cmp_unsigned(magnitude(v[i]), magnitude(v[axis])) > 0)
      axis = i;
  return axis;
}

static void set_offset(Plane &pl, const GridPoint &on)
{
  pl.d = BigInt<2>(0);
  for (int i = 0; i < 3; ++i)
  {
    pl.d = pl.d - mul<2>(pl.n[i], BigInt<1>(on.c[i]));
    pl.nd[i] = to_double(pl.n[i]);
  }
  pl.dd = to_double(pl.d);
  pl.axis = int8_t(largest(pl.n));
}

int make_plane(const GridPoint &a, const GridPoint &b, const GridPoint &c, Plane &pl)
{
  BigInt<2> n[3];
  tri_normal(a, b, c, n);
  const int orient = n[0].sign() ? n[0].sign() : n[1].sign() ? n[1].sign() : n[2].sign();
  if (!orient)
    return 0;
  pl.fp = false;
  for (int i = 0; i < 3; ++i)
    pl.n[i] = orient > 0 ? n[i] : -n[i];
  set_offset(pl, a);
  return orient;
}

int make_plane(const ExactPoint &a, const ExactPoint &b, const ExactPoint &c, ExArena &arena, Plane &pl)
{
  return a.fp ? fp::make_plane(a.approx, b.approx, c.approx, arena, pl) : make_plane(a.grid, b.grid, c.grid, pl);
}

int plane_sign(const Plane &pl, int i) { return pl.fp ? pl.fn[i].sign() : pl.n[i].sign(); }

int plane_axis(const Plane &pl) { return pl.fp ? fp::plane_axis(pl) : pl.axis; }

// the largest component's index and sign, then the other components and the offset as ratios to it: in doubles,
// exactly within 1e-9
int plane_cmp(const Plane &a, const Plane &b)
{
  if (a.fp)
    return fp::plane_cmp(a, b);
  const int axis = plane_axis(a);
  if (const int c = axis - plane_axis(b))
    return c < 0 ? -1 : 1;
  if (const int c = a.n[axis].sign() - b.n[axis].sign())
    return c < 0 ? -1 : 1;
  for (int i = 0; i < 4; ++i)
  {
    if (i == axis)
      continue;
    const double ra = (i < 3 ? a.nd[i] : a.dd) / a.nd[axis], rb = (i < 3 ? b.nd[i] : b.dd) / b.nd[axis];
    if (fabs(ra - rb) > 1e-9 * (1 + fabs(ra) + fabs(rb)))
      return ra < rb ? -1 : 1;
    if (const int c = (mul<4>(i < 3 ? a.n[i] : a.d, b.n[axis]) - mul<4>(i < 3 ? b.n[i] : b.d, a.n[axis])).sign())
      return c;
  }
  return 0;
}

static ExactDir dir_with_approx(ExactDir d)
{
  d.fp = false;
  for (int i = 0; i < 3; ++i)
    d.approx[i] = to_double(d.c[i]);
  return d;
}

ExactDir edge_dir(const GridPoint &from, const GridPoint &to)
{
  ExactDir d;
  for (int i = 0; i < 3; ++i)
    d.c[i] = BigInt<3>(to.c[i] - from.c[i]);
  return dir_with_approx(d);
}

ExactDir edge_dir(const ExactPoint &from, const ExactPoint &to)
{
  return from.fp ? fp::edge_dir(from.approx, to.approx) : edge_dir(from.grid, to.grid);
}

ExactDir line_dir(const Plane &a, const Plane &b)
{
  if (a.fp)
    return fp::line_dir(a, b);
  ExactDir d;
  cross(a.n, b.n, d.c);
  return dir_with_approx(d);
}

int dir_axis(const ExactDir &d) { return d.fp ? fp::dir_axis(d) : largest(d.c); }

int dir_sign(const ExactDir &d, int i) { return d.fp ? sign(d.f[i]) : d.c[i].sign(); }

void dir_negate(ExactDir &d)
{
  for (int i = 0; i < 3; ++i)
  {
    if (d.fp)
      negate(d.f[i]);
    else
      d.c[i] = -d.c[i];
    d.approx[i] = -d.approx[i];
  }
}

template <int N>
static void set_point_approx(ExactPoint &p, const HomPoint<N> &h)
{
  const double w = to_double(h.w);
  double top = 0;
  for (int i = 0; i < 3; ++i)
  {
    p.approx[i] = to_double(h.c[i]) / w;
    top = fabs(p.approx[i]) > top ? fabs(p.approx[i]) : top;
  }
  p.err = top * 0x1p-51; // three roundings of 2^-53 each
}

ExactPoint make_grid_point(const GridPoint &g)
{
  ExactPoint p;
  p.grid = g;
  p.approx = DPoint3(double(g.c[0]), double(g.c[1]), double(g.c[2]));
  return p;
}

ExactPoint make_fp_point(const DPoint3 &pos)
{
  ExactPoint p;
  p.fp = true;
  p.approx = pos;
  return p;
}

bool make_lpi(const GridPoint &p, const GridPoint &q, const Plane &pl, ExactPoint &out)
{
  BigInt<2> w(0), s = pl.d;
  for (int i = 0; i < 3; ++i)
  {
    w = w + mul<2>(pl.n[i], BigInt<1>(q.c[i] - p.c[i]));
    s = s + mul<2>(pl.n[i], BigInt<1>(p.c[i]));
  }
  if (w.is_zero())
    return false;
  if (w.negative())
  {
    w = -w;
    s = -s;
  }
  out.kind = PointKind::Lpi;
  out.fp = false;
  out.lpi.w = BigInt<3>(w);
  for (int i = 0; i < 3; ++i)
    out.lpi.c[i] = mul<3>(BigInt<1>(p.c[i]), w) - mul<3>(s, BigInt<1>(q.c[i] - p.c[i]));
  set_point_approx(out, out.lpi);
  return true;
}

bool make_lpi(const ExactPoint &p, const ExactPoint &q, const Plane &pl, ExArena &arena, ExactPoint &out)
{
  return p.fp ? fp::make_lpi(p.approx, q.approx, pl, arena, out) : make_lpi(p.grid, q.grid, pl, out);
}

bool make_ssi(const GridPoint &p, const GridPoint &q, const GridPoint &r, const GridPoint &s, int axis, ExactPoint &out)
{
  Plane m;
  const int j = (axis + 1) % 3, k = (axis + 2) % 3;
  m.n[axis] = BigInt<2>(0);
  m.n[j] = BigInt<2>(s.c[k] - r.c[k]);
  m.n[k] = BigInt<2>(r.c[j] - s.c[j]);
  set_offset(m, r);
  if (!make_lpi(p, q, m, out))
    return false;
  out.kind = PointKind::Ssi;
  return true;
}

bool make_ssi(const ExactPoint &p, const ExactPoint &q, const ExactPoint &r, const ExactPoint &s, int axis, ExArena &arena,
  ExactPoint &out)
{
  return p.fp ? fp::make_ssi(p.approx, q.approx, r.approx, s.approx, axis, arena, out)
              : make_ssi(p.grid, q.grid, r.grid, s.grid, axis, out);
}

bool make_tpi(const Plane &a, const Plane &b, const Plane &c, ExactPoint &out)
{
  BigInt<3> bc[3], ca[3], ab[3];
  cross(b.n, c.n, bc);
  cross(c.n, a.n, ca);
  cross(a.n, b.n, ab);
  BigInt<5> w(0);
  for (int i = 0; i < 3; ++i)
    w = w + mul<5>(a.n[i], bc[i]);
  if (w.is_zero())
    return false;
  out.kind = PointKind::Tpi;
  out.fp = false;
  out.tpi.w = w.negative() ? -w : w;
  for (int i = 0; i < 3; ++i)
  {
    const BigInt<5> x = mul<5>(a.d, bc[i]) + mul<5>(b.d, ca[i]) + mul<5>(c.d, ab[i]);
    out.tpi.c[i] = w.negative() ? x : -x;
  }
  set_point_approx(out, out.tpi);
  return true;
}

bool make_tpi(const Plane &a, const Plane &b, const Plane &c, ExArena &arena, ExactPoint &out)
{
  return a.fp ? fp::make_tpi(a, b, c, arena, out) : make_tpi(a, b, c, out);
}

static void sort2(uint32_t &a, uint32_t &b)
{
  if (b < a)
    eastl::swap(a, b);
}

void set_key(ExactPoint &p, uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
  p.keyed = true;
  if (p.kind == PointKind::Lpi || p.kind == PointKind::Ssi)
    sort2(a, b);
  if (p.kind == PointKind::Ssi)
  {
    sort2(c, d);
    if (c < a || (c == a && d < b))
    {
      eastl::swap(a, c);
      eastl::swap(b, d);
    }
  }
  if (p.kind == PointKind::Tpi)
  {
    sort2(a, b);
    sort2(b, c);
    sort2(a, b);
  }
  p.key[0] = a;
  p.key[1] = b;
  p.key[2] = c;
  p.key[3] = d;
}

bool key_less(const ExactPoint &a, const ExactPoint &b)
{
  if (a.kind != b.kind)
    return a.kind < b.kind;
  for (int i = 0; i < 4; ++i)
    if (a.key[i] != b.key[i])
      return a.key[i] < b.key[i];
  return false;
}
} // namespace dacsg::exact
