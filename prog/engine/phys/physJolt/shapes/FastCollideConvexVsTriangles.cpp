// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "FastCollideConvexVsTriangles.h"
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/ScaleHelpers.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/TransformedShape.h>
#include <Jolt/Physics/Collision/ActiveEdges.h>
#include <Jolt/Geometry/ClosestPoint.h>
#include <util/dag_compilerDefs.h>
#include <float.h>

JPH_NAMESPACE_BEGIN

namespace
{
constexpr float cTieTolerance = 1.0e-4f;
constexpr float cEdgeAxisPreference = 1.0e-5f; // an edge axis must beat a face axis by this, so face contacts stay face contacts

Vec3 closest_on_second_segment(Vec3Arg inA0, Vec3Arg inA1, Vec3Arg inB0, Vec3Arg inB1)
{
  Vec3 da = inA1 - inA0, db = inB1 - inB0, r = inA0 - inB0;
  float aa = da.Dot(da), bb = db.Dot(db), f = db.Dot(r);
  float t = 0.0f;
  if (aa > 1.0e-12f && bb > 1.0e-12f)
  {
    float c = da.Dot(r), ab = da.Dot(db), denom = aa * bb - ab * ab;
    float s = denom > 1.0e-6f * aa * bb ? Clamp((ab * f - c * bb) / denom, 0.0f, 1.0f) : 0.0f;
    t = Clamp((ab * s + f) / bb, 0.0f, 1.0f);
  }
  else if (bb > 1.0e-12f)
    t = Clamp(f / bb, 0.0f, 1.0f);
  return inB0 + db * t;
}

// The point of the triangle nearest to the line through the origin along the unit inAxis: the centre of a face to face contact
Vec3 nearest_to_axis_on_triangle(Vec3Arg inV0, Vec3Arg inV1, Vec3Arg inV2, Vec3Arg inAxis)
{
  Vec3 p0 = inV0 - inAxis * inAxis.Dot(inV0), p1 = inV1 - inAxis * inAxis.Dot(inV1), p2 = inV2 - inAxis * inAxis.Dot(inV2);
  uint32 set;
  Vec3 q = ClosestPoint::GetClosestPointOnTriangle(p0, p1, p2, set);
  float u, v, w;
  ClosestPoint::GetBaryCentricCoordinates(p0 - q, p1 - q, p2 - q, u, v, w);
  return u * inV0 + v * inV1 + w * inV2;
}

// The part of the triangle inside the box [-inHalfExtent, inHalfExtent]: each of the 6 planes adds at most one vertex
int clip_triangle_to_box(Vec3Arg inV0, Vec3Arg inV1, Vec3Arg inV2, Vec3Arg inHalfExtent, Vec3 (&outPolygon)[9])
{
  Vec3 other[9];
  Vec3 *src = outPolygon, *dst = other;
  src[0] = inV0;
  src[1] = inV1;
  src[2] = inV2;
  int count = 3;
  for (int plane = 0; plane < 6 && count > 0; ++plane)
  {
    int axis = plane >> 1;
    float side = plane & 1 ? -1.0f : 1.0f, limit = inHalfExtent[axis];
    int out_count = 0;
    for (int i = 0; i < count; ++i)
    {
      Vec3 p = src[i], q = src[i + 1 < count ? i + 1 : 0];
      float dp = side * p[axis] - limit, dq = side * q[axis] - limit;
      if (dp <= 0.0f)
        dst[out_count++] = p;
      if ((dp <= 0.0f) != (dq <= 0.0f))
        dst[out_count++] = p + (q - p) * (dp / (dp - dq));
    }
    std::swap(src, dst);
    count = out_count;
  }
  return count; // 6 swaps leave the result in outPolygon
}

Vec3 nearest_on_polygon(const Vec3 *inPolygon, int inCount, Vec3Arg inPoint)
{
  Vec3 best = inPolygon[0];
  float best_dist_sq = FLT_MAX;
  for (int i = 0; i < inCount; ++i)
  {
    Vec3 on_edge = closest_on_second_segment(inPoint, inPoint, inPolygon[i], inPolygon[i + 1 < inCount ? i + 1 : 0]);
    float dist_sq = (on_edge - inPoint).LengthSq();
    if (dist_sq < best_dist_sq)
    {
      best_dist_sq = dist_sq;
      best = on_edge;
    }
  }
  return best;
}
} // namespace

FastCollideConvexVsTriangles::FastCollideConvexVsTriangles(const ConvexShape *inShape1, Vec3Arg inScale1, Vec3Arg inScale2,
  Mat44Arg inCenterOfMassTransform1, Mat44Arg inCenterOfMassTransform2, const SubShapeID &inSubShapeID1,
  const CollideShapeSettings &inCollideShapeSettings, CollideShapeCollector &ioCollector) :
  CollideConvexVsTriangles(inShape1, inScale1, inScale2, inCenterOfMassTransform1, inCenterOfMassTransform2, inSubShapeID1,
    inCollideShapeSettings, ioCollector)
{
  if (inShape1->GetSubType() != EShapeSubType::Box || !ScaleHelpers::IsNotScaled(inScale1))
    return;
  const BoxShape *box = static_cast<const BoxShape *>(inShape1);
  mConvexRadius = ScaleHelpers::ScaleConvexRadius(box->GetConvexRadius(), inScale1);
  mInnerHalfExtent = box->GetHalfExtent() - Vec3::sReplicate(mConvexRadius);
  mBoxPath = true;
}

void FastCollideConvexVsTriangles::CollideBox(Vec3Arg inV0, Vec3Arg inV1, Vec3Arg inV2, uint8 inActiveEdges,
  const SubShapeID &inSubShapeID2)
{
  // The same rejections as the base, in the space of the box
  Vec3 v0 = mTransform2To1 * (mScale2 * inV0);
  Vec3 v1 = mTransform2To1 * (mScale2 * inV1);
  Vec3 v2 = mTransform2To1 * (mScale2 * inV2);
  Vec3 triangle_normal = mScaleSign2 * (v1 - v0).Cross(v2 - v0);
  bool back_facing = triangle_normal.Dot(v0) > 0.0f;
  if (mCollideShapeSettings.mBackFaceMode == EBackFaceMode::IgnoreBackFaces && back_facing)
    return;
  AABox triangle_bbox = AABox::sFromTwoPoints(v0, v1);
  triangle_bbox.Encapsulate(v2);
  if (!triangle_bbox.Overlaps(mBoundsOf1))
    return;
  float normal_len_sq = triangle_normal.LengthSq();
  if (normal_len_sq < 1.0e-20f)
  {
    CollideConvexVsTriangles::Collide(inV0, inV1, inV2, inActiveEdges, inSubShapeID2);
    return;
  }

  // Separating axes against the two boxes of the base: GJK's box shrunk by the convex radius, then rounded by it, and EPA's full box.
  // On each axis the separation is the gap, or minus the overlap. An inner separation past the band ends the test.
  // The largest inner one picks no contact, GJK or EPA; the largest full one is EPA's axis and depth.
  const Vec3 e = mInnerHalfExtent, h = e + Vec3::sReplicate(mConvexRadius);
  const float band = mConvexRadius + mCollideShapeSettings.mMaxSeparationDistance;
  float inner_separation = -FLT_MAX, best_separation = -FLT_MAX;
  Vec3 best_axis = Vec3::sAxisY();
  enum
  {
    BoxFace,
    TriangleFace,
    EdgeEdge
  } best_kind = BoxFace;
  int best_box_axis = 0, best_edge = 0;
  auto test_axis = [&](Vec3Arg inAxis, float inPreference) DAGOR_FORCE_INLINE_LAMBDA {
    float d0 = inAxis.Dot(v0), d1 = inAxis.Dot(v1), d2 = inAxis.Dot(v2);
    float t_min = min(d0, min(d1, d2)), t_max = max(d0, max(d1, d2));
    Vec3 abs_axis = inAxis.Abs();
    float inner_radius = e.Dot(abs_axis), full_radius = h.Dot(abs_axis);
    inner_separation = max(inner_separation, max(t_min - inner_radius, -inner_radius - t_max));
    float separation = max(t_min - full_radius, -full_radius - t_max);
    if (separation > best_separation + inPreference)
    {
      best_separation = separation;
      best_axis = t_min + t_max > 0.0f ? inAxis : -inAxis;
      return true;
    }
    return false;
  };
  const Vec3 box_axes[3] = {Vec3::sAxisX(), Vec3::sAxisY(), Vec3::sAxisZ()};
  for (int i = 0; i < 3; ++i)
  {
    if (test_axis(box_axes[i], 0.0f))
      best_kind = BoxFace;
    if (inner_separation > band)
      return;
  }
  Vec3 unit_normal = triangle_normal / sqrtf(normal_len_sq);
  if (test_axis(unit_normal, 0.0f))
    best_kind = TriangleFace;
  if (inner_separation > band)
    return;
  const Vec3 triangle[3] = {v0, v1, v2};
  for (int j = 0; j < 3; ++j)
  {
    Vec3 edge = triangle[(j + 1) % 3] - triangle[j];
    float edge_len_sq = edge.LengthSq();
    for (int i = 0; i < 3; ++i)
    {
      Vec3 axis = box_axes[i].Cross(edge);
      float axis_len_sq = axis.LengthSq();
      if (axis_len_sq <= 1.0e-10f * edge_len_sq) // the edge runs along this box axis: the face axes cover it
        continue;
      axis /= sqrtf(axis_len_sq);
      if (test_axis(axis, cEdgeAxisPreference))
      {
        best_kind = EdgeEdge;
        best_box_axis = i;
        best_edge = j;
      }
      if (inner_separation > band)
        return;
    }
  }

  // The shrunk box is apart but within the band: GJK finds the true distance, which may exceed what the axes say
  if (inner_separation >= 0.0f)
  {
    CollideConvexVsTriangles::Collide(inV0, inV1, inV2, inActiveEdges, inSubShapeID2);
    return;
  }

  // The shrunk box overlaps: EPA would take the full box, whose least overlap is its depth along that axis
  float penetration_depth = -best_separation;
  if (-penetration_depth >= mCollector.GetEarlyOutFraction())
    return;
  // The point on the triangle is taken from its part inside the box, the point on the box is where the axis leaves the box from it
  Vec3 n = best_axis;
  Vec3 inside[9];
  int inside_count = clip_triangle_to_box(v0, v1, v2, h, inside);
  if (inside_count == 0) // a touch at a box edge that the clip rounds away: the base settles it
  {
    CollideConvexVsTriangles::Collide(inV0, inV1, inV2, inActiveEdges, inSubShapeID2);
    return;
  }
  Vec3 point2;
  if (best_kind == BoxFace)
  {
    // the deepest part into the box, a tie at its centre
    float d_min = FLT_MAX;
    for (int i = 0; i < inside_count; ++i)
      d_min = min(d_min, n.Dot(inside[i]));
    Vec3 sum = Vec3::sZero();
    int num_tied = 0;
    for (int i = 0; i < inside_count; ++i)
      if (n.Dot(inside[i]) <= d_min + cTieTolerance)
      {
        sum += inside[i];
        num_tied++;
      }
    point2 = sum / float(num_tied);
  }
  else if (best_kind == TriangleFace)
  {
    // under the box point deepest along n; a coordinate that n leaves free takes the triangle point nearest the box centre
    Vec3 nearest = n.Abs().ReduceMin() < cTieTolerance ? nearest_to_axis_on_triangle(v0, v1, v2, n) : Vec3::sZero();
    Vec3 support;
    for (int k = 0; k < 3; ++k)
      support.SetComponent(k, fabsf(n[k]) < cTieTolerance ? Clamp(nearest[k], -h[k], h[k]) : (n[k] > 0.0f ? h[k] : -h[k]));
    point2 = support - penetration_depth * n;
    float u, v, w;
    ClosestPoint::GetBaryCentricCoordinates(v0 - point2, v1 - point2, v2 - point2, u, v, w);
    if (min(u, min(v, w)) < -cTieTolerance)
      point2 = nearest_on_polygon(inside, inside_count, point2);
  }
  else
  {
    Vec3 edge_end0, edge_end1;
    for (int k = 0; k < 3; ++k)
    {
      float corner = n[k] >= 0.0f ? h[k] : -h[k];
      edge_end0.SetComponent(k, k == best_box_axis ? -h[k] : corner);
      edge_end1.SetComponent(k, k == best_box_axis ? h[k] : corner);
    }
    point2 = closest_on_second_segment(edge_end0, edge_end1, triangle[best_edge], triangle[(best_edge + 1) % 3]);
  }
  if ((point2.Abs() - h).ReduceMax() > cTieTolerance) // the edges' nearest points fall past an end of the box edge
    point2 = nearest_on_polygon(inside, inside_count, point2);
  float exit_distance = FLT_MAX;
  for (int k = 0; k < 3; ++k)
    if (fabsf(n[k]) > 1.0e-6f)
      exit_distance = min(exit_distance, ((n[k] > 0.0f ? h[k] : -h[k]) - point2[k]) / n[k]);
  Vec3 point1 = point2 + max(exit_distance, 0.0f) * n;
  Vec3 penetration_axis = n;

  // As the base does from here: active edges, world space, faces
  if (mCollideShapeSettings.mActiveEdgeMode == EActiveEdgeMode::CollideOnlyWithActive && inActiveEdges != 0b111)
  {
    Vec3 active_edge_movement_direction = mTransform1.Multiply3x3Transposed(mCollideShapeSettings.mActiveEdgeMovementDirection);
    penetration_axis = ActiveEdges::FixNormal(v0, v1, v2, back_facing ? triangle_normal : -triangle_normal, inActiveEdges, point2,
      penetration_axis, active_edge_movement_direction);
  }
  point1 = mTransform1 * point1;
  point2 = mTransform1 * point2;
  Vec3 penetration_axis_world = mTransform1.Multiply3x3(penetration_axis);
  CollideShapeResult result(point1, point2, penetration_axis_world, penetration_depth, mSubShapeID1, inSubShapeID2,
    TransformedShape::sGetBodyID(mCollector.GetContext()));
  if (mCollideShapeSettings.mCollectFacesMode == ECollectFacesMode::CollectFaces)
  {
    mShape1->GetSupportingFace(SubShapeID(), -penetration_axis, mScale1, mTransform1, result.mShape1Face);
    result.mShape2Face.resize(3);
    result.mShape2Face[0] = mTransform1 * v0;
    result.mShape2Face[1] = mTransform1 * v1;
    result.mShape2Face[2] = mTransform1 * v2;
    if (mScaleSign2 < 0.0f)
      std::swap(result.mShape2Face[1], result.mShape2Face[2]);
  }
  mCollector.AddHit(result);
}

JPH_NAMESPACE_END
