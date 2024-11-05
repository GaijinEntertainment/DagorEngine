// Copyright (C) Gaijin Games KFT.  All rights reserved.

// based Jolt Physics Library (https://github.com/jrouwe/JoltPhysics)
// SPDX-FileCopyrightText: 2021 Jorrit Rouwe
// SPDX-License-Identifier: MIT

#include <Jolt/Jolt.h>

#include "HeightField16Shape.h"
#include <Jolt/Physics/Collision/Shape/ConvexShape.h>
#include <Jolt/Physics/Collision/Shape/ScaleHelpers.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollidePointResult.h>
#include <Jolt/Physics/Collision/ShapeFilter.h>
#include <Jolt/Physics/Collision/CastConvexVsTriangles.h>
#include <Jolt/Physics/Collision/CastSphereVsTriangles.h>
#include <Jolt/Physics/Collision/CollideConvexVsTriangles.h>
#include <Jolt/Physics/Collision/CollideSphereVsTriangles.h>
#include <Jolt/Physics/Collision/TransformedShape.h>
#include <Jolt/Physics/Collision/ActiveEdges.h>
#include <Jolt/Physics/Collision/CollisionDispatch.h>
#include <Jolt/Physics/Collision/SortReverseAndStore.h>
#include <Jolt/Physics/Collision/CollideSoftBodyVerticesVsTriangles.h>
#include <Jolt/Core/Profiler.h>
#include <Jolt/Core/StringTools.h>
#include <Jolt/Core/StreamIn.h>
#include <Jolt/Core/StreamOut.h>
#include <Jolt/Geometry/AABox4.h>
#include <Jolt/Geometry/RayTriangle.h>
#include <Jolt/Geometry/RayAABox.h>
#include <Jolt/Geometry/OrientedBox.h>
#include <Jolt/ObjectStream/TypeDeclarations.h>
#include <landMesh/lmeshHoles.h>

// #define JPH_DEBUG_HEIGHT_FIELD

JPH_NAMESPACE_BEGIN

#ifdef JPH_DEBUG_RENDERER
bool HeightField16Shape::sDrawTriangleOutlines = false;
#endif // JPH_DEBUG_RENDERER
bool HeightField16Shape::sUseActiveEdges = true;

using namespace HeightFieldShapeConstants;

JPH_IMPLEMENT_SERIALIZABLE_VIRTUAL(HeightField16ShapeSettings)
{
  JPH_ADD_BASE_CLASS(HeightField16ShapeSettings, ShapeSettings)

  JPH_ADD_ATTRIBUTE(HeightField16ShapeSettings, mOffset)
  JPH_ADD_ATTRIBUTE(HeightField16ShapeSettings, mScale)
  JPH_ADD_ATTRIBUTE(HeightField16ShapeSettings, mSampleCount)
  JPH_ADD_ATTRIBUTE(HeightField16ShapeSettings, mMaterialIndices)
  JPH_ADD_ATTRIBUTE(HeightField16ShapeSettings, mMaterials)
  JPH_ADD_ATTRIBUTE(HeightField16ShapeSettings, mActiveEdgeCosThresholdAngle)
}

HeightField16ShapeSettings::HeightField16ShapeSettings(const CompressedHeightmap *inData, const LandMeshHolesManager *_holes,
  Vec3Arg inOffset, Vec3Arg inScale, const uint8 *inMaterialIndices, const PhysicsMaterialList &inMaterialList) :
  mHData(inData), mOffset(inOffset), mScale(inScale), holes(_holes)
{
  JPH_ASSERT(inData);
  mSampleCount = inData->bw * inData->block_width;

  if (!inMaterialList.empty() && inMaterialIndices != nullptr)
  {
    mMaterialIndices.resize(Square(mSampleCount - 1));
    memcpy(&mMaterialIndices[0], inMaterialIndices, Square(mSampleCount - 1) * sizeof(uint8));
    mMaterials = inMaterialList;
  }
  else
  {
    JPH_ASSERT(inMaterialList.empty());
    JPH_ASSERT(inMaterialIndices == nullptr);
  }
}

ShapeSettings::ShapeResult HeightField16ShapeSettings::Create() const
{
  if (mCachedResult.IsEmpty())
    Ref<Shape> shape = new HeightField16Shape(*this, mCachedResult);
  return mCachedResult;
}

void HeightField16Shape::CalculateActiveEdges(const HeightField16ShapeSettings &inSettings)
{
  if (!sUseActiveEdges)
    return;
  // Store active edges. The triangles are organized like this:
  //  +       +
  //  | \ T1B | \ T2B
  // e0   e2  |   \
  //  | T1A \ | T2A \
  //  +--e1---+-------+
  //  | \ T3B | \ T4B
  //  |   \   |   \
  //  | T3A \ | T4A \
  //  +-------+-------+
  // We store active edges e0 .. e2 as bits 0 .. 2.
  // We store triangles horizontally then vertically (order T1A, T2A, T3A and T4A).
  // The top edge and right edge of the heightfield are always active so we do not need to store them,
  // therefore we only need to store (mSampleCount - 1)^2 * 3-bit
  // The triangles T1B, T2B, T3B and T4B do not need to be stored, their active edges can be constructed from adjacent triangles.
  // Add 1 byte padding so we can always read 1 uint16 to get the bits that cross an 8 bit boundary
  uint count_min_1 = mSampleCount - 1;
  uint count_min_1_sq = Square(count_min_1);
  mActiveEdges.resize((count_min_1_sq * 3 + 7) / 8 + 1);
  memset(&mActiveEdges[0], 0, mActiveEdges.size());

  // Calculate triangle normals and make normals zero for triangles that are missing
  Array<Vec3> normals;
  normals.resize(2 * 2 * count_min_1); // allocate storage for 2 lines: cur_y and cur_y+1

  memset(&normals[0], 0, normals.size() * sizeof(Vec3)); //-V780
  auto compute_line_of_normals = [&](uint y, uint line_idx) {
    for (uint x = 0; x < count_min_1; ++x)
      if (!IsNoCollision(x, y) && !IsNoCollision(x + 1, y + 1))
      {
        Vec3 x1y1 = GetPosition(x, y);
        Vec3 x2y2 = GetPosition(x + 1, y + 1);

        uint offset = 2 * (count_min_1 * line_idx + x);

        if (!IsNoCollision(x, y + 1))
        {
          Vec3 x1y2 = GetPosition(x, y + 1);
          normals[offset] = (x2y2 - x1y2).Cross(x1y1 - x1y2).Normalized();
        }

        if (!IsNoCollision(x + 1, y))
        {
          Vec3 x2y1 = GetPosition(x + 1, y);
          normals[offset + 1] = (x1y1 - x2y1).Cross(x2y2 - x2y1).Normalized();
        }
      }
  };

  for (uint y = 0; y < 2 && y < count_min_1; ++y) // compute normals for first 2 lines: 0 and 1
    compute_line_of_normals(y, y);

  // Calculate active edges
  for (uint y = 0; y < count_min_1; ++y)
  {
    for (uint x = 0; x < count_min_1; ++x)
    {
      // Calculate vertex positions.
      // We don't check 'no colliding' since those normals will be zero and sIsEdgeActive will return true
      Vec3 x1y1 = GetPosition(x, y);
      Vec3 x1y2 = GetPosition(x, y + 1);
      Vec3 x2y2 = GetPosition(x + 1, y + 1);

      // Calculate the edge flags (3 bits)
      uint offset = 2 * x; // offset to normal on current line: y
      bool edge0_active = x == 0 || ActiveEdges::IsEdgeActive(normals[offset], normals[offset - 1], x1y2 - x1y1,
                                      inSettings.mActiveEdgeCosThresholdAngle);
      bool edge1_active = y == count_min_1 - 1 || ActiveEdges::IsEdgeActive(normals[offset], normals[offset + 2 * count_min_1 + 1],
                                                    x2y2 - x1y2, inSettings.mActiveEdgeCosThresholdAngle);
      bool edge2_active =
        ActiveEdges::IsEdgeActive(normals[offset], normals[offset + 1], x1y1 - x2y2, inSettings.mActiveEdgeCosThresholdAngle);
      uint16 edge_flags = (edge0_active ? 0b001 : 0) | (edge1_active ? 0b010 : 0) | (edge2_active ? 0b100 : 0);

      // Store the edge flags in the array
      uint bit_pos = 3 * (y * count_min_1 + x);
      uint byte_pos = bit_pos >> 3;
      bit_pos &= 0b111;
      edge_flags <<= bit_pos;
      mActiveEdges[byte_pos] |= uint8(edge_flags);
      mActiveEdges[byte_pos + 1] |= uint8(edge_flags >> 8);
    }

    if (y + 1 == count_min_1) // last line
      break;

    // move next line to current (normals)
    memcpy(&normals[0], &normals[normals.size() / 2], sizeof(Vec3) * normals.size() / 2); //-V780
    // compute new next line for (y+2)
    memset(&normals[normals.size() / 2], 0, sizeof(Vec3) * normals.size() / 2); //-V780
    if (y + 2 < count_min_1)
      compute_line_of_normals(y + 2, 1);
  }
}

void HeightField16Shape::StoreMaterialIndices(const HeightField16ShapeSettings &inSettings)
{
  uint count_min_1 = mSampleCount - 1;

  mNumBitsPerMaterialIndex = 32 - CountLeadingZeros((uint32)mMaterials.size() - 1);
  // Add 1 byte so we don't read out of bounds when reading an uint16
  mMaterialIndices.resize(((Square(count_min_1) * mNumBitsPerMaterialIndex + 7) >> 3) + 1);

  for (uint y = 0; y < count_min_1; ++y)
    for (uint x = 0; x < count_min_1; ++x)
    {
      // Read material
      uint sample_pos = x + y * count_min_1;
      uint16 material_index = uint16(inSettings.mMaterialIndices[sample_pos]);

      // Calculate byte and bit position where the material index needs to go
      uint bit_pos = sample_pos * mNumBitsPerMaterialIndex;
      uint byte_pos = bit_pos >> 3;
      bit_pos &= 0b111;

      // Write the material index
      material_index <<= bit_pos;
      JPH_ASSERT(byte_pos + 1 < mMaterialIndices.size());
      mMaterialIndices[byte_pos] |= uint8(material_index);
      mMaterialIndices[byte_pos + 1] |= uint8(material_index >> 8);
    }
}

HeightField16Shape::HeightField16Shape(const HeightField16ShapeSettings &inSettings, ShapeResult &outResult) :
  Shape(EShapeType::HeightField, EShapeSubType::HeightField, inSettings, outResult),
  mHData(inSettings.mHData),
  holes(inSettings.holes),
  mOffset(inSettings.mOffset),
  mScale(inSettings.mScale),
  mSampleCount(inSettings.mSampleCount),
  mMaterials(inSettings.mMaterials)
{
  JPH_ASSERT(mHData);

  const auto &hrbTop = mHData->htRangeBlocks[0];
  mMinSample = min(min(hrbTop.hMin[0], hrbTop.hMin[1]), min(hrbTop.hMin[2], hrbTop.hMin[3]));
  mMaxSample = max(max(hrbTop.hMax[0], hrbTop.hMax[1]), max(hrbTop.hMax[2], hrbTop.hMax[3]));
  mBlockSize = mSampleCount >> mHData->htRangeBlocksLevels;

  // Check block size
  if (mBlockSize < 2 || mBlockSize > 64)
  {
    outResult.SetError("HeightField16Shape: Block size must be in the range [2, 64]!");
    return;
  }

  // Check sample count
  if (mSampleCount % mBlockSize != 0)
  {
    outResult.SetError("HeightField16Shape: Sample count must be a multiple of block size!");
    return;
  }

  // We stop at mBlockSize x mBlockSize height sample blocks
  uint n = mSampleCount / mBlockSize;

  // Required to be power of two to allow creating a hierarchical grid
  if (!IsPowerOf2(n))
  {
    outResult.SetError("HeightField16Shape: Sample count / block size must be power of 2!");
    return;
  }

  // We want at least 1 grid layer
  if (n < 2)
  {
    outResult.SetError("HeightField16Shape: Sample count too low!");
    return;
  }

  // Check that we don't overflow our 32 bit 'properties'
  if (n > (1 << cNumBitsXY))
  {
    outResult.SetError("HeightField16Shape: Sample count too high!");
    return;
  }

  // Check if we're not exceeding the amount of sub shape id bits
  if (GetSubShapeIDBitsRecursive() > SubShapeID::MaxBits)
  {
    outResult.SetError("HeightField16Shape: Size exceeds the amount of available sub shape ID bits!");
    return;
  }

  if (!mMaterials.empty())
  {
    // Validate materials
    if (mMaterials.size() > 256)
    {
      outResult.SetError("Supporting max 256 materials per height field");
      return;
    }
    for (uint8 s : inSettings.mMaterialIndices)
      if (s >= mMaterials.size())
      {
        outResult.SetError(StringFormat("Material %u is beyond material list (size: %u)", s, (uint)mMaterials.size()));
        return;
      }
  }
  else
  {
    // No materials assigned, validate that no materials have been specified
    if (!inSettings.mMaterialIndices.empty())
    {
      outResult.SetError("No materials present, mMaterialIndices should be empty");
      return;
    }
  }

  // Calculate the active edges
  CalculateActiveEdges(inSettings);

  // Compress material indices
  if (mMaterials.size() > 1)
    StoreMaterialIndices(inSettings);

  outResult.Set(this);
}

inline void HeightField16Shape::sGetRangeBlockOffsetAndStride(uint inNumBlocks, uint inMaxLevel, uint &outRangeBlockOffset,
  uint &outRangeBlockStride)
{
  outRangeBlockOffset = CompressedHeightmap::sHierGridOffsets[inMaxLevel - 1];
  outRangeBlockStride = inNumBlocks >> 1;
}

inline uint16 HeightField16Shape::GetHeightSample(uint inX, uint inY) const
{
  JPH_ASSERT(inX < mSampleCount);
  JPH_ASSERT(inY < mSampleCount);
  return mHData->decodePixelUnsafe(inX, inY);
}

Vec3 HeightField16Shape::GetPosition(uint inX, uint inY, bool *outNoCollision) const
{
  // Test if there are any samples
  if (!mHData->blockVariance)
    return mOffset + mScale * Vec3(float(inX), 0.0f, float(inY));
  uint16 h16 = GetHeightSample(inX, inY);
  if (outNoCollision)
    *outNoCollision =
      holes ? holes->check(Point2(inX * mScale.GetX() + mOffset.GetX(), inY * mScale.GetZ() + mOffset.GetZ())) : (h16 == mSampleMask);
  return mOffset + mScale * Vec3(float(inX), float(h16), float(inY));
}

bool HeightField16Shape::IsNoCollision(uint inX, uint inY) const
{
  if (holes)
    return holes->check(Point2(inX * mScale.GetX() + mOffset.GetX(), inY * mScale.GetZ() + mOffset.GetZ()));
  return !mHData->blockVariance || GetHeightSample(inX, inY) == mSampleMask;
}

MassProperties HeightField16Shape::GetMassProperties() const
{
  // Object should always be static, return default mass properties
  return MassProperties();
}

const PhysicsMaterial *HeightField16Shape::GetMaterial(uint inX, uint inY) const
{
  if (mMaterials.empty())
    return PhysicsMaterial::sDefault;
  if (mMaterials.size() == 1)
    return mMaterials[0];

  uint count_min_1 = mSampleCount - 1;
  JPH_ASSERT(inX < count_min_1);
  JPH_ASSERT(inY < count_min_1);

  // Calculate at which bit the material index starts
  uint bit_pos = (inX + inY * count_min_1) * mNumBitsPerMaterialIndex;
  uint byte_pos = bit_pos >> 3;
  bit_pos &= 0b111;

  // Read the material index
  JPH_ASSERT(byte_pos + 1 < mMaterialIndices.size());
  const uint8 *material_indices = mMaterialIndices.data() + byte_pos;
  uint16 material_index = uint16(material_indices[0]) + uint16(uint16(material_indices[1]) << 8);
  material_index >>= bit_pos;
  material_index &= (1 << mNumBitsPerMaterialIndex) - 1;

  // Return the material
  return mMaterials[material_index];
}

uint HeightField16Shape::GetSubShapeIDBits() const
{
  // Need to store X, Y and 1 extra bit to specify the triangle number in the quad
  return 2 * (32 - CountLeadingZeros(mSampleCount - 1)) + 1;
}

SubShapeID HeightField16Shape::EncodeSubShapeID(const SubShapeIDCreator &inCreator, uint inX, uint inY, uint inTriangle) const
{
  return inCreator.PushID((inX + inY * mSampleCount) * 2 + inTriangle, GetSubShapeIDBits()).GetID();
}

void HeightField16Shape::DecodeSubShapeID(const SubShapeID &inSubShapeID, uint &outX, uint &outY, uint &outTriangle) const
{
  // Decode sub shape id
  SubShapeID remainder;
  uint32 id = inSubShapeID.PopID(GetSubShapeIDBits(), remainder);
  JPH_ASSERT(remainder.IsEmpty(), "Invalid subshape ID");

  // Get triangle index
  outTriangle = id & 1;
  id >>= 1;

  // Fetch the x and y coordinate
  outX = id % mSampleCount;
  outY = id / mSampleCount;
}

const PhysicsMaterial *HeightField16Shape::GetMaterial(const SubShapeID &inSubShapeID) const
{
  // Decode ID
  uint x, y, triangle;
  DecodeSubShapeID(inSubShapeID, x, y, triangle);

  // Fetch the material
  return GetMaterial(x, y);
}

Vec3 HeightField16Shape::GetSurfaceNormal(const SubShapeID &inSubShapeID, Vec3Arg inLocalSurfacePosition) const
{
  // Decode ID
  uint x, y, triangle;
  DecodeSubShapeID(inSubShapeID, x, y, triangle);

  // Fetch vertices that both triangles share
  Vec3 x1y1 = GetPosition(x, y);
  Vec3 x2y2 = GetPosition(x + 1, y + 1);

  // Get normal depending on which triangle was selected
  Vec3 normal;
  if (triangle == 0)
  {
    Vec3 x1y2 = GetPosition(x, y + 1);
    normal = (x2y2 - x1y2).Cross(x1y1 - x1y2);
  }
  else
  {
    Vec3 x2y1 = GetPosition(x + 1, y);
    normal = (x1y1 - x2y1).Cross(x2y2 - x2y1);
  }

  return normal.Normalized();
}

void HeightField16Shape::GetSupportingFace(const SubShapeID &inSubShapeID, Vec3Arg inDirection, Vec3Arg inScale,
  Mat44Arg inCenterOfMassTransform, SupportingFace &outVertices) const
{
  // Decode ID
  uint x, y, triangle;
  DecodeSubShapeID(inSubShapeID, x, y, triangle);

  // Fetch the triangle
  outVertices.resize(3);
  outVertices[0] = GetPosition(x, y);
  Vec3 v2 = GetPosition(x + 1, y + 1);
  if (triangle == 0)
  {
    outVertices[1] = GetPosition(x, y + 1);
    outVertices[2] = v2;
  }
  else
  {
    outVertices[1] = v2;
    outVertices[2] = GetPosition(x + 1, y);
  }

  // Flip triangle if scaled inside out
  if (ScaleHelpers::IsInsideOut(inScale))
    swap(outVertices[1], outVertices[2]);

  // Transform to world space
  Mat44 transform = inCenterOfMassTransform.PreScaled(inScale);
  for (Vec3 &v : outVertices)
    v = transform * v;
}

inline uint8 HeightField16Shape::GetEdgeFlags(uint inX, uint inY, uint inTriangle) const
{
  if (mActiveEdges.empty())
    return 0b111;
  if (inTriangle == 0)
  {
    // The edge flags for this triangle are directly stored, find the right 3 bits
    uint bit_pos = 3 * (inX + inY * (mSampleCount - 1));
    uint byte_pos = bit_pos >> 3;
    bit_pos &= 0b111;
    JPH_ASSERT(byte_pos + 1 < mActiveEdges.size());
    const uint8 *active_edges = mActiveEdges.data() + byte_pos;
    uint16 edge_flags = uint16(active_edges[0]) + uint16(uint16(active_edges[1]) << 8);
    return uint8(edge_flags >> bit_pos) & 0b111;
  }
  else
  {
    // We don't store this triangle directly, we need to look at our three neighbours to construct the edge flags
    uint8 edge0 = (GetEdgeFlags(inX, inY, 0) & 0b100) != 0 ? 0b001 : 0;                                // Diagonal edge
    uint8 edge1 = inX == mSampleCount - 1 || (GetEdgeFlags(inX + 1, inY, 0) & 0b001) != 0 ? 0b010 : 0; // Vertical edge
    uint8 edge2 = inY == 0 || (GetEdgeFlags(inX, inY - 1, 0) & 0b010) != 0 ? 0b100 : 0;                // Horizontal edge
    return edge0 | edge1 | edge2;
  }
}

AABox HeightField16Shape::GetLocalBounds() const
{
  if (mMinSample == cNoCollisionValue16)
  {
    // This whole height field shape doesn't have any collision, return the center point
    Vec3 center = mOffset + 0.5f * mScale * Vec3(float(mSampleCount - 1), 0.0f, float(mSampleCount - 1));
    return AABox(center, center);
  }
  else
  {
    // Bounding box based on min and max sample height
    Vec3 bmin = mOffset + mScale * Vec3(0.0f, float(mMinSample), 0.0f);
    Vec3 bmax = mOffset + mScale * Vec3(float(mSampleCount - 1), float(mMaxSample), float(mSampleCount - 1));
    return AABox(bmin, bmax);
  }
}

#ifdef JPH_DEBUG_RENDERER
void HeightField16Shape::Draw(DebugRenderer *inRenderer, RMat44Arg inCenterOfMassTransform, Vec3Arg inScale, ColorArg inColor,
  bool inUseMaterialColors, bool inDrawWireframe) const
{
  // Don't draw anything if we don't have any collision
  if (!mHData->blockVariance)
    return;

  // Reset the batch if we switch coloring mode
  if (mCachedUseMaterialColors != inUseMaterialColors)
  {
    mGeometry.clear();
    mCachedUseMaterialColors = inUseMaterialColors;
  }

  if (mGeometry.empty())
  {
    // Divide terrain in triangle batches of max 64x64x2 triangles to allow better culling of the terrain
    uint32 block_size = min<uint32>(mSampleCount, 64);
    for (uint32 by = 0; by < mSampleCount; by += block_size)
      for (uint32 bx = 0; bx < mSampleCount; bx += block_size)
      {
        // Create vertices for a block
        Array<DebugRenderer::Triangle> triangles;
        triangles.resize(block_size * block_size * 2);
        DebugRenderer::Triangle *out_tri = &triangles[0];
        for (uint32 y = by, max_y = min(by + block_size, mSampleCount - 1); y < max_y; ++y)
          for (uint32 x = bx, max_x = min(bx + block_size, mSampleCount - 1); x < max_x; ++x)
            if (!IsNoCollision(x, y) && !IsNoCollision(x + 1, y + 1))
            {
              Vec3 x1y1 = GetPosition(x, y);
              Vec3 x2y2 = GetPosition(x + 1, y + 1);
              Color color = inUseMaterialColors ? GetMaterial(x, y)->GetDebugColor() : Color::sWhite;

              if (!IsNoCollision(x, y + 1))
              {
                Vec3 x1y2 = GetPosition(x, y + 1);

                x1y1.StoreFloat3(&out_tri->mV[0].mPosition);
                x1y2.StoreFloat3(&out_tri->mV[1].mPosition);
                x2y2.StoreFloat3(&out_tri->mV[2].mPosition);

                Vec3 normal = (x2y2 - x1y2).Cross(x1y1 - x1y2).Normalized();
                for (DebugRenderer::Vertex &v : out_tri->mV)
                {
                  v.mColor = color;
                  v.mUV = Float2(0, 0);
                  normal.StoreFloat3(&v.mNormal);
                }

                ++out_tri;
              }

              if (!IsNoCollision(x + 1, y))
              {
                Vec3 x2y1 = GetPosition(x + 1, y);

                x1y1.StoreFloat3(&out_tri->mV[0].mPosition);
                x2y2.StoreFloat3(&out_tri->mV[1].mPosition);
                x2y1.StoreFloat3(&out_tri->mV[2].mPosition);

                Vec3 normal = (x1y1 - x2y1).Cross(x2y2 - x2y1).Normalized();
                for (DebugRenderer::Vertex &v : out_tri->mV)
                {
                  v.mColor = color;
                  v.mUV = Float2(0, 0);
                  normal.StoreFloat3(&v.mNormal);
                }

                ++out_tri;
              }
            }

        // Resize triangles array to actual amount of triangles written
        size_t num_triangles = out_tri - &triangles[0];
        triangles.resize(num_triangles);

        // Create batch
        if (num_triangles > 0)
          mGeometry.push_back(new DebugRenderer::Geometry(inRenderer->CreateTriangleBatch(triangles),
            DebugRenderer::sCalculateBounds(&triangles[0].mV[0], int(3 * num_triangles))));
      }
  }

  // Get transform including scale
  RMat44 transform = inCenterOfMassTransform.PreScaled(inScale);

  // Test if the shape is scaled inside out
  DebugRenderer::ECullMode cull_mode =
    ScaleHelpers::IsInsideOut(inScale) ? DebugRenderer::ECullMode::CullFrontFace : DebugRenderer::ECullMode::CullBackFace;

  // Determine the draw mode
  DebugRenderer::EDrawMode draw_mode = inDrawWireframe ? DebugRenderer::EDrawMode::Wireframe : DebugRenderer::EDrawMode::Solid;

  // Draw the geometry
  for (const DebugRenderer::GeometryRef &b : mGeometry)
    inRenderer->DrawGeometry(transform, inColor, b, cull_mode, DebugRenderer::ECastShadow::On, draw_mode);

  if (sDrawTriangleOutlines)
  {
    struct Visitor
    {
      JPH_INLINE explicit Visitor(const HeightField16Shape *inShape, DebugRenderer *inRenderer, RMat44Arg inTransform) :
        mShape(inShape), mRenderer(inRenderer), mTransform(inTransform)
      {}

      JPH_INLINE bool ShouldAbort() const { return false; }

      JPH_INLINE bool ShouldVisitRangeBlock([[maybe_unused]] int inStackTop) const { return true; }

      JPH_INLINE int VisitRangeBlock(Vec4Arg inBoundsMinX, Vec4Arg inBoundsMinY, Vec4Arg inBoundsMinZ, Vec4Arg inBoundsMaxX,
        Vec4Arg inBoundsMaxY, Vec4Arg inBoundsMaxZ, UVec4 &ioProperties, [[maybe_unused]] int inStackTop) const
      {
        UVec4 valid = Vec4::sLessOrEqual(inBoundsMinY, inBoundsMaxY);
        return CountAndSortTrues(valid, ioProperties);
      }

      JPH_INLINE void VisitTriangle(uint inX, uint inY, uint inTriangle, Vec3Arg inV0, Vec3Arg inV1, Vec3Arg inV2) const
      {
        // Determine active edges
        uint8 active_edges = mShape->GetEdgeFlags(inX, inY, inTriangle);

        // Loop through edges
        Vec3 v[] = {inV0, inV1, inV2};
        for (uint edge_idx = 0; edge_idx < 3; ++edge_idx)
        {
          RVec3 v1 = mTransform * v[edge_idx];
          RVec3 v2 = mTransform * v[(edge_idx + 1) % 3];

          // Draw active edge as a green arrow, other edges as grey
          if (active_edges & (1 << edge_idx))
            mRenderer->DrawArrow(v1, v2, Color::sGreen, 0.01f);
          else
            mRenderer->DrawLine(v1, v2, Color::sGrey);
        }
      }

      const HeightField16Shape *mShape;
      DebugRenderer *mRenderer;
      RMat44 mTransform;
    };

    Visitor visitor(this, inRenderer, inCenterOfMassTransform.PreScaled(inScale));
    WalkHeightField(visitor);
  }
}
#endif // JPH_DEBUG_RENDERER

class HeightField16Shape::DecodingContext
{
public:
  JPH_INLINE explicit DecodingContext(const HeightField16Shape *inShape) : mShape(inShape)
  {
    static_assert(sizeof(CompressedHeightmap::sHierGridOffsets) / sizeof(uint) == cNumBitsXY + 1, "Offsets array is not long enough");

    // Construct root stack entry
    mPropertiesStack[0] = 0; // level: 0, x: 0, y: 0
  }

  template <class Visitor>
  JPH_INLINE void WalkHeightField(Visitor &ioVisitor)
  {
    // Early out if there's no collision
    if (!mShape->mHData->blockVariance)
      return;

    // Precalculate values relating to sample count
    uint32 sample_count = mShape->mSampleCount;
    UVec4 sample_count_min_1 = UVec4::sReplicate(sample_count - 1);

    // Precalculate values relating to block size
    uint max_level = mShape->mHData->htRangeBlocksLevels;
    uint32 block_size = sample_count >> max_level;
    uint32 block_size_plus_1 = block_size + 1;
    uint num_blocks = 1 << max_level;
    uint num_blocks_min_1 = num_blocks - 1;

    // Precalculate range block offset and stride for GetBlockOffsetAndScale
    uint range_block_offset, range_block_stride;
    sGetRangeBlockOffsetAndStride(num_blocks, max_level, range_block_offset, range_block_stride);

    // Allocate space for vertices and 'no collision' flags
    int array_size = Square(block_size_plus_1);
    Vec3 *vertices = reinterpret_cast<Vec3 *>(JPH_STACK_ALLOC(array_size * sizeof(Vec3))); //-V630
    bool *no_collision = reinterpret_cast<bool *>(JPH_STACK_ALLOC(array_size * sizeof(bool)));

    // Splat offsets
    Vec4 ox = mShape->mOffset.SplatX();
    Vec4 oy = mShape->mOffset.SplatY();
    Vec4 oz = mShape->mOffset.SplatZ();

    // Splat scales
    Vec4 sx = mShape->mScale.SplatX();
    Vec4 sy = mShape->mScale.SplatY();
    Vec4 sz = mShape->mScale.SplatZ();

    do
    {
      // Decode properties
      uint32 properties_top = mPropertiesStack[mTop];
      uint32 x = properties_top & cMaskBitsXY;
      uint32 y = (properties_top >> cNumBitsXY) & cMaskBitsXY;
      uint32 level = properties_top >> cLevelShift;

      if (level >= max_level)
      {
        // Determine actual range of samples (minus one because we eventually want to iterate over the triangles, not the samples)
        uint32 min_x = x * block_size;
        uint32 max_x = min_x + block_size;
        uint32 min_y = y * block_size;
        uint32 max_y = min_y + block_size;

        // Decompress vertices of block at (x, y)
        Vec3 *dst_vertex = vertices;
        bool *dst_no_collision = no_collision;
        for (uint32 v_y = min_y; v_y < max_y; ++v_y)
        {
          for (uint32 v_x = min_x; v_x < max_x; ++v_x)
          {
            *dst_vertex = mShape->GetPosition(v_x, v_y, dst_no_collision);
            ++dst_vertex;
            ++dst_no_collision;
          }

          // Skip last column, these values come from a different block
          ++dst_vertex;
          ++dst_no_collision;
        }

        // Decompress block (x + 1, y)
        uint32 max_x_decrement = 0;
        if (x < num_blocks_min_1)
        {
          dst_vertex = vertices + block_size;
          dst_no_collision = no_collision + block_size;
          for (uint32 v_y = min_y; v_y < max_y; ++v_y)
          {
            *dst_vertex = mShape->GetPosition(max_x, v_y, dst_no_collision);
            dst_vertex += block_size_plus_1;
            dst_no_collision += block_size_plus_1;
          }
        }
        else
          max_x_decrement = 1; // We don't have a next block, one less triangle to test

        // Decompress block (x, y + 1)
        if (y < num_blocks_min_1)
        {
          uint start = block_size * block_size_plus_1;
          dst_vertex = vertices + start;
          dst_no_collision = no_collision + start;
          for (uint32 v_x = min_x; v_x < max_x; ++v_x)
          {
            *dst_vertex = mShape->GetPosition(v_x, max_y, dst_no_collision);
            ++dst_vertex;
            ++dst_no_collision;
          }

          // Decompress single sample of block at (x + 1, y + 1)
          if (x < num_blocks_min_1)
            *dst_vertex = mShape->GetPosition(max_x, max_y, dst_no_collision);
        }
        else
          --max_y; // We don't have a next block, one less triangle to test

        // Update max_x (we've been using it so we couldn't update it earlier)
        max_x -= max_x_decrement;

        // We're going to divide the vertices in 4 blocks to do one more runtime sub-division, calculate the ranges of those blocks
        struct Range
        {
          uint32 mMinX, mMinY, mNumTrianglesX, mNumTrianglesY;
        };
        uint32 half_block_size = block_size >> 1;
        uint32 block_size_x = max_x - min_x - half_block_size;
        uint32 block_size_y = max_y - min_y - half_block_size;
        Range ranges[] = {
          {0, 0, half_block_size, half_block_size},
          {half_block_size, 0, block_size_x, half_block_size},
          {0, half_block_size, half_block_size, block_size_y},
          {half_block_size, half_block_size, block_size_x, block_size_y},
        };

        // Calculate the min and max of each of the blocks
        Mat44 block_min, block_max;
        for (int block = 0; block < 4; ++block)
        {
          // Get the range for this block
          const Range &range = ranges[block];
          uint32 start = range.mMinX + range.mMinY * block_size_plus_1;
          uint32 size_x_plus_1 = range.mNumTrianglesX + 1;
          uint32 size_y_plus_1 = range.mNumTrianglesY + 1;

          // Calculate where to start reading
          const Vec3 *src_vertex = vertices + start;
          const bool *src_no_collision = no_collision + start;
          uint32 stride = block_size_plus_1 - size_x_plus_1;

          // Start range with a very large inside-out box
          Vec3 value_min = Vec3::sReplicate(1.0e30f);
          Vec3 value_max = Vec3::sReplicate(-1.0e30f);

          // Loop over the samples to determine the min and max of this block
          for (uint32 block_y = 0; block_y < size_y_plus_1; ++block_y)
          {
            for (uint32 block_x = 0; block_x < size_x_plus_1; ++block_x)
            {
              if (!*src_no_collision)
              {
                value_min = Vec3::sMin(value_min, *src_vertex);
                value_max = Vec3::sMax(value_max, *src_vertex);
              }
              ++src_vertex;
              ++src_no_collision;
            }
            src_vertex += stride;
            src_no_collision += stride;
          }
          block_min.SetColumn4(block, Vec4(value_min));
          block_max.SetColumn4(block, Vec4(value_max));
        }

#ifdef JPH_DEBUG_HEIGHT_FIELD
        // Draw the bounding boxes of the sub-nodes
        for (int block = 0; block < 4; ++block)
        {
          AABox bounds(block_min.GetColumn3(block), block_max.GetColumn3(block));
          if (bounds.IsValid())
            DebugRenderer::sInstance->DrawWireBox(bounds, Color::sYellow);
        }
#endif // JPH_DEBUG_HEIGHT_FIELD

        // Transpose so we have the mins and maxes of each of the blocks in rows instead of columns
        Mat44 transposed_min = block_min.Transposed();
        Mat44 transposed_max = block_max.Transposed();

        // Check which blocks collide
        // Note: At this point we don't use our own stack but we do allow the visitor to use its own stack
        // to store collision distances so that we can still early out when no closer hits have been found.
        UVec4 colliding_blocks(0, 1, 2, 3);
        int num_results =
          ioVisitor.VisitRangeBlock(transposed_min.GetColumn4(0), transposed_min.GetColumn4(1), transposed_min.GetColumn4(2),
            transposed_max.GetColumn4(0), transposed_max.GetColumn4(1), transposed_max.GetColumn4(2), colliding_blocks, mTop);

        // Loop through the results backwards (closest first)
        int result = num_results - 1;
        while (result >= 0)
        {
          // Calculate the min and max of this block
          uint32 block = colliding_blocks[result];
          const Range &range = ranges[block];
          uint32 block_min_x = min_x + range.mMinX;
          uint32 block_max_x = block_min_x + range.mNumTrianglesX;
          uint32 block_min_y = min_y + range.mMinY;
          uint32 block_max_y = block_min_y + range.mNumTrianglesY;

          // Loop triangles
          for (uint32 v_y = block_min_y; v_y < block_max_y; ++v_y)
            for (uint32 v_x = block_min_x; v_x < block_max_x; ++v_x)
            {
              // Get first vertex
              const int offset = (v_y - min_y) * block_size_plus_1 + (v_x - min_x);
              const Vec3 *start_vertex = vertices + offset;
              const bool *start_no_collision = no_collision + offset;

              // Check if vertices shared by both triangles have collision
              if (!start_no_collision[0] && !start_no_collision[block_size_plus_1 + 1])
              {
                // Loop 2 triangles
                for (uint t = 0; t < 2; ++t)
                {
                  // Determine triangle vertices
                  Vec3 v0, v1, v2;
                  if (t == 0)
                  {
                    // Check third vertex
                    if (start_no_collision[block_size_plus_1])
                      continue;

                    // Get vertices for triangle
                    v0 = start_vertex[0];
                    v1 = start_vertex[block_size_plus_1];
                    v2 = start_vertex[block_size_plus_1 + 1];
                  }
                  else
                  {
                    // Check third vertex
                    if (start_no_collision[1])
                      continue;

                    // Get vertices for triangle
                    v0 = start_vertex[0];
                    v1 = start_vertex[block_size_plus_1 + 1];
                    v2 = start_vertex[1];
                  }

#ifdef JPH_DEBUG_HEIGHT_FIELD
                  DebugRenderer::sInstance->DrawWireTriangle(RVec3(v0), RVec3(v1), RVec3(v2), Color::sWhite);
#endif

                  // Call visitor
                  ioVisitor.VisitTriangle(v_x, v_y, t, v0, v1, v2);

                  // Check if we're done
                  if (ioVisitor.ShouldAbort())
                    return;
                }
              }
            }

          // Fetch next block until we find one that the visitor wants to see
          do
            --result;
          while (result >= 0 && !ioVisitor.ShouldVisitRangeBlock(mTop + result));
        }
      }
      else
      {
        // Visit child grid
        uint32 offset = mShape->mHData->sHierGridOffsets[level] + (1 << level) * y + x;

        // Decode min/max height
        UVec4 block = UVec4::sLoadInt4Aligned(reinterpret_cast<const uint32 *>(&mShape->mHData->htRangeBlocks[offset])); //-V1032
        Vec4 bounds_miny = oy + sy * block.Expand4Uint16Lo().ToFloat();
        Vec4 bounds_maxy = oy + sy * block.Expand4Uint16Hi().ToFloat();

        // Calculate size of one cell at this grid level
        UVec4 internal_cell_size = UVec4::sReplicate(block_size << (max_level - level - 1)); // subtract 1 from level because we have
                                                                                             // an internal grid of 2x2

        // Calculate min/max x and z
        UVec4 two_x = UVec4::sReplicate(2 * x); // multiply by two because we have an internal grid of 2x2
        Vec4 bounds_minx = ox + sx * (internal_cell_size * (two_x + UVec4(0, 1, 0, 1))).ToFloat();
        Vec4 bounds_maxx = ox + sx * UVec4::sMin(internal_cell_size * (two_x + UVec4(1, 2, 1, 2)), sample_count_min_1).ToFloat();

        UVec4 two_y = UVec4::sReplicate(2 * y);
        Vec4 bounds_minz = oz + sz * (internal_cell_size * (two_y + UVec4(0, 0, 1, 1))).ToFloat();
        Vec4 bounds_maxz = oz + sz * UVec4::sMin(internal_cell_size * (two_y + UVec4(1, 1, 2, 2)), sample_count_min_1).ToFloat();

        // Calculate properties of child blocks
        UVec4 properties = UVec4::sReplicate(((level + 1) << cLevelShift) + (y << (cNumBitsXY + 1)) + (x << 1)) +
                           UVec4(0, 1, 1 << cNumBitsXY, (1 << cNumBitsXY) + 1);

#ifdef JPH_DEBUG_HEIGHT_FIELD
        // Draw boxes
        for (int i = 0; i < 4; ++i)
        {
          AABox b(Vec3(bounds_minx[i], bounds_miny[i], bounds_minz[i]), Vec3(bounds_maxx[i], bounds_maxy[i], bounds_maxz[i]));
          if (b.IsValid())
            DebugRenderer::sInstance->DrawWireBox(b, Color::sGreen);
        }
#endif

        // Check which sub nodes to visit
        int num_results =
          ioVisitor.VisitRangeBlock(bounds_minx, bounds_miny, bounds_minz, bounds_maxx, bounds_maxy, bounds_maxz, properties, mTop);

        // Push them onto the stack
        JPH_ASSERT(mTop + 4 < cStackSize);
        properties.StoreInt4(&mPropertiesStack[mTop]);
        mTop += num_results;
      }

      // Check if we're done
      if (ioVisitor.ShouldAbort())
        return;

      // Fetch next node until we find one that the visitor wants to see
      do
        --mTop;
      while (mTop >= 0 && !ioVisitor.ShouldVisitRangeBlock(mTop));
    } while (mTop >= 0);
  }

  // This can be used to have the visitor early out (ioVisitor.ShouldAbort() returns true) and later continue again (call
  // WalkHeightField() again)
  JPH_INLINE bool IsDoneWalking() const { return mTop < 0; }

private:
  const HeightField16Shape *mShape;
  int mTop = 0;
  uint32 mPropertiesStack[cStackSize];
};

template <class Visitor>
void HeightField16Shape::WalkHeightField(Visitor &ioVisitor) const
{
  DecodingContext ctx(this);
  ctx.WalkHeightField(ioVisitor);
}

bool HeightField16Shape::CastRay(const RayCast &inRay, const SubShapeIDCreator &inSubShapeIDCreator, RayCastResult &ioHit) const
{
  JPH_PROFILE_FUNCTION();

  struct Visitor
  {
    JPH_INLINE explicit Visitor(const HeightField16Shape *inShape, const RayCast &inRay, const SubShapeIDCreator &inSubShapeIDCreator,
      RayCastResult &ioHit) :
      mHit(ioHit),
      mRayOrigin(inRay.mOrigin),
      mRayDirection(inRay.mDirection),
      mRayInvDirection(inRay.mDirection),
      mShape(inShape),
      mSubShapeIDCreator(inSubShapeIDCreator)
    {}

    JPH_INLINE bool ShouldAbort() const { return mHit.mFraction <= 0.0f; }

    JPH_INLINE bool ShouldVisitRangeBlock(int inStackTop) const { return mDistanceStack[inStackTop] < mHit.mFraction; }

    JPH_INLINE int VisitRangeBlock(Vec4Arg inBoundsMinX, Vec4Arg inBoundsMinY, Vec4Arg inBoundsMinZ, Vec4Arg inBoundsMaxX,
      Vec4Arg inBoundsMaxY, Vec4Arg inBoundsMaxZ, UVec4 &ioProperties, int inStackTop)
    {
      // Test bounds of 4 children
      Vec4 distance =
        RayAABox4(mRayOrigin, mRayInvDirection, inBoundsMinX, inBoundsMinY, inBoundsMinZ, inBoundsMaxX, inBoundsMaxY, inBoundsMaxZ);

      // Sort so that highest values are first (we want to first process closer hits and we process stack top to bottom)
      return SortReverseAndStore(distance, mHit.mFraction, ioProperties, &mDistanceStack[inStackTop]);
    }

    JPH_INLINE void VisitTriangle(uint inX, uint inY, uint inTriangle, Vec3Arg inV0, Vec3Arg inV1, Vec3Arg inV2)
    {
      float fraction = RayTriangle(mRayOrigin, mRayDirection, inV0, inV1, inV2);
      if (fraction < mHit.mFraction)
      {
        // It's a closer hit
        mHit.mFraction = fraction;
        mHit.mSubShapeID2 = mShape->EncodeSubShapeID(mSubShapeIDCreator, inX, inY, inTriangle);
        mReturnValue = true;
      }
    }

    RayCastResult &mHit;
    Vec3 mRayOrigin;
    Vec3 mRayDirection;
    RayInvDirection mRayInvDirection;
    const HeightField16Shape *mShape;
    SubShapeIDCreator mSubShapeIDCreator;
    bool mReturnValue = false;
    float mDistanceStack[cStackSize]; //-V730_NOINIT
  };

  Visitor visitor(this, inRay, inSubShapeIDCreator, ioHit);
  WalkHeightField(visitor);

  return visitor.mReturnValue;
}

void HeightField16Shape::CastRay(const RayCast &inRay, const RayCastSettings &inRayCastSettings,
  const SubShapeIDCreator &inSubShapeIDCreator, CastRayCollector &ioCollector, const ShapeFilter &inShapeFilter) const
{
  JPH_PROFILE_FUNCTION();

  // Test shape filter
  if (!inShapeFilter.ShouldCollide(this, inSubShapeIDCreator.GetID()))
    return;

  struct Visitor
  {
    JPH_INLINE explicit Visitor(const HeightField16Shape *inShape, const RayCast &inRay, const RayCastSettings &inRayCastSettings,
      const SubShapeIDCreator &inSubShapeIDCreator, CastRayCollector &ioCollector) :
      mCollector(ioCollector),
      mRayOrigin(inRay.mOrigin),
      mRayDirection(inRay.mDirection),
      mRayInvDirection(inRay.mDirection),
      mBackFaceMode(inRayCastSettings.mBackFaceMode),
      mShape(inShape),
      mSubShapeIDCreator(inSubShapeIDCreator)
    {}

    JPH_INLINE bool ShouldAbort() const { return mCollector.ShouldEarlyOut(); }

    JPH_INLINE bool ShouldVisitRangeBlock(int inStackTop) const
    {
      return mDistanceStack[inStackTop] < mCollector.GetEarlyOutFraction();
    }

    JPH_INLINE int VisitRangeBlock(Vec4Arg inBoundsMinX, Vec4Arg inBoundsMinY, Vec4Arg inBoundsMinZ, Vec4Arg inBoundsMaxX,
      Vec4Arg inBoundsMaxY, Vec4Arg inBoundsMaxZ, UVec4 &ioProperties, int inStackTop)
    {
      // Test bounds of 4 children
      Vec4 distance =
        RayAABox4(mRayOrigin, mRayInvDirection, inBoundsMinX, inBoundsMinY, inBoundsMinZ, inBoundsMaxX, inBoundsMaxY, inBoundsMaxZ);

      // Sort so that highest values are first (we want to first process closer hits and we process stack top to bottom)
      return SortReverseAndStore(distance, mCollector.GetEarlyOutFraction(), ioProperties, &mDistanceStack[inStackTop]);
    }

    JPH_INLINE void VisitTriangle(uint inX, uint inY, uint inTriangle, Vec3Arg inV0, Vec3Arg inV1, Vec3Arg inV2) const
    {
      // Back facing check
      if (mBackFaceMode == EBackFaceMode::IgnoreBackFaces && (inV2 - inV0).Cross(inV1 - inV0).Dot(mRayDirection) < 0)
        return;

      // Check the triangle
      float fraction = RayTriangle(mRayOrigin, mRayDirection, inV0, inV1, inV2);
      if (fraction < mCollector.GetEarlyOutFraction())
      {
        RayCastResult hit;
        hit.mBodyID = TransformedShape::sGetBodyID(mCollector.GetContext());
        hit.mFraction = fraction;
        hit.mSubShapeID2 = mShape->EncodeSubShapeID(mSubShapeIDCreator, inX, inY, inTriangle);
        mCollector.AddHit(hit);
      }
    }

    CastRayCollector &mCollector;
    Vec3 mRayOrigin;
    Vec3 mRayDirection;
    RayInvDirection mRayInvDirection;
    EBackFaceMode mBackFaceMode;
    const HeightField16Shape *mShape;
    SubShapeIDCreator mSubShapeIDCreator;
    float mDistanceStack[cStackSize]; //-V730_NOINIT
  };

  Visitor visitor(this, inRay, inRayCastSettings, inSubShapeIDCreator, ioCollector);
  WalkHeightField(visitor);
}

void HeightField16Shape::CollidePoint(Vec3Arg inPoint, const SubShapeIDCreator &inSubShapeIDCreator,
  CollidePointCollector &ioCollector, const ShapeFilter &inShapeFilter) const
{
  // A height field doesn't have volume, so we can't test insideness
}

void HeightField16Shape::CollideSoftBodyVertices(Mat44Arg inCenterOfMassTransform, Vec3Arg inScale, SoftBodyVertex *ioVertices,
  uint inNumVertices, [[maybe_unused]] float inDeltaTime, [[maybe_unused]] Vec3Arg inDisplacementDueToGravity,
  int inCollidingShapeIndex) const
{
  JPH_PROFILE_FUNCTION();

  struct Visitor : public CollideSoftBodyVerticesVsTriangles
  {
    using CollideSoftBodyVerticesVsTriangles::CollideSoftBodyVerticesVsTriangles;

    JPH_INLINE bool ShouldAbort() const { return false; }

    JPH_INLINE bool ShouldVisitRangeBlock([[maybe_unused]] int inStackTop) const
    {
      return mDistanceStack[inStackTop] < mClosestDistanceSq;
    }

    JPH_INLINE int VisitRangeBlock(Vec4Arg inBoundsMinX, Vec4Arg inBoundsMinY, Vec4Arg inBoundsMinZ, Vec4Arg inBoundsMaxX,
      Vec4Arg inBoundsMaxY, Vec4Arg inBoundsMaxZ, UVec4 &ioProperties, int inStackTop)
    {
      // Get distance to vertex
      Vec4 dist_sq =
        AABox4DistanceSqToPoint(mLocalPosition, inBoundsMinX, inBoundsMinY, inBoundsMinZ, inBoundsMaxX, inBoundsMaxY, inBoundsMaxZ);

      // Sort so that highest values are first (we want to first process closer hits and we process stack top to bottom)
      return SortReverseAndStore(dist_sq, mClosestDistanceSq, ioProperties, &mDistanceStack[inStackTop]);
    }

    JPH_INLINE void VisitTriangle([[maybe_unused]] uint inX, [[maybe_unused]] uint inY, [[maybe_unused]] uint inTriangle, Vec3Arg inV0,
      Vec3Arg inV1, Vec3Arg inV2)
    {
      ProcessTriangle(inV0, inV1, inV2);
    }

    float mDistanceStack[cStackSize];
  };

  Visitor visitor(inCenterOfMassTransform, inScale);

  for (SoftBodyVertex *v = ioVertices, *sbv_end = ioVertices + inNumVertices; v < sbv_end; ++v)
    if (v->mInvMass > 0.0f)
    {
      visitor.StartVertex(*v);
      WalkHeightField(visitor);
      visitor.FinishVertex(*v, inCollidingShapeIndex);
    }
}

void HeightField16Shape::sCastConvexVsHeightField(const ShapeCast &inShapeCast, const ShapeCastSettings &inShapeCastSettings,
  const Shape *inShape, Vec3Arg inScale, [[maybe_unused]] const ShapeFilter &inShapeFilter, Mat44Arg inCenterOfMassTransform2,
  const SubShapeIDCreator &inSubShapeIDCreator1, const SubShapeIDCreator &inSubShapeIDCreator2, CastShapeCollector &ioCollector)
{
  JPH_PROFILE_FUNCTION();

  struct Visitor : public CastConvexVsTriangles
  {
    using CastConvexVsTriangles::CastConvexVsTriangles;

    JPH_INLINE bool ShouldAbort() const { return mCollector.ShouldEarlyOut(); }

    JPH_INLINE bool ShouldVisitRangeBlock(int inStackTop) const
    {
      return mDistanceStack[inStackTop] < mCollector.GetPositiveEarlyOutFraction();
    }

    JPH_INLINE int VisitRangeBlock(Vec4Arg inBoundsMinX, Vec4Arg inBoundsMinY, Vec4Arg inBoundsMinZ, Vec4Arg inBoundsMaxX,
      Vec4Arg inBoundsMaxY, Vec4Arg inBoundsMaxZ, UVec4 &ioProperties, int inStackTop)
    {
      // Scale the bounding boxes of this node
      Vec4 bounds_min_x, bounds_min_y, bounds_min_z, bounds_max_x, bounds_max_y, bounds_max_z;
      AABox4Scale(mScale, inBoundsMinX, inBoundsMinY, inBoundsMinZ, inBoundsMaxX, inBoundsMaxY, inBoundsMaxZ, bounds_min_x,
        bounds_min_y, bounds_min_z, bounds_max_x, bounds_max_y, bounds_max_z);

      // Enlarge them by the casted shape's box extents
      AABox4EnlargeWithExtent(mBoxExtent, bounds_min_x, bounds_min_y, bounds_min_z, bounds_max_x, bounds_max_y, bounds_max_z);

      // Test bounds of 4 children
      Vec4 distance =
        RayAABox4(mBoxCenter, mInvDirection, bounds_min_x, bounds_min_y, bounds_min_z, bounds_max_x, bounds_max_y, bounds_max_z);

      // Clear distance for invalid bounds
      distance = Vec4::sSelect(Vec4::sReplicate(FLT_MAX), distance, Vec4::sLessOrEqual(inBoundsMinY, inBoundsMaxY));

      // Sort so that highest values are first (we want to first process closer hits and we process stack top to bottom)
      return SortReverseAndStore(distance, mCollector.GetPositiveEarlyOutFraction(), ioProperties, &mDistanceStack[inStackTop]);
    }

    JPH_INLINE void VisitTriangle(uint inX, uint inY, uint inTriangle, Vec3Arg inV0, Vec3Arg inV1, Vec3Arg inV2)
    {
      // Create sub shape id for this part
      SubShapeID triangle_sub_shape_id = mShape2->EncodeSubShapeID(mSubShapeIDCreator2, inX, inY, inTriangle);

      // Determine active edges
      uint8 active_edges = mShape2->GetEdgeFlags(inX, inY, inTriangle);

      Cast(inV0, inV1, inV2, active_edges, triangle_sub_shape_id);
    }

    const HeightField16Shape *mShape2;
    RayInvDirection mInvDirection;
    Vec3 mBoxCenter;
    Vec3 mBoxExtent;
    SubShapeIDCreator mSubShapeIDCreator2;
    float mDistanceStack[cStackSize]; //-V730_NOINIT
  };

  JPH_ASSERT(inShape->GetSubType() == EShapeSubType::HeightField);
  const HeightField16Shape *shape = static_cast<const HeightField16Shape *>(inShape);

  Visitor visitor(inShapeCast, inShapeCastSettings, inScale, inCenterOfMassTransform2, inSubShapeIDCreator1, ioCollector);
  visitor.mShape2 = shape;
  visitor.mInvDirection.Set(inShapeCast.mDirection);
  visitor.mBoxCenter = inShapeCast.mShapeWorldBounds.GetCenter();
  visitor.mBoxExtent = inShapeCast.mShapeWorldBounds.GetExtent();
  visitor.mSubShapeIDCreator2 = inSubShapeIDCreator2;
  shape->WalkHeightField(visitor);
}

void HeightField16Shape::sCastSphereVsHeightField(const ShapeCast &inShapeCast, const ShapeCastSettings &inShapeCastSettings,
  const Shape *inShape, Vec3Arg inScale, [[maybe_unused]] const ShapeFilter &inShapeFilter, Mat44Arg inCenterOfMassTransform2,
  const SubShapeIDCreator &inSubShapeIDCreator1, const SubShapeIDCreator &inSubShapeIDCreator2, CastShapeCollector &ioCollector)
{
  JPH_PROFILE_FUNCTION();

  struct Visitor : public CastSphereVsTriangles
  {
    using CastSphereVsTriangles::CastSphereVsTriangles;

    JPH_INLINE bool ShouldAbort() const { return mCollector.ShouldEarlyOut(); }

    JPH_INLINE bool ShouldVisitRangeBlock(int inStackTop) const
    {
      return mDistanceStack[inStackTop] < mCollector.GetPositiveEarlyOutFraction();
    }

    JPH_INLINE int VisitRangeBlock(Vec4Arg inBoundsMinX, Vec4Arg inBoundsMinY, Vec4Arg inBoundsMinZ, Vec4Arg inBoundsMaxX,
      Vec4Arg inBoundsMaxY, Vec4Arg inBoundsMaxZ, UVec4 &ioProperties, int inStackTop)
    {
      // Scale the bounding boxes of this node
      Vec4 bounds_min_x, bounds_min_y, bounds_min_z, bounds_max_x, bounds_max_y, bounds_max_z;
      AABox4Scale(mScale, inBoundsMinX, inBoundsMinY, inBoundsMinZ, inBoundsMaxX, inBoundsMaxY, inBoundsMaxZ, bounds_min_x,
        bounds_min_y, bounds_min_z, bounds_max_x, bounds_max_y, bounds_max_z);

      // Enlarge them by the radius of the sphere
      AABox4EnlargeWithExtent(Vec3::sReplicate(mRadius), bounds_min_x, bounds_min_y, bounds_min_z, bounds_max_x, bounds_max_y,
        bounds_max_z);

      // Test bounds of 4 children
      Vec4 distance =
        RayAABox4(mStart, mInvDirection, bounds_min_x, bounds_min_y, bounds_min_z, bounds_max_x, bounds_max_y, bounds_max_z);

      // Clear distance for invalid bounds
      distance = Vec4::sSelect(Vec4::sReplicate(FLT_MAX), distance, Vec4::sLessOrEqual(inBoundsMinY, inBoundsMaxY));

      // Sort so that highest values are first (we want to first process closer hits and we process stack top to bottom)
      return SortReverseAndStore(distance, mCollector.GetPositiveEarlyOutFraction(), ioProperties, &mDistanceStack[inStackTop]);
    }

    JPH_INLINE void VisitTriangle(uint inX, uint inY, uint inTriangle, Vec3Arg inV0, Vec3Arg inV1, Vec3Arg inV2)
    {
      // Create sub shape id for this part
      SubShapeID triangle_sub_shape_id = mShape2->EncodeSubShapeID(mSubShapeIDCreator2, inX, inY, inTriangle);

      // Determine active edges
      uint8 active_edges = mShape2->GetEdgeFlags(inX, inY, inTriangle);

      Cast(inV0, inV1, inV2, active_edges, triangle_sub_shape_id);
    }

    const HeightField16Shape *mShape2;
    RayInvDirection mInvDirection;
    SubShapeIDCreator mSubShapeIDCreator2;
    float mDistanceStack[cStackSize]; //-V730_NOINIT
  };

  JPH_ASSERT(inShape->GetSubType() == EShapeSubType::HeightField);
  const HeightField16Shape *shape = static_cast<const HeightField16Shape *>(inShape);

  Visitor visitor(inShapeCast, inShapeCastSettings, inScale, inCenterOfMassTransform2, inSubShapeIDCreator1, ioCollector);
  visitor.mShape2 = shape;
  visitor.mInvDirection.Set(inShapeCast.mDirection);
  visitor.mSubShapeIDCreator2 = inSubShapeIDCreator2;
  shape->WalkHeightField(visitor);
}

struct HeightField16Shape::HSGetTrianglesContext
{
  HSGetTrianglesContext(const HeightField16Shape *inShape, const AABox &inBox, Vec3Arg inPositionCOM, QuatArg inRotation,
    Vec3Arg inScale) :
    mDecodeCtx(inShape),
    mShape(inShape),
    mLocalBox(Mat44::sInverseRotationTranslation(inRotation, inPositionCOM), inBox),
    mHeightFieldScale(inScale),
    mLocalToWorld(Mat44::sRotationTranslation(inRotation, inPositionCOM) * Mat44::sScale(inScale)),
    mIsInsideOut(ScaleHelpers::IsInsideOut(inScale))
  {}

  bool ShouldAbort() const { return mShouldAbort; }

  bool ShouldVisitRangeBlock([[maybe_unused]] int inStackTop) const { return true; }

  int VisitRangeBlock(Vec4Arg inBoundsMinX, Vec4Arg inBoundsMinY, Vec4Arg inBoundsMinZ, Vec4Arg inBoundsMaxX, Vec4Arg inBoundsMaxY,
    Vec4Arg inBoundsMaxZ, UVec4 &ioProperties, [[maybe_unused]] int inStackTop) const
  {
    // Scale the bounding boxes of this node
    Vec4 bounds_min_x, bounds_min_y, bounds_min_z, bounds_max_x, bounds_max_y, bounds_max_z;
    AABox4Scale(mHeightFieldScale, inBoundsMinX, inBoundsMinY, inBoundsMinZ, inBoundsMaxX, inBoundsMaxY, inBoundsMaxZ, bounds_min_x,
      bounds_min_y, bounds_min_z, bounds_max_x, bounds_max_y, bounds_max_z);

    // Test which nodes collide
    UVec4 collides = AABox4VsBox(mLocalBox, bounds_min_x, bounds_min_y, bounds_min_z, bounds_max_x, bounds_max_y, bounds_max_z);

    // Filter out invalid bounding boxes
    collides = UVec4::sAnd(collides, Vec4::sLessOrEqual(inBoundsMinY, inBoundsMaxY));

    return CountAndSortTrues(collides, ioProperties);
  }

  void VisitTriangle(uint inX, uint inY, [[maybe_unused]] uint inTriangle, Vec3Arg inV0, Vec3Arg inV1, Vec3Arg inV2)
  {
    // When the buffer is full and we cannot process the triangles, abort the height field walk. The next time GetTrianglesNext is
    // called we will continue here.
    if (mNumTrianglesFound + 1 > mMaxTrianglesRequested)
    {
      mShouldAbort = true;
      return;
    }

    // Store vertices as Float3
    if (mIsInsideOut)
    {
      // Reverse vertices
      (mLocalToWorld * inV0).StoreFloat3(mTriangleVertices++);
      (mLocalToWorld * inV2).StoreFloat3(mTriangleVertices++);
      (mLocalToWorld * inV1).StoreFloat3(mTriangleVertices++);
    }
    else
    {
      // Normal scale
      (mLocalToWorld * inV0).StoreFloat3(mTriangleVertices++);
      (mLocalToWorld * inV1).StoreFloat3(mTriangleVertices++);
      (mLocalToWorld * inV2).StoreFloat3(mTriangleVertices++);
    }

    // Decode material
    if (mMaterials != nullptr)
      *mMaterials++ = mShape->GetMaterial(inX, inY);

    // Accumulate triangles found
    mNumTrianglesFound++;
  }

  DecodingContext mDecodeCtx;
  const HeightField16Shape *mShape;
  OrientedBox mLocalBox;
  Vec3 mHeightFieldScale;
  Mat44 mLocalToWorld;
  int mMaxTrianglesRequested;         //-V730_NOINIT
  Float3 *mTriangleVertices;          //-V730_NOINIT
  int mNumTrianglesFound;             //-V730_NOINIT
  const PhysicsMaterial **mMaterials; //-V730_NOINIT
  bool mShouldAbort;                  //-V730_NOINIT
  bool mIsInsideOut;
};

void HeightField16Shape::GetTrianglesStart(GetTrianglesContext &ioContext, const AABox &inBox, Vec3Arg inPositionCOM,
  QuatArg inRotation, Vec3Arg inScale) const
{
  static_assert(sizeof(HSGetTrianglesContext) <= sizeof(GetTrianglesContext), "GetTrianglesContext too small");
  JPH_ASSERT(IsAligned(&ioContext, alignof(HSGetTrianglesContext)));

  new (&ioContext) HSGetTrianglesContext(this, inBox, inPositionCOM, inRotation, inScale);
}

int HeightField16Shape::GetTrianglesNext(GetTrianglesContext &ioContext, int inMaxTrianglesRequested, Float3 *outTriangleVertices,
  const PhysicsMaterial **outMaterials) const
{
  static_assert(cGetTrianglesMinTrianglesRequested >= 1, "cGetTrianglesMinTrianglesRequested is too small");
  JPH_ASSERT(inMaxTrianglesRequested >= cGetTrianglesMinTrianglesRequested);

  // Check if we're done
  HSGetTrianglesContext &context = (HSGetTrianglesContext &)ioContext;
  if (context.mDecodeCtx.IsDoneWalking())
    return 0;

  // Store parameters on context
  context.mMaxTrianglesRequested = inMaxTrianglesRequested;
  context.mTriangleVertices = outTriangleVertices;
  context.mMaterials = outMaterials;
  context.mShouldAbort = false; // Reset the abort flag
  context.mNumTrianglesFound = 0;

  // Continue (or start) walking the height field
  context.mDecodeCtx.WalkHeightField(context);
  return context.mNumTrianglesFound;
}

void HeightField16Shape::sCollideConvexVsHeightField(const Shape *inShape1, const Shape *inShape2, Vec3Arg inScale1, Vec3Arg inScale2,
  Mat44Arg inCenterOfMassTransform1, Mat44Arg inCenterOfMassTransform2, const SubShapeIDCreator &inSubShapeIDCreator1,
  const SubShapeIDCreator &inSubShapeIDCreator2, const CollideShapeSettings &inCollideShapeSettings,
  CollideShapeCollector &ioCollector, [[maybe_unused]] const ShapeFilter &inShapeFilter)
{
  JPH_PROFILE_FUNCTION();

  // Get the shapes
  JPH_ASSERT(inShape1->GetType() == EShapeType::Convex);
  JPH_ASSERT(inShape2->GetType() == EShapeType::HeightField);
  const ConvexShape *shape1 = static_cast<const ConvexShape *>(inShape1);
  const HeightField16Shape *shape2 = static_cast<const HeightField16Shape *>(inShape2);

  struct Visitor : public CollideConvexVsTriangles
  {
    using CollideConvexVsTriangles::CollideConvexVsTriangles;

    JPH_INLINE bool ShouldAbort() const { return mCollector.ShouldEarlyOut(); }

    JPH_INLINE bool ShouldVisitRangeBlock([[maybe_unused]] int inStackTop) const { return true; }

    JPH_INLINE int VisitRangeBlock(Vec4Arg inBoundsMinX, Vec4Arg inBoundsMinY, Vec4Arg inBoundsMinZ, Vec4Arg inBoundsMaxX,
      Vec4Arg inBoundsMaxY, Vec4Arg inBoundsMaxZ, UVec4 &ioProperties, [[maybe_unused]] int inStackTop) const
    {
      // Scale the bounding boxes of this node
      Vec4 bounds_min_x, bounds_min_y, bounds_min_z, bounds_max_x, bounds_max_y, bounds_max_z;
      AABox4Scale(mScale2, inBoundsMinX, inBoundsMinY, inBoundsMinZ, inBoundsMaxX, inBoundsMaxY, inBoundsMaxZ, bounds_min_x,
        bounds_min_y, bounds_min_z, bounds_max_x, bounds_max_y, bounds_max_z);

      // Test which nodes collide
      UVec4 collides =
        AABox4VsBox(mBoundsOf1InSpaceOf2, bounds_min_x, bounds_min_y, bounds_min_z, bounds_max_x, bounds_max_y, bounds_max_z);

      // Filter out invalid bounding boxes
      collides = UVec4::sAnd(collides, Vec4::sLessOrEqual(inBoundsMinY, inBoundsMaxY));

      return CountAndSortTrues(collides, ioProperties);
    }

    JPH_INLINE void VisitTriangle(uint inX, uint inY, uint inTriangle, Vec3Arg inV0, Vec3Arg inV1, Vec3Arg inV2)
    {
      // Create ID for triangle
      SubShapeID triangle_sub_shape_id = mShape2->EncodeSubShapeID(mSubShapeIDCreator2, inX, inY, inTriangle);

      // Determine active edges
      uint8 active_edges = mShape2->GetEdgeFlags(inX, inY, inTriangle);

      Collide(inV0, inV1, inV2, active_edges, triangle_sub_shape_id);
    }

    const HeightField16Shape *mShape2;
    SubShapeIDCreator mSubShapeIDCreator2;
  };

  Visitor visitor(shape1, inScale1, inScale2, inCenterOfMassTransform1, inCenterOfMassTransform2, inSubShapeIDCreator1.GetID(),
    inCollideShapeSettings, ioCollector);
  visitor.mShape2 = shape2;
  visitor.mSubShapeIDCreator2 = inSubShapeIDCreator2;
  shape2->WalkHeightField(visitor);
}

void HeightField16Shape::sCollideSphereVsHeightField(const Shape *inShape1, const Shape *inShape2, Vec3Arg inScale1, Vec3Arg inScale2,
  Mat44Arg inCenterOfMassTransform1, Mat44Arg inCenterOfMassTransform2, const SubShapeIDCreator &inSubShapeIDCreator1,
  const SubShapeIDCreator &inSubShapeIDCreator2, const CollideShapeSettings &inCollideShapeSettings,
  CollideShapeCollector &ioCollector, [[maybe_unused]] const ShapeFilter &inShapeFilter)
{
  JPH_PROFILE_FUNCTION();

  // Get the shapes
  JPH_ASSERT(inShape1->GetSubType() == EShapeSubType::Sphere);
  JPH_ASSERT(inShape2->GetType() == EShapeType::HeightField);
  const SphereShape *shape1 = static_cast<const SphereShape *>(inShape1);
  const HeightField16Shape *shape2 = static_cast<const HeightField16Shape *>(inShape2);

  struct Visitor : public CollideSphereVsTriangles
  {
    using CollideSphereVsTriangles::CollideSphereVsTriangles;

    JPH_INLINE bool ShouldAbort() const { return mCollector.ShouldEarlyOut(); }

    JPH_INLINE bool ShouldVisitRangeBlock([[maybe_unused]] int inStackTop) const { return true; }

    JPH_INLINE int VisitRangeBlock(Vec4Arg inBoundsMinX, Vec4Arg inBoundsMinY, Vec4Arg inBoundsMinZ, Vec4Arg inBoundsMaxX,
      Vec4Arg inBoundsMaxY, Vec4Arg inBoundsMaxZ, UVec4 &ioProperties, [[maybe_unused]] int inStackTop) const
    {
      // Scale the bounding boxes of this node
      Vec4 bounds_min_x, bounds_min_y, bounds_min_z, bounds_max_x, bounds_max_y, bounds_max_z;
      AABox4Scale(mScale2, inBoundsMinX, inBoundsMinY, inBoundsMinZ, inBoundsMaxX, inBoundsMaxY, inBoundsMaxZ, bounds_min_x,
        bounds_min_y, bounds_min_z, bounds_max_x, bounds_max_y, bounds_max_z);

      // Test which nodes collide
      UVec4 collides = AABox4VsSphere(mSphereCenterIn2, mRadiusPlusMaxSeparationSq, bounds_min_x, bounds_min_y, bounds_min_z,
        bounds_max_x, bounds_max_y, bounds_max_z);

      // Filter out invalid bounding boxes
      collides = UVec4::sAnd(collides, Vec4::sLessOrEqual(inBoundsMinY, inBoundsMaxY));

      return CountAndSortTrues(collides, ioProperties);
    }

    JPH_INLINE void VisitTriangle(uint inX, uint inY, uint inTriangle, Vec3Arg inV0, Vec3Arg inV1, Vec3Arg inV2)
    {
      // Create ID for triangle
      SubShapeID triangle_sub_shape_id = mShape2->EncodeSubShapeID(mSubShapeIDCreator2, inX, inY, inTriangle);

      // Determine active edges
      uint8 active_edges = mShape2->GetEdgeFlags(inX, inY, inTriangle);

      Collide(inV0, inV1, inV2, active_edges, triangle_sub_shape_id);
    }

    const HeightField16Shape *mShape2;
    SubShapeIDCreator mSubShapeIDCreator2;
  };

  Visitor visitor(shape1, inScale1, inScale2, inCenterOfMassTransform1, inCenterOfMassTransform2, inSubShapeIDCreator1.GetID(),
    inCollideShapeSettings, ioCollector);
  visitor.mShape2 = shape2;
  visitor.mSubShapeIDCreator2 = inSubShapeIDCreator2;
  shape2->WalkHeightField(visitor);
}

void HeightField16Shape::SaveBinaryState(StreamOut &inStream) const
{
  Shape::SaveBinaryState(inStream);

  inStream.Write(mOffset);
  inStream.Write(mScale);
  inStream.Write(mSampleCount);
  inStream.Write(mActiveEdges);
  inStream.Write(mMaterialIndices);
  inStream.Write(mNumBitsPerMaterialIndex);
}

void HeightField16Shape::RestoreBinaryState(StreamIn &inStream)
{
  Shape::RestoreBinaryState(inStream);

  inStream.Read(mOffset);
  inStream.Read(mScale);
  inStream.Read(mSampleCount);
  inStream.Read(mActiveEdges);
  inStream.Read(mMaterialIndices);
  inStream.Read(mNumBitsPerMaterialIndex);
}

void HeightField16Shape::SaveMaterialState(PhysicsMaterialList &outMaterials) const { outMaterials = mMaterials; }

void HeightField16Shape::RestoreMaterialState(const PhysicsMaterialRefC *inMaterials, uint inNumMaterials)
{
  mMaterials.assign(inMaterials, inMaterials + inNumMaterials);
}

Shape::Stats HeightField16Shape::GetStats() const
{
  return Stats(sizeof(*this) + mMaterials.size() * sizeof(Ref<PhysicsMaterial>) + mActiveEdges.size() * sizeof(uint8) +
                 mMaterialIndices.size() * sizeof(uint8),
    !mHData->blockVariance ? 0 : Square(mSampleCount - 1) * 2);
}

void HeightField16Shape::sRegister()
{
  ShapeFunctions &f = ShapeFunctions::sGet(EShapeSubType::HeightField);
  f.mConstruct = []() -> Shape * { return new HeightField16Shape; };
  f.mColor = Color::sPurple;

  for (EShapeSubType s : sConvexSubShapeTypes)
  {
    CollisionDispatch::sRegisterCollideShape(s, EShapeSubType::HeightField, sCollideConvexVsHeightField);
    CollisionDispatch::sRegisterCastShape(s, EShapeSubType::HeightField, sCastConvexVsHeightField);

    CollisionDispatch::sRegisterCastShape(EShapeSubType::HeightField, s, CollisionDispatch::sReversedCastShape);
    CollisionDispatch::sRegisterCollideShape(EShapeSubType::HeightField, s, CollisionDispatch::sReversedCollideShape);
  }

  // Specialized collision functions
  CollisionDispatch::sRegisterCollideShape(EShapeSubType::Sphere, EShapeSubType::HeightField, sCollideSphereVsHeightField);
  CollisionDispatch::sRegisterCastShape(EShapeSubType::Sphere, EShapeSubType::HeightField, sCastSphereVsHeightField);
}

JPH_NAMESPACE_END
