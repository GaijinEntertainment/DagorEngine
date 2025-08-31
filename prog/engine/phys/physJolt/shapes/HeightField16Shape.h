// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// based on Jolt Physics Library (https://github.com/jrouwe/JoltPhysics)
// SPDX-FileCopyrightText: 2021 Jorrit Rouwe
// SPDX-License-Identifier: MIT

#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/PhysicsMaterial.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#ifdef JPH_DEBUG_RENDERER
#include <Jolt/Renderer/DebugRenderer.h>
#endif // JPH_DEBUG_RENDERER
#include <heightMapLand/dag_compressedHeightmap.h>

class LandMeshHolesManager;

JPH_NAMESPACE_BEGIN

/// Class that constructs a HeightField16Shape
class JPH_EXPORT HeightField16ShapeSettings final : public ShapeSettings
{
  JPH_DECLARE_SERIALIZABLE_VIRTUAL(JPH_EXPORT, HeightField16ShapeSettings)

public:
  /// Default constructor for deserialization
  HeightField16ShapeSettings() {}

  /// Create a height field shape of inSampleCount * inSampleCount vertices.
  /// The height field is a surface defined by: inOffset + inScale * (x, inSamples[y * inSampleCount + x], y).
  /// where x and y are integers in the range x and y e [0, inSampleCount - 1].
  /// inSampleCount: inSampleCount / mBlockSize must be a power of 2 and minimally 2.
  /// inSamples: persistent storage with inSampleCount^2 height (16 bit int).
  /// inMaterialIndices: (inSampleCount - 1)^2 indices that index into inMaterialList.
  HeightField16ShapeSettings(const CompressedHeightmap *inData, const LandMeshHolesManager *_holes, Vec3Arg inOffset, Vec3Arg inScale,
    const uint8 *inMaterialIndices = nullptr, const PhysicsMaterialList &inMaterialList = PhysicsMaterialList());

  // See: ShapeSettings
  virtual ShapeResult Create() const override;

  /// The height field is a surface defined by: mOffset + mScale * (x, mHeightSamples[y * mSampleCount + x], y).
  /// where x and y are integers in the range x and y e [0, mSampleCount - 1].
  Vec3 mOffset = Vec3::sZero();
  Vec3 mScale = Vec3::sOne();
  uint32 mSampleCount = 0;

  const CompressedHeightmap *mHData = nullptr;
  const LandMeshHolesManager *holes = nullptr;
  Array<uint8> mMaterialIndices;

  /// The materials of square at (x, y) is: mMaterials[mMaterialIndices[x + y * (mSampleCount - 1)]]
  PhysicsMaterialList mMaterials;

  /// Cosine of the threshold angle (if the angle between the two triangles is bigger than this, the edge is active, note that a
  /// concave edge is always inactive). Setting this value too small can cause ghost collisions with edges, setting it too big can
  /// cause depenetration artifacts (objects not depenetrating quickly). Valid ranges are between cos(0 degrees) and cos(90 degrees).
  /// The default value is cos(5 degrees).
  float mActiveEdgeCosThresholdAngle = 0.996195f; // cos(5 degrees)
};

/// A height field shape. Cannot be used as a dynamic object.
class JPH_EXPORT HeightField16Shape final : public Shape
{
public:
  JPH_OVERRIDE_NEW_DELETE

  /// Constructor
  HeightField16Shape() : Shape(EShapeType::HeightField, EShapeSubType::HeightField) {}
  HeightField16Shape(const HeightField16ShapeSettings &inSettings, ShapeResult &outResult);

  // See Shape::MustBeStatic
  virtual bool MustBeStatic() const override { return true; }

  // See Shape::GetLocalBounds
  virtual AABox GetLocalBounds() const override;

  // See Shape::GetSubShapeIDBitsRecursive
  virtual uint GetSubShapeIDBitsRecursive() const override { return GetSubShapeIDBits(); }

  // See Shape::GetInnerRadius
  virtual float GetInnerRadius() const override { return 0.0f; }

  // See Shape::GetMassProperties
  virtual MassProperties GetMassProperties() const override;

  // See Shape::GetMaterial
  virtual const PhysicsMaterial *GetMaterial(const SubShapeID &inSubShapeID) const override;

  /// Overload to get the material at a particular location
  const PhysicsMaterial *GetMaterial(uint inX, uint inY) const;

  // See Shape::GetSurfaceNormal
  virtual Vec3 GetSurfaceNormal(const SubShapeID &inSubShapeID, Vec3Arg inLocalSurfacePosition) const override;

  // See Shape::GetSupportingFace
  virtual void GetSupportingFace(const SubShapeID &inSubShapeID, Vec3Arg inDirection, Vec3Arg inScale,
    Mat44Arg inCenterOfMassTransform, SupportingFace &outVertices) const override;

  // See Shape::GetSubmergedVolume
  virtual void GetSubmergedVolume(Mat44Arg inCenterOfMassTransform, Vec3Arg inScale, const Plane &inSurface, float &outTotalVolume,
    float &outSubmergedVolume, Vec3 &outCenterOfBuoyancy JPH_IF_DEBUG_RENDERER(, RVec3Arg inBaseOffset)) const override
  {
    JPH_ASSERT(false, "Not supported");
  }

#ifdef JPH_DEBUG_RENDERER
  // See Shape::Draw
  virtual void Draw(DebugRenderer *inRenderer, RMat44Arg inCenterOfMassTransform, Vec3Arg inScale, ColorArg inColor,
    bool inUseMaterialColors, bool inDrawWireframe) const override;
#endif // JPH_DEBUG_RENDERER

  // See Shape::CastRay
  virtual bool CastRay(const RayCast &inRay, const SubShapeIDCreator &inSubShapeIDCreator, RayCastResult &ioHit) const override;
  virtual void CastRay(const RayCast &inRay, const RayCastSettings &inRayCastSettings, const SubShapeIDCreator &inSubShapeIDCreator,
    CastRayCollector &ioCollector, const ShapeFilter &inShapeFilter = {}) const override;

  // See: Shape::CollidePoint
  virtual void CollidePoint(Vec3Arg inPoint, const SubShapeIDCreator &inSubShapeIDCreator, CollidePointCollector &ioCollector,
    const ShapeFilter &inShapeFilter = {}) const override;

  // See: Shape::CollideSoftBodyVertices
  virtual void CollideSoftBodyVertices(Mat44Arg inCenterOfMassTransform, Vec3Arg inScale,
    const CollideSoftBodyVertexIterator &inVertices, uint inNumVertices, int inCollidingShapeIndex) const override;

  // See Shape::GetTrianglesStart
  virtual void GetTrianglesStart(GetTrianglesContext &ioContext, const AABox &inBox, Vec3Arg inPositionCOM, QuatArg inRotation,
    Vec3Arg inScale) const override;

  // See Shape::GetTrianglesNext
  virtual int GetTrianglesNext(GetTrianglesContext &ioContext, int inMaxTrianglesRequested, Float3 *outTriangleVertices,
    const PhysicsMaterial **outMaterials = nullptr) const override;

  /// Get height field position at sampled location (inX, inY).
  /// where inX and inY are integers in the range inX e [0, mSampleCount - 1] and inY e [0, mSampleCount - 1].
  Vec3 GetPosition(uint inX, uint inY, bool *outNoCollision = nullptr) const;

  /// Check if height field at sampled location (inX, inY) has collision (has a hole or not)
  bool IsNoCollision(uint inX, uint inY) const;

  // See Shape
  virtual void SaveBinaryState(StreamOut &inStream) const override;
  virtual void SaveMaterialState(PhysicsMaterialList &outMaterials) const override;
  virtual void RestoreMaterialState(const PhysicsMaterialRefC *inMaterials, uint inNumMaterials) override;

  // See Shape::GetStats
  virtual Stats GetStats() const override;

  // See Shape::GetVolume
  virtual float GetVolume() const override { return 0; }

#ifdef JPH_DEBUG_RENDERER
  // Settings
  static bool sDrawTriangleOutlines;
#endif // JPH_DEBUG_RENDERER
  static bool sUseActiveEdges;

  // Register shape functions with the registry
  static void sRegister();

protected:
  // See: Shape::RestoreBinaryState
  virtual void RestoreBinaryState(StreamIn &inStream) override;

private:
  class DecodingContext;        ///< Context class for walking through all nodes of a heightfield
  struct HSGetTrianglesContext; ///< Context class for GetTrianglesStart/Next

  /// Calculate bit mask for all active edges in the heightfield
  void CalculateActiveEdges(const HeightField16ShapeSettings &inSettings);

  /// Store material indices in the least amount of bits per index possible
  void StoreMaterialIndices(const HeightField16ShapeSettings &inSettings);

  /// Get the maximum level (amount of grids) of the tree
  static inline uint sGetMaxLevel(uint inNumBlocks) { return CountTrailingZeros(inNumBlocks); }

  /// Get the range block offset and stride for GetBlockOffsetAndScale
  static inline void sGetRangeBlockOffsetAndStride(uint inNumBlocks, uint inMaxLevel, uint &outRangeBlockOffset,
    uint &outRangeBlockStride);

  /// Get the height sample at position (inX, inY)
  inline uint16 GetHeightSample(uint inX, uint inY) const;

  /// Determine amount of bits needed to encode sub shape id
  uint GetSubShapeIDBits() const;

  /// En/decode a sub shape ID. inX and inY specify the coordinate of the triangle. inTriangle == 0 is the lower triangle, inTriangle
  /// == 1 is the upper triangle.
  inline SubShapeID EncodeSubShapeID(const SubShapeIDCreator &inCreator, uint inX, uint inY, uint inTriangle) const;
  inline void DecodeSubShapeID(const SubShapeID &inSubShapeID, uint &outX, uint &outY, uint &outTriangle) const;

  /// Get the edge flags for a triangle
  inline uint8 GetEdgeFlags(uint inX, uint inY, uint inTriangle) const;

  // Helper functions called by CollisionDispatch
  static void sCollideConvexVsHeightField(const Shape *inShape1, const Shape *inShape2, Vec3Arg inScale1, Vec3Arg inScale2,
    Mat44Arg inCenterOfMassTransform1, Mat44Arg inCenterOfMassTransform2, const SubShapeIDCreator &inSubShapeIDCreator1,
    const SubShapeIDCreator &inSubShapeIDCreator2, const CollideShapeSettings &inCollideShapeSettings,
    CollideShapeCollector &ioCollector, const ShapeFilter &inShapeFilter);
  static void sCollideSphereVsHeightField(const Shape *inShape1, const Shape *inShape2, Vec3Arg inScale1, Vec3Arg inScale2,
    Mat44Arg inCenterOfMassTransform1, Mat44Arg inCenterOfMassTransform2, const SubShapeIDCreator &inSubShapeIDCreator1,
    const SubShapeIDCreator &inSubShapeIDCreator2, const CollideShapeSettings &inCollideShapeSettings,
    CollideShapeCollector &ioCollector, const ShapeFilter &inShapeFilter);
  static void sCastConvexVsHeightField(const ShapeCast &inShapeCast, const ShapeCastSettings &inShapeCastSettings,
    const Shape *inShape, Vec3Arg inScale, const ShapeFilter &inShapeFilter, Mat44Arg inCenterOfMassTransform2,
    const SubShapeIDCreator &inSubShapeIDCreator1, const SubShapeIDCreator &inSubShapeIDCreator2, CastShapeCollector &ioCollector);
  static void sCastSphereVsHeightField(const ShapeCast &inShapeCast, const ShapeCastSettings &inShapeCastSettings,
    const Shape *inShape, Vec3Arg inScale, const ShapeFilter &inShapeFilter, Mat44Arg inCenterOfMassTransform2,
    const SubShapeIDCreator &inSubShapeIDCreator1, const SubShapeIDCreator &inSubShapeIDCreator2, CastShapeCollector &ioCollector);

  /// Visit the entire height field using a visitor pattern
  template <class Visitor>
  void WalkHeightField(Visitor &ioVisitor) const;

  /// The height field is a surface defined by: mOffset + mScale * (x, mHeightSamples[y * mSampleCount + x], y).
  /// where x and y are integers in the range x and y e [0, mSampleCount - 1].
  Vec3 mOffset = Vec3::sZero();
  Vec3 mScale = Vec3::sOne();

  /// Height data
  uint32 mSampleCount = 0;                            ///< See HeightField16ShapeSettings::mSampleCount
  uint32 mBlockSize = 0;                              ///< 1 << mHData->htRangeBlockLevels;
  static constexpr const uint16 mSampleMask = 0xffff; ///< All bits set for a sample: (1 << mBitsPerSample) - 1, used to indicate that
                                                      ///< there's no collision
  uint16 mMinSample = HeightFieldShapeConstants::cNoCollisionValue16; ///< Min and max value in mHeightSamples quantized to 16 bit, for
                                                                      ///< calculating bounding box
  uint16 mMaxSample = HeightFieldShapeConstants::cNoCollisionValue16;
  const CompressedHeightmap *mHData = nullptr;
  const LandMeshHolesManager *holes = nullptr;
  Array<uint8> mActiveEdges; ///< (mSampleCount - 1)^2 * 3-bit active edge flags.

  /// Materials
  PhysicsMaterialList mMaterials; ///< The materials of square at (x, y) is: mMaterials[mMaterialIndices[x + y * (mSampleCount - 1)]]
  Array<uint8> mMaterialIndices;  ///< Compressed to the minimum amount of bits per material index (mSampleCount - 1) * (mSampleCount -
                                  ///< 1) * mNumBitsPerMaterialIndex bits of data
  uint32 mNumBitsPerMaterialIndex = 0; ///< Number of bits per material index

#ifdef JPH_DEBUG_RENDERER
  /// Temporary rendering data
  mutable Array<DebugRenderer::GeometryRef> mGeometry;
  mutable bool mCachedUseMaterialColors = false; ///< This is used to regenerate the triangle batch if the drawing settings change
#endif                                           // JPH_DEBUG_RENDERER
};

JPH_NAMESPACE_END
