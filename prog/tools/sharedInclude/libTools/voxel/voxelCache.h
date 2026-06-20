// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>
#include <math/dag_Point3.h>
#include <math/integer/dag_IPoint3.h>
#include <math/integer/dag_IBBox3.h>
#include <generic/dag_carray.h>
#include <generic/dag_patchTab.h>

namespace defaults
{
static constexpr float projScale = 1080; // diameter in pixels of a disk of radius R when viewed from distance R
static constexpr float triangleThreshold = 70;
static constexpr int minMapSize = 4;
static constexpr int maxMapSize = 256;
static constexpr float voxelSize = -1;
static constexpr float minVoxelSize = 0.001f;
static constexpr int lodToBake = 0;
static constexpr bool adjustMeshLodRanges = true;
} // namespace defaults

namespace voxelcache
{

static constexpr char VOXEL_CACHE_NAME[] = "voxCache.bin";

static constexpr uint32_t VERSION = 0x26041000;

struct Pixel
{
  uint8_t albedoB;
  uint8_t albedoG;
  uint8_t albedoR;
  uint8_t alpha;

  uint8_t normalX;
  uint8_t normalY;
  uint8_t normalZ;
  uint8_t coloring;

  uint8_t smoothness;
  uint8_t reflectance;
  uint8_t metalness;
  uint8_t translucency;
};

struct BakedFace
{
  uint32_t nx, ny;
  PatchableTab<uint32_t> pixelIndex;
  PatchableTab<float> depth;
  PatchableTab<Pixel> pixel;

  void patch(void *base)
  {
    pixelIndex.patch(base);
    depth.patch(base);
    pixel.patch(base);
  }
};

struct CacheDump
{
  double voxelSize;
  double targetVoxelSize;
  IPoint3 mapSize;
  Point3 boxMin;
  IBBox3 ibbox;
  carray<PatchablePtr<BakedFace>, 6> surface;
  PatchableTab<uint32_t> insideBits;

  static constexpr carray<uint32_t, 6> FACE_X = {2, 2, 2, 2, 0, 0};
  static constexpr carray<uint32_t, 6> FACE_Y = {1, 1, 0, 0, 1, 1};
  static constexpr carray<uint32_t, 6> FACE_Z = {0, 0, 1, 1, 2, 2};

  uint8_t *dumpBase() const { return (uint8_t *)this; }

  void patch()
  {
    auto base = dumpBase();
    for (auto &s : surface)
    {
      s.patch(base);
      s->patch(base);
    }
    insideBits.patch(base);
  }
};

} // namespace voxelcache
