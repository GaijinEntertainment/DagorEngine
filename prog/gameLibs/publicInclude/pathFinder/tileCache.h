//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>
#include <math/dag_frustum.h>
#include <detourTileCache.h>
#include <detourTileCacheBuilder.h>
#include <detourNavMeshBuilder.h>
#include <ska_hash_map/flat_hash_map2.hpp>

struct ZSTD_DCtx_s;
struct ZSTD_DDict_s;

namespace scene
{
class TiledScene;
}
class dtNavMeshQuery;

namespace pathfinder
{
struct TileCacheCompressor : public dtTileCacheCompressor
{
  ~TileCacheCompressor();

  void reset(bool isZSTD, const Tab<char> &zstdDictBuff = Tab<char>());

  int maxCompressedSize(const int bufferSize) override;

  dtStatus compress(const unsigned char *buffer, const int bufferSize, unsigned char *compressed, const int /*maxCompressedSize*/,
    int *compressedSize) override;

  dtStatus decompress(const unsigned char *compressed, const int compressedSize, unsigned char *buffer, const int maxBufferSize,
    int *bufferSize) override;

private:
  ZSTD_DCtx_s *dctx = nullptr;
  ZSTD_DDict_s *dDict = nullptr;
};

struct TileCacheMeshProcess : public dtTileCacheMeshProcess
{
  inline void setNavMesh(dtNavMesh *m) { mesh = m; }
  inline void setNavMeshQuery(dtNavMeshQuery *q) { meshQuery = q; }
  inline void setTileCache(dtTileCache *tileCache) { tc = tileCache; }

  inline void forceStaticLinks() { staticLinks = true; }

  inline void setLadders(const scene::TiledScene *scn) { ladders = scn; }
  static void calcQueryLaddersBBox(BBox3 &out_box, const float *bmin, const float *bmax);

  const scene::TiledScene *getLadders() const { return ladders; }

  bool checkOverLink(const float *from, const float *to, unsigned int &user_id) const;

  void process(struct dtNavMeshCreateParams *params, unsigned char *polyAreas, unsigned short *polyFlags,
    dtCompressedTileRef ref) override;
  void remove(int tx, int ty, int tlayer) override;

private:
  dtNavMesh *mesh = nullptr;
  dtNavMeshQuery *meshQuery = nullptr;
  dtTileCache *tc = nullptr;
  bool staticLinks = false;
  const scene::TiledScene *ladders = nullptr;

  Tab<float> offMeshConVerts;
  Tab<float> offMeshConRads;
  Tab<unsigned char> offMeshConDirs;
  Tab<unsigned char> offMeshConAreas;
  Tab<unsigned short> offMeshConFlags;
  Tab<unsigned int> offMeshConId;
};

void tilecache_init(dtTileCache *tc, const ska::flat_hash_set<uint32_t> &obstacle_res_name_hashes);
void tilecache_cleanup();
void tilecache_render_debug(const Frustum *frustum);

BBox3 tilecache_calc_tile_bounds(const dtTileCacheParams *params, const dtTileCacheLayerHeader *header);
}; // namespace pathfinder
