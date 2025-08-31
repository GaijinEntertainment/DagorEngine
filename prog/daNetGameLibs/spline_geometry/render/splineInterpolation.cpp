// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "splineInterpolation.h"
#include <math/dag_Quat.h>
#include <util/dag_convar.h>
#include <util/dag_stlqsort.h>
#include <math/dag_hlsl_floatx.h>
#include "../shaders/spline_gen_buffer.hlsli"

#define SMALL_NUMBER 0.0001
CONSOLE_BOOL_VAL("spline_gen", smart_bitangent, true);

namespace
{
template <typename T>
class FirstOrderPolynom
{
public:
  FirstOrderPolynom(T x0, T x1, float t0_, float t1_) : a(x1 - x0), b(x0), t0(t0_), t1(t1_), invIntervalSize(1.0f / (t1_ - t0_)) {}

  T sample(float global_t) const
  {
    float t = (global_t - t0) * invIntervalSize;
    return a * t + b;
  }

  bool isInInterval(float global_t) const { return global_t >= t0 && global_t < t1; }

private:
  T a, b;
  float t0, t1;
  float invIntervalSize;
};

class FirstOrderPolynomPoint3 : public FirstOrderPolynom<Point3>
{
public:
  FirstOrderPolynomPoint3(const Point3 &x0, const Point3 &x1, float t0_, float t1_) :
    FirstOrderPolynom<Point3>(x0, x1, t0_, t1_), tangent(normalize(x1 - x0))
  {}

  const Point3 &getTangent() const { return tangent; }

private:
  Point3 tangent;
};

#if DAGOR_DBGLEVEL > 0
static bool is_radii_sorted(const ecs::List<Point2> &radii)
{
  G_ASSERTF(radii.size() >= 2, "Radii list must have at least 2 elements.");
  for (int i = 1; i < radii.size(); i++)
  {
    if (radii[i].x <= radii[i - 1].x)
      return false;
  }
  return true;
}
#endif

static float get_emissive_intensity(const ecs::List<Point3> &emissive_points, float t)
{
  if (emissive_points.size() == 0)
    return 0.0f;

  float currentIntensity = 0.0f;
  for (int i = 0; i < emissive_points.size(); i++)
  {
    float intensityInIntervalAtT = saturate(1.0f - abs(t - emissive_points[i].x) / emissive_points[i].y);
    intensityInIntervalAtT *= emissive_points[i].z;
    currentIntensity += intensityInIntervalAtT;
  }

  return currentIntensity;
}

class FirstOrderPolynomSpline
{
public:
  FirstOrderPolynomSpline(const ecs::List<Point3> &points_dirty, const ecs::List<Point2> &radii)
  {
    totalLength = 0.0f;
    rootPos = points_dirty[0];
    eastl::vector<Point3, framemem_allocator> points;
#if DAGOR_DBGLEVEL > 0
    G_ASSERTF(is_radii_sorted(radii), "Radii list must be sorted in ascending order.");
#endif

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
        FirstOrderPolynomPoint3 p = FirstOrderPolynomPoint3(points[i - 1], points[i], currentLength, currentLength + l);
        pointPolynoms.push_back(p);
        currentLength += l;
      }
    }

    {
      float currentLength = 0.0f;
      for (int i = 1; i < radii.size(); i++)
      {
        float l = (radii[i].x - radii[i - 1].x) * totalLength;
        if (l > SMALL_NUMBER)
        {
          FirstOrderPolynom<float> p = FirstOrderPolynom<float>(radii[i - 1].y, radii[i].y, currentLength, currentLength + l);
          radiiPolynoms.push_back(p);
          currentLength += l;
        }
      }
    }
  }
  // Sampling with monotone increasing "t"
  // For random sampling reset "i" to 0 after every call
  SplineGenSpline sample(float t, int &i, int &j, SplineGenSpline *prev_spline = nullptr)
  {
    if (pointPolynoms.size() == 0)
      return SplineGenSpline{rootPos, 0, Point3(0, 1, 0), 0, Point3(1, 0, 0), 0};

    t *= totalLength;
    while (i < (int)pointPolynoms.size() - 1 && !pointPolynoms[i].isInInterval(t))
      i++;

    while (j < (int)radiiPolynoms.size() - 1 && !radiiPolynoms[j].isInInterval(t))
      j++;

    float sampledRad = 0.0f;
    // It's possible radiiPolynoms.size() == 0, if all radii polynoms' interval was
    // too small (smaller than SMALL_NUMBER). In those cases a 0 radius is returned.
    if (radiiPolynoms.size() != 0)
    {
      sampledRad = radiiPolynoms[j].sample(t);
    }

    SplineGenSpline result = SplineGenSpline{pointPolynoms[i].sample(t), sampledRad, pointPolynoms[i].getTangent(), 0, {}, 0};
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
  eastl::vector<FirstOrderPolynomPoint3, framemem_allocator> pointPolynoms;
  eastl::vector<FirstOrderPolynom<float>, framemem_allocator> radiiPolynoms;
  float totalLength;
  Point3 rootPos;
};
} // namespace

eastl::vector<SplineGenSpline, framemem_allocator> interpolate_points(
  const ecs::List<Point3> &points, const ecs::List<Point2> &radii, const ecs::List<Point3> &emissive_points, int stripes)
{
  FirstOrderPolynomSpline polynomSpline(points, radii);
  eastl::vector<SplineGenSpline, framemem_allocator> splineVec(stripes + 1);
  int splinePointHelperIndex = 0;
  int splineRadiiHelperIndex = 0;
  for (int i = 0; i < stripes + 1; i++)
  {
    auto prev_spline = i == 0 ? nullptr : &splineVec[i - 1];
    float t = (float)i / (float)stripes;
    splineVec[i] = polynomSpline.sample(t, splinePointHelperIndex, splineRadiiHelperIndex, prev_spline);
    splineVec[i].emissiveIntensity = get_emissive_intensity(emissive_points, t);
  }
  return splineVec;
}