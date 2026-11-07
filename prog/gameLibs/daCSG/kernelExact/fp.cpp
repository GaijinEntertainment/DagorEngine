// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "kernelExact/exactPoint.h"
#include <stddef.h>
#include <string.h>

namespace dacsg::exact
{
const double *ExArena::put(const double *e, int n)
{
  constexpr uint32_t CHUNK = 1 << 16;
  if (chunks.empty() || used + n > CHUNK)
  {
    chunks.push_back(eastl::unique_ptr<double[]>(new double[CHUNK]));
    used = 0;
  }
  double *at = chunks.back().get() + used;
  for (int i = 0; i < n; ++i)
    at[i] = e[i];
  used += n;
  return at;
}

void ExArena::adopt(ExArena &other)
{
  if (other.chunks.empty())
    return;
  for (eastl::unique_ptr<double[]> &c : other.chunks)
    chunks.push_back(eastl::move(c));
  used = other.used;
  other.chunks.clear();
  other.used = 0;
}

namespace fp
{
namespace
{
ExS load(const ExView &v)
{
  ExS r;
  r.n = v.n;
  for (int i = 0; i < v.n; ++i)
    r.e[i] = v.terms()[i];
  return r;
}

constexpr int BLOCK = ExView::PREFIX + int(sizeof(ExS::e) / sizeof(double));

// the block of a stored expansion: its double-double with the bound, then the terms
ExView store(ExArena &arena, const ExS &e)
{
  double block[BLOCK];
  const Dd d = dd_of(e);
  block[0] = d.hi;
  block[1] = d.lo;
  block[2] = d.err;
  memcpy(block + ExView::PREFIX, e.e, e.n * sizeof(double));
  return ExView{arena.put(block, ExView::PREFIX + e.n), uint8_t(e.n)};
}

// a view of a block on the caller's stack with no double-double: load() reads it, dd_view() must not
ExView local_view(const ExS &e, double *block)
{
  memcpy(block + ExView::PREFIX, e.e, e.n * sizeof(double));
  return ExView{block, uint8_t(e.n)};
}

Dd dd_view(const ExView &v) { return Dd{v.e[0], v.e[1], v.e[2]}; }

ExS of(double v)
{
  ExS r;
  from_double(v, r);
  return r;
}

Ex<2> diff2(double x, double y)
{
  Ex<2> r;
  r.n = 0;
  double hi, lo;
  two_sum(x, -y, hi, lo);
  push(r, lo);
  push(r, hi);
  return r;
}

template <int A, int B>
ExT prod(const Ex<A> &a, const Ex<B> &b)
{
  ExT r;
  mul(a, b, r);
  return r;
}

template <int A, int B>
ExT sum(const Ex<A> &a, const Ex<B> &b)
{
  ExT r;
  add_signed(a, b, 1, r);
  compress(r);
  return r;
}

template <int A, int B>
ExT diff(const Ex<A> &a, const Ex<B> &b)
{
  ExT r;
  add_signed(a, b, -1, r);
  compress(r);
  return r;
}

ExS stored(const ExT &t)
{
  ExS r;
  assign(t, r);
  return r;
}

// the point as c / w: an input vertex is its doubles over one
void hom(const ExactPoint &p, ExS c[3], ExS &w)
{
  if (p.kind == PointKind::Grid)
  {
    for (int i = 0; i < 3; ++i)
      c[i] = of(p.approx[i]);
    w = of(1);
    return;
  }
  for (int i = 0; i < 3; ++i)
    c[i] = load(p.fh.c[i]);
  w = load(p.fh.w);
}

void cross(const ExS a[3], const ExS b[3], ExS out[3])
{
  for (int i = 0; i < 3; ++i)
  {
    const int j = (i + 1) % 3, k = (i + 2) % 3;
    out[i] = stored(diff(prod(a[j], b[k]), prod(a[k], b[j])));
  }
}

void plane_normal(const Plane &pl, ExS n[3])
{
  for (int i = 0; i < 3; ++i)
    n[i] = load(pl.fn[i]);
}

// d + n . p for an explicit p, d w + n . c for a constructed one
ExT plane_value(const Plane &pl, const ExS c[3], const ExS &w)
{
  ExT s = prod(load(pl.fd), w);
  for (int i = 0; i < 3; ++i)
    s = sum(s, prod(load(pl.fn[i]), c[i]));
  return s;
}

// |c - q w| <= w ulp(q) / 2 proves the quotient q within half an ulp of c / w
bool within_half_ulp(const ExS &c, const ExS &w, double q)
{
  if (q == 0)
    return is_zero(c);
  const double half = (nextafter(fabs(q), DBL_MAX) - fabs(q)) * 0.5;
  ExT r = diff(c, prod(w, of(q)));
  if (sign(r) < 0)
    negate(r);
  return sign(diff(prod(w, of(half)), r)) >= 0;
}

// the error of the double triple: half an ulp when the residual check proves each quotient correctly rounded, else
// the bound of the estimates (within two ulps each) and the division
void set_approx(ExactPoint &p, const ExS c[3], const ExS &w)
{
  const double wd = estimate(w);
  double top = 0;
  bool rounded = true;
  for (int i = 0; i < 3; ++i)
  {
    p.approx[i] = estimate(c[i]) / wd;
    top = fabs(p.approx[i]) > top ? fabs(p.approx[i]) : top;
    rounded = rounded && within_half_ulp(c[i], w, p.approx[i]);
  }
  // a coordinate that underflowed to zero keeps a nonzero error (the least normal double: a subnormal flushes to zero
  // under fast math), so the expansions decide its equality
  p.err = rounded ? top * 0x1p-52 : max(top * 0x1p-49, 0x1p-1022);
}

// the overflow stays set for arrange to report
bool store_point(ExArena &arena, const ExS c[3], const ExS &w, PointKind kind, ExactPoint &out)
{
  if (ex_overflow)
    return false;
  out.kind = kind;
  out.fp = true;
  for (int i = 0; i < 3; ++i)
    out.fh.c[i] = store(arena, c[i]);
  out.fh.w = store(arena, w);
  set_approx(out, c, w);
  return true;
}
} // namespace

int make_plane(const DPoint3 &a, const DPoint3 &b, const DPoint3 &c, ExArena &arena, Plane &pl)
{
  Ex<2> e1[3], e2[3];
  for (int i = 0; i < 3; ++i)
  {
    e1[i] = diff2(b[i], a[i]);
    e2[i] = diff2(c[i], a[i]);
  }
  ExS n[3];
  for (int i = 0; i < 3; ++i)
  {
    const int j = (i + 1) % 3, k = (i + 2) % 3;
    n[i] = stored(diff(prod(e1[j], e2[k]), prod(e1[k], e2[j])));
  }
  const int orient = sign(n[0]) ? sign(n[0]) : sign(n[1]) ? sign(n[1]) : sign(n[2]);
  if (!orient)
    return 0;
  ExT d;
  d.n = 0;
  for (int i = 0; i < 3; ++i)
  {
    if (orient < 0)
      negate(n[i]);
    d = diff(d, prod(n[i], of(a[i])));
    pl.nd[i] = estimate(n[i]);
  }
  pl.fp = true;
  for (int i = 0; i < 3; ++i)
    pl.fn[i] = store(arena, n[i]);
  pl.fd = store(arena, stored(d));
  pl.dd = estimate(d);
  return ex_overflow ? 0 : orient;
}

// the normals are canonical in sign: the order of (n, d) as ratios to the first non-zero normal component
int plane_cmp(const Plane &a, const Plane &b)
{
  int f = -1;
  for (int i = 0; i < 3 && f < 0; ++i)
  {
    const int sa = a.fn[i].sign(), sb = b.fn[i].sign();
    if (sa != sb)
      return sa < sb ? -1 : 1;
    if (sa)
      f = i;
  }
  const ExView *ac[4] = {&a.fn[0], &a.fn[1], &a.fn[2], &a.fd}, *bc[4] = {&b.fn[0], &b.fn[1], &b.fn[2], &b.fd};
  const double ad[4] = {a.nd[0], a.nd[1], a.nd[2], a.dd}, bd[4] = {b.nd[0], b.nd[1], b.nd[2], b.dd};
  for (int i = f + 1; i < 4; ++i)
  {
    const double ra = ad[i] / ad[f], rb = bd[i] / bd[f];
    if (fabs(ra - rb) > 1e-12 * (fabs(ra) + fabs(rb)))
      return ra < rb ? -1 : 1;
    if (const int s = sign(diff(prod(load(*ac[i]), load(*bc[f])), prod(load(*bc[i]), load(*ac[f])))))
      return s;
  }
  return 0;
}

ExactDir edge_dir(const DPoint3 &from, const DPoint3 &to)
{
  ExactDir d;
  d.fp = true;
  for (int i = 0; i < 3; ++i)
  {
    assign(diff2(to[i], from[i]), d.f[i]);
    d.approx[i] = to[i] - from[i];
  }
  return d;
}

ExactDir line_dir(const Plane &a, const Plane &b)
{
  ExactDir d;
  d.fp = true;
  ExS na[3], nb[3];
  plane_normal(a, na);
  plane_normal(b, nb);
  cross(na, nb, d.f);
  for (int i = 0; i < 3; ++i)
    d.approx[i] = estimate(d.f[i]);
  return d;
}

bool make_lpi(const DPoint3 &p, const DPoint3 &q, const Plane &pl, ExArena &arena, ExactPoint &out)
{
  ExS pe[3], dq[3], n[3];
  for (int i = 0; i < 3; ++i)
  {
    pe[i] = of(p[i]);
    assign(diff2(q[i], p[i]), dq[i]);
  }
  plane_normal(pl, n);
  ExT w, s = prod(load(pl.fd), of(1));
  w.n = 0;
  for (int i = 0; i < 3; ++i)
  {
    w = sum(w, prod(n[i], dq[i]));
    s = sum(s, prod(n[i], pe[i]));
  }
  if (is_zero(w))
    return false;
  if (sign(w) < 0)
  {
    negate(w);
    negate(s);
  }
  ExS c[3];
  for (int i = 0; i < 3; ++i)
    c[i] = stored(diff(prod(pe[i], w), prod(s, dq[i])));
  return store_point(arena, c, stored(w), PointKind::Lpi, out);
}

bool make_ssi(const DPoint3 &p, const DPoint3 &q, const DPoint3 &r, const DPoint3 &s, int axis, ExArena &arena, ExactPoint &out)
{
  const int j = (axis + 1) % 3, k = (axis + 2) % 3;
  ExS n[3];
  n[axis].n = 0;
  assign(diff2(s[k], r[k]), n[j]);
  assign(diff2(r[j], s[j]), n[k]);
  ExT d;
  d.n = 0;
  for (int i = 0; i < 3; ++i)
    d = diff(d, prod(n[i], of(r[i])));
  const ExS ds = stored(d);
  double blocks[4][BLOCK];
  Plane m;
  m.fp = true;
  for (int i = 0; i < 3; ++i)
    m.fn[i] = local_view(n[i], blocks[i]);
  m.fd = local_view(ds, blocks[3]);
  if (!make_lpi(p, q, m, arena, out))
    return false;
  out.kind = PointKind::Ssi;
  return true;
}

bool make_tpi(const Plane &a, const Plane &b, const Plane &c, ExArena &arena, ExactPoint &out)
{
  ExS na[3], nb[3], nc[3], bc[3], ca[3], ab[3];
  plane_normal(a, na);
  plane_normal(b, nb);
  plane_normal(c, nc);
  cross(nb, nc, bc);
  cross(nc, na, ca);
  cross(na, nb, ab);
  ExT w;
  w.n = 0;
  for (int i = 0; i < 3; ++i)
    w = sum(w, prod(na[i], bc[i]));
  if (is_zero(w))
    return false;
  const int flip = sign(w) < 0 ? 1 : -1;
  if (flip > 0)
    negate(w);
  ExS x[3];
  for (int i = 0; i < 3; ++i)
  {
    ExT t = sum(sum(prod(load(a.fd), bc[i]), prod(load(b.fd), ca[i])), prod(load(c.fd), ab[i]));
    if (flip < 0)
      negate(t);
    x[i] = stored(t);
  }
  return store_point(arena, x, stored(w), PointKind::Tpi, out);
}

// The double-double stage of a sign: the interval filter failed on the point's error bound, but the stored
// expansions give each coordinate to 2^-100, which decides a near tie; only a tie or a nearer miss goes to expansions.
void dd_hom(const ExactPoint &p, Dd c[3], Dd &w)
{
  for (int i = 0; i < 3; ++i)
    c[i] = p.kind == PointKind::Grid ? dd_of(p.approx[i]) : dd_view(p.fh.c[i]);
  w = p.kind == PointKind::Grid ? dd_of(1.0) : dd_view(p.fh.w);
}

int point_plane(const ExactPoint &p, const Plane &pl)
{
  Dd dc[3], dw;
  dd_hom(p, dc, dw);
  Dd s = dd_mul(dd_view(pl.fd), dw);
  for (int i = 0; i < 3; ++i)
    s = dd_add(s, dd_mul(dd_view(pl.fn[i]), dc[i]));
  if (const int r = dd_sign(s))
    return r;
  ExS c[3], w;
  hom(p, c, w);
  return sign(plane_value(pl, c, w));
}

// the 3x3 determinant of the rows (c[u], c[v], w)
static int orient2d_sign(const ExactPoint &a, const ExactPoint &b, const ExactPoint &c, int u, int v)
{
  Dd da[3], daw, db[3], dbw, dc[3], dcw;
  dd_hom(a, da, daw);
  dd_hom(b, db, dbw);
  dd_hom(c, dc, dcw);
  const Dd dmv = dd_sub(dd_mul(db[v], dcw), dd_mul(dc[v], dbw)), dmu = dd_sub(dd_mul(db[u], dcw), dd_mul(dc[u], dbw));
  const Dd dmuv = dd_sub(dd_mul(db[u], dc[v]), dd_mul(dc[u], db[v]));
  if (const int r = dd_sign(dd_add(dd_sub(dd_mul(da[u], dmv), dd_mul(da[v], dmu)), dd_mul(daw, dmuv))))
    return r;
  ExS ac[3], aw, bc[3], bw, cc[3], cw;
  hom(a, ac, aw);
  hom(b, bc, bw);
  hom(c, cc, cw);
  const ExT mv = diff(prod(bc[v], cw), prod(cc[v], bw)), mu = diff(prod(bc[u], cw), prod(cc[u], bw));
  const ExT muv = diff(prod(bc[u], cc[v]), prod(cc[u], bc[v]));
  return sign(sum(diff(prod(ac[u], mv), prod(ac[v], mu)), prod(aw, muv)));
}

// the triangulation asks the same triple of constructed points again and again; an unkeyed point (the ray seed)
// carries vertex 0's all-zero name, so it never enters the memo
int orient2d(const ExactPoint &a, const ExactPoint &b, const ExactPoint &c, int u, int v, Memo *memo)
{
  if (!memo || !a.keyed || !b.keyed || !c.keyed)
    return orient2d_sign(a, b, c, u, v);
  if (memo->slots.empty())
    memo->init();
  MemoSlot k;
  const ExactPoint *pts[3] = {&a, &b, &c};
  uint64_t h = 0x9E3779B97F4A7C15ull;
  for (int i = 0; i < 3; ++i)
  {
    k.kind[i] = uint8_t(pts[i]->kind);
    h = (h ^ k.kind[i]) * 0xC2B2AE3D27D4EB4Full;
    for (int j = 0; j < 4; ++j)
    {
      k.key[i][j] = pts[i]->key[j];
      h = (h ^ k.key[i][j]) * 0xC2B2AE3D27D4EB4Full;
    }
  }
  k.axis = uint8_t(3 - u - v);
  h = (h ^ k.axis) * 0xC2B2AE3D27D4EB4Full;
  MemoSlot &slot = memo->slots[(h >> 20) & (memo->slots.size() - 1)];
  if (slot.used && memcmp(&slot, &k, offsetof(MemoSlot, sign)) == 0)
    return slot.sign;
  k.sign = int8_t(orient2d_sign(a, b, c, u, v));
  k.used = true;
  slot = k;
  return k.sign;
}

int compare(const ExactPoint &a, const ExactPoint &b, int axis)
{
  Dd da[3], daw, db[3], dbw;
  dd_hom(a, da, daw);
  dd_hom(b, db, dbw);
  if (const int r = dd_sign(dd_sub(dd_mul(da[axis], dbw), dd_mul(db[axis], daw))))
    return r;
  ExS ac[3], aw, bc[3], bw;
  hom(a, ac, aw);
  hom(b, bc, bw);
  return sign(diff(prod(ac[axis], bw), prod(bc[axis], aw)));
}

int orient3d(const DPoint3 &a, const DPoint3 &b, const DPoint3 &c, const DPoint3 &d)
{
  const DPoint3 *row[3] = {&b, &c, &d};
  Ex<2> m[3][3];
  for (int q = 0; q < 3; ++q)
    for (int i = 0; i < 3; ++i)
      m[q][i] = diff2((*row[q])[i], a[i]);
  ExT s;
  s.n = 0;
  for (int i = 0; i < 3; ++i)
  {
    const int j = (i + 1) % 3, k = (i + 2) % 3;
    s = sum(s, prod(m[0][i], diff(prod(m[1][j], m[2][k]), prod(m[1][k], m[2][j]))));
  }
  return sign(s);
}

int wedge_order(const Plane &a, const Plane &b, const ExactDir &d)
{
  ExS na[3], nb[3], ab[3];
  plane_normal(a, na);
  plane_normal(b, nb);
  cross(na, nb, ab);
  ExT s;
  s.n = 0;
  for (int i = 0; i < 3; ++i)
    s = sum(s, prod(d.f[i], ab[i]));
  return sign(s);
}

int sign_dot(const Plane &a, const Plane &b)
{
  ExT s;
  s.n = 0;
  for (int i = 0; i < 3; ++i)
    s = sum(s, prod(load(a.fn[i]), load(b.fn[i])));
  return sign(s);
}

int beyond_sign(const Plane &t, const Plane &f, const DPoint3 &q, int axis)
{
  const int u = (axis + 1) % 3, v = (axis + 2) % 3;
  const int axisSigns = t.fn[axis].sign() * f.fn[axis].sign();
  ExS c[3], w = of(1);
  for (int i = 0; i < 3; ++i)
    c[i] = of(i == axis ? 0.0 : q[i]);
  const ExT at = plane_value(t, c, w), af = plane_value(f, c, w);
  const ExS tn = load(t.fn[axis]), fn = load(f.fn[axis]);
  if (const int s = sign(diff(prod(af, tn), prod(at, fn))))
    return s * axisSigns;
  if (const int s = sign(diff(prod(load(f.fn[u]), tn), prod(load(t.fn[u]), fn))))
    return s * axisSigns;
  return sign(diff(prod(load(f.fn[v]), tn), prod(load(t.fn[v]), fn))) * axisSigns;
}

// the largest component by its exact value, the choice the int64grid backend makes
static int largest(const ExS v[3])
{
  int axis = 0;
  for (int i = 1; i < 3; ++i)
  {
    ExS a = v[i], b = v[axis];
    if (sign(a) < 0)
      negate(a);
    if (sign(b) < 0)
      negate(b);
    if (sign(diff(a, b)) > 0)
      axis = i;
  }
  return axis;
}

int plane_axis(const Plane &pl)
{
  ExS n[3];
  plane_normal(pl, n);
  return largest(n);
}

int dir_axis(const ExactDir &d) { return largest(d.f); }
} // namespace fp
} // namespace dacsg::exact
