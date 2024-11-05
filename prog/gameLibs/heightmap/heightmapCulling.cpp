// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <heightmap/heightmapHandler.h>
#include <math/dag_bounds2.h>
#include <math/dag_mathUtils.h>


HeightmapHeightCulling::~HeightmapHeightCulling() { clear_and_shrink(minMaxHeights); }

Point2 HeightmapHeightCulling::generateLodPoint(int offset, int i, int j, const int lod_size)
{
  Point2 result;
  result.x = min(min(minMaxHeights[offset + j * lod_size + i].x, minMaxHeights[offset + j * lod_size + i + 1].x),
    min(minMaxHeights[offset + (j + 1) * lod_size + i].x, minMaxHeights[offset + (j + 1) * lod_size + i + 1].x));
  result.y = min(min(minMaxHeights[offset + j * lod_size + i].y, minMaxHeights[offset + j * lod_size + i + 1].y),
    min(minMaxHeights[offset + (j + 1) * lod_size + i].y, minMaxHeights[offset + (j + 1) * lod_size + i + 1].y));
  return result;
}

void HeightmapHeightCulling::updateMinMaxHeights(HeightmapHandler *handler, const IBBox2 &ib)
{
  displacementUpwardMaxOffset = handler->getMaxUpwardDisplacement();
  displacementDownwardMaxOffset = handler->getMaxDownwardDisplacement();

  IBBox2 updateBox = {ib[0] / chunkSizeInTexels, ib[1] / chunkSizeInTexels + IPoint2(1, 1)};
  updateBox[1] = min(updateBox[1], IPoint2(HEIGHT_CULLING_BUFFER_SIZE, HEIGHT_CULLING_BUFFER_SIZE));
  // fill lod 0
  for (int j = updateBox[0].y; j < updateBox[1].y; j++)
  {
    for (int i = updateBox[0].x; i < updateBox[1].x; i++)
    {
      real hMin, hMax;
      Point2 xy = Point2(origin.x + (float)i * chunkSize, origin.y + (float)j * chunkSize);
      handler->getHeightmapHeightMinMaxInChunk(xy, chunkSize, hMin, hMax);
      minMaxHeights[j * HEIGHT_CULLING_BUFFER_SIZE + i] = Point2(hMin, -hMax);
    }
  }

  // fill other lods
  int tSize = HEIGHT_CULLING_BUFFER_SIZE / 2;
  for (int k = 1; k < HEIGHT_CULLING_LOD_COUNT; k++)
  {
    updateBox[0] /= 2;
    updateBox[1] = (updateBox[1] + IPoint2(1, 1)) / 2;
    for (int j = updateBox[0].y; j < updateBox[1].y; j++)
    {
      for (int i = updateBox[0].x; i < updateBox[1].x; i++)
      {
        int curLodOffset = lodOffsets[k];
        int prevLodOffset = lodOffsets[k - 1];
        minMaxHeights[curLodOffset + j * tSize + i] = generateLodPoint(prevLodOffset, i * 2, j * 2, tSize * 2);
      }
    }
    tSize /= 2;
  }

  absMin = minMaxHeights[lodOffsets[HEIGHT_CULLING_LOD_COUNT - 1]].x;
  absMax = -minMaxHeights[lodOffsets[HEIGHT_CULLING_LOD_COUNT - 1]].y;
}

bool HeightmapHeightCulling::init(HeightmapHandler *handler)
{
  if (handler == NULL)
  {
    return false;
  }

  G_ASSERT(handler->getHeightmapSizeX() == handler->getHeightmapSizeY());

  hmapSize = handler->getHeightmapCellSize() * (float)handler->getHeightmapSizeX();
  chunkSizeInTexels = max<int>(1, (handler->getHeightmapSizeX() + HEIGHT_CULLING_BUFFER_SIZE - 1) / HEIGHT_CULLING_BUFFER_SIZE);
  chunkSize = hmapSize / (float)HEIGHT_CULLING_BUFFER_SIZE;

  arraySize = 0;

  int tSize = HEIGHT_CULLING_BUFFER_SIZE;
  for (int i = 0; i < HEIGHT_CULLING_LOD_COUNT; i++)
  {
    lodChunkSizes[i] = chunkSize * (1 << i);
    lodOffsets[i] = arraySize;
    arraySize += tSize * tSize;
    tSize /= 2;
  }

  clear_and_resize(minMaxHeights, arraySize);

  origin = Point2::xz(handler->getHeightmapOffset());

  updateMinMaxHeights(handler, IBBox2{{0, 0}, {handler->getHeightmapSizeX(), handler->getHeightmapSizeY()}});
  return true;
}

float HeightmapHeightCulling::getLod(float patch_size) const
{
  G_ASSERT(patch_size > 0);
  int lod = ceilf(log2f(max(patch_size, chunkSize) / chunkSize));
  return clamp(lod, 0, (int)HEIGHT_CULLING_LOD_COUNT - 1);
}

void HeightmapHeightCulling::getMinMax(int lod, const Point2 &patch_corner, const real &patch_size, real &hMin, real &hMax) const
{
  if (lod >= HEIGHT_CULLING_LOD_COUNT - 1)
  {
    hMin = absMin - displacementDownwardMaxOffset;
    hMax = absMax + displacementUpwardMaxOffset;
    return;
  }

  int tShift = HEIGHT_CULLING_LOD_COUNT - lod - 1;
  int tSize = 1 << tShift;
  float lodChunkSize = lodChunkSizes[lod];
  int offset = lodOffsets[lod];
  vec4f pt = v_sub(v_ldu_half(&patch_corner.x), v_ldu_half(&origin.x));
  pt = v_perm_xyab(pt, v_add(pt, v_splats(patch_size)));
  vec4i cellStartEnd = v_cvt_floori(v_div(pt, v_splats(lodChunkSize)));
  cellStartEnd = v_maxi(cellStartEnd, v_cast_vec4i(v_zero()));
  cellStartEnd = v_mini(cellStartEnd, v_splatsi(tSize - 1));
  vec4i cellAddresses = v_addi(v_cast_vec4i(v_perm_xxzz(v_cast_vec4f(cellStartEnd))),
    v_cast_vec4i(v_perm_ywyw(v_cast_vec4f(v_slli_n(cellStartEnd, tShift)))));
  cellAddresses = v_addi(cellAddresses, v_splatsi(offset));
  alignas(16) uint32_t address[4]; // 0 == [start.x, start.y], [start.x, end.y], [end.x, start.y],  [end.x, end.y]
  v_sti(address, cellAddresses);

  if (address[0] == address[3])
  {
    // sample 1 'texel'
    Point2 minMax = minMaxHeights[address[0]];
    hMin = minMax.x - displacementDownwardMaxOffset;
    hMax = -minMax.y + displacementUpwardMaxOffset;
    return;
  }

  // sample 4 'texels'
  vec4f top = v_perm_xyab(v_ldu_half(&minMaxHeights[address[0]].x), v_ldu_half(&minMaxHeights[address[2]].x));
  vec4f bottom = v_perm_xyab(v_ldu_half(&minMaxHeights[address[1]].x), v_ldu_half(&minMaxHeights[address[3]].x));

  vec4f result = v_min(top, bottom);
  result = v_min(result, v_rot_2(result));

  Point4 ret;
  v_stu_half(&ret.x, result);
  hMin = ret.x - displacementDownwardMaxOffset;
  hMax = -ret.y + displacementUpwardMaxOffset;
}

void HeightmapHeightCulling::getMinMaxInterpolated(int lod, const Point2 &at, real &hMin, real &hMax) const
{
  if (lod >= HEIGHT_CULLING_LOD_COUNT - 1)
  {
    hMin = absMin - displacementDownwardMaxOffset;
    hMax = absMax + displacementUpwardMaxOffset;
    return;
  }

  int tShift = HEIGHT_CULLING_LOD_COUNT - lod - 1;
  int tSize = 1 << tShift;
  float lodChunkSize = lodChunkSizes[lod];
  int offset = lodOffsets[lod];
  vec4f pt = v_sub(v_ldu_half(&at.x), v_ldu_half(&origin.x));
  vec4f tc = v_div(pt, v_splats(lodChunkSize));
  vec4f tcFloored = v_floor(tc);
  vec4f tcFrac = v_sub(tc, tcFloored);
  vec4i cellStartEnd = v_cvt_vec4i(v_perm_xyab(tcFloored, v_ceil(tc)));
  cellStartEnd = v_maxi(cellStartEnd, v_cast_vec4i(v_zero()));
  cellStartEnd = v_mini(cellStartEnd, v_splatsi(tSize - 1));
  vec4i cellAddresses = v_addi(v_cast_vec4i(v_perm_xxzz(v_cast_vec4f(cellStartEnd))),
    v_cast_vec4i(v_perm_ywyw(v_cast_vec4f(v_slli_n(cellStartEnd, tShift)))));
  cellAddresses = v_addi(cellAddresses, v_splatsi(offset));
  alignas(16) uint32_t address[4]; // 0 == [start.x, start.y], [start.x, end.y], [end.x, start.y],  [end.x, end.y]
  v_sti(address, cellAddresses);

  if (address[0] == address[3])
  {
    // sample 1 'texel'
    Point2 minMax = minMaxHeights[address[0]];
    hMin = minMax.x - displacementDownwardMaxOffset;
    hMax = -minMax.y + displacementUpwardMaxOffset;
    return;
  }

  // sample 4 'texels'
  vec4f top = v_perm_xyab(v_ldu_half(&minMaxHeights[address[0]].x), v_ldu_half(&minMaxHeights[address[2]].x));
  vec4f bottom = v_perm_xyab(v_ldu_half(&minMaxHeights[address[1]].x), v_ldu_half(&minMaxHeights[address[3]].x));

  vec4f result = v_lerp_vec4f(v_splat_y(tcFrac), top, bottom); // minMax, minMax
  result = v_lerp_vec4f(v_splat_x(tcFrac), result, v_perm_zwxy(result));

  Point4 ret;
  v_stu_half(&ret.x, result);
  hMin = ret.x - displacementDownwardMaxOffset;
  hMax = -ret.y + displacementUpwardMaxOffset;
}
