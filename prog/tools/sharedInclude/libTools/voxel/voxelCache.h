// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>
#include <math/dag_Point3.h>
#include <math/integer/dag_IPoint3.h>
#include <math/integer/dag_IBBox3.h>
#include <generic/dag_carray.h>
#include <generic/dag_patchTab.h>
#include <ioSys/dag_dataBlock.h>
#include <libTools/util/appDirRelativePath.h>

namespace defaults
{
static constexpr float targetFov = 70; // degrees
static constexpr float targetResolution = 1080;
static constexpr float triangleThreshold = 70;
static constexpr int minMapSize = 4;
static constexpr int maxMapSize = 256;
static constexpr float voxelSize = -1;
static constexpr float minVoxelSize = 0.001f;
static constexpr int lodToBake = 0;
static constexpr bool adjustMeshLodRanges = true;
static constexpr float range = 0;        // maximum range, <=0 - automatic from billboard impostor or voxel size
static constexpr int maxTotalLods = 6;   // still limited by maximum allowed as well
static constexpr int maxVoxelLods = 999; // no limit by default
} // namespace defaults

namespace voxelcache
{

inline String make_cache_fname(const char *cache_path, const char *asset_name)
{
  return String(0, "%s/%s.voxcache.bin", cache_path, asset_name);
}

inline String get_cache_path(const DataBlock &appblk)
{
  const DataBlock &expblk = *appblk.getBlockByNameEx("assets")->getBlockByNameEx("export");
  String cacheBase;
  cacheBase.setStrCat(make_eff_app_relative_path(expblk.getStr("cache", "develop/.cache"), true), "voxels~baked");
  dd_simplify_fname_c(cacheBase);
  cacheBase.resize(strlen(cacheBase) + 1);
  return cacheBase;
}

static constexpr uint32_t VERSION = 0x26071000;

inline float compute_proj_scale(float fov_degrees, float resolution) { return resolution * 0.5f / tanf(DegToRad(fov_degrees) * 0.5f); }

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

  uint8_t palette;
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
  PatchableTab<uint16_t> palette;
  carray<PatchablePtr<BakedFace>, 6> surface;
  PatchableTab<uint32_t> insideBits;

  static constexpr carray<uint32_t, 6> FACE_X = {2, 2, 2, 2, 0, 0};
  static constexpr carray<uint32_t, 6> FACE_Y = {1, 1, 0, 0, 1, 1};
  static constexpr carray<uint32_t, 6> FACE_Z = {0, 0, 1, 1, 2, 2};

  uint8_t *dumpBase() const { return (uint8_t *)this; }

  void patch()
  {
    auto base = dumpBase();
    palette.patch(base);
    for (auto &s : surface)
    {
      s.patch(base);
      s->patch(base);
    }
    insideBits.patch(base);
  }
};

} // namespace voxelcache
