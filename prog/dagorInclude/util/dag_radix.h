//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

//
// inspired by Zeux code
//

struct radix_unsigned_int_predicate
{
  unsigned int operator()(const unsigned int &v) const { return v; }
};

struct radix_int_predicate
{
  unsigned int operator()(const int &v) const
  {
    // flip sign bit
    return *reinterpret_cast<const unsigned int *>(&v) ^ 0x80000000;
  }
};

struct radix_unsigned_float_predicate
{
  unsigned int operator()(const float &v) const { return *reinterpret_cast<const unsigned int *>(&v); }
};

struct radix_float_predicate
{
  unsigned int operator()(const float &v) const
  {
    // if sign bit is 0, flip sign bit
    // if sign bit is 1, flip everything
    unsigned int f = *reinterpret_cast<const unsigned int *>(&v);
    unsigned int mask = -int(f >> 31) | 0x80000000;
    return f ^ mask;
  }
};

inline void radix_sort_3pass_compute_offsets(unsigned int *h0, unsigned int *h1, unsigned int *h2)
{
  unsigned int sum0 = 0, sum1 = 0, sum2 = 0;
  unsigned int tsum;

  for (unsigned int i = 0; i < 2048; i += 4)
  {
    tsum = h0[i + 0] + sum0;
    h0[i + 0] = sum0 - 1;
    sum0 = tsum;
    tsum = h0[i + 1] + sum0;
    h0[i + 1] = sum0 - 1;
    sum0 = tsum;
    tsum = h0[i + 2] + sum0;
    h0[i + 2] = sum0 - 1;
    sum0 = tsum;
    tsum = h0[i + 3] + sum0;
    h0[i + 3] = sum0 - 1;
    sum0 = tsum;

    tsum = h1[i + 0] + sum1;
    h1[i + 0] = sum1 - 1;
    sum1 = tsum;
    tsum = h1[i + 1] + sum1;
    h1[i + 1] = sum1 - 1;
    sum1 = tsum;
    tsum = h1[i + 2] + sum1;
    h1[i + 2] = sum1 - 1;
    sum1 = tsum;
    tsum = h1[i + 3] + sum1;
    h1[i + 3] = sum1 - 1;
    sum1 = tsum;

    tsum = h2[i + 0] + sum2;
    h2[i + 0] = sum2 - 1;
    sum2 = tsum;
    tsum = h2[i + 1] + sum2;
    h2[i + 1] = sum2 - 1;
    sum2 = tsum;
    tsum = h2[i + 2] + sum2;
    h2[i + 2] = sum2 - 1;
    sum2 = tsum;
    tsum = h2[i + 3] + sum2;
    h2[i + 3] = sum2 - 1;
    sum2 = tsum;
  }
}

inline void radix_sort_4pass_compute_offsets(unsigned int *h0, unsigned int *h1, unsigned int *h2, unsigned int *h3)
{
  unsigned int sum0 = 0, sum1 = 0, sum2 = 0, sum3 = 0;
  unsigned int tsum;

  for (unsigned int i = 0; i < 256; i += 4)
  {
    tsum = h0[i + 0] + sum0;
    h0[i + 0] = sum0 - 1;
    sum0 = tsum;
    tsum = h0[i + 1] + sum0;
    h0[i + 1] = sum0 - 1;
    sum0 = tsum;
    tsum = h0[i + 2] + sum0;
    h0[i + 2] = sum0 - 1;
    sum0 = tsum;
    tsum = h0[i + 3] + sum0;
    h0[i + 3] = sum0 - 1;
    sum0 = tsum;

    tsum = h1[i + 0] + sum1;
    h1[i + 0] = sum1 - 1;
    sum1 = tsum;
    tsum = h1[i + 1] + sum1;
    h1[i + 1] = sum1 - 1;
    sum1 = tsum;
    tsum = h1[i + 2] + sum1;
    h1[i + 2] = sum1 - 1;
    sum1 = tsum;
    tsum = h1[i + 3] + sum1;
    h1[i + 3] = sum1 - 1;
    sum1 = tsum;

    tsum = h2[i + 0] + sum2;
    h2[i + 0] = sum2 - 1;
    sum2 = tsum;
    tsum = h2[i + 1] + sum2;
    h2[i + 1] = sum2 - 1;
    sum2 = tsum;
    tsum = h2[i + 2] + sum2;
    h2[i + 2] = sum2 - 1;
    sum2 = tsum;
    tsum = h2[i + 3] + sum2;
    h2[i + 3] = sum2 - 1;
    sum2 = tsum;

    tsum = h3[i + 0] + sum3;
    h3[i + 0] = sum3 - 1;
    sum3 = tsum;
    tsum = h3[i + 1] + sum3;
    h3[i + 1] = sum3 - 1;
    sum3 = tsum;
    tsum = h3[i + 2] + sum3;
    h3[i + 2] = sum3 - 1;
    sum3 = tsum;
    tsum = h3[i + 3] + sum3;
    h3[i + 3] = sum3 - 1;
    sum3 = tsum;
  }
}

#define RADIX_PASS(src, src_end, dst, hist, func) \
  for (const T *i = src; i != src_end; ++i)       \
  {                                               \
    unsigned int h = pred(*i);                    \
    dst[++hist[func(h)]] = *i;                    \
  }

#define RADIX_PASS4(src, src_end, dst, hist, func) \
  for (const T *i = src; i != src_end; i += 4)     \
  {                                                \
    unsigned int p0 = pred(*(i + 0));              \
    unsigned int p1 = pred(*(i + 1));              \
    unsigned int p2 = pred(*(i + 2));              \
    unsigned int p3 = pred(*(i + 3));              \
                                                   \
    dst[++hist[func(p0)]] = *(i + 0);              \
    dst[++hist[func(p1)]] = *(i + 1);              \
    dst[++hist[func(p2)]] = *(i + 2);              \
    dst[++hist[func(p3)]] = *(i + 3);              \
  }

template <typename T, typename Pred>
inline T *radix_sort_3pass(T *e0, T *e1, size_t count, Pred pred)
{
  unsigned int histograms[2048 * 3];

  for (size_t i = 0; i < 2048 * 3; i += 4)
  {
    histograms[i + 0] = 0;
    histograms[i + 1] = 0;
    histograms[i + 2] = 0;
    histograms[i + 3] = 0;
  }

  unsigned int *h0 = histograms;
  unsigned int *h1 = histograms + 2048;
  unsigned int *h2 = histograms + 2048 * 2;

  T *e0_end = e0 + count;
  T *e1_end = e1 + count;

#define _0(h) ((h)&2047)
#define _1(h) (((h) >> 11) & 2047)
#define _2(h) ((h) >> 22)

  // fill histogram
  if (count & 3)
  {
    for (const T *i = e0; i != e0_end; ++i)
    {
      unsigned int h = pred(*i);

      h0[_0(h)]++;
      h1[_1(h)]++;
      h2[_2(h)]++;
    }
  }
  else
  {
    for (const T *i = e0; i != e0_end; i += 4)
    {
      unsigned int p0 = pred(*(i + 0));
      unsigned int p1 = pred(*(i + 1));
      unsigned int p2 = pred(*(i + 2));
      unsigned int p3 = pred(*(i + 3));

      h0[_0(p0)]++;
      h1[_1(p0)]++;
      h2[_2(p0)]++;
      h0[_0(p1)]++;
      h1[_1(p1)]++;
      h2[_2(p1)]++;
      h0[_0(p2)]++;
      h1[_1(p2)]++;
      h2[_2(p2)]++;
      h0[_0(p3)]++;
      h1[_1(p3)]++;
      h2[_2(p3)]++;
    }
  }

  // compute offsets
  radix_sort_3pass_compute_offsets(h0, h1, h2);

  if (count & 3)
  {
    RADIX_PASS(e0, e0_end, e1, h0, _0);
    RADIX_PASS(e1, e1_end, e0, h1, _1);
    RADIX_PASS(e0, e0_end, e1, h2, _2);
  }
  else
  {
    RADIX_PASS4(e0, e0_end, e1, h0, _0);
    RADIX_PASS4(e1, e1_end, e0, h1, _1);
    RADIX_PASS4(e0, e0_end, e1, h2, _2);
  }

#undef _0
#undef _1
#undef _2

  return e1;
}

template <typename T, typename Pred>
T *radix_sort_4pass(T *e0, T *e1, size_t count, Pred pred)
{
  unsigned int histograms[256 * 4];

  for (size_t i = 0; i < 256 * 4; i += 4)
  {
    histograms[i + 0] = 0;
    histograms[i + 1] = 0;
    histograms[i + 2] = 0;
    histograms[i + 3] = 0;
  }

  unsigned int *h0 = histograms;
  unsigned int *h1 = histograms + 256;
  unsigned int *h2 = histograms + 256 * 2;
  unsigned int *h3 = histograms + 256 * 3;

  T *e0_end = e0 + count;
  T *e1_end = e1 + count;

#define _0(h) ((h)&255)
#define _1(h) (((h) >> 8) & 255)
#define _2(h) (((h) >> 16) & 255)
#define _3(h) ((h) >> 24)

  // fill histogram
  if (count & 3)
  {
    for (const T *i = e0; i != e0_end; ++i)
    {
      unsigned int h = pred(*i);

      h0[_0(h)]++;
      h1[_1(h)]++;
      h2[_2(h)]++;
      h3[_3(h)]++;
    }
  }
  else
  {
    for (const T *i = e0; i != e0_end; i += 4)
    {
      unsigned int p0 = pred(*(i + 0));
      unsigned int p1 = pred(*(i + 1));
      unsigned int p2 = pred(*(i + 2));
      unsigned int p3 = pred(*(i + 3));

      h0[_0(p0)]++;
      h1[_1(p0)]++;
      h2[_2(p0)]++;
      h3[_3(p0)]++;
      h0[_0(p1)]++;
      h1[_1(p1)]++;
      h2[_2(p1)]++;
      h3[_3(p1)]++;
      h0[_0(p2)]++;
      h1[_1(p2)]++;
      h2[_2(p2)]++;
      h3[_3(p2)]++;
      h0[_0(p3)]++;
      h1[_1(p3)]++;
      h2[_2(p3)]++;
      h3[_3(p3)]++;
    }
  }

  // compute offsets
  radix_sort_4pass_compute_offsets(h0, h1, h2, h3);

  if (count & 3)
  {
    RADIX_PASS(e0, e0_end, e1, h0, _0);
    RADIX_PASS(e1, e1_end, e0, h1, _1);
    RADIX_PASS(e0, e0_end, e1, h2, _2);
    RADIX_PASS(e1, e1_end, e0, h3, _3);
  }
  else
  {
    RADIX_PASS4(e0, e0_end, e1, h0, _0);
    RADIX_PASS4(e1, e1_end, e0, h1, _1);
    RADIX_PASS4(e0, e0_end, e1, h2, _2);
    RADIX_PASS4(e1, e1_end, e0, h3, _3);
  }

#undef _0
#undef _1
#undef _2
#undef _3

  return e0;
}

#undef RADIX_PASS
#undef RADIX_PASS4
