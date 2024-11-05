// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "impostorTexturePacker.h"

static eastl::vector<int> build_convex_shape(int height, const int *x_values, int direction)
{
  eastl::vector<int> ret;
  ret.push_back(0);
  while (ret.back() != height - 1)
  {
    int currentIndex = ret.back();
    int currentX = x_values[currentIndex];
    int nextIndex = currentIndex + 1;
    IPoint2 nextDir = IPoint2(x_values[nextIndex] - currentX, nextIndex - currentIndex);
    for (int index = currentIndex + 2; index < height; ++index)
    {
      IPoint2 dir = IPoint2(x_values[index] - currentX, index - currentIndex);
      // from the view of the (x_values[currentIndex], currentIndex),
      // compared to the direction towards the next point (nextDir)
      // 0 means same direction
      int d = nextDir.x * dir.y - nextDir.y * dir.x;
      if (d * direction >= 0)
      {
        // equality is allowed to reduce the number of convex points
        nextDir = dir;
        nextIndex = index;
      }
    }
    ret.push_back(nextIndex);
  }
  return ret;
}

static SliceData init_slice(IPoint2 slice_resolution, const uint8_t *data, int stride, int h, int v, const char *asset_name)
{
  SliceData slice;
  eastl::vector<int> unscaledLeftmostAlongY;
  eastl::vector<int> unscaledRightmostAlongY;
  unscaledLeftmostAlongY.resize(slice_resolution.y, slice_resolution.x + 10000);
  unscaledRightmostAlongY.resize(slice_resolution.y, -10000);

  for (int y0 = 0; y0 < slice_resolution.y; ++y0)
  {
    for (int x0 = 0; x0 < slice_resolution.x; ++x0)
    {
      const int idx = static_cast<int>(y0 * stride) + x0;
      const bool empty = data[idx] == 0;
      if (empty)
        continue;
      if (x0 < unscaledLeftmostAlongY[y0])
        unscaledLeftmostAlongY[y0] = x0;
      if (x0 > unscaledRightmostAlongY[y0])
        unscaledRightmostAlongY[y0] = x0;
      slice.contentBbox.add(IPoint2(x0, y0));
    }
  }

  G_ASSERTF_RETURN(!slice.contentBbox.isEmpty(), slice,
    "Impostor texture empty. There is probably an error while loading the requested asset <%s>. "
    "Check the error log and try opening the asset with Asset Viewer",
    asset_name);

  slice.scaledLeftmostAlongY.resize(slice_resolution.y);
  slice.scaledRightmostAlongY.resize(slice_resolution.y);
  for (int i = 0; i < slice_resolution.y; ++i)
  {
    int y0 = floorf((float(i) / slice_resolution.y) * slice.contentBbox.width().y) + slice.contentBbox.getMin().y;
    int y1 = ceilf((float(i + 1) / slice_resolution.y) * slice.contentBbox.width().y) + slice.contentBbox.getMin().y;
    slice.scaledLeftmostAlongY[i] = unscaledLeftmostAlongY[y0];
    slice.scaledRightmostAlongY[i] = unscaledRightmostAlongY[y0];
    for (int y = y0 + 1; y < y1; ++y)
    {
      slice.scaledLeftmostAlongY[i] = min(slice.scaledLeftmostAlongY[i], unscaledLeftmostAlongY[y]);
      slice.scaledRightmostAlongY[i] = max(slice.scaledRightmostAlongY[i], unscaledRightmostAlongY[y]);
    }
  }
  slice.leftConvexPointIndices = build_convex_shape(slice_resolution.y, slice.scaledLeftmostAlongY.data(), 1);
  slice.rightConvexPointIndices = build_convex_shape(slice_resolution.y, slice.scaledRightmostAlongY.data(), -1);

  return slice;
}

struct PackSliceData
{
  SliceSeparator separator;
  int nextSliceOffset;
};

static int get_x_at(int slice_height, IPoint2 p0, IPoint2 p1, int y)
{
  IPoint2 d = p1 - p0;
  IPoint2 n = IPoint2(d.y, -d.x); // normal
  return roundf(safediv(float(n.y), float(n.x)) * (p0.y - y) + p0.x);
}

static SliceSeparator get_separator(int slice_height, IPoint2 p0, IPoint2 p1)
{
  SliceSeparator ret;
  ret.topXOffset = get_x_at(slice_height, p0, p1, 0);
  ret.bottomXOffset = get_x_at(slice_height, p0, p1, slice_height - 1);
  return ret;
}

// flip = one of the shapes is flipped compared to the other
static PackSliceData pack_on_side(IPoint2 slice_resolution, int side, bool flip, int convex_points_count, const int *convex_points,
  const int *points, int opposite_convex_point_count, const int *opposite_convex_points, const int *opposite_points)
{
  // The optimal separator
  // * touches one of the sides of the convex bounding polygon of one of the slices
  // * and it touches a point from the opposite side convex polygon
  constexpr int border = 1; // there should be some empty pixels between the two slices
  PackSliceData ret;
  ret.nextSliceOffset = slice_resolution.x + border * 2;
  ret.separator.bottomXOffset = ret.separator.topXOffset = slice_resolution.x + border;
  IPoint2 yTm = flip ? IPoint2(-1, slice_resolution.y - 1) : IPoint2(1, 0);
  for (int sideId = 0; sideId < convex_points_count - 1; ++sideId)
  {
    int ry0 = convex_points[sideId];
    int ry1 = convex_points[sideId + 1];
    IPoint2 p0 = IPoint2(points[ry0] + border * side, ry0);
    IPoint2 p1 = IPoint2(points[ry1] + border * side, ry1);
    if (p0.x < 0 || p1.x < 0 || p0.x >= slice_resolution.x || p1.x >= slice_resolution.x)
      continue;
    SliceSeparator separator = get_separator(slice_resolution.y, p0, p1);
    int xOffset = 0;
    for (int oppositeConvPointId = 0; oppositeConvPointId < opposite_convex_point_count; ++oppositeConvPointId)
    {
      IPoint2 p = IPoint2(opposite_points[opposite_convex_points[oppositeConvPointId]], opposite_convex_points[oppositeConvPointId]);
      p.y = yTm.x * p.y + yTm.y;
      if (p.x < 0 || p.x >= slice_resolution.x)
        continue;
      int diff = side * (get_x_at(slice_resolution.y, p0, p1, p.y) - p.x);
      int offset = border + diff;
      if (offset > xOffset)
        xOffset = offset;
    }
    if (xOffset < ret.nextSliceOffset)
    {
      ret.nextSliceOffset = xOffset;
      ret.separator = separator;
    }
  }
  return ret;
}

static PackSliceData pack_slices(IPoint2 slice_resolution, const SliceData &s1, const SliceData &s2, bool flip)
{
  PackSliceData packToRight = pack_on_side(slice_resolution, 1, flip, s1.rightConvexPointIndices.size(),
    s1.rightConvexPointIndices.data(), s1.scaledRightmostAlongY.data(), s2.leftConvexPointIndices.size(),
    s2.leftConvexPointIndices.data(), s2.scaledLeftmostAlongY.data());
  if (s1.flipped)
    eastl::swap(packToRight.separator.bottomXOffset, packToRight.separator.topXOffset);
  PackSliceData packToLeft = pack_on_side(slice_resolution, -1, flip, s2.leftConvexPointIndices.size(),
    s2.leftConvexPointIndices.data(), s2.scaledLeftmostAlongY.data(), s1.rightConvexPointIndices.size(),
    s1.rightConvexPointIndices.data(), s1.scaledRightmostAlongY.data());
  packToLeft.separator.bottomXOffset += packToLeft.nextSliceOffset;
  packToLeft.separator.topXOffset += packToLeft.nextSliceOffset;
  if (s1.flipped != flip)
    eastl::swap(packToLeft.separator.bottomXOffset, packToLeft.separator.topXOffset);
  return packToRight.nextSliceOffset <= packToLeft.nextSliceOffset ? packToRight : packToLeft;
}

static TexturePackingProfilingInfo get_packing_quality(IPoint2 tex_resolution, IPoint2 slice_resolution, const char *asset_name,
  float xScale, int num_silces, const SliceData *slices)
{
  TexturePackingProfilingInfo ret;
  ret.assetName = eastl::string{asset_name};
  ret.texWidth = tex_resolution.x;
  ret.texHeight = tex_resolution.y;
  ret.sliceWidth = 0;
  ret.sliceHeight = tex_resolution.y;
  ret.sliceStretchX = xScale;
  ret.sliceStretchY_min = ret.sliceStretchY_max = double(slice_resolution.y) / slices[0].contentBbox.width().y;
  for (int i = 0; i < num_silces; ++i)
  {
    IBBox2 bbox = slices[i].contentBbox;
    ret.sliceWidth = eastl::max(ret.sliceWidth, uint32_t(bbox.width().x * xScale));
    double yStretch = double(slice_resolution.y) / slices[i].contentBbox.width().y;
    ret.sliceStretchY_min = min(yStretch, ret.sliceStretchY_min);
    ret.sliceStretchY_max = max(yStretch, ret.sliceStretchY_max);
  }
  ret.arrayTexPixelCount = uint64_t(ret.sliceWidth) * uint64_t(ret.sliceHeight) * 9;
  ret.packedTexPixelCount = uint64_t(ret.texWidth) * uint64_t(ret.texHeight);
  ret.qualityImprovement = sqrt(double(ret.arrayTexPixelCount) / double(ret.packedTexPixelCount)) - 1.0;
  if (strstr(asset_name, "bush") != nullptr || strstr(asset_name, "fern") != nullptr)
    ret.type = TexturePackingProfilingInfo::BUSH;
  else
    ret.type = strstr(asset_name, "tree") != nullptr ? TexturePackingProfilingInfo::TREE : TexturePackingProfilingInfo::UNKNOWN;
  return ret;
}

PackedImpostorSlices::PackedImpostorSlices(const char *asset_name, int num_slices, IPoint2 slice_resolution) :
  assetName(asset_name), sliceResolution(slice_resolution)
{
  slices.resize(num_slices);
}

void PackedImpostorSlices::setSlice(int slice_id, const uint8_t *data, int stride)
{
  slices[slice_id] = init_slice(sliceResolution, data, stride, slice_id, 0, assetName.c_str());
}

void PackedImpostorSlices::packTexture(IPoint2 destination_resolution)
{
  for (int h = 0; h < (int)slices.size() - 1; ++h)
  {
    PackSliceData separator = pack_slices(sliceResolution, slices[h], slices[h + 1], false);
    PackSliceData flippedSeparator = pack_slices(sliceResolution, slices[h], slices[h + 1], true);
    if (flippedSeparator.nextSliceOffset < separator.nextSliceOffset)
    {
      separator = flippedSeparator;
      slices[h + 1].flipped = !slices[h].flipped;
    }
    else
      slices[h + 1].flipped = slices[h].flipped;
    slices[h].separator = separator.separator;
    slices[h + 1].xOffset =
      slices[h + 1].contentBbox.getMin().x - slices[h].contentBbox.getMin().x + slices[h].xOffset + separator.nextSliceOffset;
  }

  int packedResolutionX = slices.back().xOffset + slices.back().contentBbox.width().x;
  xScaling = packedResolutionX > 0 ? float(destination_resolution.x) / packedResolutionX : 1;

  for (int h = 0; h < slices.size(); ++h)
    slices[h].normalizedSeparatorLines = Point4(0, 0, 1, 1);
  for (int h = 0; h < slices.size(); ++h)
  {
    if (h + 1 < slices.size())
    {
      slices[h + 1].normalizedSeparatorLines.x = slices[h].normalizedSeparatorLines.z =
        float(slices[h].separator.topXOffset - slices[h].contentBbox.getMin().x + slices[h].xOffset) * xScaling /
        destination_resolution.x;
      slices[h + 1].normalizedSeparatorLines.y = slices[h].normalizedSeparatorLines.w =
        float(slices[h].separator.bottomXOffset - slices[h].contentBbox.getMin().x + slices[h].xOffset) * xScaling /
        destination_resolution.x;
    }
  }

  quality = get_packing_quality(destination_resolution, sliceResolution, assetName.c_str(), xScaling, slices.size(), slices.data());
}

TMatrix PackedImpostorSlices::getViewToContent(int slice_id, IPoint2 destination_resolution) const
{
  TMatrix flipY;
  flipY.identity();
  flipY.setcol(1, Point3(0, -1, 0));

  TMatrix viewToSlice; // (-1,-1):(1,1) -> (0,0):(sliceWidth,sliceHeight)
  viewToSlice.identity();
  viewToSlice.setcol(0, Point3(float(sliceResolution.x) * 0.5f, 0, 0));
  viewToSlice.setcol(1, Point3(0, float(sliceResolution.y) * 0.5f, 0));
  viewToSlice.setcol(3, Point3(float(sliceResolution.x) * 0.5f, float(sliceResolution.y) * 0.5, 0));

  TMatrix sliceToContentBox; // set origin to contentBox.min.xy (0,0):(contentBox.width.xy)
  sliceToContentBox.identity();
  sliceToContentBox.setcol(3, Point3(-slices[slice_id].contentBbox.getMin().x, -slices[slice_id].contentBbox.getMin().y, 0));

  TMatrix contentBoxToTexture; // (0,0):(destination_resolution)
  contentBoxToTexture.identity();
  contentBoxToTexture.setcol(0, Point3(xScaling, 0, 0));
  contentBoxToTexture.setcol(1, Point3(0, float(destination_resolution.y) / slices[slice_id].contentBbox.width().y, 0));
  contentBoxToTexture.setcol(3, Point3(xScaling * slices[slice_id].xOffset, 0, 0));

  TMatrix normalizeTexture; // (0,0):(destination_resolution) -> (-1,-1):(1,1)
  normalizeTexture.identity();
  normalizeTexture.setcol(0, Point3(2.f / destination_resolution.x, 0, 0));
  normalizeTexture.setcol(1, Point3(0, 2.f / destination_resolution.y, 0));
  normalizeTexture.setcol(3, Point3(-1, -1, 0));


  return flipY * normalizeTexture * contentBoxToTexture * sliceToContentBox * viewToSlice * flipY;
}
