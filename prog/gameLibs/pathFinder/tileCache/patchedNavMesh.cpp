// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <pathFinder/pathFinder.h>
#include <DetourCommon.h>
#include <math/dag_mathUtils.h>
#include <DetourNavMesh.h>

#include <regExp/regExp.h>

#include <osApiWrappers/dag_direct.h>
#include <rendInst/riexHashMap.h>
#include <rendInst/rendInstGen.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstAccess.h>

#include <pathFinder/tileCache.h>
#include <pathFinder/tileCacheRI.h>
#include <pathFinder/tileCacheUtil.h>

#include <pathFinder/tileRICommon.h>
#include <recastTools/recastNavMeshTile.h>
#include <recastTools/recastBuildEdges.h>
#include <recastTools/recastBuildJumpLinks.h>
#include <gamePhys/collision/collisionLib.h>

#include <osApiWrappers/dag_files.h>
#include <ioSys/dag_dataBlock.h>

#include <util/dag_string.h>

#include <scene/dag_tiledScene.h>

namespace pathfinder
{

PatchedNavMeshFileInfo patchedNavMesh_getFileInfo(const char *file_name)
{
  PatchedNavMeshFileInfo info;

  eastl::unique_ptr<void, decltype(&df_close)> h(df_open(file_name, DF_READ | DF_IGNORE_MISSING), &df_close);

  if (!h)
    return info;

  int fileSize = df_length(h.get());
  if (fileSize < sizeof(uint32_t))
    return info;

  uint32_t totalSize = 0u;
  if (df_read(h.get(), &totalSize, sizeof(uint32_t)) != sizeof(uint32_t))
    return info;

  if (totalSize != fileSize - sizeof(uint32_t))
    return info;

  // the payload size is known from here on; a malformed body below still reports it
  // so the loader reserves the patch storage the load attempt needs
  info.fileSize = totalSize;

  // skip removed obstacles
  {
    uint32_t count = 0u;
    if (df_read(h.get(), &count, sizeof(uint32_t)) != sizeof(uint32_t))
      return info;
    if (df_seek_rel(h.get(), count * sizeof(uint32_t)) != 0)
      return info;
  }

  // skip added obstacles
  {
    uint32_t count = 0u;
    if (df_read(h.get(), &count, sizeof(uint32_t)) != sizeof(uint32_t))
      return info;
    if (df_seek_rel(h.get(), count * sizeof(uint32_t)) != 0)
      return info;
  }

  // read tilecache tiles
  {
    uint32_t count = 0u;
    if (df_read(h.get(), &count, sizeof(uint32_t)) != sizeof(uint32_t))
      return info;

    info.numTcTiles = count;
    if (info.numTcTiles < 0)
      info.numTcTiles = 0;

    for (uint32_t i = 0u; i < count; ++i)
    {
      uint32_t dataSize = 0u;
      if (df_read(h.get(), &dataSize, sizeof(uint32_t)) != sizeof(uint32_t))
        return info;
      if (df_seek_rel(h.get(), dataSize) != 0)
        return info;
    }
  }

  // read navmesh tiles
  {
    uint32_t count = 0u;
    if (df_read(h.get(), &count, sizeof(uint32_t)) != sizeof(uint32_t))
      return info;

    info.numNavTiles = count;
    if (info.numNavTiles < 0)
      info.numNavTiles = 0;

    // These tiles restore into fixed tile array slots (see patchedNavMesh_loadFromFile),
    // so report the highest pinned index for the loader to size the tile array by.
    for (uint32_t i = 0u; i < count; ++i)
    {
      dtTileRef tileRef = 0;
      uint32_t dataSize = 0u;
      if (df_read(h.get(), &tileRef, sizeof(tileRef)) != sizeof(tileRef))
        return info;
      if (df_read(h.get(), &dataSize, sizeof(uint32_t)) != sizeof(uint32_t))
        return info;
      if (df_seek_rel(h.get(), dataSize) != 0)
        return info;
#ifdef DT_POLYREF64
      const int tileIndex = (int)((tileRef >> DT_POLY_BITS) & (((dtPolyRef)1 << DT_TILE_BITS) - 1));
      info.maxNavTileIndex = max(info.maxNavTileIndex, tileIndex);
#endif
    }
  }

  return info;
}

bool patchedNavMesh_loadFromFile(const char *fileName, dtTileCache *tlCache, uint8_t *storageData,
  ska::flat_hash_set<uint32_t> &obstacles)
{
  dtNavMesh *navMesh = getNavMeshPtr();
  eastl::unique_ptr<void, decltype(&df_close)> h(df_open(fileName, DF_READ | DF_IGNORE_MISSING), &df_close);

  if (!h)
    return false;

  uint32_t totalSize = 0u;

  // Step 1: load total size data to the buffer

  if (df_read(h.get(), &totalSize, sizeof(uint32_t)) != sizeof(uint32_t))
    return false;

  if (df_read(h.get(), storageData, totalSize) != totalSize)
    return false;

  uint8_t *navData = storageData;
  int64_t sizeLeft = totalSize;

  // Step 2: load removed obstacles
  {
    if (sizeLeft < 4)
      return false;
    uint32_t count = *(uint32_t *)navData;
    navData += 4;
    sizeLeft -= 4;

    int64_t sz = (int64_t)count * 4;
    if (sizeLeft < sz)
      return false;
    for (uint32_t i = 0u; i < count; ++i)
    {
      uint32_t obstacle = *(const uint32_t *)navData;
      navData += 4;
      sizeLeft -= 4;

      obstacles.erase(obstacle);
    }
  }

  // Step 3: load added obstacles
  {
    if (sizeLeft < 4)
      return false;
    uint32_t count = *(uint32_t *)navData;
    navData += 4;
    sizeLeft -= 4;

    int64_t sz = (int64_t)count * 4;
    if (sizeLeft < sz)
      return false;
    for (uint32_t i = 0u; i < count; ++i)
    {
      uint32_t obstacle = *(const uint32_t *)navData;
      navData += 4;
      sizeLeft -= 4;

      obstacles.insert(obstacle);
    }
  }

  // Step 4: load tile cached tiles
  {
    if (sizeLeft < 4)
      return false;
    uint32_t count = *(uint32_t *)navData;
    navData += 4;
    sizeLeft -= 4;

    uint8_t *tmp = navData;
    int64_t tmpsz = sizeLeft;

    for (uint32_t i = 0u; i < count; ++i)
    {
      if (sizeLeft < 4)
        return false;
      uint32_t dataSize = *(uint32_t *)navData;
      navData += 4;
      sizeLeft -= 4;

      if (dataSize == 0 || sizeLeft < dataSize)
        return false;
      unsigned char *data = (unsigned char *)navData;
      navData += dataSize;
      sizeLeft -= dataSize;

      const dtTileCacheLayerHeader *header = (const dtTileCacheLayerHeader *)data;

      // force remove tiles for all layers at (tx,ty)
      const int tx = header->tx;
      const int ty = header->ty;
      if (tlCache)
      {
        const int maxTiles = 32;
        dtCompressedTileRef tiles[maxTiles];
        const int ntiles = tlCache->getTilesAt(tx, ty, tiles, maxTiles);

        for (int j = 0; j < ntiles; ++j)
        {
          const dtCompressedTile *tile = tlCache->getTileByRef(tiles[j]);
          if (!tile)
            continue;

          dtTileRef tileRef = navMesh->getTileRefAt(tile->header->tx, tile->header->ty, tile->header->tlayer);

          navMesh->removeTile(tileRef, 0, 0);
          tlCache->removeTile(tiles[j], NULL, NULL);
        }
      }
    }

    navData = tmp;
    sizeLeft = tmpsz;

    for (uint32_t i = 0u; i < count; ++i)
    {
      if (sizeLeft < 4)
        return false;
      uint32_t dataSize = *(uint32_t *)navData;
      navData += 4;
      sizeLeft -= 4;

      if (dataSize == 0 || sizeLeft < dataSize)
        return false;
      unsigned char *data = (unsigned char *)navData;
      navData += dataSize;
      sizeLeft -= dataSize;

      if (tlCache)
      {
        tlCache->addTile(data, dataSize, 0, 0);
      }
    }
  }

  // Step 5: load nav mesh tiles
  {
    if (sizeLeft < 4)
      return false;
    uint32_t count = *(uint32_t *)navData;
    navData += 4;
    sizeLeft -= 4;

    for (uint32_t i = 0u; i < count; ++i)
    {
      if (sizeLeft < sizeof(dtTileRef))
        return false;
      const dtTileRef tileToSave = *(const dtTileRef *)navData;
      navData += sizeof(dtTileRef);
      sizeLeft -= sizeof(dtTileRef);

      if (sizeLeft < 4)
        return false;
      uint32_t dataSize = *(uint32_t *)navData;
      navData += 4;
      sizeLeft -= 4;

      if (dataSize == 0 || sizeLeft < dataSize)
        return false;
      unsigned char *data = (unsigned char *)navData;
      navData += dataSize;
      sizeLeft -= dataSize;

      // pinned restore fails when the tile array was sized below the saved index or
      // the slot is taken; a partially applied patch must not pass as a loaded one
      if (dtStatusFailed(navMesh->addTile(data, dataSize, 0, tileToSave, 0)))
      {
        logerr("patched navmesh: failed to restore nav tile into slot %d", (int)navMesh->decodePolyIdTile(tileToSave));
        return false;
      }
    }
  }

  if (sizeLeft > 0)
    return false;

  return true;
}

} // namespace pathfinder