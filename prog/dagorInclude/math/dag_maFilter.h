//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point3.h>

template <class T>
class MeanAvgFilter
{
  T *buf = nullptr;
  int bufLen = 0, bufPos = 0;
  T sum;

public:
  MeanAvgFilter() = default;
  MeanAvgFilter(int n) { init(n); }
  MeanAvgFilter(const MeanAvgFilter &f) { operator=(f); }
  MeanAvgFilter(MeanAvgFilter &&f) : buf(f.buf), bufLen(f.bufLen), bufPos(f.bufPos), sum(f.sum) { f.buf = nullptr; }
  ~MeanAvgFilter()
  {
    if (buf)
      delete[] buf;
    buf = NULL;
  }

  int getBufLen() const { return bufLen; }

  void init(int n)
  {
    if (n < 2)
      return;

    if (buf)
      delete[] buf;
    buf = new (midmem_ptr()) T[n];
    memset(buf, 0, n * sizeof(T));
    bufLen = n;
    bufPos = 0;
    memset(&sum, 0, sizeof(sum));
  }
  void fill(const T &v)
  {
    sum = v * bufLen;
    for (int i = 0; i < bufLen; i++)
      buf[i] = v;
    bufPos = 0;
  }

  void clear()
  {
    if (buf)
    {
      delete[] buf;
      buf = NULL;
    }
    bufLen = bufPos = 0;
  }

  inline void inputVal(T v)
  {
    sum -= buf[bufPos];
    sum += v;
    buf[bufPos] = v;
    bufPos++;
    if (bufPos >= bufLen)
      bufPos = 0;
  }
  inline T outputVal() const { return bufLen ? sum / bufLen : 0; }

  inline MeanAvgFilter &operator=(MeanAvgFilter &&f) = default;
  inline MeanAvgFilter &operator=(const MeanAvgFilter &f)
  {
    init(f.bufLen);
    bufPos = f.bufPos;
    for (int i = 0; i < bufLen; i++)
      buf[i] = f.buf[i];
    sum = f.sum;
    return *this;
  }

  inline void operator=(T x) { inputVal(x); }
  inline operator T() const { return outputVal(); }
  inline const T &operator[](int i) const { return buf[i]; }
  inline T operator[](int i) { return buf[i]; }
};

template <>
inline Point3 MeanAvgFilter<Point3>::outputVal() const
{
  return bufLen ? sum / bufLen : Point3(0, 0, 0);
}

typedef MeanAvgFilter<float> MeanAverageFilter;
typedef MeanAvgFilter<Point3> MeanAverageFilterP3;
