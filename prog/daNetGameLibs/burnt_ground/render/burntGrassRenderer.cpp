// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "burntGrassRenderer.h"

#include <perfMon/dag_cpuFreq.h>
#include <3d/dag_lockSbuffer.h>
#include <EASTL/numeric.h>
#include <gamePhys/collision/collisionLib.h>
#include <landMesh/biomeQuery.h>
#include <hash/xxh3.h>
#include <perfMon/dag_statDrv.h>

#define BURNT_GRASS_VARS                \
  VAR(burnt_grass_tile_toroidal_params) \
  VAR(burnt_grass_tile_id)              \
  VAR(burnt_grass_fire_source_count)    \
  VAR(burnt_grass_tile)

#define VAR(a) static int a##VarId = -1;
BURNT_GRASS_VARS
#undef VAR

size_t eastl::hash<IPoint2>::operator()(const IPoint2 &p) const
{
  XXH3_state_t state;
  XXH3_64bits_reset(&state);
  XXH3_64bits_update(&state, &p, sizeof(p));
  return XXH3_64bits_digest(&state);
}

BurntGrassRenderer::BurntGrassRenderer() :
  gpuTileIndices(BURNT_GRASS_INDEX_MAP_RESOLUTION * BURNT_GRASS_INDEX_MAP_RESOLUTION, BURNT_GRASS_INVALID_GPU_TILE_INDEX),
  tilesToGenerateTable(BURNT_GRASS_INDEX_MAP_RESOLUTION * BURNT_GRASS_INDEX_MAP_RESOLUTION, false)
{
#define VAR(a) a##VarId = get_shader_variable_id(#a);
  BURNT_GRASS_VARS
#undef VAR

  freeTextureSlices.resize(GPU_TEXTURE_TILES);
  eastl::generate(freeTextureSlices.begin(), freeTextureSlices.end(), [i = GPU_TEXTURE_TILES]() mutable { return --i; });
  tilesCoordsToGenerate.reserve(tilesToGenerateTable.size());

  clearTileShader = new_compute_shader("clear_burnt_grass_tile");
  generateTileShader = new_compute_shader("generate_burnt_grass_tile");

  constexpr int INDEX_BUFFER_SIZE = BURNT_GRASS_INDEX_MAP_RESOLUTION * BURNT_GRASS_INDEX_MAP_RESOLUTION;

  burntTiles = dag::buffers::create_ua_sr_structured(sizeof(uint32_t),
    BURNT_GRASS_TILE_RESOLUTION * BURNT_GRASS_TILE_RESOLUTION * GPU_TEXTURE_TILES, "burnt_grass_tiles", d3d::buffers::Init::Zero);
  burntTiles.setVar();
  burntTileIndices = dag::buffers::create_persistent_sr_structured(sizeof(uint32_t), INDEX_BUFFER_SIZE, "burnt_grass_tiles_indices",
    d3d::buffers::Init::Zero);
  burntTileIndices.setVar();

  fireSourcesBuf = dag::buffers::create_one_frame_sr_structured(sizeof(GpuFireSource), BURNT_GRASS_FIRE_GENERATOR_MAX_ITEM_COUNT,
    "burnt_grass_fire_sources_buf");
  fireSourcesBuf.setVar();

  if (auto indices = lock_sbuffer<uint32_t>(burntTileIndices.getBuf(), 0, INDEX_BUFFER_SIZE, VBLOCK_WRITEONLY))
    for (int j = 0; j < INDEX_BUFFER_SIZE; ++j)
      indices[j] = BURNT_GRASS_INVALID_GPU_TILE_INDEX;
}

BurntGrassRenderer::~BurntGrassRenderer() = default;

void BurntGrassRenderer::setAllowedBiomeGroups(const ecs::StringList &biome_groups)
{
  allowedBiomeGroupIds.clear();
  for (const auto &biomeGroupName : biome_groups)
  {
    const auto groupId = biome_query::get_biome_group_id(biomeGroupName.c_str());
    if (groupId >= 0)
      allowedBiomeGroupIds.push_back(groupId);
  }
}


void BurntGrassRenderer::onBurnGrass(const FireOnGroundEvent &fire_event)
{
  bool hadAllowedBiome = false;
  for (int i = 0; i < fire_event.biomeGroupsIds.size(); ++i)
    if (fire_event.biomeGroupWeights[i] > 0 && eastl::find(allowedBiomeGroupIds.begin(), allowedBiomeGroupIds.end(),
                                                 fire_event.biomeGroupsIds[i]) != allowedBiomeGroupIds.end())
      hadAllowedBiome = true;
  if (!hadAllowedBiome)
    return;

  float placementTime = ::get_shader_global_time();
  if (!fire_event.animated)
    placementTime -= 1000;
  FireSource fireSource = {Point4::xyzV(fire_event.position, fire_event.adjustedRadius), placementTime};
  IPoint2 minTile = getMinGpuTile(fireSource);
  IPoint2 maxTile = getMaxGpuTile(fireSource);
  for (int y = minTile.y; y <= maxTile.y; ++y)
    for (int x = minTile.x; x <= maxTile.x; ++x)
    {
      auto &cell = cells[IPoint2(x, y)];
      if (cell.size() < MAX_SOURCES_PER_CELL)
        cell.push_back(fireSource);
    }

  if (maxTile.x >= previousEyeGpuTileCoord.x - MIN_CORNER_TILE_OFFSET &&
      minTile.x <= previousEyeGpuTileCoord.x + MAX_CORNER_TILE_OFFSET &&
      maxTile.y >= previousEyeGpuTileCoord.y - MIN_CORNER_TILE_OFFSET &&
      minTile.y <= previousEyeGpuTileCoord.y + MAX_CORNER_TILE_OFFSET)
    fireSourcesToAppend.push_back(fireSource);
}

bool BurntGrassRenderer::hasAnyFire() const { return !cells.empty(); }

bool BurntGrassRenderer::prepare(const Point2 &eye_pos_xz)
{
  TIME_PROFILE(prepare_burnt_grass);
  IPoint2 eyeGpuTileCoords = getGpuTile(eye_pos_xz);
  bool hadNoFreeTextures = freeTextureSlices.empty();
  updateEyePosition(eyeGpuTileCoords);
  if (hadNoFreeTextures && !freeTextureSlices.empty())
  {
    // While there are no free texture slices, generation queue is emptied out. Need to fill it back up
    tilesCoordsToGenerate.clear();
    for (int y = previousEyeGpuTileCoord.x - MIN_CORNER_TILE_OFFSET; y <= previousEyeGpuTileCoord.x + MAX_CORNER_TILE_OFFSET; ++y)
      for (int x = previousEyeGpuTileCoord.x - MIN_CORNER_TILE_OFFSET; x <= previousEyeGpuTileCoord.x + MAX_CORNER_TILE_OFFSET; ++x)
        if (tilesToGenerateTable[getToroidalIndex(IPoint2(x, y))])
          tilesCoordsToGenerate.push_back(IPoint2(x, y));
  }
  appendNewFireSources();
  generateNewTiles();

  ShaderGlobal::set_int4(burnt_grass_tile_toroidal_paramsVarId, previousEyeGpuTileCoord.x, previousEyeGpuTileCoord.y,
    previousEyeToroidalCoords.x, previousEyeToroidalCoords.y);

  if (needsToUploadNewIndices)
  {
    burntTileIndices->updateData(0, data_size(gpuTileIndices), gpuTileIndices.data(), VBLOCK_WRITEONLY);
    needsToUploadNewIndices = false;
  }
  return freeTextureSlices.size() < GPU_TEXTURE_TILES;
}

IPoint2 BurntGrassRenderer::getToroidalCoords(const IPoint2 &tile_coords) const
{
  auto coords = (tile_coords - previousEyeGpuTileCoord + previousEyeToroidalCoords) % BURNT_GRASS_INDEX_MAP_RESOLUTION;
  if (coords.x < 0)
    coords.x += BURNT_GRASS_INDEX_MAP_RESOLUTION;
  if (coords.y < 0)
    coords.y += BURNT_GRASS_INDEX_MAP_RESOLUTION;
  return coords;
}

IPoint2 BurntGrassRenderer::getGpuTile(const Point2 &world_xz) const
{
  return IPoint2(floor(world_xz.x / BURNT_GRASS_TILE_SIZE), floor(world_xz.y / BURNT_GRASS_TILE_SIZE));
}

IPoint2 BurntGrassRenderer::getMinGpuTile(const FireSource &fire) const
{
  return getGpuTile(Point2(fire.sphere.x - fire.sphere.w, fire.sphere.z - fire.sphere.w));
}
IPoint2 BurntGrassRenderer::getMaxGpuTile(const FireSource &fire) const
{
  return getGpuTile(Point2(fire.sphere.x + fire.sphere.w, fire.sphere.z + fire.sphere.w));
}

bool BurntGrassRenderer::isInRange(const IPoint2 &tile_coords) const
{
  return tile_coords.x >= previousEyeGpuTileCoord.x - MIN_CORNER_TILE_OFFSET &&
         tile_coords.x <= previousEyeGpuTileCoord.x + MAX_CORNER_TILE_OFFSET &&
         tile_coords.y >= previousEyeGpuTileCoord.y - MIN_CORNER_TILE_OFFSET &&
         tile_coords.y <= previousEyeGpuTileCoord.y + MAX_CORNER_TILE_OFFSET;
}

int BurntGrassRenderer::getToroidalIndex(const IPoint2 &tile_coords) const
{
  auto coords = getToroidalCoords(tile_coords);
  return coords.x + coords.y * BURNT_GRASS_INDEX_MAP_RESOLUTION;
}

void BurntGrassRenderer::updateEyePosition(const IPoint2 &eye_gpu_tile_coord)
{
  if (eye_gpu_tile_coord == previousEyeGpuTileCoord)
    return;

  auto diff = eye_gpu_tile_coord - previousEyeGpuTileCoord;
  if (::abs(diff.x) >= BURNT_GRASS_INDEX_MAP_RESOLUTION || ::abs(diff.y) >= BURNT_GRASS_INDEX_MAP_RESOLUTION)
  {
    // Camera moved too much, reset everything
    eastl::fill(gpuTileIndices.begin(), gpuTileIndices.end(), BURNT_GRASS_INVALID_GPU_TILE_INDEX);
    needsToUploadNewIndices = true;
    freeTextureSlices.resize(GPU_TEXTURE_TILES);
    eastl::generate(freeTextureSlices.begin(), freeTextureSlices.end(), [i = GPU_TEXTURE_TILES]() mutable { return --i; });
    previousEyeToroidalCoords = IPoint2(0, 0);
    previousEyeGpuTileCoord = eye_gpu_tile_coord;
    eastl::fill(tilesToGenerateTable.begin(), tilesToGenerateTable.end(), true);
    tilesCoordsToGenerate.clear();
    for (int y = previousEyeGpuTileCoord.y - MIN_CORNER_TILE_OFFSET; y <= previousEyeGpuTileCoord.y + MAX_CORNER_TILE_OFFSET; ++y)
      for (int x = previousEyeGpuTileCoord.x - MIN_CORNER_TILE_OFFSET; x <= previousEyeGpuTileCoord.x + MAX_CORNER_TILE_OFFSET; ++x)
        tilesCoordsToGenerate.push_back(IPoint2(x, y));
    return;
  }

  // These are exclusive ranges: [first, last)
  const IPoint2 previousXRange =
    IPoint2(previousEyeGpuTileCoord.x - MIN_CORNER_TILE_OFFSET, previousEyeGpuTileCoord.x + MAX_CORNER_TILE_OFFSET + 1);
  const IPoint2 currentXRange =
    IPoint2(eye_gpu_tile_coord.x - MIN_CORNER_TILE_OFFSET, eye_gpu_tile_coord.x + MAX_CORNER_TILE_OFFSET + 1);
  const IPoint2 previousYRange =
    IPoint2(previousEyeGpuTileCoord.y - MIN_CORNER_TILE_OFFSET, previousEyeGpuTileCoord.y + MAX_CORNER_TILE_OFFSET + 1);
  const IPoint2 currentYRange =
    IPoint2(eye_gpu_tile_coord.y - MIN_CORNER_TILE_OFFSET, eye_gpu_tile_coord.y + MAX_CORNER_TILE_OFFSET + 1);

  const IPoint2 oldXRange = eye_gpu_tile_coord.x >= previousEyeGpuTileCoord.x ? IPoint2(previousXRange.x, currentXRange.x)
                                                                              : IPoint2(currentXRange.y, previousXRange.y);
  const IPoint2 oldYRange = eye_gpu_tile_coord.y >= previousEyeGpuTileCoord.y ? IPoint2(previousYRange.x, currentYRange.x)
                                                                              : IPoint2(currentYRange.y, previousYRange.y);
  const IPoint2 newXRange = eye_gpu_tile_coord.x >= previousEyeGpuTileCoord.x ? IPoint2(previousXRange.y, currentXRange.y)
                                                                              : IPoint2(currentXRange.x, previousXRange.x);
  const IPoint2 newYRange = eye_gpu_tile_coord.y >= previousEyeGpuTileCoord.y ? IPoint2(previousYRange.y, currentYRange.y)
                                                                              : IPoint2(currentYRange.x, previousYRange.x);


  // Release texture slices from areas that are too far away now
  if (oldXRange.y > oldXRange.x)
  {
    for (int y = previousYRange.x; y < previousYRange.y; ++y)
    {
      for (int x = oldXRange.x; x < oldXRange.y; ++x)
      {
        int toroidalIndex = getToroidalIndex(IPoint2(x, y));

        // No need to also remove from update queue, it will be ignored
        tilesToGenerateTable[toroidalIndex] = false;

        auto &index = gpuTileIndices[toroidalIndex];
        if (index != BURNT_GRASS_INVALID_GPU_TILE_INDEX)
        {
          freeTextureSlices.push_back(index);
          index = BURNT_GRASS_INVALID_GPU_TILE_INDEX;
          needsToUploadNewIndices = true;
        }
      }
    }
  }
  for (int y = oldYRange.x; y < oldYRange.y; ++y)
  {
    for (int x = previousXRange.x; x < previousXRange.y; ++x)
    {
      int toroidalIndex = getToroidalIndex(IPoint2(x, y));

      // No need to also remove from update queue, it will be ignored
      tilesToGenerateTable[toroidalIndex] = false;

      auto &index = gpuTileIndices[toroidalIndex];
      if (index != BURNT_GRASS_INVALID_GPU_TILE_INDEX)
      {
        freeTextureSlices.push_back(index);
        index = BURNT_GRASS_INVALID_GPU_TILE_INDEX;
        needsToUploadNewIndices = true;
      }
    }
  }

  // Don't reorder these two, getToroidalCoords uses the previous gpu tile coord
  previousEyeToroidalCoords = getToroidalCoords(eye_gpu_tile_coord);
  previousEyeGpuTileCoord = eye_gpu_tile_coord;

  // Schedule preparation of tiles that became visible
  if (newXRange.y > newXRange.x)
    for (int y = currentYRange.x; y < currentYRange.y; ++y)
      for (int x = newXRange.x; x < newXRange.y; ++x)
      {
        int toroidalIndex = getToroidalIndex(IPoint2(x, y));
        if (!eastl::exchange(tilesToGenerateTable[toroidalIndex], true))
          tilesCoordsToGenerate.push_back(IPoint2(x, y));
        G_ASSERT(gpuTileIndices[toroidalIndex] == BURNT_GRASS_INVALID_GPU_TILE_INDEX);
      }
  for (int y = newYRange.x; y < newYRange.y; ++y)
    for (int x = currentXRange.x; x < currentXRange.y; ++x)
    {
      int toroidalIndex = getToroidalIndex(IPoint2(x, y));
      if (!eastl::exchange(tilesToGenerateTable[toroidalIndex], true))
        tilesCoordsToGenerate.push_back(IPoint2(x, y));
      G_ASSERT(gpuTileIndices[toroidalIndex] == BURNT_GRASS_INVALID_GPU_TILE_INDEX);
    }
}

void BurntGrassRenderer::generateNewTiles()
{
  int canGenerateCount = MAX_TILES_GENERATED_PER_FRAME;
  while (canGenerateCount > 0 && !tilesCoordsToGenerate.empty() && !freeTextureSlices.empty())
  {
    auto tileCoords = tilesCoordsToGenerate.back();
    tilesCoordsToGenerate.pop_back();
    if (!isInRange(tileCoords))
      continue;
    int toroidalIndex = getToroidalIndex(tileCoords);
    if (!tilesToGenerateTable[toroidalIndex])
      continue;
    tilesToGenerateTable[toroidalIndex] = false;
    auto cellItr = cells.find(tileCoords);
    if (cellItr == cells.end() || cellItr->second.empty())
      continue;
    int texIndex = freeTextureSlices.back();
    freeTextureSlices.pop_back();
    G_ASSERT(gpuTileIndices[toroidalIndex] == BURNT_GRASS_INVALID_GPU_TILE_INDEX);
    gpuTileIndices[toroidalIndex] = texIndex;
    needsToUploadNewIndices = true;
    generateNewTile(tileCoords, texIndex);
    canGenerateCount--;
  }
  if (freeTextureSlices.empty())
  {
    LOGERR_ONCE("All burnt grass texture slices have been used up. Some fire sources won't cause the grass to burn...");
    tilesCoordsToGenerate.clear(); // later these are added back
  }
  else
  {
    tilesCoordsToGenerate.erase(eastl::remove_if(tilesCoordsToGenerate.begin(), tilesCoordsToGenerate.end(),
                                  [this](const IPoint2 &tile_coords) { return !isInRange(tile_coords); }),
      tilesCoordsToGenerate.end());
  }
}

void BurntGrassRenderer::generateNewTile(const IPoint2 &tile_coords, uint32_t texture_slice_index)
{
  clearTile(texture_slice_index);
  Point2 from = Point2(BURNT_GRASS_TILE_SIZE * tile_coords.x, BURNT_GRASS_TILE_SIZE * tile_coords.y);
  auto cellItr = cells.find(tile_coords);
  if (cellItr == cells.end() || cellItr->second.empty())
    return;
  renderFireSourcesToTile(from, texture_slice_index, cellItr->second.data(), cellItr->second.size());
}

void BurntGrassRenderer::appendNewFireSources()
{
  int offset = 0;
  while (offset < fireSourcesToAppend.size())
  {
    const auto &fireSource = fireSourcesToAppend[offset];
    const IPoint2 minTile = getGpuTile(Point2(fireSource.sphere.x - fireSource.sphere.w, fireSource.sphere.z - fireSource.sphere.w));
    const IPoint2 maxTile = getGpuTile(Point2(fireSource.sphere.x + fireSource.sphere.w, fireSource.sphere.z + fireSource.sphere.w));
    int count = 1;
    while (offset + count < fireSourcesToAppend.size() && minTile == getMinGpuTile(fireSourcesToAppend[offset + count]) &&
           maxTile == getMaxGpuTile(fireSourcesToAppend[offset + count]))
      count++;
    for (int y = minTile.y; y <= maxTile.y; ++y)
    {
      for (int x = minTile.x; x <= maxTile.x; ++x)
      {
        int toroidalIndex = getToroidalIndex(IPoint2(x, y));
        if (gpuTileIndices[toroidalIndex] == BURNT_GRASS_INVALID_GPU_TILE_INDEX)
        {
          if (tilesToGenerateTable[toroidalIndex])
            continue;
          tilesToGenerateTable[toroidalIndex] = true;
          tilesCoordsToGenerate.push_back(IPoint2(x, y));
        }
        else
        {
          Point2 corner = Point2(BURNT_GRASS_TILE_SIZE * x, BURNT_GRASS_TILE_SIZE * y);
          renderFireSourcesToTile(corner, gpuTileIndices[toroidalIndex], &fireSourcesToAppend[offset], count);
        }
      }
    }
    offset += count;
  }
  fireSourcesToAppend.clear();
}

void BurntGrassRenderer::clearTile(uint32_t texture_slice_index)
{
  ShaderGlobal::set_int(burnt_grass_tile_idVarId, texture_slice_index);
  d3d::resource_barrier({burntTiles.getBuf(), RB_RW_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});
  clearTileShader->dispatchThreads(BURNT_GRASS_TILE_RESOLUTION, BURNT_GRASS_TILE_RESOLUTION, 1);
}

void BurntGrassRenderer::renderFireSourcesToTile(
  const Point2 &tile_corner, uint32_t texture_slice_index, const FireSource *sources, uint32_t fire_source_count)
{
  if (fire_source_count == 0)
    return;

  ShaderGlobal::set_int(burnt_grass_tile_idVarId, texture_slice_index);
  ShaderGlobal::set_float4(burnt_grass_tileVarId, tile_corner.x, tile_corner.y, 0, 0);

  d3d::resource_barrier({burntTiles.getBuf(), RB_RW_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});

  for (int i = 0; i < fire_source_count; i += BURNT_GRASS_FIRE_GENERATOR_MAX_ITEM_COUNT)
  {
    int offset = i;
    int count = std::min(BURNT_GRASS_FIRE_GENERATOR_MAX_ITEM_COUNT, static_cast<int>(fire_source_count - offset));
    ShaderGlobal::set_int(burnt_grass_fire_source_countVarId, count);

    if (auto data = lock_sbuffer<GpuFireSource>(fireSourcesBuf.getBuf(), 0, count, VBLOCK_WRITEONLY | VBLOCK_DISCARD))
    {
      for (int j = 0; j < count; ++j)
      {
        data[j].pos_radius = sources[offset + j].sphere;
        data[j].time = sources[offset + j].startTime;
      }
    }
    else
    {
      LOGERR_ONCE("Couldn't upload data for fire sources");
      break;
    }

    generateTileShader->dispatchThreads(BURNT_GRASS_TILE_RESOLUTION, BURNT_GRASS_TILE_RESOLUTION, count);
  }

  d3d::resource_barrier({burntTiles.getBuf(), RB_RO_SRV | RB_STAGE_VERTEX});
}

void BurntGrassRenderer::afterDeviceReset()
{
  previousEyeGpuTileCoord = IPoint2(-100000, -100000);
  previousEyeToroidalCoords = IPoint2(0, 0);
}