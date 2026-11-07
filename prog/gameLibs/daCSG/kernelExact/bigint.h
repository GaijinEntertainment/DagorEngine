// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/dag_assert.h>
#include <math/dag_intrin.h>
#include <util/dag_globDef.h>
#include <util/dag_stdint.h>
#include <EASTL/utility.h>
#include <math.h>
#if defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64)
#include <intrin.h>
#endif

namespace dacsg::exact
{
inline void check_overflow(bool overflow)
{
  G_ASSERTF(!overflow, "daCSG: BigInt overflow");
  G_UNUSED(overflow);
}

inline uint64_t mul_u64_portable(uint64_t a, uint64_t b, uint64_t &hi)
{
  const uint64_t a0 = uint32_t(a), a1 = a >> 32, b0 = uint32_t(b), b1 = b >> 32;
  const uint64_t p00 = a0 * b0, p01 = a0 * b1, p10 = a1 * b0;
  const uint64_t mid = (p00 >> 32) + uint32_t(p01) + uint32_t(p10);
  hi = a1 * b1 + (p01 >> 32) + (p10 >> 32) + (mid >> 32);
  return (mid << 32) | uint32_t(p00);
}

// returns the low half of a * b
inline uint64_t mul_u64(uint64_t a, uint64_t b, uint64_t &hi)
{
#if defined(__SIZEOF_INT128__)
  const unsigned __int128 p = (unsigned __int128)a * b;
  hi = uint64_t(p >> 64);
  return uint64_t(p);
#elif defined(_MSC_VER) && defined(_M_X64)
  return _umul128(a, b, &hi);
#else
  return mul_u64_portable(a, b, hi);
#endif
}

inline uint64_t add_carry(uint64_t a, uint64_t b, uint64_t &carry)
{
  const uint64_t s = a + b, r = s + carry;
  carry = uint64_t(s < a) | uint64_t(r < s);
  return r;
}

inline uint64_t sub_borrow(uint64_t a, uint64_t b, uint64_t &borrow)
{
  const uint64_t d = a - b, r = d - borrow;
  borrow = uint64_t(a < b) | uint64_t(d < borrow);
  return r;
}

template <int N>
struct BigInt
{
  uint64_t limb[N]; // two's complement, the lowest limb first

  BigInt() = default;
  BigInt(int64_t v)
  {
    limb[0] = uint64_t(v);
    for (int i = 1; i < N; ++i)
      limb[i] = v < 0 ? ~0ull : 0ull;
  }
  // asserts that a narrowed value fits
  template <int M>
  explicit BigInt(const BigInt<M> &o)
  {
    for (int i = 0; i < N; ++i)
      limb[i] = i < M ? o.limb[i] : o.ext();
    bool lost = negative() != o.negative();
    for (int i = N; i < M; ++i)
      lost |= o.limb[i] != o.ext();
    check_overflow(lost);
  }

  bool negative() const { return int64_t(limb[N - 1]) < 0; }
  uint64_t ext() const { return negative() ? ~0ull : 0ull; }
  bool is_zero() const
  {
    uint64_t any = 0;
    for (int i = 0; i < N; ++i)
      any |= limb[i];
    return any == 0;
  }
  int sign() const { return negative() ? -1 : is_zero() ? 0 : 1; }
};

template <int N>
BigInt<N> wrap_neg(const BigInt<N> &a)
{
  BigInt<N> r;
  uint64_t carry = 1;
  for (int i = 0; i < N; ++i)
    r.limb[i] = add_carry(~a.limb[i], 0, carry);
  return r;
}

template <int N>
BigInt<N> wrap_sub(const BigInt<N> &a, const BigInt<N> &b)
{
  BigInt<N> r;
  uint64_t borrow = 0;
  for (int i = 0; i < N; ++i)
    r.limb[i] = sub_borrow(a.limb[i], b.limb[i], borrow);
  return r;
}

// unsigned; exact for every value, the most negative one included
template <int N>
BigInt<N> magnitude(const BigInt<N> &a)
{
  return a.negative() ? wrap_neg(a) : a;
}

template <int N>
BigInt<N> operator-(const BigInt<N> &a)
{
  const BigInt<N> r = wrap_neg(a);
  check_overflow(a.negative() && r.negative());
  return r;
}

template <int N>
BigInt<N> operator+(const BigInt<N> &a, const BigInt<N> &b)
{
  BigInt<N> r;
  uint64_t carry = 0;
  for (int i = 0; i < N; ++i)
    r.limb[i] = add_carry(a.limb[i], b.limb[i], carry);
  check_overflow(a.negative() == b.negative() && r.negative() != a.negative());
  return r;
}

template <int N>
BigInt<N> operator-(const BigInt<N> &a, const BigInt<N> &b)
{
  const BigInt<N> r = wrap_sub(a, b);
  check_overflow(a.negative() != b.negative() && r.negative() != a.negative());
  return r;
}

template <int N>
int cmp_unsigned(const BigInt<N> &a, const BigInt<N> &b)
{
  for (int i = N - 1; i >= 0; --i)
    if (a.limb[i] != b.limb[i])
      return a.limb[i] < b.limb[i] ? -1 : 1;
  return 0;
}

// the low R limbs of the unsigned product
template <int R, int A, int B>
BigInt<R> mul_unsigned(const BigInt<A> &a, const BigInt<B> &b, bool &lost)
{
  BigInt<R> r(0);
  for (int i = 0; i < A; ++i)
  {
    uint64_t carry = 0;
    for (int j = 0; j < B; ++j)
    {
      uint64_t hi, lo = mul_u64(a.limb[i], b.limb[j], hi);
      lo += carry;
      hi += lo < carry;
      if (i + j < R)
      {
        lo += r.limb[i + j];
        hi += lo < r.limb[i + j];
        r.limb[i + j] = lo;
      }
      else
        lost |= lo != 0;
      carry = hi;
    }
    if (i + B < R)
      r.limb[i + B] = carry;
    else
      lost |= carry != 0;
  }
  return r;
}

// R limbs, A + B when R is 0; asserts that the product fits
template <int R = 0, int A, int B>
BigInt<R ? R : A + B> mul(const BigInt<A> &a, const BigInt<B> &b)
{
  constexpr int N = R ? R : A + B;
  bool lost = false;
  const BigInt<N> r = mul_unsigned<N>(magnitude(a), magnitude(b), lost);
  check_overflow(lost || r.negative());
  return a.negative() != b.negative() ? wrap_neg(r) : r;
}

inline BigInt<2> mul64(int64_t a, int64_t b) { return mul(BigInt<1>(a), BigInt<1>(b)); }

// correctly rounded to nearest
template <int N>
double to_double(const BigInt<N> &a)
{
  const BigInt<N> m = magnitude(a);
  int top = N - 1;
  while (top > 0 && m.limb[top] == 0)
    --top;
  double v = double(m.limb[top]);
  if (top > 0)
  {
    const int lz = int(__clz_unsafe(m.limb[top]));
    const uint64_t bits = lz ? (m.limb[top] << lz) | (m.limb[top - 1] >> (64 - lz)) : m.limb[top];
    uint64_t sticky = m.limb[top - 1] << lz;
    for (int i = 0; i < top - 1; ++i)
      sticky |= m.limb[i];
    v = ldexp(double(bits | uint64_t(sticky != 0)), 64 * top - lz);
  }
  return a.negative() ? -v : v;
}
} // namespace dacsg::exact
