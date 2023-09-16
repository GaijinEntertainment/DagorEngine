//
// DaEditor3
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <de3_hmapService.h>
#include <de3_hmapStorage.h>
#include <heightMapLand/dag_hmlGetHeight.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_color.h>

class IDagorEdCustomCollider;


class GenHmapData
{
public:
  bool getHeight(float &ht, float x, float z) const
  {
    if (!hmap)
      return false;
    return get_height_midpoint_heightmap(*this, Point2(x, z), ht, NULL);
  }

  E3DCOLOR getColor(float x, float z)
  {
    return colorMap ? colorMap->getData((x - offset.x) / cellSize, (z - offset.y) / cellSize) : E3DCOLOR(0);
  }

  Color4 getLighting(float x, float z)
  {
    if (!ltMap)
      return Color4(1, 1, 1, 1);

    float lmCellSz = cellSize * hmap->getMapSizeX() / ltMap->getMapSizeX();
    unsigned lt = ltMap->getData((x - offset.x) / lmCellSz, (z - offset.y) / lmCellSz);
    int skyLight = (lt >> 8) & 0xFF;
    int sunLight = lt & 0xFF;
    return color4(skyColor * (skyLight / 255.0) + sunColor * (sunLight / 255.0), 1);
  }

  real getHeightmapCellSize() const { return cellSize; }
  bool getHeightmapCell5Pt(const IPoint2 &c, real &h0, real &hx, real &hy, real &hxy, real &hmid) const
  {
    const HeightMapStorage &heightMap = *hmap;
    if (c.x < 0 || c.y < 0 || c.x + 1 >= heightMap.getMapSizeX() || c.y + 1 >= heightMap.getMapSizeY())
      return false;

    h0 = heightMap.getFinalData(c.x, c.y);
    hx = heightMap.getFinalData(c.x + 1, c.y);
    hy = heightMap.getFinalData(c.x, c.y + 1);
    hxy = heightMap.getFinalData(c.x + 1, c.y + 1);

    hmid = (h0 + hx + hy + hxy) * 0.25f;

    return true;
  }
  Point3 getHeightmapOffset() const { return Point3(offset[0], 0, offset[1]); }
  int getHeightmapSizeX() const { return hmap->getMapSizeX(); }
  int getHeightmapSizeY() const { return hmap->getMapSizeY(); }

public:
  HeightMapStorage *hmap;
  Uint32MapStorage *landClassMap;
  dag::Span<HmapBitLayerDesc> landClassLayers;
  ColorMapStorage *colorMap;
  Uint32MapStorage *ltMap;
  IDagorEdCustomCollider *altCollider;

  Point2 offset;
  float cellSize;
  Color3 sunColor, skyColor;
};
