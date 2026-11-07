// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_bitwise_cast.h>
#include <float.h>
#include <math.h>

namespace dacsg::exact
{
// A sum of doubles that do not overlap, the smallest first (Shewchuk 1997): every operation below is exact.
// A result that needs more than N terms is cut and raises the sticky overflow, which the kernel reads as an error.
template <int N>
struct Ex
{
  int n; // every operation sets it; trivial, so a union can hold one
  double e[N];
};

using ExS = Ex<24>;  // a stored value: a plane, a direction, a point
using ExT = Ex<128>; // a temporary of a predicate

inline thread_local bool ex_overflow = false;

inline bool take_ex_overflow()
{
  const bool o = ex_overflow;
  ex_overflow = false;
  return o;
}

inline void two_sum(double a, double b, double &x, double &y)
{
  x = a + b;
  const double bv = x - a, av = x - bv;
  y = (a - av) + (b - bv);
}

// |a| >= |b|
inline void fast_two_sum(double a, double b, double &x, double &y)
{
  x = a + b;
  y = b - (x - a);
}

// a product of a and b can reach 2^1000
inline bool far_product(double a, double b)
{
  return (dag::bit_cast<uint64_t>(a) >> 52 & 0x7ff) + (dag::bit_cast<uint64_t>(b) >> 52 & 0x7ff) > 2 * 1023 + 998;
}

// A far product is flagged unmade, so no product or sum of products is an infinity; a product of nonzero operands that
// underflows to zero loses its sign
inline void two_prod(double a, double b, double &x, double &y)
{
  const bool far = far_product(a, b);
  x = far ? 0 : a * b;
  y = far ? 0 : fma(a, b, -x);
  ex_overflow |= far || (x == 0 && a != 0 && b != 0);
}

template <int N>
inline void push(Ex<N> &h, double v)
{
  if (v == 0)
    return;
  if (h.n == N)
  {
    ex_overflow = true;
    return;
  }
  h.e[h.n++] = v;
}

template <int R, int A>
void assign(const Ex<A> &a, Ex<R> &h)
{
  h.n = 0;
  for (int i = 0; i < a.n; ++i)
    push(h, a.e[i]);
}

template <int R>
void from_double(double v, Ex<R> &h)
{
  h.n = 0;
  push(h, v);
}

// h = a + sign * b, by a merge of the two sequences in the order of magnitude
template <int R, int A, int B>
void add_signed(const Ex<A> &a, const Ex<B> &b, double sign, Ex<R> &h)
{
  h.n = 0;
  int i = 0, j = 0;
  auto next = [&]() { return j >= b.n || (i < a.n && fabs(a.e[i]) < fabs(b.e[j])) ? a.e[i++] : sign * b.e[j++]; };
  const int total = a.n + b.n;
  if (total == 0)
    return;
  double q = next();
  for (int k = 1; k < total; ++k)
  {
    double r, low;
    two_sum(q, next(), r, low);
    push(h, low);
    q = r;
  }
  push(h, q);
}

template <int R, int A>
void scale(const Ex<A> &a, double b, Ex<R> &h)
{
  h.n = 0;
  if (a.n == 0 || b == 0)
    return;
  double q, low;
  two_prod(a.e[0], b, q, low);
  push(h, low);
  for (int i = 1; i < a.n; ++i)
  {
    double p, pe, s;
    two_prod(a.e[i], b, p, pe);
    two_sum(q, pe, s, low);
    push(h, low);
    fast_two_sum(p, s, q, low);
    push(h, low);
  }
  push(h, q);
}

// the same value in the fewest terms that do not overlap and are not adjacent
template <int N>
void compress(Ex<N> &e)
{
  if (e.n < 2)
    return;
  int bottom = e.n - 1;
  double q = e.e[bottom], r, low;
  for (int i = e.n - 2; i >= 0; --i)
  {
    fast_two_sum(q, e.e[i], r, low);
    if (low != 0)
    {
      e.e[bottom--] = r;
      q = low;
    }
    else
      q = r;
  }
  int top = 0;
  for (int i = bottom + 1; i < e.n; ++i)
  {
    fast_two_sum(e.e[i], q, r, low);
    if (low != 0)
      e.e[top++] = low;
    q = r;
  }
  e.e[top] = q;
  e.n = top + 1;
}

template <int N>
void negate(Ex<N> &e)
{
  for (int i = 0; i < e.n; ++i)
    e.e[i] = -e.e[i];
}

// h = a * b, compressed after every term of b so that no temporary outgrows R
template <int R, int A, int B>
void mul(const Ex<A> &a, const Ex<B> &b, Ex<R> &h)
{
  Ex<2 * A> term;
  Ex<R> other;
  Ex<R> *acc = &h, *next = &other;
  acc->n = 0;
  for (int i = 0; i < b.n; ++i)
  {
    scale(a, b.e[i], term);
    add_signed(*acc, term, 1, *next);
    if (next->n > R / 2 || i + 1 == b.n)
      compress(*next);
    Ex<R> *t = acc;
    acc = next;
    next = t;
  }
  if (acc != &h)
    h = *acc;
}

template <int N>
int sign(const Ex<N> &e)
{
  return e.n == 0 ? 0 : e.e[e.n - 1] > 0 ? 1 : -1;
}

// within two ulps of the value for a compressed expansion
template <int N>
double estimate(const Ex<N> &e)
{
  double q = 0;
  for (int i = 0; i < e.n; ++i)
    q += e.e[i];
  return q;
}

template <int N>
bool is_zero(const Ex<N> &e)
{
  return e.n == 0;
}

// A double-double with an absolute error bound: the stage between the interval filter and the expansions, which
// decides a sign down to 2^-100 of the operands. The bound takes 2^-100 of each result for its own rounding.
struct Dd
{
  double hi, lo, err;
};

inline Dd dd_of(double x) { return Dd{x, 0, 0}; }

// the estimate with the bound of its roundings: each partial sum rounds within its own ulp
template <int N>
double estimate_bounded(const Ex<N> &e, double &bound)
{
  double q = 0;
  bound = 0;
  for (int i = 0; i < e.n; ++i)
  {
    q += e.e[i];
    bound += fabs(q) * 0x1p-52;
  }
  return q;
}

// hi + lo within err of the value: the estimate, then the estimate of the exact remainder, renormalized
template <int N>
Dd dd_of(const Ex<N> &e)
{
  const double hi = estimate(e);
  Ex<1> minus;
  from_double(-hi, minus);
  Ex<N + 1> rem;
  add_signed(e, minus, 1, rem);
  Dd r;
  const double lo = estimate_bounded(rem, r.err);
  fast_two_sum(hi, lo, r.hi, r.lo);
  r.err += DBL_MIN;
  return r;
}

inline double dd_abs(const Dd &a) { return fabs(a.hi) + fabs(a.lo); }

inline Dd dd_add(const Dd &a, const Dd &b)
{
  double s, e;
  two_sum(a.hi, b.hi, s, e);
  e += a.lo + b.lo;
  Dd r;
  fast_two_sum(s, e, r.hi, r.lo);
  r.err = a.err + b.err + (dd_abs(a) + dd_abs(b)) * 0x1p-100 + DBL_MIN;
  return r;
}

inline Dd dd_neg(const Dd &a) { return Dd{-a.hi, -a.lo, a.err}; }
inline Dd dd_sub(const Dd &a, const Dd &b) { return dd_add(a, dd_neg(b)); }

// a far product of the values or the bounds is undecided, unmade: the expansions decide or flag it
inline Dd dd_mul(const Dd &a, const Dd &b)
{
  const double aa = dd_abs(a), ab = dd_abs(b);
  if (far_product(aa > a.err ? aa : a.err, ab > b.err ? ab : b.err))
    return Dd{0, 0, 0x1p1000};
  double p, e;
  two_prod(a.hi, b.hi, p, e);
  e += a.hi * b.lo + a.lo * b.hi;
  Dd r;
  fast_two_sum(p, e, r.hi, r.lo);
  r.err = a.err * ab + b.err * aa + a.err * b.err + aa * ab * 0x1p-100 + DBL_MIN;
  return r;
}

// 0 when the bound holds zero
inline int dd_sign(const Dd &a) { return fabs(a.hi) - fabs(a.lo) > a.err ? (a.hi > 0 ? 1 : -1) : 0; }
} // namespace dacsg::exact
