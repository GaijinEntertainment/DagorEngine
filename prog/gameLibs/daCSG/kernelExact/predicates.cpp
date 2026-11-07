// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "kernelExact/predicates.h"
#include <debug/dag_assert.h>
#include <float.h>

namespace dacsg::exact
{
static thread_local uint64_t exact_fallbacks = 0;

uint64_t take_exact_fallbacks()
{
  const uint64_t n = exact_fallbacks;
  exact_fallbacks = 0;
  return n;
}

void add_exact_fallbacks(uint64_t n) { exact_fallbacks += n; }

namespace
{
// Each bound widens by an ulp or more; DBL_MIN, not a subnormal, keeps it sound under flush to zero.
// Inputs stay below 2^300, so no bound overflows.
struct Iv
{
  double lo, hi;
};

inline double up(double x) { return x + fabs(x) * 0x1p-52 + DBL_MIN; }
inline double down(double x) { return x - fabs(x) * 0x1p-52 - DBL_MIN; }
inline Iv operator+(Iv a, Iv b) { return {down(a.lo + b.lo), up(a.hi + b.hi)}; }
inline Iv operator-(Iv a, Iv b) { return {down(a.lo - b.hi), up(a.hi - b.lo)}; }
inline Iv operator*(Iv a, Iv b)
{
  const double p[4] = {a.lo * b.lo, a.lo * b.hi, a.hi * b.lo, a.hi * b.hi};
  double lo = p[0], hi = p[0];
  for (int i = 1; i < 4; ++i)
  {
    lo = p[i] < lo ? p[i] : lo;
    hi = p[i] > hi ? p[i] : hi;
  }
  return {down(lo), up(hi)};
}
inline Iv exact(double x) { return {x, x}; }
inline Iv rounded(double x) { return {down(x), up(x)}; }
// an fp plane's double is an estimate of its expansion, within two ulps
inline Iv rounded(double x, bool fp) { return fp ? Iv{down(down(x)), up(up(x))} : rounded(x); }
inline Iv coord(const ExactPoint &p, int i)
{
  const double a = p.approx[i];
  return p.err == 0 ? exact(a) : Iv{down(a - p.err), up(a + p.err)};
}
// 0: the interval holds zero
inline int sign_of(Iv v) { return v.lo > 0 ? 1 : v.hi < 0 ? -1 : 0; }

bool decided(int s)
{
  exact_fallbacks += s == 0;
  return s != 0;
}

// the sign of the 3x3 determinant of the rows when the intervals decide it
int det3_sign(const Iv m[3][3])
{
  Iv r = exact(0);
  for (int i = 0; i < 3; ++i)
  {
    const int j = (i + 1) % 3, k = (i + 2) % 3;
    r = r + m[0][i] * (m[1][j] * m[2][k] - m[1][k] * m[2][j]);
  }
  return sign_of(r);
}

// |c| <= 2^x_bits, |w| <= 2^w_bits for a point held in limbs
constexpr int x_bits(int limbs) { return limbs == 1 ? MAX_GRID_BITS : limbs == 3 ? LPI_X_BITS : TPI_X_BITS; }
constexpr int w_bits(int limbs) { return limbs == 1 ? 0 : limbs == 3 ? LPI_W_BITS : TPI_W_BITS; }

constexpr int max_of(int a, int b) { return a > b ? a : b; }
// bits of a sum of `terms` values, each at most 2^top
constexpr int sum_bits(int top, int terms) { return terms > 1 ? sum_bits(top + 1, (terms + 1) / 2) : top; }

HomPoint<1> grid_hom(const GridPoint &g)
{
  HomPoint<1> h;
  for (int i = 0; i < 3; ++i)
    h.c[i] = BigInt<1>(g.c[i]);
  h.w = BigInt<1>(1);
  return h;
}

template <class F>
int visit(const ExactPoint &p, F &&f)
{
  switch (p.kind)
  {
    case PointKind::Grid: return f(grid_hom(p.grid));
    case PointKind::Tpi: return f(p.tpi);
    default: return f(p.lpi);
  }
}

template <int A>
int point_plane_exact(const HomPoint<A> &p, const Plane &pl)
{
  constexpr int R = limbs_for(sum_bits(max_of(NORMAL_BITS + x_bits(A), OFFSET_BITS + w_bits(A)), 4));
  BigInt<R> s = mul<R>(pl.d, p.w);
  for (int i = 0; i < 3; ++i)
    s = s + mul<R>(pl.n[i], p.c[i]);
  return s.sign();
}

// the 3x3 determinant of the rows (c[u], c[v], w), w > 0
template <int A, int B, int C>
int orient2d_exact(const HomPoint<A> &a, const HomPoint<B> &b, const HomPoint<C> &c, int u, int v)
{
  constexpr int xa = x_bits(A), wa = w_bits(A), xb = x_bits(B), wb = w_bits(B), xc = x_bits(C), wc = w_bits(C);
  constexpr int M1 = limbs_for(sum_bits(max_of(xb + wc, xc + wb), 2)), M2 = limbs_for(sum_bits(xb + xc, 2));
  constexpr int R = limbs_for(sum_bits(max_of(max_of(xa + xb + wc, xa + wb + xc), wa + xb + xc), 6));
  const BigInt<M1> mv = mul<M1>(b.c[v], c.w) - mul<M1>(c.c[v], b.w), mu = mul<M1>(b.c[u], c.w) - mul<M1>(c.c[u], b.w);
  const BigInt<M2> muv = mul<M2>(b.c[u], c.c[v]) - mul<M2>(c.c[u], b.c[v]);
  return (mul<R>(a.c[u], mv) - mul<R>(a.c[v], mu) + mul<R>(a.w, muv)).sign();
}

template <int A, int B>
int compare_exact(const HomPoint<A> &a, const HomPoint<B> &b, int axis)
{
  constexpr int R = limbs_for(sum_bits(max_of(x_bits(A) + w_bits(B), x_bits(B) + w_bits(A)), 2));
  return (mul<R>(a.c[axis], b.w) - mul<R>(b.c[axis], a.w)).sign();
}
} // namespace

int sign_point_plane(const ExactPoint &p, const Plane &pl)
{
  Iv r = rounded(pl.dd, pl.fp);
  for (int i = 0; i < 3; ++i)
    r = r + rounded(pl.nd[i], pl.fp) * coord(p, i);
  if (const int s = sign_of(r); decided(s))
    return s;
  if (pl.fp)
    return fp::point_plane(p, pl);
  return visit(p, [&](const auto &h) { return point_plane_exact(h, pl); });
}

// p lies on the line of the leaf edge v0 v1 by its construction: a vertex of the edge, an LPI or an SSI of it
static bool on_edge(const ExactPoint &p, uint32_t v0, uint32_t v1)
{
  switch (p.kind)
  {
    case PointKind::Grid: return p.key[0] == v0 || p.key[0] == v1;
    case PointKind::Lpi: return p.key[0] == v0 && p.key[1] == v1;
    case PointKind::Ssi: return (p.key[0] == v0 && p.key[1] == v1) || (p.key[2] == v0 && p.key[3] == v1);
    default: return false;
  }
}

// Three points on one leaf edge's line are a tie the filter never sees; their keys say so without arithmetic.
static bool collinear_by_keys(const ExactPoint &a, const ExactPoint &b, const ExactPoint &c)
{
  if (!a.keyed || !b.keyed || !c.keyed)
    return false;
  for (const ExactPoint *p : {&a, &b, &c})
  {
    if (p->kind != PointKind::Lpi && p->kind != PointKind::Ssi)
      continue;
    for (int e = 0; e < (p->kind == PointKind::Ssi ? 2 : 1); ++e)
    {
      const uint32_t v0 = p->key[2 * e], v1 = p->key[2 * e + 1];
      if (on_edge(a, v0, v1) && on_edge(b, v0, v1) && on_edge(c, v0, v1))
        return true;
    }
  }
  return false;
}

// p lies on plane pl by its construction
static bool on_plane(const ExactPoint &p, uint32_t pl)
{
  switch (p.kind)
  {
    case PointKind::Lpi: return p.key[2] == pl;
    case PointKind::Tpi: return p.key[0] == pl || p.key[1] == pl || p.key[2] == pl;
    default: return false;
  }
}

// Three constructed points of the plane group all built on one other plane lie on the line of the two planes.
static bool collinear_by_planes(const ExactPoint &a, const ExactPoint &b, const ExactPoint &c, uint32_t group)
{
  if (group == NO_INDEX || !a.keyed || !b.keyed || !c.keyed)
    return false;
  const int planes = a.kind == PointKind::Lpi ? 1 : a.kind == PointKind::Tpi ? 3 : 0;
  for (int i = 0; i < planes; ++i)
  {
    const uint32_t pl = a.key[planes == 1 ? 2 : i];
    if (pl != group && on_plane(b, pl) && on_plane(c, pl))
      return true;
  }
  return false;
}

int orient2d(const ExactPoint &a, const ExactPoint &b, const ExactPoint &c, int axis, uint32_t group, fp::Memo *memo)
{
  const int u = (axis + 1) % 3, v = (axis + 2) % 3;
  const Iv au = coord(a, u), av = coord(a, v);
  if (const int s = sign_of((coord(b, u) - au) * (coord(c, v) - av) - (coord(b, v) - av) * (coord(c, u) - au)); decided(s))
    return s;
  if (collinear_by_keys(a, b, c) || collinear_by_planes(a, b, c, group))
    return 0;
  if (a.fp)
    return fp::orient2d(a, b, c, u, v, memo);
  return visit(a, [&](const auto &ha) {
    return visit(b, [&](const auto &hb) { return visit(c, [&](const auto &hc) { return orient2d_exact(ha, hb, hc, u, v); }); });
  });
}

int compare_coord(const ExactPoint &a, const ExactPoint &b, int axis)
{
  if (a.err == 0 && b.err == 0)
    return (a.approx[axis] > b.approx[axis]) - (a.approx[axis] < b.approx[axis]);
  if (const int s = sign_of(coord(a, axis) - coord(b, axis)); decided(s))
    return s;
  if (a.fp)
    return fp::compare(a, b, axis);
  return visit(a, [&](const auto &ha) { return visit(b, [&](const auto &hb) { return compare_exact(ha, hb, axis); }); });
}

bool point_equal(const ExactPoint &a, const ExactPoint &b)
{
  const bool approxExact = a.err == 0 && b.err == 0;
  bool apart = false;
  for (int i = 0; i < 3; ++i)
    apart |= sign_of(coord(a, i) - coord(b, i)) != 0;
  if (apart || approxExact)
    return !apart && a.approx == b.approx;
  ++exact_fallbacks;
  if (a.fp)
    return fp::compare(a, b, 0) == 0 && fp::compare(a, b, 1) == 0 && fp::compare(a, b, 2) == 0;
  return visit(a, [&](const auto &ha) {
    return visit(b, [&](const auto &hb) {
      return int(compare_exact(ha, hb, 0) == 0 && compare_exact(ha, hb, 1) == 0 && compare_exact(ha, hb, 2) == 0);
    });
  }) != 0;
}

int orient3d(const ExactPoint &a, const ExactPoint &b, const ExactPoint &c, const ExactPoint &d)
{
  G_ASSERT(a.kind == PointKind::Grid && b.kind == PointKind::Grid && c.kind == PointKind::Grid && d.kind == PointKind::Grid);
  if (!a.fp)
    return orient3d(a.grid, b.grid, c.grid, d.grid);
  const ExactPoint *row[3] = {&b, &c, &d};
  Iv mi[3][3];
  for (int q = 0; q < 3; ++q)
    for (int i = 0; i < 3; ++i)
      mi[q][i] = rounded(row[q]->approx[i] - a.approx[i]);
  if (const int s = det3_sign(mi); decided(s))
    return s;
  return fp::orient3d(a.approx, b.approx, c.approx, d.approx);
}

int orient3d(const GridPoint &a, const GridPoint &b, const GridPoint &c, const GridPoint &d)
{
  static_assert(limbs_for(sum_bits(3 * (MAX_GRID_BITS + 1), 6)) == 2, "orient3d of grid points fits int128");
  const GridPoint *row[3] = {&b, &c, &d};
  int64_t m[3][3];
  Iv mi[3][3];
  for (int q = 0; q < 3; ++q)
    for (int i = 0; i < 3; ++i)
    {
      m[q][i] = row[q]->c[i] - a.c[i];
      mi[q][i] = exact(double(m[q][i]));
    }
  if (const int s = det3_sign(mi); decided(s))
    return s;
  BigInt<2> s(0);
  for (int i = 0; i < 3; ++i)
  {
    const int j = (i + 1) % 3, k = (i + 2) % 3;
    s = s + mul<2>(BigInt<1>(m[0][i]), mul64(m[1][j], m[2][k]) - mul64(m[1][k], m[2][j]));
  }
  return s.sign();
}

int wedge_order(const Plane &a, const Plane &b, const ExactDir &d)
{
  Iv r = exact(0);
  for (int i = 0; i < 3; ++i)
  {
    const int j = (i + 1) % 3, k = (i + 2) % 3;
    r = r + rounded(d.approx[i], d.fp) *
              (rounded(a.nd[j], a.fp) * rounded(b.nd[k], b.fp) - rounded(a.nd[k], a.fp) * rounded(b.nd[j], b.fp));
  }
  if (const int s = sign_of(r); decided(s))
    return s;
  if (a.fp)
    return fp::wedge_order(a, b, d);
  BigInt<3> ab[3];
  cross(a.n, b.n, ab);
  constexpr int R = limbs_for(sum_bits(2 * CROSS_BITS, 3));
  BigInt<R> s(0);
  for (int i = 0; i < 3; ++i)
    s = s + mul<R>(d.c[i], ab[i]);
  return s.sign();
}

int sign_dot(const Plane &a, const Plane &b)
{
  Iv r = exact(0);
  for (int i = 0; i < 3; ++i)
    r = r + rounded(a.nd[i], a.fp) * rounded(b.nd[i], b.fp);
  if (const int s = sign_of(r); decided(s))
    return s;
  if (a.fp)
    return fp::sign_dot(a, b);
  constexpr int R = limbs_for(sum_bits(2 * NORMAL_BITS, 3));
  BigInt<R> s(0);
  for (int i = 0; i < 3; ++i)
    s = s + mul<R>(a.n[i], b.n[i]);
  return s.sign();
}
} // namespace dacsg::exact
