//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <generic/dag_smallTab.h>
#include <util/dag_globDef.h>
#include <memory/dag_mem.h>

typedef Point3 BezierPoint3;
#define Bezierinline __forceinline

template <class BezierPoint>
class BezierSplineInt
{
protected:
public:
  DAG_DECLARE_NEW(tmpmem)

  float len, tlen;
  float s2T[3];
  BezierPoint sk[4];
  Bezierinline BezierPoint *get_sk() const { return sk; }
  Bezierinline BezierSplineInt() { len = 1; }

  //! computes point on curve (using parameter t [0..1], not distance s!)
  Bezierinline BezierPoint point(float t) const { return ((sk[3] * t + sk[2]) * t + sk[1]) * t + sk[0]; }

  //! computes tangent on curve, first derivative by parameter t, not distance s!
  //! must be normalized to be used as direction
  Bezierinline BezierPoint tang(float t) const { return (sk[3] * 3 * t + sk[2] * 2) * t + sk[1]; }

  //! computes second first derivative on curve by parameter t, not distance s!
  //! nothing common with curvature - use k3, k2 or k2s instead!
  Bezierinline BezierPoint criv(float t) const { return sk[3] * 6 * t + sk[2] * 2; }

  Bezierinline void calculate(const BezierPoint *V)
  {
    sk[0] = V[0];
    sk[1] = 3.0 * (V[1] - V[0]);
    sk[2] = 3.0 * (V[0] - 2.0 * V[1] + V[2]);
    sk[3] = (V[3] - V[0] + 3.0 * (V[1] - V[2]));
    len = getLength();
    calculateS2T();
  }
  void calculateBack(BezierPoint *V) const
  {
    V[0] = sk[0];
    V[1] = sk[1] / 3.0 + sk[0];
    V[2] = sk[2] / 3.0 + 2.0 * sk[1] / 3.0 + sk[0];
    V[3] = sk[0] + sk[1] + sk[2] + sk[3];
  }

  Bezierinline void calculateCatmullRom(const BezierPoint *V)
  {
    sk[0] = V[1];
    sk[1] = 0.5 * (V[2] - V[0]);
    sk[2] = V[0] - 2.5 * V[1] + 2.0 * V[2] - 0.5 * V[3];
    sk[3] = 0.5 * (V[3] - V[0]) + 1.5 * (V[1] - V[2]);
    len = getLength();
    calculateS2T();
  }
  void calculateS2T()
  {
    /*float len1 = getLength(0.5);
    s2T[1] = (0.5*len - len1)/(len*len1*(len1-len));
    s2T[0] = (1.0f-s2T[1]*len*len)/len;*/
    float L1 = getLength(1.0f / 3.0f);
    float L2 = getLength(2.0f / 3.0f);
    float L1_2 = L1 * L1, L2_2 = L2 * L2, len_2 = len * len;
    float L2_3 = L2_2 * L2, len_3 = len_2 * len;
    if (len < 1e-6f)
      goto degen;
    s2T[2] = 1 / 3.0f * ((-len_2 + len * L2) * L2 + (-3 * L2_2 + 2 * len_2 + (3 * L2 - 2 * len) * L1) * L1) / len / L2 /
             ((-len_2 + len * L2) * L2 + (len_2 - L2_2 + (-len + L2) * L1) * L1) / L1;
    s2T[1] = -1 / 3.0f * ((-len_3 + len * L2_2) * L2 + (-3 * L2_3 + 2 * len_3 + (3 * L2 - 2 * len) * L1_2) * L1) / len / L1 / L2 /
             ((-len_2 + len * L2) * L2 + (len_2 - L2_2 + (-len + L2) * L1) * L1);
    s2T[0] = 1 / 3.0f * ((-len_3 + L2 * len_2) * L2_2 + (-3 * L2_3 + 2 * len_3 + (-2 * len_2 + 3 * L2_2) * L1) * L1_2) / len / L1 /
             L2 / ((-len_2 + len * L2) * L2 + (len_2 - L2_2 + (-len + L2) * L1) * L1);

    if (check_nan(s2T[0]) || check_nan(s2T[1]) || check_nan(s2T[2]))
    {
    degen:
      len = s2T[0] = s2T[1] = s2T[2] = 0;
      sk[0] = point(0);
      sk[1].zero();
      sk[2].zero();
      sk[3].zero();
    }
  }
  float getTFromS(float s) const
  {
    float t = ((s2T[2] * s + s2T[1]) * s + s2T[0]) * s; // s2T[3]*s+
    if (t < 0)
      return 0;
    if (t > 1.0f)
      return 1.0f;
    return t;
    // return ((s2T[1])*s+s2T[0])*s;//s2T[3]*s+
  }
  Bezierinline float getLength() const
  {
    float a = 9 * (sk[3] * sk[3]);
    float b = 12 * (sk[3] * sk[2]);
    float c = 4 * (sk[2] * sk[2]) + 6 * (sk[3] * sk[1]);
    float d = 4 * (sk[2] * sk[1]);
    float e = (sk[1] * sk[1]);
#define CONST1 0.887298334620741689f
#define CONST2 0.5f
#define CONST3 0.112701665379258312f
    // Due to float imprecisions calculated value might become negative (hence safe_sqrt)
    return (5 * safe_sqrt((((a * CONST1 + b) * CONST1 + c) * CONST1 + d) * CONST1 + e) +
             8 * safe_sqrt((((a * CONST2 + b) * CONST2 + c) * CONST2 + d) * CONST2 + e) +
             5 * safe_sqrt((((a * CONST3 + b) * CONST3 + c) * CONST3 + d) * CONST3 + e)) *
           0.055555555555555556f;
#undef CONST1
#undef CONST2
#undef CONST3
  }
  float getLength(float at) const
  {
    float at2 = at * at;
    float at3 = at2 * at;
    float a = 9 * (sk[3] * sk[3] * at3 * at3);
    float b = 12 * (sk[3] * sk[2] * at3 * at2);
    float c = (4 * (sk[2] * sk[2]) + 6 * (sk[3] * sk[1])) * at2 * at2;
    float d = 4 * (sk[2] * sk[1] * at3);
    float e = (sk[1] * sk[1] * at2);
#define CONST1 0.887298334620741689f
#define CONST2 0.5f
#define CONST3 0.112701665379258312f
    // Due to float imprecisions calculated value might become negative (hence safe_sqrt)
    return (5 * safe_sqrt((((a * CONST1 + b) * CONST1 + c) * CONST1 + d) * CONST1 + e) +
             8 * safe_sqrt((((a * CONST2 + b) * CONST2 + c) * CONST2 + d) * CONST2 + e) +
             5 * safe_sqrt((((a * CONST3 + b) * CONST3 + c) * CONST3 + d) * CONST3 + e)) *
           0.055555555555555556f;
#undef CONST1
#undef CONST2
#undef CONST3
  }

  //! absolute curvature (1/rad of curv) in 3D-space
  Bezierinline float k3(float t) const
  {
    BezierPoint a = criv(t), v = tang(t);
    float vl = v.lengthSq();
    return length(v % a) / (vl * sqrtf(vl));
  }

  //! absolute curvature (1/rad of curv) of projection to 2D-space
  Bezierinline float k2(float t, int ax = 1) const
  {
    BezierPoint a = criv(t), v = tang(t);
    a[ax] = v[ax] = 0;
    float vl = v.lengthSq();
    return length(v % a) / (vl * sqrtf(vl));
  }

  //! signed curvature (1/rad of curv) of projection to 2D-space,
  //! positive when curves to the right along the spline
  Bezierinline float k2s(float t, int ax = 1) const
  {
    BezierPoint a = criv(t), v = tang(t);
    a[ax] = v[ax] = 0;
    float vl = v.lengthSq();
    return (v % a)[1] / (vl * sqrtf(vl));
  }
};

template <class BezierPoint, class BezierSplineSegment, class Mem = MidmemAlloc>
class BezierSpline
{
public:
  DAG_DECLARE_NEW(midmem)

protected:
  bool closed;

public:
  float leng;
  SmallTab<BezierSplineSegment, Mem> segs;
  Bezierinline BezierSpline() { closed = false; }
  bool isClosed() const { return closed; }
  float getLength() const { return leng; }
  int findSegmentClamped(float t, float &segt) const // return seg id
  {
    int a = 0, b = segs.size() - 1;
    if (b == 0 || segs[a].tlen >= t)
    {
      segt = segs[0].getTFromS(t);
      return a;
    }
    if (segs[b - 1].tlen <= t)
    {
      segt = segs[b].getTFromS(t - segs[b - 1].tlen);
      return b;
    }
    while (a <= b)
    {
      int c = (a + b) / 2;
      float tlen = segs[c].tlen;
      if (c == a)
      {
        segt = segs[c].getTFromS(t + segs[c].len - tlen);
        // segt=1+(t-tlen)/segs[c].len;
        return a;
      }
      if (tlen < t)
        a = c;
      else if (segs[c - 1].tlen > t)
        b = c;
      else
      {
        segt = segs[c].getTFromS(t + segs[c].len - tlen);
        // segt=1+(t-tlen)/segs[c].len;
        return c;
      }
    }
    segt = segs[b].getTFromS(t + segs[b].len - segs[b].tlen);
    // segt=1+(t-segs[b].tlen)/segs[b].len;
    return b;
  }
  int findSegment(float t, float &segt) const // return seg id
  {
    if (closed)
    {
      while (t > leng)
        t -= leng;
      while (t < 0)
        t += leng;
    }
    else
    {
      if (t > leng)
        t = leng - 0.0001f;
      else if (t < 0)
        t = 0;
    }
    return findSegmentClamped(t, segt);
  }

  Bezierinline BezierPoint interp_pt(float t) const
  { //[0,1)
    float st;
    int i = findSegmentClamped(t, st);
    return segs[i].point(st);
  }
  Bezierinline BezierPoint get_pt(float t) const
  { //(-inf,inf)
    float st;
    int i = findSegment(t, st);
    return segs[i].point(st);
  }
  Bezierinline BezierPoint interp_tang(float t) const
  { //[0,1)
    float st;
    int i = findSegmentClamped(t, st);
    return segs[i].tang(st);
  }
  Bezierinline BezierPoint get_tang(float t) const
  { //(-inf,inf)
    float st;
    int i = findSegment(t, st);
    return segs[i].tang(st);
  }
  Bezierinline BezierPoint interp_criv(float t) const
  { //[0,1)
    float st;
    int i = findSegmentClamped(t, st);
    return segs[i].criv(st);
  }
  Bezierinline BezierPoint get_criv(float t) const
  { //(-inf,inf)
    float st;
    int i = findSegment(t, st);
    return segs[i].criv(st);
  }
  Bezierinline float interp_k3(float t) const
  { //[0,1)
    float st;
    int i = findSegmentClamped(t, st);
    return segs[i].k3(st);
  }
  Bezierinline float get_k3(float t) const
  { //(-inf,inf)
    float st;
    int i = findSegment(t, st);
    return segs[i].k3(st);
  }
  Bezierinline float interp_k2(float t, int ax = 1) const
  { //[0,1)
    float st;
    int i = findSegmentClamped(t, st);
    return segs[i].k2(st, ax);
  }
  Bezierinline float get_k2(float t, int ax = 1) const
  { //(-inf,inf)
    float st;
    int i = findSegment(t, st);
    return segs[i].k2(st, ax);
  }

  void calculateLength()
  {
    leng = 0;
    for (int i = 0; i < segs.size(); ++i)
    {
      leng += segs[i].len;
      segs[i].tlen = leng;
    }
  }

  bool calculate(const BezierPoint *pt, int nm, bool clsd)
  {

    if (nm < 5)
      return false;

    closed = clsd;
    int seg_count = (nm - 1) / 3 + (closed ? 1 : 0);
    clear_and_resize(segs, seg_count);
    BezierPoint V[4];
    int i, use;
    for (i = 0, use = 0; i < seg_count - (closed ? 1 : 0); ++i, use += 3)
    {
      V[0] = pt[use + 1];
      V[1] = pt[use + 2];
      V[2] = pt[use + 3];
      V[3] = pt[use + 4];
      segs[i].calculate(V);
    }
    if (closed)
    {
      V[0] = pt[nm - 2];
      V[1] = pt[nm - 1];
      V[2] = pt[0];
      V[3] = pt[1];
      segs[i].calculate(V);
    }
    calculateLength();
    return true;
  }

  bool calculateCatmullRom(const BezierPoint *pt, int nm, bool clsd)
  {
    if (nm < 2)
      return false;

    closed = clsd;
    if (nm < 3)
      closed = false;
    int seg_count = (nm - 1) + (closed ? 1 : 0);
    clear_and_resize(segs, seg_count);
    BezierPoint V[4];
    int use = 0;

    if (closed)
    {
      V[0] = pt[nm - 1];
      V[1] = pt[0];
      V[2] = pt[1];
      V[3] = pt[2];
    }
    else
    {
      V[0] = pt[0];
      V[1] = pt[0];
      V[2] = pt[1];
      V[3] = pt[nm < 3 ? 1 : 2];
    }
    segs[0].calculateCatmullRom(V);

    for (use = 1; use < nm - 1; use += 1)
    {
      V[0] = pt[use - 1];
      V[1] = pt[use];
      V[2] = pt[use + 1];
      if (use == nm - 2)
      {
        if (closed)
          V[3] = pt[0];
        else
          V[3] = pt[use + 1];
      }
      else
        V[3] = pt[use + 2];

      segs[use].calculateCatmullRom(V);
    }
    if (closed)
    {
      V[0] = pt[nm - 2];
      V[1] = pt[nm - 1];
      V[2] = pt[0];
      V[3] = pt[1];
      segs[nm - 1].calculateCatmullRom(V);
    }
    calculateLength();
    return true;
  }
};


class BezierSpline3d : public BezierSpline<Point3, BezierSplineInt<Point3>>
{
public:
};
class BezierSpline2d : public BezierSpline<Point2, BezierSplineInt<Point2>>
{
public:
};

typedef BezierSplineInt<Point3> BezierSplineInt3d;
typedef BezierSplineInt<Point2> BezierSplineInt2d;

class BezierSplines
{
public:
  SmallTab<BezierSpline3d *, MidmemAlloc> splines;
  BezierSplines() {}
  void close()
  {
    for (int i = 0; i < splines.size(); i++)
      del_it(splines[i]);
    clear_and_shrink(splines);
  }
  virtual ~BezierSplines() { close(); }
};

#undef Bezierinline
