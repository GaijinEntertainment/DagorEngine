// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "splineInterpolation.h"
#include <math/dag_Quat.h>
#include <util/dag_convar.h>
#include <math/dag_hlsl_floatx.h>
#include "../shaders/spline_gen_buffer.hlsli"

#define SMALL_NUMBER 0.0001
CONSOLE_BOOL_VAL("spline_gen", smart_bitangent, true);

namespace
{
class FirstOrderPolynom
{
public:
  FirstOrderPolynom(Point3 x0, Point3 x1, float t0_, float t1_) :
    a(x1 - x0), b(x0), t0(t0_), t1(t1_), invIntervalSize(1.0f / (t1_ - t0_)), tangent(normalize(x1 - x0))
  {}

  Point3 sample(float global_t) const
  {
    float t = (global_t - t0) * invIntervalSize;
    return a * t + b;
  }

  const Point3 &getTangent() const { return tangent; }

  bool isInInterval(float global_t) const { return global_t >= t0 && global_t < t1; }

private:
  Point3 a, b;
  float t0, t1;
  float invIntervalSize;
  Point3 tangent;
};

class FirstOrderPolynomSpline
{
public:
  FirstOrderPolynomSpline(const ecs::List<Point3> &points_dirty)
  {
    totalLength = 0.0f;
    rootPos = points_dirty[0];
    eastl::vector<Point3, framemem_allocator> points;
    points.reserve(points_dirty.size());
    points.push_back(points_dirty[0]);
    for (int i = 1; i < points_dirty.size(); i++)
    {
      float l = length(points_dirty[i] - points.back());
      if (l > SMALL_NUMBER)
      {
        points.push_back(points_dirty[i]);
        totalLength += l;
      }
    }

    if (points.size() > 1)
    {
      float currentLength = 0.0f;
      for (int i = 1; i < points.size(); i++)
      {
        float l = length(points[i] - points[i - 1]);
        FirstOrderPolynom p = FirstOrderPolynom(points[i - 1], points[i], currentLength, currentLength + l);
        polynoms.push_back(p);
        currentLength += l;
      }
    }
  }
  // Sampling with monotone increasing "t"
  // For random sampling reset "i" to 0 after every call
  SplineGenSpline sample(float t, int &i, SplineGenSpline *prev_spline = nullptr)
  {
    if (polynoms.size() == 0)
      return SplineGenSpline{rootPos, 0, Point3(0, 1, 0), 0, Point3(1, 0, 0), 0};

    t *= totalLength;
    while (i < (int)polynoms.size() - 1 && !polynoms[i].isInInterval(t))
      i++;
    SplineGenSpline result = SplineGenSpline{polynoms[i].sample(t), 0, polynoms[i].getTangent(), 0, {}, 0};
    if (prev_spline && smart_bitangent)
    {
      Quat quat = quat_rotation_arc(prev_spline->tangent, result.tangent);
      result.bitangent = quat * prev_spline->bitangent;
    }
    else
    {
      result.bitangent = cross(float3(0, 1, 0), result.tangent);
      if (length(result.bitangent) < SMALL_NUMBER)
        result.bitangent = float3(1, 0, 0);
      else
        result.bitangent = normalize(result.bitangent);
    }
    return result;
  }

private:
  eastl::vector<FirstOrderPolynom, framemem_allocator> polynoms;
  float totalLength;
  Point3 rootPos;
};
} // namespace

eastl::vector<SplineGenSpline, framemem_allocator> interpolate_points(const ecs::List<Point3> &points, int stripes)
{
  FirstOrderPolynomSpline polynomSpline(points);
  eastl::vector<SplineGenSpline, framemem_allocator> splineVec(stripes + 1);
  int splineHelperIndex = 0;
  for (int i = 0; i < stripes + 1; i++)
  {
    splineVec[i] = polynomSpline.sample((float)i / (float)stripes, splineHelperIndex, i == 0 ? nullptr : &splineVec[i - 1]);
  }
  return splineVec;
}