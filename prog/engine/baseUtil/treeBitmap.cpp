// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <generic/dag_span.h>
#include <generic/dag_staticTab.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint4.h>
#include <math/dag_mathUtils.h>

#include <ioSys/dag_genIo.h>

#include <util/dag_treeBitmap.h>

const int finalMipSize = 16;

enum
{
  RES_ONE_COLOR = 0,
  RES_MAPDATA,
  RES_MAPDATA2X2,
  RES_MAPDATA4X4,
  RES_MAPDATA8X8,
  RES_SUBNODES
};

template <typename SizeType>
static int test_fits(dag::ConstSpan<uint8_t> data, const IPoint2 &data_dimensions, const IPoint4 &rect,
  StaticTab<uint8_t, 4> *bytes_used, int *num_used)
{
  bool fits = true;
  for (int y = rect.y; y < rect.w && fits; y += sizeof(SizeType))
    for (int x = rect.x; x < rect.z && fits; x += sizeof(SizeType))
    {
      SizeType curColor;
      memset(&curColor, data[y * data_dimensions.x + x], sizeof(SizeType));
      for (int i = 0; i < sizeof(SizeType) && fits; ++i)
      {
        fits &= (*((SizeType *)&data[(y + i) * data_dimensions.x + x]) == curColor);
        if (!bytes_used || !num_used)
          continue;
        if (find_value_idx(*bytes_used, uint8_t(curColor & 0xff)) < 0)
        {
          if (*num_used < 4)
            bytes_used->push_back(uint8_t(curColor & 0xff));
          (*num_used)++;
        }
      }
    }
  return fits;
}

static bool check_all_same(dag::ConstSpan<uint8_t> data, const IPoint2 &data_dimensions, const IPoint4 &rect)
{
  uint8_t defaultFillColor = data[rect.y * data_dimensions.x + rect.x];
  uint32_t fillPattern = defaultFillColor | (defaultFillColor << 8) | (defaultFillColor << 16) | (defaultFillColor << 24);

  bool allSame = true;
  for (int y = rect.y; y < rect.w && allSame; ++y)
    for (int x = rect.x; x < rect.z && allSame; x += 4)
      allSame &= (*((uint32_t *)&data[y * data_dimensions.x + x]) == fillPattern);
  return allSame;
}

static int test_create(dag::ConstSpan<uint8_t> data, const IPoint2 &data_dimensions, const IPoint4 &rect,
  StaticTab<uint8_t, 4> *bytes_used, int *num_used)
{
  if (check_all_same(data, data_dimensions, rect)) // all have same color - we can save it as it is
    return RES_ONE_COLOR;

  int width = rect.z - rect.x;
  int height = rect.w - rect.y;
  bool finalMip = width == finalMipSize && height == finalMipSize;
  if (finalMip)
    return RES_MAPDATA;

  if (width == finalMipSize * 8 && test_fits<uint64_t>(data, data_dimensions, rect, bytes_used, num_used))
    return RES_MAPDATA8X8;
  else if (width == finalMipSize * 4 && test_fits<uint32_t>(data, data_dimensions, rect, bytes_used, num_used))
    return RES_MAPDATA4X4;
  else if (width == finalMipSize * 2 && test_fits<uint16_t>(data, data_dimensions, rect, bytes_used, num_used))
    return RES_MAPDATA2X2;
  return RES_SUBNODES;
}

static void *save_map_data(dag::ConstSpan<uint8_t> data, const IPoint2 &data_dimensions, const IPoint4 &rect,
  const StaticTab<uint8_t, 4> &bytes_used, int num_used, int width, int height, int skip, uint8_t &fill_color)
{
  uint8_t *mapData = NULL;
  if (num_used == 2)
  {
    fill_color |= TreeBitmapNode::MAPDATA_1BIT;
    mapData = new uint8_t[width * height / 8 + 2];
    memset(mapData, 0, width * height / 8 + 2);
    mapData[0] = bytes_used[0];
    mapData[1] = bytes_used[1];
    int curBit = 16;
    for (int y = rect.y; y < rect.w; y += skip)
      for (int x = rect.x; x < rect.z; x += skip, ++curBit)
      {
        int bitValue = (data[y * data_dimensions.x + x] == mapData[1]) ? 1 : 0;
        mapData[curBit >> 3] |= (bitValue << (curBit & 7));
      }
  }
  else if (num_used < 5)
  {
    fill_color |= TreeBitmapNode::MAPDATA_2BIT;
    mapData = new uint8_t[width * height / 4 + 4];
    memset(mapData, 0, width * height / 4 + 4);
    for (int i = 0; i < bytes_used.size(); ++i)
      mapData[i] = bytes_used[i];
    int curBit = 32;
    for (int y = rect.y; y < rect.w; y += skip)
      for (int x = rect.x; x < rect.z; x += skip, curBit += 2)
      {
        int val = data[y * data_dimensions.x + x];
        int bitValue = val == mapData[0] ? 0 : val == mapData[1] ? 1 : val == mapData[2] ? 2 : 3;
        mapData[curBit >> 3] |= (bitValue << (curBit & 7));
      }
  }
  else
  {
    mapData = new uint8_t[width * height];
    for (int y = rect.y; y < rect.w; y += skip)
      if (skip == 1) // it's faster to just memcpy
        memcpy(&mapData[(y - rect.y) * width], &data[y * data_dimensions.x + rect.x], width);
      else
        for (int x = rect.x; x < rect.z; x += skip)
          mapData[(y - rect.y) / skip * width + (x - rect.x) / skip] = data[y * data_dimensions.x + rect.x];
  }
  return mapData;
}

TreeBitmapNode::TreeBitmapNode() : fillColor(0), subnodesOrMapdata(NULL) {}

TreeBitmapNode::~TreeBitmapNode()
{
  if (subnodesOrMapdata)
  {
    if (fillColor & HAS_SUBNODES)
      delete[] (TreeBitmapNode *)(subnodesOrMapdata);
    else
      delete[] (uint8_t *)subnodesOrMapdata;
  }
}

void *TreeBitmapNode::generateSubnodes(dag::ConstSpan<uint8_t> data, const IPoint2 &data_dimensions, const IPoint4 &rect, int size,
  int dim, int subnode_shift)
{
  bool allNodesSubdivided = true;
  int subnodeSize = size >> subnode_shift;
  for (int y = 0; y < dim && allNodesSubdivided; ++y)
    for (int x = 0; x < dim && allNodesSubdivided; ++x)
    {
      int subnodeX = rect.x + x * subnodeSize;
      int subnodeY = rect.y + y * subnodeSize;
      IPoint4 newRect = IPoint4(subnodeX, subnodeY, subnodeX + subnodeSize, subnodeY + subnodeSize);
      allNodesSubdivided &= test_create(data, data_dimensions, newRect, NULL, NULL) == RES_SUBNODES;
    }
  if (!allNodesSubdivided)
  {
    TreeBitmapNode *subnodes = new TreeBitmapNode[dim * dim];
    for (int y = 0; y < dim; ++y)
      for (int x = 0; x < dim; ++x)
      {
        int subnodeX = rect.x + x * subnodeSize;
        int subnodeY = rect.y + y * subnodeSize;
        IPoint4 newRect = IPoint4(subnodeX, subnodeY, subnodeX + subnodeSize, subnodeY + subnodeSize);
        subnodes[y * dim + x].create(data, data_dimensions, newRect);
      }
    fillColor = HAS_SUBNODES | (subnode_shift - 1);
    return subnodes;
  }
  return generateSubnodes(data, data_dimensions, rect, size, dim << 1, subnode_shift + 1);
}

uint8_t TreeBitmapNode::get(uint16_t x, uint16_t y, int pix_size) const
{
  if (subnodesOrMapdata)
  {
    if (fillColor & HAS_SUBNODES)
    {
      int subnodeShift = (fillColor & 0x7f) + 1;
      int subnodeDimensions = 1 << subnodeShift;
      dag::Span<TreeBitmapNode> subnodes((TreeBitmapNode *)subnodesOrMapdata, sqr(subnodeDimensions));

      uint16_t xIdx = x * subnodeDimensions / pix_size;
      uint16_t yIdx = y * subnodeDimensions / pix_size;
      G_ASSERT(xIdx < subnodeDimensions);
      G_ASSERT(yIdx < subnodeDimensions);
      int newPixSize = pix_size >> subnodeShift;
      return subnodes[subnodeDimensions * yIdx + xIdx].get(x - xIdx * newPixSize, y - yIdx * newPixSize, newPixSize);
    }
    else
    {
      uint8_t shift = 0;
      if ((fillColor & 0x70) == MAPDATA_PIX2X2)
        shift = 1;
      else if ((fillColor & 0x70) == MAPDATA_PIX4X4)
        shift = 2;
      else if ((fillColor & 0x70) == MAPDATA_PIX8X8)
        shift = 3;
      uint16_t localX = x >> shift;
      uint16_t localY = y >> shift;
      G_ASSERT(localX < finalMipSize);
      G_ASSERT(localY < finalMipSize);

      uint16_t mapIdx;
      uint8_t *mapData = static_cast<uint8_t *>(subnodesOrMapdata);
      if (fillColor & MAPDATA_1BIT)
      {
        int curBit = 16 + localY * finalMipSize + localX;
        mapIdx = (mapData[curBit >> 3] >> (curBit & 7)) & 1;
      }
      else if (fillColor & MAPDATA_2BIT)
      {
        int curBit = 32 + (localY * finalMipSize + localX) * 2;
        mapIdx = (mapData[curBit >> 3] >> (curBit & 7)) & 3;
      }
      else
        mapIdx = finalMipSize * localY + localX;
      return mapData[mapIdx];
    }
  }
  return fillColor;
}

void TreeBitmapNode::gatherPixels(const IPoint4 &rect, dag::Span<uint8_t> out, const int width, const int pix_size) const
{
  if (subnodesOrMapdata)
  {
    if (fillColor & HAS_SUBNODES)
    {
      int subnodeShift = (fillColor & 0x7f) + 1;
      int subnodeDimensions = 1 << subnodeShift;
      dag::Span<TreeBitmapNode> subnodes((TreeBitmapNode *)subnodesOrMapdata, sqr(subnodeDimensions));

      const int newPixSize = pix_size >> subnodeShift;
      for (int j = 0; j < subnodeDimensions; ++j)
        for (int i = 0; i < subnodeDimensions; ++i)
        {
          const uint16_t xOffs = i * newPixSize;
          const uint16_t yOffs = j * newPixSize;
          const IPoint4 subRect(rect.x - xOffs, rect.y - yOffs, min(rect.z - xOffs, newPixSize), min(rect.w - yOffs, newPixSize));
          if (subRect.z <= 0 || subRect.w <= 0 || subRect.x >= newPixSize || subRect.y >= newPixSize)
            continue;
          subnodes[subnodeDimensions * j + i].gatherPixels(subRect, out, width, newPixSize);
        }
    }
    else
    {
      uint8_t shift = 0;
      if ((fillColor & 0x70) == MAPDATA_PIX2X2)
        shift = 1;
      else if ((fillColor & 0x70) == MAPDATA_PIX4X4)
        shift = 2;
      else if ((fillColor & 0x70) == MAPDATA_PIX8X8)
        shift = 3;

      uint8_t subpixSize = 1 << shift;

      uint16_t mapIdx;
      uint8_t *mapData = static_cast<uint8_t *>(subnodesOrMapdata);
      for (int y = 0; y < finalMipSize; ++y)
        for (int x = 0; x < finalMipSize; ++x)
        {
          if (fillColor & MAPDATA_1BIT)
          {
            int curBit = 16 + y * finalMipSize + x;
            mapIdx = (mapData[curBit >> 3] >> (curBit & 7)) & 1;
          }
          else if (fillColor & MAPDATA_2BIT)
          {
            int curBit = 32 + (y * finalMipSize + x) * 2;
            mapIdx = (mapData[curBit >> 3] >> (curBit & 7)) & 3;
          }
          else
            mapIdx = finalMipSize * y + x;
          // figure out what pixels we want to color by it
          uint8_t pixData = mapData[mapIdx];
          int beginX = max(0, -rect.x + x * subpixSize);
          int endX = min(width, -rect.x + (x + 1) * subpixSize);
          if (endX > beginX)
            for (int j = max(0, -rect.y + y * subpixSize); j < min(width, -rect.y + (y + 1) * subpixSize); ++j)
              memset(out.data() + j * width + beginX, pixData, endX - beginX);
        }
    }
  }
  else
  {
    int beginX = max(-rect.x, 0);
    int endX = min(rect.z - rect.x, width);
    if (endX > beginX)
      for (int y = max(-rect.y, 0); y < min(rect.w - rect.y, width); ++y)
        memset(out.data() + y * width + beginX, fillColor, endX - beginX);
  }
}

void TreeBitmapNode::create(dag::ConstSpan<uint8_t> data, const IPoint2 &data_dimensions, const IPoint4 &rect)
{
  subnodesOrMapdata = NULL;
  int width = rect.z - rect.x;
  int height = rect.w - rect.y;
#if DAGOR_DBGLEVEL > 0
  G_ASSERTF(width == height, "width %d != height %d", width, height);
#endif
  int numUsed = 0;
  if (width == 0 && height == 0)
  {
    fillColor = 0;
    return;
  }
  StaticTab<uint8_t, 4> bytesUsed;
  int res = test_create(data, data_dimensions, rect, &bytesUsed, &numUsed);
  int mapDataSkip = 1;
  switch (res)
  {
    case RES_ONE_COLOR: fillColor = data[rect.y * data_dimensions.x + rect.x]; return;
    case RES_MAPDATA:
      for (int y = rect.y; y < rect.w; ++y)
        for (int x = rect.x; x < rect.z; ++x)
          if (find_value_idx(bytesUsed, data[y * data_dimensions.x + x]) < 0)
          {
            if (numUsed < 4)
              bytesUsed.push_back(data[y * data_dimensions.x + x]);
            numUsed++;
          }
      G_ASSERT(numUsed > 1);
      mapDataSkip = 1;
      fillColor = MAPDATA_PIX1X1;
      break;
    case RES_MAPDATA2X2:
      fillColor = MAPDATA_PIX2X2;
      mapDataSkip = 2;
      break;
    case RES_MAPDATA4X4:
      fillColor = MAPDATA_PIX4X4;
      mapDataSkip = 4;
      break;
    case RES_MAPDATA8X8:
      fillColor = MAPDATA_PIX8X8;
      mapDataSkip = 8;
      break;
    case RES_SUBNODES:
      // We need to create subnodes to store information about this megapixel
      subnodesOrMapdata = generateSubnodes(data, data_dimensions, rect, width, 2, 1);
      return;
    default: return; // do nothing shouldn't be here at all
  };
  subnodesOrMapdata =
    save_map_data(data, data_dimensions, rect, bytesUsed, numUsed, finalMipSize, finalMipSize, mapDataSkip, fillColor);
}

void TreeBitmapNode::create(dag::ConstSpan<uint8_t> data, const IPoint2 &data_dimensions)
{
  create(data, data_dimensions, IPoint4(0, 0, data_dimensions.x, data_dimensions.y));
}

void TreeBitmapNode::load(IGenLoad &cb)
{
  fillColor = cb.readIntP<1>();
  uint8_t hasSubnodesOrMapData = cb.readIntP<1>();
  if (!hasSubnodesOrMapData)
  {
    subnodesOrMapdata = NULL;
    return;
  }

  if (fillColor & HAS_SUBNODES)
  {
    int subnodeDimensions = 1 << ((fillColor & 0x7f) + 1);
    TreeBitmapNode *subnodes = new TreeBitmapNode[subnodeDimensions * subnodeDimensions];
    for (int y = 0; y < subnodeDimensions; ++y)
      for (int x = 0; x < subnodeDimensions; ++x)
        subnodes[y * subnodeDimensions + x].load(cb);
    subnodesOrMapdata = subnodes;
  }
  else
  {
    int size = finalMipSize * finalMipSize;
    if (fillColor & MAPDATA_1BIT)
      size = finalMipSize * finalMipSize / 8 + 2;
    else if (fillColor & MAPDATA_2BIT)
      size = finalMipSize * finalMipSize / 4 + 4;
    else
      size = finalMipSize * finalMipSize;
    uint8_t *mapData = new uint8_t[size];
    cb.read(mapData, size);
    subnodesOrMapdata = mapData;
  }
}

#define EXPORT_PULL dll_pull_baseutil_treeBitmap
#include <supp/exportPull.h>
