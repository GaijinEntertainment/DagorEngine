// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <math/dag_Point4.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_computeShaders.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <math/dag_hlsl_floatx.h>
#include <dag/dag_vector.h>
#include <generic/dag_relocatableFixedVector.h>
#include <burnt_ground/render/burntGroundEvents.h>


#include "burnt_ground/shaders/burnt_grass.hlsli"

class ComputeShaderElement;

namespace eastl
{
template <typename T>
struct hash;
template <>
struct hash<IPoint2>
{
  size_t operator()(const IPoint2 &p) const;
};
} // namespace eastl

class BurntGrassRenderer
{
public:
  BurntGrassRenderer();
  ~BurntGrassRenderer();

  BurntGrassRenderer(const BurntGrassRenderer &) = delete;
  BurntGrassRenderer &operator=(const BurntGrassRenderer &) = delete;

  void setAllowedBiomeGroups(const ecs::StringList &biome_groups);

  void onBurnGrass(const FireOnGroundEvent &fire_event);

  [[nodiscard]] bool hasAnyFire() const;

  bool prepare(const Point2 &eye_pos_xz);

  void afterDeviceReset();

private:
  // min corner tile coord = center tile coord - int2(MIN_CORNER_TILE_OFFSET, MIN_CORNER_TILE_OFFSET)
  // max corner tile coord = center tile coord + int2(MAX_CORNER_TILE_OFFSET, MAX_CORNER_TILE_OFFSET)
  static constexpr int MIN_CORNER_TILE_OFFSET = BURNT_GRASS_INDEX_MAP_RESOLUTION / 2;
  static constexpr int MAX_CORNER_TILE_OFFSET = MIN_CORNER_TILE_OFFSET - 1;
  static constexpr int GPU_TEXTURE_TILES = 512;
  static constexpr int MAX_TILES_GENERATED_PER_FRAME = 8;
  static constexpr int MAX_SOURCES_PER_CELL = 256;

  struct FireSource
  {
    Point4 sphere;
    float startTime;
  };

  struct PendingFireSource
  {
    int biomeQueryId = -1;
    Point4 sphere;
  };

  ska::flat_hash_map<IPoint2, dag::Vector<FireSource>> cells;

  Ptr<ComputeShaderElement> clearTileShader;
  Ptr<ComputeShaderElement> generateTileShader;
  UniqueBufHolder burntTiles;
  UniqueBufHolder burntTileIndices;
  UniqueBufHolder fireSourcesBuf;

  // Vector is used with linear search, because it will contain a very few elements only
  dag::RelocatableFixedVector<int, 4> allowedBiomeGroupIds;

  IPoint2 previousEyeGpuTileCoord = IPoint2(-100000, -100000);
  IPoint2 previousEyeToroidalCoords = IPoint2(0, 0);
  dag::Vector<uint32_t> freeTextureSlices;
  dag::Vector<uint32_t> gpuTileIndices;
  dag::Vector<bool> tilesToGenerateTable;
  dag::Vector<IPoint2> tilesCoordsToGenerate;
  // These are fire sources that appear on existing tiles, so they need to be added to the tile
  dag::Vector<FireSource> fireSourcesToAppend;
  bool needsToUploadNewIndices = true;


  void updateEyePosition(const IPoint2 &eye_gpu_tile_coord);
  void generateNewTiles();
  void appendNewFireSources();

  [[nodiscard]] IPoint2 getToroidalCoords(const IPoint2 &tile_coords) const;
  [[nodiscard]] bool isInRange(const IPoint2 &tile_coords) const;
  [[nodiscard]] int getToroidalIndex(const IPoint2 &tile_coords) const;
  [[nodiscard]] IPoint2 getGpuTile(const Point2 &world_xz) const;
  [[nodiscard]] IPoint2 getMinGpuTile(const FireSource &fire) const;
  [[nodiscard]] IPoint2 getMaxGpuTile(const FireSource &fire) const;
  void generateNewTile(const IPoint2 &tile_coords, uint32_t texture_slice_index);
  void clearTile(uint32_t texture_slice_index);
  void renderFireSourcesToTile(
    const Point2 &tile_corner, uint32_t texture_slice_index, const FireSource *sources, uint32_t fire_source_count);
};

ECS_DECLARE_BOXED_TYPE(BurntGrassRenderer);