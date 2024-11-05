//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point2.h>
#include <math/dag_bounds2.h>
#include <generic/dag_tab.h>
#include <generic/dag_carray.h>

struct TreeBitmapNode;

struct PhysMap
{
  Point2 worldOffset;
  float invScale;
  float scale;
  uint16_t size; // it only supports rectangular bitmaps
  Tab<int> materials;

  // simple 32x32 tex
  template <typename SizeType, int Width>
  struct BitmapPhysTex
  {
    static constexpr int WIDTH = Width;
    carray<SizeType, Width> pixels;
  };

  struct DecalMesh
  {
    struct MaterialIndices
    {
      int matId;
      int bitmapTexId;
      Tab<uint16_t> indices;
      Tab<uint16_t> tindices;
    };
    BBox2 box;

    Tab<Point2> vertices;
    Tab<Point2> texCoords;
    Tab<MaterialIndices> matIndices;

    DecalMesh() { box.setempty(); }
  };

  Tab<DecalMesh> decals;
  Tab<BitmapPhysTex<uint32_t, 32>> physTextures;
  TreeBitmapNode *parent;

  float gridScale = 1.f;
  float invGridScale = 1.f;
  int gridSz = 0;
  // TODO: simplify to one array, instead of 2 arrays. It requires to rebuild meshes in
  // the processing step, though.
  Tab<Tab<DecalMesh>> gridDecals;

  PhysMap() : worldOffset(0.f, 0.f), scale(1.f), invScale(1.f), size(0), parent(NULL) {}
  ~PhysMap() { clear(); }

  void clear();
  int getMaterialAt(const Point2 &pos) const;
  void fillMaterialsRegion(const BBox2 &box, dag::Span<uint8_t> map, const int width, const int height) const;
};
