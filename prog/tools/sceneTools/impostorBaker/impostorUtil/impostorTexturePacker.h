// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/vector.h>
#include <EASTL/string.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_Point2.h>
#include <math/integer/dag_IBBox2.h>
#include <util/dag_baseDef.h>
#include <math/dag_Matrix3.h>
#include <math/dag_Point4.h>
#include <math/dag_TMatrix.h>

enum class SaveResult
{
  OK = 0,
  FAILED,
  SKIPPED
};

struct SliceSeparator
{
  // connects the top and bottom edges of the textures using these offsets
  int topXOffset = 0;    // y = 0
  int bottomXOffset = 0; // y = sliceHeight-1
};

struct SliceData
{
  IBBox2 contentBbox; // in slice space
  SliceSeparator separator;
  eastl::vector<int> scaledLeftmostAlongY;
  eastl::vector<int> scaledRightmostAlongY;
  // convex point indices point into to previous arrays
  // they are also the y coordinates of the points
  eastl::vector<int> leftConvexPointIndices;  // connecting these defines a convex bounding shape for the left side
  eastl::vector<int> rightConvexPointIndices; // same for the right side
  int xOffset = 0;
  bool flipped = false;
  Point4 normalizedSeparatorLines;
};

struct TexturePackingProfilingInfo
{
  eastl::string assetName;
  uint32_t texWidth = 0;
  uint32_t texHeight = 0;
  uint32_t sliceWidth = 0;
  uint32_t sliceHeight = 0;
  uint64_t arrayTexPixelCount = 0;
  uint64_t packedTexPixelCount = 0;
  // To get the same quality improvement with the texture array representation:
  // higherQualitySliceWidth := originalSliceWidth * (1+qualityImprovement)
  // higherQualitySliceHeight := originalSliceHeight * (1+qualityImprovement)
  double qualityImprovement = 0;
  double sliceStretchX = 1;
  double sliceStretchY_min = 1;
  double sliceStretchY_max = 1;

  float albedoAlphaSimilarity = 0;
  float normalTranslucencySimilarity = 0;
  float aoSmoothnessSimilarity = 0;

  SaveResult albedoAlphaBaked = SaveResult::FAILED;
  SaveResult normalTranslucencyBaked = SaveResult::FAILED;
  SaveResult aoSmoothnessBaked = SaveResult::FAILED;

  // bushes and trees have different shapes => separate statistics is useful
  enum Type
  {
    UNKNOWN = 0,
    BUSH,
    TREE
  } type;
  operator bool() const { return texWidth != 0; }
};

class PackedImpostorSlices
{
public:
  PackedImpostorSlices(const char *asset_name, int num_slices, IPoint2 slice_resolution);

  void setSlice(int slice_id, const uint8_t *data, int stride);
  TMatrix getViewToContent(int slice_id, IPoint2 destination_resolution) const;

  void packTexture(IPoint2 destination_resolution);
  // private:
  eastl::string assetName;
  IPoint2 sliceResolution;
  TexturePackingProfilingInfo quality;
  eastl::vector<SliceData> slices;
  float xScaling = 1;
};

PackedImpostorSlices pack_impostor_texture(int num_samples, IPoint2 slice_resolution, IPoint2 destination_resolution,
  const uint8_t *data, int stride, const char *asset_name);
