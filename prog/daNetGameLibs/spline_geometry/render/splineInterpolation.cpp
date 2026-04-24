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

  void setT1(float new_t1)
  {
    t1 = new_t1;
    invIntervalSize = 1.0f / (t1 - t0);
  }

  bool isInInterval(float global_t) const { return global_t >= t0 && global_t < t1; }

protected:
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

struct SplineGenClosestShapes
{
  int closestShapeOfs;
  int closestShapeSegmentNr;
  int secondClosestShapeOfs;
  int secondClosestShapeSegmentNr;
  float distToClosest;
};

struct SplineGenShapePosAndOfs
{
  float pos;
  IPoint2 ofsData;

  SplineGenShapePosAndOfs(float p, IPoint2 ofs) : pos(p), ofsData(ofs) {}
};

class FirstOrderPolynomShape : public FirstOrderPolynom<float>
{
private:
  SplineGenShapePosAndOfs pointA, pointB;

public:
  FirstOrderPolynomShape(SplineGenShapePosAndOfs x0, SplineGenShapePosAndOfs x1, float t0_, float t1_) :
    FirstOrderPolynom<float>(x0.pos, x1.pos, t0_, t1_), pointA(x0), pointB(x1)
  {}

  SplineGenClosestShapes sample(float global_t) const
  {
    float t = (global_t - t0) * invIntervalSize;
    SplineGenShapePosAndOfs closestShape = t < 0.5f ? pointA : pointB;
    SplineGenShapePosAndOfs nextClosestShape = t < 0.5f ? pointB : pointA;
    float distToClosest = t < 0.5f ? t : (1.0f - t);
    return {.closestShapeOfs = closestShape.ofsData.x,
      .closestShapeSegmentNr = closestShape.ofsData.y,
      .secondClosestShapeOfs = nextClosestShape.ofsData.x,
      .secondClosestShapeSegmentNr = nextClosestShape.ofsData.y,
      .distToClosest = distToClosest};
  }
};

eastl::vector<SplineGenShapePosAndOfs, framemem_allocator> decode_shape_indices(const ecs::List<Point2> &shape_positions,
  const ecs::List<IPoint2> &cached_shape_ofs_data)
{
  eastl::vector<SplineGenShapePosAndOfs, framemem_allocator> sortedPositions;
  sortedPositions.reserve(shape_positions.size());
  if (cached_shape_ofs_data.size() == 0)
    return sortedPositions;
  for (const Point2 &p : shape_positions)
  {
    int idx = (int)p.y;
    if (idx < 0 || idx >= cached_shape_ofs_data.size())
    {
      logerr("SplineGenGeometry: shape index %d is out of bounds [0, %d) for 'used_shapes' list. Using shape at position 0 instead.",
        idx, cached_shape_ofs_data.size());
      idx = 0;
    }
    sortedPositions.emplace_back(p.x, cached_shape_ofs_data[idx]);
  }

  return sortedPositions;
}

#if DAGOR_DBGLEVEL > 0
static bool is_data_sorted(const ecs::List<Point2> &data, const char *data_name)
{
  G_ASSERTF(data.size() >= 2, "%s list must have at least 2 elements.", data_name);
  for (int i = 1; i < data.size(); i++)
  {
    if (data[i].x <= data[i - 1].x)
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
  FirstOrderPolynomSpline(const ecs::List<Point3> &points_dirty,
    const ecs::List<Point2> &radii,
    const ecs::List<IPoint2> &cached_shape_ofs_data,
    const ecs::List<Point2> &shape_positions,
    Point3 first_normal = Point3(0, 1, 0),
    Point3 first_bitangent = Point3(1, 0, 0)) :
    firstNormal(first_normal), firstBitangent(first_bitangent)
  {
    totalLength = 0.0f;
    rootPos = points_dirty[0];
    eastl::vector<Point3, framemem_allocator> points;
    eastl::vector<SplineGenShapePosAndOfs, framemem_allocator> shapes = decode_shape_indices(shape_positions, cached_shape_ofs_data);
#if DAGOR_DBGLEVEL > 0
    G_ASSERTF(is_data_sorted(radii, "radii"), "Radii list must be sorted in ascending order.");
    if (shape_positions.size() != 0)
      G_ASSERTF(is_data_sorted(shape_positions, "shape_positions"), "Shape positions list must be sorted in ascending order.");
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
      if (pointPolynoms.size() > 0)
        pointPolynoms[pointPolynoms.size() - 1].setT1(totalLength + SMALL_NUMBER);
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
      if (radiiPolynoms.size() > 0)
        radiiPolynoms[radiiPolynoms.size() - 1].setT1(totalLength + SMALL_NUMBER);
    }

    {
      float currentLength = 0.0f;
      for (int i = 1; i < shapes.size(); i++)
      {
        float l = (shapes[i].pos - shapes[i - 1].pos) * totalLength;
        if (l > SMALL_NUMBER)
        {
          FirstOrderPolynomShape p = FirstOrderPolynomShape(shapes[i - 1], shapes[i], currentLength, currentLength + l);
          shapePolynoms.push_back(p);
          currentLength += l;
        }
      }
      if (shapePolynoms.size() > 0)
        shapePolynoms[shapePolynoms.size() - 1].setT1(totalLength + SMALL_NUMBER);
    }
  }
  // Sampling with monotone increasing "t"
  // For random sampling reset "pointHelperIndex" to 0 after every call
  SplineGenSpline sample(float t,
    int &point_helper_index,
    int &radii_helper_index,
    int &shape_helper_index,
    bool process_in_reverse,
    SplineGenSpline *prev_spline = nullptr)
  {
    if (pointPolynoms.size() == 0)
      return SplineGenSpline{rootPos, 0, Point3(0, 1, 0), 0, Point3(1, 0, 0), 0, -1, 0, -1, 0};

    t *= totalLength;
    if (!process_in_reverse)
    {
      while (point_helper_index < (int)pointPolynoms.size() - 1 && !pointPolynoms[point_helper_index].isInInterval(t))
        point_helper_index++;

      while (radii_helper_index < (int)radiiPolynoms.size() - 1 && !radiiPolynoms[radii_helper_index].isInInterval(t))
        radii_helper_index++;

      while (shape_helper_index < (int)shapePolynoms.size() - 1 && !shapePolynoms[shape_helper_index].isInInterval(t))
        shape_helper_index++;
    }
    else
    {
      while (point_helper_index > 0 && !pointPolynoms[point_helper_index].isInInterval(t))
        point_helper_index--;

      while (radii_helper_index > 0 && !radiiPolynoms[radii_helper_index].isInInterval(t))
        radii_helper_index--;

      while (shape_helper_index > 0 && !shapePolynoms[shape_helper_index].isInInterval(t))
        shape_helper_index--;
    }

    float sampledRad = 0.0f;
    // It's possible radiiPolynoms.size() == 0, if all radii polynoms' interval was
    // too small (smaller than SMALL_NUMBER). In those cases a 0 radius is returned.
    if (radiiPolynoms.size() != 0)
    {
      sampledRad = radiiPolynoms[radii_helper_index].sample(t);
    }

    SplineGenSpline result = SplineGenSpline{
      pointPolynoms[point_helper_index].sample(t), sampledRad, pointPolynoms[point_helper_index].getTangent(), 0, {}, 0, -1, 0, -1, 0};
    if (prev_spline && smart_bitangent)
    {
      Quat quat = quat_rotation_arc(prev_spline->tangent, result.tangent);
      result.bitangent = quat * prev_spline->bitangent;
    }
    else
    {
      result.bitangent = cross(firstNormal, result.tangent);
      if (length(result.bitangent) < SMALL_NUMBER)
        result.bitangent = firstBitangent;
      else
        result.bitangent = normalize(result.bitangent);
    }

    result.closestShapeOfs = -1;
    result.secondClosestShapeOfs = -1;
    if (shapePolynoms.size() != 0)
    {
      SplineGenClosestShapes closestShapes = shapePolynoms[shape_helper_index].sample(t);
      result.distToClosest = closestShapes.distToClosest;
      result.closestShapeOfs = closestShapes.closestShapeOfs;
      result.closestShapeSegmentNr = closestShapes.closestShapeSegmentNr;
      result.secondClosestShapeOfs = closestShapes.secondClosestShapeOfs;
      result.secondClosestShapeSegmentNr = closestShapes.secondClosestShapeSegmentNr;
    }

    return result;
  }

  void getLastIndicesForPolynoms(int &pointPolynomSize, int &radiiPolynomSize, int &shapePolynomSize) const
  {
    pointPolynomSize = (int)pointPolynoms.size() - 1;
    radiiPolynomSize = (int)radiiPolynoms.size() - 1;
    shapePolynomSize = (int)shapePolynoms.size() - 1;
  }

private:
  eastl::vector<FirstOrderPolynomPoint3, framemem_allocator> pointPolynoms;
  eastl::vector<FirstOrderPolynom<float>, framemem_allocator> radiiPolynoms;
  eastl::vector<FirstOrderPolynomShape, framemem_allocator> shapePolynoms;
  float totalLength;
  Point3 rootPos;
  Point3 firstNormal;
  Point3 firstBitangent;
};
} // namespace

eastl::vector<SplineGenSpline, framemem_allocator> interpolate_points(const ecs::List<Point3> &points,
  const ecs::List<Point2> &radii,
  const ecs::List<Point3> &emissive_points,
  const ecs::List<Point2> &shape_positions,
  const ecs::List<IPoint2> &cached_shape_ofs_data,
  Point3 first_normal,
  Point3 first_bitangent,
  bool use_last_point_to_orient_spline,
  int stripes)
{
  FirstOrderPolynomSpline polynomSpline(points, radii, cached_shape_ofs_data, shape_positions, first_normal, first_bitangent);
  eastl::vector<SplineGenSpline, framemem_allocator> splineVec(stripes + 1);
  int splinePointHelperIndex = 0;
  int splineRadiiHelperIndex = 0;
  int splineShapeHelperIndex = 0;
  bool processInReverse = use_last_point_to_orient_spline;

  if (processInReverse)
    polynomSpline.getLastIndicesForPolynoms(splinePointHelperIndex, splineRadiiHelperIndex, splineShapeHelperIndex);

  for (int i = 0; i < stripes + 1; i++)
  {
    int index = processInReverse ? (stripes - i) : i;
    int boundary = processInReverse ? stripes : 0;
    int prev_index = processInReverse ? (index + 1) : (index - 1);
    auto prev_spline = index == boundary ? nullptr : &splineVec[prev_index];
    float t = (float)index / (float)stripes;
    splineVec[index] =
      polynomSpline.sample(t, splinePointHelperIndex, splineRadiiHelperIndex, splineShapeHelperIndex, processInReverse, prev_spline);
    splineVec[index].emissiveIntensity = get_emissive_intensity(emissive_points, t);
  }
  return splineVec;
}