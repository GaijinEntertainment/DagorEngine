// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/CollideConvexVsTriangles.h>

JPH_NAMESPACE_BEGIN

// CollideConvexVsTriangles with a separating axis path for an unscaled box; any other shape takes the base path.
// Against the box shrunk by its convex radius, a triangle past the convex radius plus the max separation is dropped before GJK,
// and one that overlaps the shrunk box takes the axis of least overlap in place of EPA: the same depth, its own axis on a tie.
// A triangle in between keeps the GJK path of the base, so its contact does not change.
class FastCollideConvexVsTriangles : public CollideConvexVsTriangles
{
public:
  FastCollideConvexVsTriangles(const ConvexShape *inShape1, Vec3Arg inScale1, Vec3Arg inScale2, Mat44Arg inCenterOfMassTransform1,
    Mat44Arg inCenterOfMassTransform2, const SubShapeID &inSubShapeID1, const CollideShapeSettings &inCollideShapeSettings,
    CollideShapeCollector &ioCollector);

  // inline, so the base path does not pay the box path's prologue
  JPH_INLINE void Collide(Vec3Arg inV0, Vec3Arg inV1, Vec3Arg inV2, uint8 inActiveEdges, const SubShapeID &inSubShapeID2)
  {
    if (mBoxPath)
      CollideBox(inV0, inV1, inV2, inActiveEdges, inSubShapeID2);
    else
      CollideConvexVsTriangles::Collide(inV0, inV1, inV2, inActiveEdges, inSubShapeID2);
  }

private:
  void CollideBox(Vec3Arg inV0, Vec3Arg inV1, Vec3Arg inV2, uint8 inActiveEdges, const SubShapeID &inSubShapeID2);

  bool mBoxPath = false;
  Vec3 mInnerHalfExtent = Vec3::sZero();
  float mConvexRadius = 0.0f;
};

JPH_NAMESPACE_END
