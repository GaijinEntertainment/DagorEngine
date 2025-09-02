//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_mathUtils.h>
#include <generic/dag_tab.h>
#include <generic/dag_span.h>

/**
 * This file contains the implementation of Catmull-Rom spline builders for 2D and 3D points.
 *
 * The Catmull-Rom spline is a type of interpolating spline that passes through a set of control points.
 * This implementation supports both open and closed splines, and allows for tension adjustment.
 *
 * Classes:
 * - CatmullRomSplineBuilder<T>: Template class for building Catmull-Rom splines with generic point types.
 * - CatmullRomSplineBuilder2D: Specialization of Catmull-Rom spline builder for 2D points.
 * - CatmullRomSplineBuilder3D: Typedef for Catmull-Rom spline builder for 3D points.
 *
 * - bool build(dag::ConstSpan<T> key_points, bool is_closed, float tension = 0.0f):
 *     Builds the spline with given key points, closed flag, and tension.
 *
 * After building the spline, the following methods can be used:
 * - Point2/Point3 getMonotonicPoint(float t): Retrieves a point on the spline at parameter t with constant speed parametrization.
 * - Point2/Point3 getRawPoint(float t): Retrieves a point on the spline at parameter t without arc length normalization.
 * - float getTotalSplineLength(): Returns the total length of the spline.
 * - Point2 getMonotonicNormal(float t) const (CatmullRomSplineBuilder2D only): Retrieves the normal vector with constant speed
 * parametrization.
 * - Point2 getRawNormal(float t) const (CatmullRomSplineBuilder2D only): Retrieves the normal vector without arc length normalization.
 */


template <typename T>
class CatmullRomSplineBuilder
{
public:
  bool build(dag::ConstSpan<T> key_points, bool is_closed, float tension_ = 0.0f)
  {
    tension = tension_;
    isClosed = is_closed;

    // if ((!is_closed && key_points.size() < 2) || (is_closed && key_points.size() < 2))
    //   return false;
    if (key_points.size() < 2)
      return false;

    controlPoints = preparePoints(key_points, is_closed);
    precalculateArcLengths();
    return true;
  }

  T getMonotonicPoint(float t) const
  {
    float rawT = tToArcLength(mapT(t));
    return getRawPointImpl(rawT);
  }

  T getRawPoint(float t) const { return getRawPointImpl(mapT(t)); }

  float getTotalSplineLength() { return totalLength; }

protected:
  Tab<T> controlPoints;
  float tension = 0.0f;
  bool isClosed = false;
  float totalLength = 0.0f;
  Tab<float> arcLengths;

  float mapT(float t) const { return isClosed ? (t - floorf(t)) : saturate(t); }

  Tab<T> preparePoints(const dag::ConstSpan<T> key_points, bool is_closed) const
  {
    Tab<T> points;

    if (is_closed)
    {
      int n = key_points.size();
      points.push_back(key_points[n - 1]);
      points.insert(points.end(), key_points.begin(), key_points.end());
      points.push_back(key_points[0]);
      points.push_back(key_points[1]);
    }
    else
    {
      points.push_back(key_points[0]);
      points.insert(points.end(), key_points.begin(), key_points.end());
      points.push_back(key_points[key_points.size() - 1]);
    }

    return points;
  }

  T calculateSegmentPoint(const T &p0, const T &p1, const T &p2, const T &p3, float t) const
  {
    float tau = 0.5f * (1.0f - tension);
    float t2 = t * t;
    float t3 = t2 * t;

    T m1 = (p2 - p0) * tau;
    T m2 = (p3 - p1) * tau;

    return ((2 * t3 - 3 * t2 + 1) * p1) + ((t3 - 2 * t2 + t) * m1) + ((-2 * t3 + 3 * t2) * p2) + ((t3 - t2) * m2);
  }

  T getRawPointImpl(float t) const
  {
    int numSegments = int(controlPoints.size()) - 3;
    if (numSegments <= 0)
      return controlPoints.empty() ? T() : controlPoints[0];

    float segmentF = t * numSegments;
    int segment = int(floorf(segmentF));

    if (segment >= numSegments)
    {
      segment = numSegments - 1;
      segmentF = float(segment) + 1.0f;
    }
    float localT = segmentF - segment;

    const T &p0 = controlPoints[segment];
    const T &p1 = controlPoints[segment + 1];
    const T &p2 = controlPoints[segment + 2];
    const T &p3 = controlPoints[segment + 3];

    return calculateSegmentPoint(p0, p1, p2, p3, localT);
  }

  void precalculateArcLengths()
  {
    int numSegments = int(controlPoints.size()) - 3;
    const int samplesPerSegment = 16;
    int totalSamples = numSegments * samplesPerSegment;
    arcLengths.resize(totalSamples + 1);

    arcLengths[0] = 0.0f;
    T prevPoint = getRawPointImpl(0.0f);

    for (int i = 1; i <= totalSamples; ++i)
    {
      float t = float(i) / totalSamples;
      T pt = getRawPointImpl(t);
      float dist = (pt - prevPoint).length();
      arcLengths[i] = arcLengths[i - 1] + dist;
      prevPoint = pt;
    }
    totalLength = arcLengths[totalSamples];
  }

  float tToArcLength(float t) const
  {
    if (arcLengths.empty() || totalLength == 0.0f)
      return t;

    float targetLength = t * totalLength;
    int totalSamples = int(arcLengths.size()) - 1;

    int low = 0;
    int high = totalSamples;
    while (low <= high)
    {
      int mid = (low + high) / 2;
      if (arcLengths[mid] < targetLength)
        low = mid + 1;
      else
        high = mid - 1;
    }

    int index = min(low, totalSamples);
    if (index == 0)
      return 0.0f;

    float lengthBefore = arcLengths[index - 1];
    float lengthAfter = arcLengths[index];
    float segmentFraction = (targetLength - lengthBefore) / (lengthAfter - lengthBefore);
    float rawTBefore = float(index - 1) / totalSamples;
    float rawTAfter = float(index) / totalSamples;
    return rawTBefore + segmentFraction * (rawTAfter - rawTBefore);
  }
};


class CatmullRomSplineBuilder2D : public CatmullRomSplineBuilder<Point2>
{
public:
  Point2 getMonotonicNormal(float t) const
  {
    float rawT = tToArcLength(mapT(t));
    return getRawNormalImpl(rawT);
  }

  Point2 getRawNormal(float t) const { return getRawNormalImpl(mapT(t)); }

private:
  Point2 getRawNormalImpl(float t) const
  {
    int numSegments = int(controlPoints.size()) - 3;
    if (numSegments <= 0)
      return Point2(0.0f, 1.0f);

    float segmentF = t * numSegments;
    int segment = int(floorf(segmentF));

    if (segment >= numSegments)
    {
      segment = numSegments - 1;
      segmentF = float(segment) + 1.0f;
    }
    float localT = segmentF - segment;

    const Point2 &p0 = controlPoints[segment];
    const Point2 &p1 = controlPoints[segment + 1];
    const Point2 &p2 = controlPoints[segment + 2];
    const Point2 &p3 = controlPoints[segment + 3];

    Point2 tangent = calculateSegmentTangent(p0, p1, p2, p3, localT);

    tangent.normalize();

    return Point2(-tangent.y, tangent.x);
  }

  Point2 calculateSegmentTangent(const Point2 &p0, const Point2 &p1, const Point2 &p2, const Point2 &p3, float t) const
  {
    float tau = 0.5f * (1.0f - tension);
    float t2 = t * t;

    Point2 m1 = (p2 - p0) * tau;
    Point2 m2 = (p3 - p1) * tau;

    return ((6 * t2 - 6 * t) * p1) + ((3 * t2 - 4 * t + 1) * m1) + ((-6 * t2 + 6 * t) * p2) + ((3 * t2 - 2 * t) * m2);
  }
};

typedef CatmullRomSplineBuilder<Point3> CatmullRomSplineBuilder3D;
